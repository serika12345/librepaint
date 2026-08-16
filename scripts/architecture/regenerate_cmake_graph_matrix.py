#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
import tempfile
from pathlib import Path
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
PLATFORM_PROFILES = {
    "macos": "tdd-macos",
    "linux": "tdd-linux",
    "ios": "ios-device-incremental",
    "android": "android-arm64-v8a-incremental",
    "windows": "windows-x86_64-incremental",
}


class MatrixError(RuntimeError):
    """Raised when the recorded platform ledgers do not form a valid matrix."""


def _load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise MatrixError(f"platform ledger does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise MatrixError(
            f"invalid JSON in {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise MatrixError(f"expected a JSON object in {path}")
    return value


def _validate_graph(platform: str, graph: dict[str, Any], path: Path) -> None:
    if graph.get("schemaVersion") != 1:
        raise MatrixError(f"unsupported graph schema in {path}")
    if graph.get("platform") != platform:
        raise MatrixError(f"platform mismatch in {path}: {graph.get('platform')!r}")
    if graph.get("buildProfile") != PLATFORM_PROFILES[platform]:
        raise MatrixError(
            f"build profile mismatch in {path}: {graph.get('buildProfile')!r}"
        )
    targets = graph.get("targets")
    if not isinstance(targets, list):
        raise MatrixError(f"expected a target array in {path}")

    names: list[str] = []
    for target in targets:
        if not isinstance(target, dict):
            raise MatrixError(f"expected a target object in {path}")
        if set(target) != {"name", "type", "sourceDirectory", "dependencies"}:
            raise MatrixError(f"unexpected target fields in {path}: {target!r}")
        name = target["name"]
        dependencies = target["dependencies"]
        if not isinstance(name, str) or not name:
            raise MatrixError(f"invalid target name in {path}: {name!r}")
        if not isinstance(target["type"], str) or not target["type"]:
            raise MatrixError(f"invalid target type for {name} in {path}")
        if not isinstance(target["sourceDirectory"], str):
            raise MatrixError(f"invalid source directory for {name} in {path}")
        if not isinstance(dependencies, list) or not all(
            isinstance(dependency, str) for dependency in dependencies
        ):
            raise MatrixError(f"invalid dependencies for {name} in {path}")
        if dependencies != sorted(set(dependencies)):
            raise MatrixError(f"dependencies are not sorted and unique for {name} in {path}")
        names.append(name)
    if names != sorted(set(names)):
        raise MatrixError(f"targets are not sorted and unique in {path}")


def load_graphs(directory: Path) -> dict[str, dict[str, Any]]:
    graphs: dict[str, dict[str, Any]] = {}
    for platform in PLATFORM_PROFILES:
        path = directory / f"cmake-targets-{platform}.json"
        graph = _load_json(path)
        _validate_graph(platform, graph, path)
        graphs[platform] = graph
    return graphs


def build_matrix(graphs: dict[str, dict[str, Any]]) -> dict[str, Any]:
    platforms = list(PLATFORM_PROFILES)
    targets_by_platform = {
        platform: {target["name"]: target for target in graphs[platform]["targets"]}
        for platform in platforms
    }
    all_names = sorted(
        set().union(*(set(targets) for targets in targets_by_platform.values()))
    )

    common_targets: list[str] = []
    conditional_targets: list[dict[str, Any]] = []
    variant_targets: list[str] = []
    for name in all_names:
        available_platforms = [
            platform for platform in platforms if name in targets_by_platform[platform]
        ]
        if len(available_platforms) == len(platforms):
            common_targets.append(name)
        else:
            conditional_targets.append(
                {"name": name, "platforms": available_platforms}
            )

        definitions = {
            json.dumps(
                targets_by_platform[platform][name],
                ensure_ascii=False,
                sort_keys=True,
            )
            for platform in available_platforms
        }
        if len(available_platforms) > 1 and len(definitions) > 1:
            variant_targets.append(name)

    return {
        "schemaVersion": 1,
        "platforms": platforms,
        "targetCounts": {
            platform: len(targets_by_platform[platform]) for platform in platforms
        },
        "commonTargets": common_targets,
        "conditionalTargets": conditional_targets,
        "variantTargets": variant_targets,
    }


def serialize_matrix(matrix: dict[str, Any]) -> str:
    return json.dumps(matrix, ensure_ascii=False, indent=2) + "\n"


def _write_atomically(output_path: Path, content: str) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            dir=output_path.parent,
            prefix=f".{output_path.name}.",
            delete=False,
        ) as temporary_file:
            temporary_file.write(content)
            temporary_path = Path(temporary_file.name)
        temporary_path.replace(output_path)
    finally:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)


def regenerate(directory: Path, output_path: Path, *, check: bool) -> None:
    content = serialize_matrix(build_matrix(load_graphs(directory)))
    if check:
        try:
            recorded_content = output_path.read_text(encoding="utf-8")
        except FileNotFoundError as error:
            raise MatrixError(f"recorded matrix does not exist: {output_path}") from error
        if recorded_content != content:
            raise MatrixError(
                f"recorded matrix is stale: {output_path}; run "
                "scripts/architecture/regenerate_cmake_graph_matrix.py"
            )
    else:
        _write_atomically(output_path, content)


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Regenerate the joint CMake target matrix for every platform."
    )
    parser.add_argument(
        "--directory",
        type=Path,
        default=REPO_ROOT / "docs/architecture",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=REPO_ROOT / "docs/architecture/cmake-target-matrix.json",
    )
    parser.add_argument("--check", action="store_true")
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        regenerate(options.directory, options.output, check=options.check)
    except (MatrixError, OSError) as error:
        print(f"regenerate-cmake-graph-matrix: {error}", file=sys.stderr)
        return 1
    action = "verified" if options.check else "updated"
    print(f"cmake target matrix {action}: {options.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
