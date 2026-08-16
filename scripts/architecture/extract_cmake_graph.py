#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path, PurePosixPath
from typing import Any


class GraphExtractionError(RuntimeError):
    """Raised when a CMake File API reply cannot satisfy the graph contract."""


def _load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise GraphExtractionError(f"referenced reply does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise GraphExtractionError(
            f"invalid JSON in {path}: line {error.lineno}, column {error.colno}"
        ) from error

    if not isinstance(value, dict):
        raise GraphExtractionError(f"expected a JSON object in {path}")
    return value


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise GraphExtractionError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise GraphExtractionError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value:
        raise GraphExtractionError(f"expected a non-empty string for {description}")
    return value


def _find_codemodel(reply_directory: Path) -> tuple[dict[str, Any], Path]:
    index_paths = sorted(reply_directory.glob("index-*.json"))
    if not index_paths:
        raise GraphExtractionError(
            f"codemodel-v2 reply was not found in {reply_directory}"
        )

    index_path = index_paths[-1]
    index = _load_json(index_path)
    reply = _require_object(index.get("reply"), f"reply in {index_path}")
    reference = _require_object(
        reply.get("codemodel-v2"), f"codemodel-v2 in {index_path}"
    )
    json_file = _require_string(
        reference.get("jsonFile"), f"codemodel-v2.jsonFile in {index_path}"
    )
    codemodel_path = reply_directory / json_file
    return _load_json(codemodel_path), codemodel_path


def _select_configuration(
    codemodel: dict[str, Any],
    codemodel_path: Path,
    requested_name: str | None,
) -> dict[str, Any]:
    configurations = _require_array(
        codemodel.get("configurations"), f"configurations in {codemodel_path}"
    )
    if requested_name is not None:
        matches = [
            value
            for value in configurations
            if isinstance(value, dict) and value.get("name") == requested_name
        ]
        if len(matches) != 1:
            raise GraphExtractionError(
                f"configuration {requested_name!r} was not found exactly once in "
                f"{codemodel_path}"
            )
        return matches[0]

    if len(configurations) != 1:
        names = [
            value.get("name", "<unnamed>")
            for value in configurations
            if isinstance(value, dict)
        ]
        raise GraphExtractionError(
            "configuration must be selected when the codemodel contains "
            f"{len(configurations)} configurations: {names}"
        )
    return _require_object(configurations[0], "the only codemodel configuration")


def _target_references(
    configuration: dict[str, Any], key: str
) -> list[dict[str, Any]]:
    values = _require_array(configuration.get(key, []), key)
    return [
        _require_object(value, f"entry in {key}")
        for value in values
    ]


def _load_targets(
    reply_directory: Path,
    references: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    targets: list[dict[str, Any]] = []
    for reference in references:
        json_file = _require_string(reference.get("jsonFile"), "target jsonFile")
        targets.append(_load_json(reply_directory / json_file))
    return targets


def _normalized_source_directory(
    target: dict[str, Any], source_root: str
) -> str:
    target_name = _require_string(target.get("name"), "target name")
    paths = _require_object(target.get("paths"), f"paths for target {target_name}")
    source = _require_string(
        paths.get("source"), f"source directory for target {target_name}"
    )
    source_path = PurePosixPath(source)
    if source_path.is_absolute():
        root_path = PurePosixPath(source_root)
        try:
            source_path = source_path.relative_to(root_path)
        except ValueError as error:
            raise GraphExtractionError(
                f"target {target_name} has a source directory outside the repository: "
                f"{source}"
            ) from error
    return source_path.as_posix()


def extract_graph(
    reply_directory: Path,
    *,
    platform: str,
    build_profile: str,
    configuration_name: str | None = None,
) -> dict[str, Any]:
    codemodel, codemodel_path = _find_codemodel(reply_directory)
    version = _require_object(codemodel.get("version"), "codemodel version")
    major_version = version.get("major")
    minor_version = version.get("minor")
    if major_version != 2:
        raise GraphExtractionError(
            f"unsupported codemodel major version in {codemodel_path}: "
            f"{major_version!r}"
        )
    if not isinstance(minor_version, int) or minor_version < 9:
        raise GraphExtractionError(
            "direct link extraction requires codemodel version 2.9 or newer; "
            f"found 2.{minor_version!r} in {codemodel_path}"
        )

    configuration = _select_configuration(
        codemodel, codemodel_path, configuration_name
    )
    references = _target_references(configuration, "targets")
    abstract_references = _target_references(configuration, "abstractTargets")
    targets = _load_targets(reply_directory, references)
    abstract_targets = _load_targets(reply_directory, abstract_references)

    names_by_id: dict[str, str] = {}
    for target in [*targets, *abstract_targets]:
        target_id = _require_string(target.get("id"), "target id")
        target_name = _require_string(target.get("name"), f"name for {target_id}")
        names_by_id[target_id] = target_name

    paths = _require_object(codemodel.get("paths"), "codemodel paths")
    source_root = _require_string(paths.get("source"), "codemodel source path")
    graph_targets: list[dict[str, Any]] = []
    target_names: set[str] = set()
    for target in targets:
        if target.get("isGeneratorProvided") is True:
            continue

        name = _require_string(target.get("name"), "target name")
        if name in target_names:
            raise GraphExtractionError(f"duplicate build target name: {name}")
        target_names.add(name)

        dependency_names: set[str] = set()
        link_libraries = _require_array(
            target.get("linkLibraries", []), f"linkLibraries for target {name}"
        )
        for dependency in link_libraries:
            dependency_object = _require_object(
                dependency, f"linkLibraries entry for target {name}"
            )
            dependency_id = dependency_object.get("id")
            if dependency_id is None:
                continue
            dependency_id = _require_string(
                dependency_id, f"dependency id for target {name}"
            )
            try:
                dependency_names.add(names_by_id[dependency_id])
            except KeyError as error:
                raise GraphExtractionError(
                    f"target {name} references unknown target id: {dependency_id}"
                ) from error

        graph_targets.append(
            {
                "name": name,
                "type": _require_string(target.get("type"), f"type for target {name}"),
                "sourceDirectory": _normalized_source_directory(target, source_root),
                "dependencies": sorted(dependency_names),
            }
        )

    graph_targets.sort(key=lambda target: target["name"])
    return {
        "schemaVersion": 1,
        "platform": platform,
        "buildProfile": build_profile,
        "configuration": _require_string(
            configuration.get("name"), "configuration name"
        ),
        "targets": graph_targets,
    }


def serialize_graph(graph: dict[str, Any]) -> str:
    return json.dumps(graph, ensure_ascii=False, indent=2) + "\n"


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Extract a deterministic target ledger from CMake File API replies."
    )
    parser.add_argument("reply_directory", type=Path)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--build-profile", required=True)
    parser.add_argument("--configuration")
    parser.add_argument("--output", type=Path)
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        graph = extract_graph(
            options.reply_directory,
            platform=options.platform,
            build_profile=options.build_profile,
            configuration_name=options.configuration,
        )
        serialized = serialize_graph(graph)
        if options.output is None:
            sys.stdout.write(serialized)
        else:
            options.output.write_text(serialized, encoding="utf-8")
    except (GraphExtractionError, OSError) as error:
        print(f"extract-cmake-graph: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
