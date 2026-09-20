#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path, PurePosixPath
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
if str(SCRIPT_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIRECTORY))

import extract_cmake_graph  # noqa: E402


POLICY_PATH = REPO_ROOT / "docs/architecture/package-boundaries.json"
PRODUCTION_SOURCE_DIRECTORIES = frozenset(
    {
        "krita",
        "libs",
        "packaging",
        "pch",
        "plugins",
        "qmlmodules",
        "sdk",
        "winquirks",
    }
)
TEST_PATH_PARTS = frozenset({"benchmarks", "test", "tests"})
PRODUCT_TARGET_TYPES = frozenset(
    {
        "EXECUTABLE",
        "MODULE_LIBRARY",
        "OBJECT_LIBRARY",
        "SHARED_LIBRARY",
        "STATIC_LIBRARY",
    }
)


class PackageBoundaryError(RuntimeError):
    """Raised when the current package graph violates its policy."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise PackageBoundaryError(f"{description} does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise PackageBoundaryError(
            f"invalid JSON in {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise PackageBoundaryError(f"expected a JSON object in {path}")
    return value


def load_policy(path: Path = POLICY_PATH) -> dict[str, Any]:
    return _load_json(path, "package boundary policy")


def _string_list(value: object, description: str) -> list[str]:
    if not isinstance(value, list) or not all(
        isinstance(entry, str) and entry for entry in value
    ):
        raise PackageBoundaryError(f"{description} must be a string array")
    if value != sorted(set(value)):
        raise PackageBoundaryError(f"{description} must be sorted and unique")
    return value


def validate_policy(policy: dict[str, Any]) -> None:
    if set(policy) != {"schemaVersion", "responsibilities"}:
        raise PackageBoundaryError("package boundary policy has unexpected fields")
    if policy.get("schemaVersion") != 1:
        raise PackageBoundaryError("package boundary schemaVersion must be 1")
    entries = policy.get("responsibilities")
    if not isinstance(entries, list):
        raise PackageBoundaryError("responsibilities must be an array")

    identifiers: list[str] = []
    target_owners: dict[str, str] = {}
    dependencies: dict[str, list[str]] = {}
    for index, item in enumerate(entries):
        if not isinstance(item, dict) or set(item) != {
            "id",
            "ownerTargets",
            "allowedDependencies",
        }:
            raise PackageBoundaryError(
                f"responsibility {index} has unexpected fields"
            )
        identifier = item.get("id")
        if not isinstance(identifier, str) or not identifier:
            raise PackageBoundaryError(f"responsibility {index} requires an id")
        owner_targets = _string_list(
            item.get("ownerTargets"), f"owner targets for {identifier}"
        )
        if not owner_targets:
            raise PackageBoundaryError(f"{identifier} requires an owner target")
        allowed = _string_list(
            item.get("allowedDependencies"),
            f"allowed dependencies for {identifier}",
        )
        identifiers.append(identifier)
        dependencies[identifier] = allowed
        for target in owner_targets:
            previous = target_owners.setdefault(target, identifier)
            if previous != identifier:
                raise PackageBoundaryError(
                    f"owner target has multiple responsibilities: {target}"
                )

    if identifiers != sorted(set(identifiers)):
        raise PackageBoundaryError("responsibility ids must be sorted and unique")
    known = set(identifiers)
    for identifier, allowed in dependencies.items():
        unknown = sorted(set(allowed) - known)
        if unknown:
            raise PackageBoundaryError(
                f"unknown dependencies for {identifier}: {unknown}"
            )
        if identifier in allowed:
            raise PackageBoundaryError(
                f"responsibility cannot depend on itself: {identifier}"
            )

    components = strongly_connected_components(
        known,
        {identifier: set(allowed) for identifier, allowed in dependencies.items()},
    )
    if components:
        raise PackageBoundaryError(
            f"allowed package dependency graph contains cycles: {components}"
        )


def _policy_relations(
    policy: dict[str, Any],
) -> tuple[dict[str, str], dict[str, set[str]]]:
    target_owners: dict[str, str] = {}
    allowed: dict[str, set[str]] = {}
    for item in policy["responsibilities"]:
        identifier = item["id"]
        allowed[identifier] = set(item["allowedDependencies"])
        for target in item["ownerTargets"]:
            target_owners[target] = identifier
    return target_owners, allowed


def strongly_connected_components(
    names: set[str], adjacency: dict[str, set[str]]
) -> list[list[str]]:
    index = 0
    indices: dict[str, int] = {}
    lowlinks: dict[str, int] = {}
    stack: list[str] = []
    on_stack: set[str] = set()
    components: list[list[str]] = []

    def visit(name: str) -> None:
        nonlocal index
        indices[name] = index
        lowlinks[name] = index
        index += 1
        stack.append(name)
        on_stack.add(name)
        for dependency in sorted(adjacency.get(name, set()) & names):
            if dependency not in indices:
                visit(dependency)
                lowlinks[name] = min(lowlinks[name], lowlinks[dependency])
            elif dependency in on_stack:
                lowlinks[name] = min(lowlinks[name], indices[dependency])
        if lowlinks[name] != indices[name]:
            return
        component: list[str] = []
        while True:
            member = stack.pop()
            on_stack.remove(member)
            component.append(member)
            if member == name:
                break
        component.sort()
        if len(component) > 1 or name in adjacency.get(name, set()):
            components.append(component)

    for name in sorted(names):
        if name not in indices:
            visit(name)
    return sorted(components)


def _is_product_target(target: dict[str, Any]) -> bool:
    source_directory = target.get("sourceDirectory")
    if (
        not isinstance(source_directory, str)
        or target.get("type") not in PRODUCT_TARGET_TYPES
    ):
        return False
    path = PurePosixPath(source_directory)
    return (
        bool(path.parts)
        and path.parts[0] in PRODUCTION_SOURCE_DIRECTORIES
        and not any(part in TEST_PATH_PARTS for part in path.parts)
    )


def validate_graph(policy: dict[str, Any], graph: dict[str, Any]) -> None:
    validate_policy(policy)
    if graph.get("schemaVersion") != 1:
        raise PackageBoundaryError("CMake graph schemaVersion must be 1")
    platform = graph.get("platform")
    targets_value = graph.get("targets")
    if not isinstance(platform, str) or not isinstance(targets_value, list):
        raise PackageBoundaryError("CMake graph requires a platform and targets")

    targets: dict[str, dict[str, Any]] = {}
    for item in targets_value:
        if not isinstance(item, dict) or not isinstance(item.get("name"), str):
            raise PackageBoundaryError("CMake graph contains an invalid target")
        name = item["name"]
        if name in targets:
            raise PackageBoundaryError(f"duplicate CMake target: {name}")
        targets[name] = item

    target_owners, allowed = _policy_relations(policy)
    missing = sorted(set(target_owners) - set(targets))
    if missing:
        raise PackageBoundaryError(
            f"owner targets are missing on {platform}: {missing}"
        )

    forbidden: list[str] = []
    for source_target, source_responsibility in sorted(target_owners.items()):
        dependencies = _string_list(
            targets[source_target].get("dependencies"),
            f"dependencies for {source_target}",
        )
        for dependency_target in dependencies:
            dependency_responsibility = target_owners.get(dependency_target)
            if dependency_responsibility is None:
                continue
            if dependency_responsibility == source_responsibility:
                continue
            if dependency_responsibility not in allowed[source_responsibility]:
                forbidden.append(
                    f"{source_target} ({source_responsibility}) -> "
                    f"{dependency_target} ({dependency_responsibility})"
                )
    if forbidden:
        raise PackageBoundaryError(
            f"forbidden package dependencies on {platform}: {forbidden}"
        )

    product_names = {
        name for name, target in targets.items() if _is_product_target(target)
    }
    adjacency = {
        name: set(_string_list(targets[name].get("dependencies"), name))
        for name in product_names
    }
    cycles = strongly_connected_components(product_names, adjacency)
    if cycles:
        raise PackageBoundaryError(f"product target cycles on {platform}: {cycles}")


def prepare_query(build_directory: Path) -> Path:
    query = build_directory / ".cmake/api/v1/query/codemodel-v2"
    query.parent.mkdir(parents=True, exist_ok=True)
    query.touch()
    return query


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Validate package boundaries against the current CMake graph."
    )
    parser.add_argument("--policy", type=Path, default=POLICY_PATH)
    parser.add_argument("--prepare-query", type=Path)
    parser.add_argument("--reply-directory", type=Path)
    parser.add_argument("--platform")
    parser.add_argument("--build-profile")
    parser.add_argument("--configuration")
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        policy = load_policy(options.policy)
        validate_policy(policy)
        if options.prepare_query is not None:
            query = prepare_query(options.prepare_query)
            print(f"prepared CMake File API query: {query}")
            return 0
        if options.reply_directory is not None:
            if not options.platform or not options.build_profile:
                raise PackageBoundaryError(
                    "--reply-directory requires --platform and --build-profile"
                )
            graph = extract_cmake_graph.extract_graph(
                options.reply_directory,
                platform=options.platform,
                build_profile=options.build_profile,
                configuration_name=options.configuration,
            )
            validate_graph(policy, graph)
            print(
                f"package boundaries verified on {options.platform}: "
                f"{len(graph['targets'])} targets"
            )
        else:
            print(
                "package boundary policy verified: "
                f"{len(policy['responsibilities'])} responsibilities"
            )
    except (
        OSError,
        PackageBoundaryError,
        extract_cmake_graph.GraphExtractionError,
    ) as error:
        print(f"check-package-boundaries: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
