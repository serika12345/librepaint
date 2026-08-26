#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path, PurePosixPath
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
sys.path.insert(0, str(SCRIPT_DIRECTORY))

import check_package_dependencies as dependency_contract


PLATFORMS = ("macos", "linux", "ios", "android", "windows")
PRODUCT_TARGET_TYPES = frozenset(
    {
        "EXECUTABLE",
        "MODULE_LIBRARY",
        "OBJECT_LIBRARY",
        "SHARED_LIBRARY",
        "STATIC_LIBRARY",
    }
)


class StructuralDependencyError(RuntimeError):
    """Raised when structural package dependencies violate their contract."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise StructuralDependencyError(
            f"{description} does not exist: {path}"
        ) from error
    except json.JSONDecodeError as error:
        raise StructuralDependencyError(
            f"invalid JSON in {path}: line {error.lineno}, "
            f"column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise StructuralDependencyError(f"expected a JSON object in {path}")
    return value


def contract_inputs() -> dict[str, str]:
    return {
        "allowedDependencyPolicy": (
            "docs/architecture/allowed-package-dependencies.json"
        ),
        "packageResponsibilityMap": (
            "docs/architecture/package-responsibilities.json"
        ),
        "publicSurfaceInventory": (
            "docs/architecture/public-surface-inventory.json"
        ),
        "cmakeGraphs": "docs/architecture/cmake-targets-<platform>.json",
    }


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise StructuralDependencyError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise StructuralDependencyError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise StructuralDependencyError(
            f"expected a non-empty string for {description}"
        )
    return value


def _repository_inputs(repository_root: Path) -> dict[str, dict[str, Any]]:
    inputs = contract_inputs()
    return {
        name: _load_json(repository_root / path, name)
        for name, path in inputs.items()
        if name != "cmakeGraphs"
    }


def _load_graphs(
    repository_root: Path,
) -> dict[str, dict[str, dict[str, Any]]]:
    result: dict[str, dict[str, dict[str, Any]]] = {}
    for platform in PLATFORMS:
        path = (
            repository_root
            / f"docs/architecture/cmake-targets-{platform}.json"
        )
        graph = _load_json(path, f"{platform} CMake graph")
        if graph.get("schemaVersion") != 1 or graph.get("platform") != platform:
            raise StructuralDependencyError(f"invalid CMake graph: {path}")
        result[platform] = {
            _require_string(entry.get("name"), "target name"): entry
            for item in _require_array(graph.get("targets"), "targets")
            for entry in [_require_object(item, "target")]
        }
    return result


def _candidate_links(
    policy: dict[str, Any],
) -> dict[tuple[str, str], set[tuple[str, str]]]:
    result: dict[tuple[str, str], set[tuple[str, str]]] = {}
    for edge_item in _require_array(
        policy.get("currentTargetEdges"), "current target edges"
    ):
        edge = _require_object(edge_item, "current target edge")
        link = (
            _require_string(edge.get("sourceTarget"), "source target"),
            _require_string(edge.get("dependencyTarget"), "dependency target"),
        )
        for projection_item in _require_array(
            edge.get("projections"), "target edge projections"
        ):
            projection = _require_object(projection_item, "target projection")
            if projection.get("status") != "violates-policy":
                continue
            pair = (
                _require_string(
                    projection.get("sourceResponsibility"),
                    "source responsibility",
                ),
                _require_string(
                    projection.get("dependencyResponsibility"),
                    "dependency responsibility",
                ),
            )
            result.setdefault(pair, set()).add(link)
    return result


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


def _is_production_target(target: dict[str, Any]) -> bool:
    source_directory = target.get("sourceDirectory")
    target_type = target.get("type")
    if not isinstance(source_directory, str) or target_type not in PRODUCT_TARGET_TYPES:
        return False
    path = PurePosixPath(source_directory)
    return (
        bool(path.parts)
        and path.parts[0]
        in dependency_contract.PRODUCTION_SOURCE_DIRECTORIES
        and not any(
            part in dependency_contract.TEST_PATH_PARTS for part in path.parts
        )
    )


def discover_target_cycles(
    *,
    responsibility_map: dict[str, Any],
    graphs: dict[str, dict[str, dict[str, Any]]],
) -> list[dict[str, Any]]:
    core_targets = {
        _require_string(entry.get("name"), "core target name")
        for item in _require_array(
            responsibility_map.get("targetRelations"), "target relations"
        )
        for entry in [_require_object(item, "target relation")]
    }
    scopes = (
        ("core-owner-targets", lambda entries: set(entries) & core_targets),
        (
            "all-production-build-targets",
            lambda entries: {
                name
                for name, target in entries.items()
                if _is_production_target(target)
            },
        ),
    )
    result: list[dict[str, Any]] = []
    for scope, select in scopes:
        platforms: list[dict[str, Any]] = []
        for platform in PLATFORMS:
            entries = graphs[platform]
            names = select(entries)
            adjacency = {
                name: {
                    dependency
                    for dependency in _require_array(
                        entry.get("dependencies"), f"dependencies for {name}"
                    )
                    if isinstance(dependency, str) and dependency in entries
                }
                for name, entry in entries.items()
            }
            platforms.append(
                {
                    "platform": platform,
                    "targetCount": len(names),
                    "components": strongly_connected_components(
                        names, adjacency
                    ),
                }
            )
        result.append({"scope": scope, "platforms": platforms})
    return result


def discover_internal_headers(
    public_surface: dict[str, Any],
) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    for item in _require_array(
        public_surface.get("publicHeaderSets"), "public header sets"
    ):
        header_set = _require_object(item, "public header set")
        owner = _require_string(header_set.get("ownerTarget"), "header owner")
        headers = []
        for header_item in _require_array(header_set.get("headers"), "headers"):
            header = _require_object(header_item, "public header")
            evidence = _require_array(
                header.get("publicationEvidence"), "publication evidence"
            )
            if "external-include" not in evidence or any(
                item in evidence for item in ("export-macro", "compile-contract")
            ):
                continue
            headers.append(
                {
                    "path": _require_string(header.get("path"), "header path"),
                    "consumerPaths": _require_array(
                        header.get("consumerPaths"), "header consumer paths"
                    ),
                }
            )
        result[owner] = {
            "sourceDirectory": _require_string(
                header_set.get("sourceDirectory"), "header source directory"
            ),
            "platforms": _require_array(
                header_set.get("platforms"), "header platforms"
            ),
            "headers": headers,
        }
    return result


def validate_structural_dependencies(*, repository_root: Path) -> None:
    inputs = _repository_inputs(repository_root)
    graphs = _load_graphs(repository_root)
    projection_candidates = _candidate_links(inputs["allowedDependencyPolicy"])
    if projection_candidates:
        raise StructuralDependencyError(
            "package responsibility projections must remain empty: "
            f"{sorted(projection_candidates)}"
        )

    cycles = discover_target_cycles(
        responsibility_map=inputs["packageResponsibilityMap"], graphs=graphs
    )
    cycle_components = {
        f"{scope['scope']}/{platform['platform']}": platform["components"]
        for scope in cycles
        for platform in scope["platforms"]
        if platform["components"]
    }
    if cycle_components:
        raise StructuralDependencyError(
            f"production target cycles must remain empty: {cycle_components}"
        )

    internal_headers = discover_internal_headers(inputs["publicSurfaceInventory"])
    unpublished = {
        owner: evidence["headers"]
        for owner, evidence in internal_headers.items()
        if evidence["headers"]
    }
    if unpublished:
        counts = {
            owner: {
                "headers": len(headers),
                "references": sum(
                    len(header["consumerPaths"]) for header in headers
                ),
            }
            for owner, headers in unpublished.items()
        }
        raise StructuralDependencyError(
            f"external internal-header references must remain empty: {counts}"
        )


def _argument_parser() -> argparse.ArgumentParser:
    return argparse.ArgumentParser(
        description=(
            "Require attributable package projections, acyclic product targets, "
            "and declared cross-package header surfaces."
        )
    )


def main(arguments: list[str] | None = None) -> int:
    _argument_parser().parse_args(arguments)
    try:
        validate_structural_dependencies(repository_root=REPO_ROOT)
    except (OSError, StructuralDependencyError) as error:
        print(f"check-structural-dependencies: {error}", file=sys.stderr)
        return 1
    print(
        "structural dependencies verified: 0 projections, 0 cycles, "
        "0 external internal-header references"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
