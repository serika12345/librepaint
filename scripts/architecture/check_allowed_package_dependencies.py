#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
POLICY_SCOPE = "r1-package-responsibility-dependency-policy"
RESPONSIBILITY_MAP_REFERENCE = (
    "docs/architecture/package-responsibilities.json"
)
RESPONSIBILITY_FIELDS = {
    "id",
    "layer",
    "responsibility",
    "publicSurfaces",
    "allowedDependencies",
}
PUBLIC_SURFACE_FIELDS = {"id", "purpose", "lifetime", "errorBehavior"}
DEPENDENCY_FIELDS = {"responsibility", "surface"}
TARGET_EDGE_FIELDS = {
    "sourceTarget",
    "dependencyTarget",
    "sourceResponsibilities",
    "dependencyResponsibilities",
    "projections",
}
PROJECTION_FIELDS = {
    "sourceResponsibility",
    "dependencyResponsibility",
    "status",
}
PROJECTION_STATUSES = {
    "allowed",
    "internal",
    "requires-r1-g4-baseline",
}


class DependencyPolicyError(RuntimeError):
    """Raised when the R1 package dependency policy is inconsistent."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise DependencyPolicyError(
            f"{description} does not exist: {path}"
        ) from error
    except json.JSONDecodeError as error:
        raise DependencyPolicyError(
            f"invalid JSON in {path}: line {error.lineno}, "
            f"column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise DependencyPolicyError(f"expected a JSON object in {path}")
    return value


def load_policy(path: Path) -> dict[str, Any]:
    return _load_json(path, "allowed package dependency policy")


def load_responsibility_map(path: Path) -> dict[str, Any]:
    return _load_json(path, "package responsibility map")


def projection_policy() -> dict[str, str]:
    return {
        "dependencyKind": "direct-link-library",
        "targetScope": "selected-owner-targets",
        "sharedTargetHandling": "conservative-cartesian-product",
        "sameResponsibilityHandling": "internal",
    }


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise DependencyPolicyError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise DependencyPolicyError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise DependencyPolicyError(
            f"expected a non-empty string for {description}"
        )
    return value


def _require_fields(
    value: dict[str, Any], expected: set[str], description: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise DependencyPolicyError(
            f"unexpected fields for {description}; "
            f"missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def _string_list(value: Any, description: str) -> list[str]:
    entries = [
        _require_string(entry, description)
        for entry in _require_array(value, description)
    ]
    if entries != sorted(set(entries)):
        raise DependencyPolicyError(
            f"{description} must be sorted and unique"
        )
    return entries


def _manual_responsibility(
    value: dict[str, Any], index: int
) -> dict[str, Any]:
    _require_fields(value, RESPONSIBILITY_FIELDS, f"responsibility {index}")
    identifier = _require_string(value.get("id"), f"responsibility id {index}")
    layer = value.get("layer")
    if not isinstance(layer, int) or isinstance(layer, bool) or layer < 0:
        raise DependencyPolicyError(
            f"layer must be a non-negative integer for {identifier}"
        )
    surfaces: list[dict[str, str]] = []
    for surface_index, item in enumerate(
        _require_array(
            value.get("publicSurfaces"),
            f"public surfaces for {identifier}",
        )
    ):
        surface = _require_object(
            item, f"public surface {surface_index} for {identifier}"
        )
        _require_fields(
            surface,
            PUBLIC_SURFACE_FIELDS,
            f"public surface {surface_index} for {identifier}",
        )
        surfaces.append(
            {
                "id": _require_string(
                    surface.get("id"), f"public surface id for {identifier}"
                ),
                "purpose": _require_string(
                    surface.get("purpose"),
                    f"public surface purpose for {identifier}",
                ),
                "lifetime": _require_string(
                    surface.get("lifetime"),
                    f"public surface lifetime for {identifier}",
                ),
                "errorBehavior": _require_string(
                    surface.get("errorBehavior"),
                    f"public surface error behavior for {identifier}",
                ),
            }
        )
    if not surfaces:
        raise DependencyPolicyError(
            f"responsibility has no public surface: {identifier}"
        )
    surface_ids = [surface["id"] for surface in surfaces]
    if surface_ids != sorted(set(surface_ids)):
        raise DependencyPolicyError(
            f"public surfaces must be sorted and unique for {identifier}"
        )

    dependencies: list[dict[str, str]] = []
    for dependency_index, item in enumerate(
        _require_array(
            value.get("allowedDependencies"),
            f"allowed dependencies for {identifier}",
        )
    ):
        dependency = _require_object(
            item, f"allowed dependency {dependency_index} for {identifier}"
        )
        _require_fields(
            dependency,
            DEPENDENCY_FIELDS,
            f"allowed dependency {dependency_index} for {identifier}",
        )
        dependencies.append(
            {
                "responsibility": _require_string(
                    dependency.get("responsibility"),
                    f"dependency responsibility for {identifier}",
                ),
                "surface": _require_string(
                    dependency.get("surface"),
                    f"dependency surface for {identifier}",
                ),
            }
        )
    dependency_keys = [
        (dependency["responsibility"], dependency["surface"])
        for dependency in dependencies
    ]
    if dependency_keys != sorted(set(dependency_keys)):
        raise DependencyPolicyError(
            f"allowed dependencies must be sorted and unique for {identifier}"
        )
    return {
        "id": identifier,
        "layer": layer,
        "responsibility": _require_string(
            value.get("responsibility"), f"responsibility for {identifier}"
        ),
        "publicSurfaces": surfaces,
        "allowedDependencies": dependencies,
    }


def _responsibility_map_owners(
    responsibility_map: dict[str, Any],
) -> tuple[list[str], dict[str, list[str]], list[dict[str, Any]]]:
    if (
        responsibility_map.get("schemaVersion") != 1
        or responsibility_map.get("scope")
        != "current-production-package-responsibilities"
    ):
        raise DependencyPolicyError("invalid package responsibility map")
    responsibility_ids: list[str] = []
    target_owners: dict[str, list[str]] = {}
    for index, item in enumerate(
        _require_array(
            responsibility_map.get("responsibilities"),
            "responsibility map responsibilities",
        )
    ):
        entry = _require_object(item, f"mapped responsibility {index}")
        identifier = _require_string(
            entry.get("id"), f"mapped responsibility id {index}"
        )
        responsibility_ids.append(identifier)
        for owner in _string_list(
            entry.get("ownerTargets"),
            f"mapped owner targets for {identifier}",
        ):
            target_owners.setdefault(owner, []).append(identifier)
    if responsibility_ids != sorted(set(responsibility_ids)):
        raise DependencyPolicyError(
            "mapped responsibilities must be sorted and unique"
        )
    for owners in target_owners.values():
        owners.sort()
    target_relations = [
        _require_object(item, f"target relation {index}")
        for index, item in enumerate(
            _require_array(
                responsibility_map.get("targetRelations"), "target relations"
            )
        )
    ]
    return responsibility_ids, target_owners, target_relations


def build_current_target_edges(
    *,
    responsibilities: list[dict[str, Any]],
    responsibility_map: dict[str, Any],
) -> list[dict[str, Any]]:
    _, target_owners, target_relations = _responsibility_map_owners(
        responsibility_map
    )
    allowed_pairs = {
        (entry["id"], dependency["responsibility"])
        for entry in responsibilities
        for dependency in entry["allowedDependencies"]
    }
    result: list[dict[str, Any]] = []
    for relation in target_relations:
        source_target = _require_string(
            relation.get("name"), "target relation name"
        )
        if source_target not in target_owners:
            continue
        for dependency_target in _string_list(
            relation.get("repositoryDependencies"),
            f"repository dependencies for {source_target}",
        ):
            if dependency_target not in target_owners:
                continue
            projections: list[dict[str, str]] = []
            for source_responsibility in target_owners[source_target]:
                for dependency_responsibility in target_owners[
                    dependency_target
                ]:
                    if source_responsibility == dependency_responsibility:
                        status = "internal"
                    elif (
                        source_responsibility,
                        dependency_responsibility,
                    ) in allowed_pairs:
                        status = "allowed"
                    else:
                        status = "requires-r1-g4-baseline"
                    projections.append(
                        {
                            "sourceResponsibility": source_responsibility,
                            "dependencyResponsibility": (
                                dependency_responsibility
                            ),
                            "status": status,
                        }
                    )
            result.append(
                {
                    "sourceTarget": source_target,
                    "dependencyTarget": dependency_target,
                    "sourceResponsibilities": target_owners[source_target],
                    "dependencyResponsibilities": target_owners[
                        dependency_target
                    ],
                    "projections": projections,
                }
            )
    return sorted(
        result,
        key=lambda edge: (edge["sourceTarget"], edge["dependencyTarget"]),
    )


def updated_policy(
    policy: dict[str, Any], *, responsibility_map: dict[str, Any]
) -> dict[str, Any]:
    responsibilities = [
        _manual_responsibility(
            _require_object(item, f"responsibility {index}"), index
        )
        for index, item in enumerate(
            _require_array(policy.get("responsibilities"), "responsibilities")
        )
    ]
    responsibilities.sort(key=lambda entry: entry["id"])
    return {
        "schemaVersion": 1,
        "scope": POLICY_SCOPE,
        "responsibilityMap": RESPONSIBILITY_MAP_REFERENCE,
        "projectionPolicy": projection_policy(),
        "responsibilities": responsibilities,
        "currentTargetEdges": build_current_target_edges(
            responsibilities=responsibilities,
            responsibility_map=responsibility_map,
        ),
    }


def validate_policy(
    policy: dict[str, Any], *, responsibility_map_path: Path
) -> None:
    _require_fields(
        policy,
        {
            "schemaVersion",
            "scope",
            "responsibilityMap",
            "projectionPolicy",
            "responsibilities",
            "currentTargetEdges",
        },
        "allowed package dependency policy",
    )
    if policy.get("schemaVersion") != 1:
        raise DependencyPolicyError("dependency policy schemaVersion must be 1")
    if policy.get("scope") != POLICY_SCOPE:
        raise DependencyPolicyError("dependency policy has invalid scope")
    if policy.get("responsibilityMap") != RESPONSIBILITY_MAP_REFERENCE:
        raise DependencyPolicyError(
            "dependency policy must reference the package responsibility map"
        )
    if policy.get("projectionPolicy") != projection_policy():
        raise DependencyPolicyError("dependency projection policy is invalid")

    responsibility_map = _load_json(
        responsibility_map_path, "package responsibility map"
    )
    known_ids, _, _ = _responsibility_map_owners(responsibility_map)
    responsibilities = [
        _manual_responsibility(
            _require_object(item, f"responsibility {index}"), index
        )
        for index, item in enumerate(
            _require_array(policy.get("responsibilities"), "responsibilities")
        )
    ]
    identifiers = [entry["id"] for entry in responsibilities]
    if identifiers != sorted(set(identifiers)):
        raise DependencyPolicyError(
            "responsibilities must be sorted and unique by id"
        )
    missing = sorted(set(known_ids) - set(identifiers))
    unexpected = sorted(set(identifiers) - set(known_ids))
    if missing or unexpected:
        raise DependencyPolicyError(
            "dependency policy does not match the responsibility map; "
            f"missing={missing}, unexpected={unexpected}"
        )

    by_id = {entry["id"]: entry for entry in responsibilities}
    surfaces = {
        entry["id"]: {surface["id"] for surface in entry["publicSurfaces"]}
        for entry in responsibilities
    }
    graph: dict[str, set[str]] = {
        identifier: set() for identifier in identifiers
    }
    for entry in responsibilities:
        source = entry["id"]
        for dependency in entry["allowedDependencies"]:
            target = dependency["responsibility"]
            if target == source:
                raise DependencyPolicyError(
                    f"self dependency is not allowed for {source}"
                )
            if target not in by_id:
                raise DependencyPolicyError(
                    f"unknown dependency responsibility for {source}: {target}"
                )
            if dependency["surface"] not in surfaces[target]:
                raise DependencyPolicyError(
                    f"unknown public surface for {source}: "
                    f"{target}/{dependency['surface']}"
                )
            graph[source].add(target)

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(identifier: str) -> None:
        if identifier in visiting:
            raise DependencyPolicyError(
                f"dependency policy contains a cycle at {identifier}"
            )
        if identifier in visited:
            return
        visiting.add(identifier)
        for dependency in sorted(graph[identifier]):
            visit(dependency)
        visiting.remove(identifier)
        visited.add(identifier)

    for identifier in identifiers:
        visit(identifier)

    for entry in responsibilities:
        source = entry["id"]
        for target in sorted(graph[source]):
            if entry["layer"] <= by_id[target]["layer"]:
                raise DependencyPolicyError(
                    f"dependency must point to a lower layer: {source} -> {target}"
                )

    current_edges = _require_array(
        policy.get("currentTargetEdges"), "current target edges"
    )
    for edge_index, item in enumerate(current_edges):
        edge = _require_object(item, f"current target edge {edge_index}")
        _require_fields(
            edge, TARGET_EDGE_FIELDS, f"current target edge {edge_index}"
        )
        _require_string(edge.get("sourceTarget"), "current edge source target")
        _require_string(
            edge.get("dependencyTarget"), "current edge dependency target"
        )
        _string_list(
            edge.get("sourceResponsibilities"),
            "current edge source responsibilities",
        )
        _string_list(
            edge.get("dependencyResponsibilities"),
            "current edge dependency responsibilities",
        )
        for projection_index, projection_item in enumerate(
            _require_array(edge.get("projections"), "current edge projections")
        ):
            projection = _require_object(
                projection_item,
                f"current projection {edge_index}/{projection_index}",
            )
            _require_fields(
                projection,
                PROJECTION_FIELDS,
                f"current projection {edge_index}/{projection_index}",
            )
            status = _require_string(
                projection.get("status"), "current projection status"
            )
            if status not in PROJECTION_STATUSES:
                raise DependencyPolicyError(
                    f"unknown current projection status: {status}"
                )

    expected = updated_policy(policy, responsibility_map=responsibility_map)
    if current_edges != expected["currentTargetEdges"]:
        raise DependencyPolicyError(
            "current target edge projection is stale; run "
            "scripts/architecture/update_allowed_package_dependencies.py"
        )


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Validate the allowed package dependency graph and its projection "
            "of current CMake target links."
        )
    )
    parser.add_argument(
        "--policy",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/allowed-package-dependencies.json"
        ),
    )
    parser.add_argument(
        "--responsibility-map",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/package-responsibilities.json"
        ),
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        validate_policy(
            load_policy(options.policy),
            responsibility_map_path=options.responsibility_map,
        )
    except (OSError, DependencyPolicyError) as error:
        print(f"check-allowed-package-dependencies: {error}", file=sys.stderr)
        return 1
    print(f"allowed package dependency policy verified: {options.policy}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
