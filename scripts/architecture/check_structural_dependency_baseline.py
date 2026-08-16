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

import check_dependency_violation_baseline as dependency_contract


PLATFORMS = ("macos", "linux", "ios", "android", "windows")
BASELINE_SCOPE = "r1-g4b-structural-dependency-baseline"
PRODUCT_TARGET_TYPES = frozenset(
    {
        "EXECUTABLE",
        "MODULE_LIBRARY",
        "OBJECT_LIBRARY",
        "SHARED_LIBRARY",
        "STATIC_LIBRARY",
    }
)
MANUAL_INTERNAL_FIELDS = {
    "ownerTarget",
    "owner",
    "reason",
    "maximumHeaders",
    "maximumDirectReferences",
    "removalCondition",
}
DERIVED_INTERNAL_FIELDS = {"sourceDirectory", "platforms", "headers"}


class StructuralBaselineError(RuntimeError):
    """Raised when the R1-G4b structural baseline is inconsistent."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise StructuralBaselineError(
            f"{description} does not exist: {path}"
        ) from error
    except json.JSONDecodeError as error:
        raise StructuralBaselineError(
            f"invalid JSON in {path}: line {error.lineno}, "
            f"column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise StructuralBaselineError(f"expected a JSON object in {path}")
    return value


def load_baseline(path: Path) -> dict[str, Any]:
    return _load_json(path, "structural dependency baseline")


def baseline_inputs() -> dict[str, str]:
    return {
        "allowedDependencyPolicy": (
            "docs/architecture/allowed-package-dependencies.json"
        ),
        "dependencyViolationBaseline": (
            "docs/architecture/dependency-violation-baseline.json"
        ),
        "packageResponsibilityMap": (
            "docs/architecture/package-responsibilities.json"
        ),
        "publicSurfaceInventory": (
            "docs/architecture/public-surface-inventory.json"
        ),
        "uiClassInventory": (
            "docs/architecture/ui-class-responsibilities.json"
        ),
        "uiToolClassInventory": (
            "docs/architecture/ui-tool-class-responsibilities.json"
        ),
        "cmakeGraphs": "docs/architecture/cmake-targets-<platform>.json",
    }


def projection_resolution_policy() -> dict[str, Any]:
    return {
        "candidateScope": "r1-g4-projections-without-confirmed-violation",
        "evidence": "uniquely-attributed-direct-includes-for-each-target-link",
        "successfulDisposition": "disproved-by-direct-include-attribution",
    }


def target_cycle_policy() -> dict[str, Any]:
    return {
        "dependencyKind": "direct-link-library",
        "scopes": ["core-owner-targets", "all-production-build-targets"],
        "productionSourceDirectories": list(
            dependency_contract.PRODUCTION_SOURCE_DIRECTORIES
        ),
        "excludedPathParts": sorted(dependency_contract.TEST_PATH_PARTS),
        "targetTypes": sorted(PRODUCT_TARGET_TYPES),
        "componentKind": "nontrivial-strongly-connected-component",
    }


def internal_header_policy() -> dict[str, Any]:
    return {
        "ownerScope": "complete-public-header-sets",
        "headerSelection": "external-include-without-export-macro",
        "referenceKind": "direct-include-from-outside-owner-source-directory",
        "publicationEvidence": "de-facto-only",
    }


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise StructuralBaselineError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise StructuralBaselineError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise StructuralBaselineError(
            f"expected a non-empty string for {description}"
        )
    return value


def _require_nonnegative_int(value: Any, description: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise StructuralBaselineError(
            f"expected a nonnegative integer for {description}"
        )
    return value


def _require_fields(
    value: dict[str, Any], expected: set[str], description: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise StructuralBaselineError(
            f"unexpected fields for {description}; "
            f"missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def _repository_inputs(repository_root: Path) -> dict[str, dict[str, Any]]:
    inputs = baseline_inputs()
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
            raise StructuralBaselineError(f"invalid CMake graph: {path}")
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
            if projection.get("status") != "requires-r1-g4-baseline":
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


def _target_link_observations(
    *,
    repository_root: Path,
    responsibility_map: dict[str, Any],
    ui_class_inventory: dict[str, Any],
    ui_tool_class_inventory: dict[str, Any],
    links: set[tuple[str, str]],
) -> dict[tuple[str, str], list[dict[str, str]]]:
    target_owners, responsibility_directories, target_sources = (
        dependency_contract._responsibility_data(responsibility_map)
    )
    public_paths, class_paths = dependency_contract._path_classifications(
        responsibility_map, ui_class_inventory, ui_tool_class_inventory
    )
    all_headers = dependency_contract._all_production_headers(repository_root)
    source_cache: dict[str, list[tuple[str, Path]]] = {}
    header_cache: dict[str, dict[str, list[str]]] = {}
    result: dict[tuple[str, str], list[dict[str, str]]] = {}

    for source_target, dependency_target in sorted(links):
        source_directory = target_sources[source_target]
        dependency_directory = target_sources[dependency_target]
        source_files = source_cache.setdefault(
            source_directory,
            dependency_contract._files_below(
                repository_root,
                source_directory,
                dependency_contract.SOURCE_SUFFIXES,
            ),
        )
        if dependency_directory not in header_cache:
            headers_by_name: dict[str, list[str]] = {}
            for header_path, _path in dependency_contract._files_below(
                repository_root,
                dependency_directory,
                dependency_contract.HEADER_SUFFIXES,
            ):
                headers_by_name.setdefault(
                    PurePosixPath(header_path).name, []
                ).append(header_path)
            header_cache[dependency_directory] = headers_by_name
        headers_by_name = header_cache[dependency_directory]
        resolved: set[tuple[str, str, str]] = set()
        for source_path, path in source_files:
            text = path.read_text(encoding="utf-8")
            for include in dependency_contract.INCLUDE_PATTERN.findall(text):
                name = PurePosixPath(include).name
                candidates = headers_by_name.get(name, [])
                suffix_matches = [
                    candidate
                    for candidate in candidates
                    if candidate == include or candidate.endswith(f"/{include}")
                ]
                if len(suffix_matches) == 1:
                    header_path = suffix_matches[0]
                elif len(candidates) == 1 and len(all_headers.get(name, [])) == 1:
                    header_path = candidates[0]
                else:
                    continue
                resolved.add((source_path, include, header_path))

        observations: list[dict[str, str]] = []
        for source_path, include, header_path in sorted(resolved):
            source_responsibilities, source_method = (
                dependency_contract._classify_path(
                    path=source_path,
                    target=source_target,
                    target_owners=target_owners,
                    responsibility_source_directories=(
                        responsibility_directories
                    ),
                    public_paths=public_paths,
                    class_paths=class_paths,
                )
            )
            dependency_responsibilities, dependency_method = (
                dependency_contract._classify_path(
                    path=header_path,
                    target=dependency_target,
                    target_owners=target_owners,
                    responsibility_source_directories=(
                        responsibility_directories
                    ),
                    public_paths=public_paths,
                    class_paths=class_paths,
                )
            )
            if (
                len(source_responsibilities) != 1
                or len(dependency_responsibilities) != 1
            ):
                raise StructuralBaselineError(
                    "direct include remains ambiguous for "
                    f"{source_target} -> {dependency_target}: "
                    f"{source_path} includes {header_path}"
                )
            observations.append(
                {
                    "sourceTarget": source_target,
                    "dependencyTarget": dependency_target,
                    "sourcePath": source_path,
                    "include": include,
                    "headerPath": header_path,
                    "sourceResponsibility": source_responsibilities[0],
                    "dependencyResponsibility": dependency_responsibilities[0],
                    "sourceAttribution": source_method,
                    "dependencyAttribution": dependency_method,
                }
            )
        result[(source_target, dependency_target)] = observations
    return result


def discover_projection_resolutions(
    *,
    repository_root: Path,
    policy: dict[str, Any],
    dependency_baseline: dict[str, Any],
    responsibility_map: dict[str, Any],
    ui_class_inventory: dict[str, Any],
    ui_tool_class_inventory: dict[str, Any],
    graphs: dict[str, dict[str, dict[str, Any]]],
) -> list[dict[str, Any]]:
    candidates = _candidate_links(policy)
    accounted_pairs = {
        (
            _require_string(entry.get("sourceResponsibility"), "source"),
            _require_string(entry.get("dependencyResponsibility"), "dependency"),
        )
        for key in ("violations", "unresolvedProjections")
        for item in _require_array(dependency_baseline.get(key), key)
        for entry in [_require_object(item, key)]
    }
    resolved_pairs = set(candidates) - accounted_pairs
    links = {link for pair in resolved_pairs for link in candidates[pair]}
    observations = _target_link_observations(
        repository_root=repository_root,
        responsibility_map=responsibility_map,
        ui_class_inventory=ui_class_inventory,
        ui_tool_class_inventory=ui_tool_class_inventory,
        links=links,
    )
    result: list[dict[str, Any]] = []
    for pair in sorted(resolved_pairs):
        missing = sorted(link for link in candidates[pair] if not observations[link])
        if missing:
            raise StructuralBaselineError(
                f"projection remains unresolved for {pair[0]} -> {pair[1]}; "
                f"target links without attributed includes: {missing}"
            )
        evidence = [
            entry
            for link in sorted(candidates[pair])
            for entry in observations[link]
        ]
        actual_pairs = sorted(
            {
                (
                    entry["sourceResponsibility"],
                    entry["dependencyResponsibility"],
                )
                for entry in evidence
            }
        )
        if pair in actual_pairs:
            raise StructuralBaselineError(
                f"resolved projection still has direct evidence: {pair}"
            )
        result.append(
            {
                "sourceResponsibility": pair[0],
                "dependencyResponsibility": pair[1],
                "status": "disproved-by-direct-include-attribution",
                "targetLinks": [
                    {
                        "sourceTarget": source_target,
                        "dependencyTarget": dependency_target,
                        "platforms": dependency_contract._target_link_platforms(
                            source_target=source_target,
                            dependency_target=dependency_target,
                            graphs=graphs,
                        ),
                    }
                    for source_target, dependency_target in sorted(
                        candidates[pair]
                    )
                ],
                "actualResponsibilityPairs": [
                    {
                        "sourceResponsibility": source,
                        "dependencyResponsibility": dependency,
                    }
                    for source, dependency in actual_pairs
                ],
                "directIncludes": evidence,
            }
        )
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
            if "external-include" not in evidence or "export-macro" in evidence:
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


def _manual_internal(entry: dict[str, Any], index: int) -> dict[str, Any]:
    if not MANUAL_INTERNAL_FIELDS.issubset(entry):
        raise StructuralBaselineError(
            f"missing manual internal-header fields for entry {index}"
        )
    owner = _require_string(entry.get("ownerTarget"), "internal header owner")
    return {
        "ownerTarget": owner,
        "owner": _require_string(entry.get("owner"), f"owner for {owner}"),
        "reason": _require_string(entry.get("reason"), f"reason for {owner}"),
        "maximumHeaders": _require_nonnegative_int(
            entry.get("maximumHeaders"), f"maximum headers for {owner}"
        ),
        "maximumDirectReferences": _require_nonnegative_int(
            entry.get("maximumDirectReferences"),
            f"maximum direct references for {owner}",
        ),
        "removalCondition": _require_string(
            entry.get("removalCondition"), f"removal condition for {owner}"
        ),
    }


def updated_baseline(
    baseline: dict[str, Any], *, repository_root: Path
) -> dict[str, Any]:
    inputs = _repository_inputs(repository_root)
    graphs = _load_graphs(repository_root)
    internal_discovery = discover_internal_headers(
        inputs["publicSurfaceInventory"]
    )
    manual_internal = [
        _manual_internal(_require_object(item, "internal baseline"), index)
        for index, item in enumerate(
            _require_array(
                baseline.get("internalHeaderBaseline"),
                "internal header baseline",
            )
        )
    ]
    if {entry["ownerTarget"] for entry in manual_internal} != set(
        internal_discovery
    ):
        raise StructuralBaselineError(
            "internal header owners do not match complete public surfaces"
        )
    cycle_baseline = _require_object(
        baseline.get("targetCycleBaseline"), "target cycle baseline"
    )
    maximum_components = _require_nonnegative_int(
        cycle_baseline.get("maximumComponents"), "maximum cycle components"
    )
    return {
        "schemaVersion": 1,
        "scope": BASELINE_SCOPE,
        "inputs": baseline_inputs(),
        "projectionResolutionPolicy": projection_resolution_policy(),
        "projectionResolutions": discover_projection_resolutions(
            repository_root=repository_root,
            policy=inputs["allowedDependencyPolicy"],
            dependency_baseline=inputs["dependencyViolationBaseline"],
            responsibility_map=inputs["packageResponsibilityMap"],
            ui_class_inventory=inputs["uiClassInventory"],
            ui_tool_class_inventory=inputs["uiToolClassInventory"],
            graphs=graphs,
        ),
        "targetCycleBaseline": {
            "policy": target_cycle_policy(),
            "owner": "R1-G6",
            "maximumComponents": maximum_components,
            "removalCondition": (
                "Production target relations remain acyclic through the "
                "R1 package split and final dependency policy validation."
            ),
            "scopes": discover_target_cycles(
                responsibility_map=inputs["packageResponsibilityMap"],
                graphs=graphs,
            ),
        },
        "internalHeaderPolicy": internal_header_policy(),
        "internalHeaderBaseline": [
            {**manual, **internal_discovery[manual["ownerTarget"]]}
            for manual in sorted(
                manual_internal, key=lambda entry: entry["ownerTarget"]
            )
        ],
    }


def validate_baseline(
    baseline: dict[str, Any], *, repository_root: Path
) -> None:
    _require_fields(
        baseline,
        {
            "schemaVersion",
            "scope",
            "inputs",
            "projectionResolutionPolicy",
            "projectionResolutions",
            "targetCycleBaseline",
            "internalHeaderPolicy",
            "internalHeaderBaseline",
        },
        "structural dependency baseline",
    )
    if baseline.get("schemaVersion") != 1 or baseline.get("scope") != BASELINE_SCOPE:
        raise StructuralBaselineError("structural baseline identity is invalid")
    if baseline.get("inputs") != baseline_inputs():
        raise StructuralBaselineError("structural baseline inputs are invalid")
    if baseline.get("projectionResolutionPolicy") != projection_resolution_policy():
        raise StructuralBaselineError("projection resolution policy is invalid")
    if baseline.get("internalHeaderPolicy") != internal_header_policy():
        raise StructuralBaselineError("internal header policy is invalid")

    expected = updated_baseline(baseline, repository_root=repository_root)
    if baseline.get("projectionResolutions") != expected["projectionResolutions"]:
        raise StructuralBaselineError(
            "projection resolutions are stale; run "
            "scripts/architecture/update_structural_dependency_baseline.py"
        )

    cycle = _require_object(
        baseline.get("targetCycleBaseline"), "target cycle baseline"
    )
    _require_fields(
        cycle,
        {
            "policy",
            "owner",
            "maximumComponents",
            "removalCondition",
            "scopes",
        },
        "target cycle baseline",
    )
    maximum = _require_nonnegative_int(
        cycle.get("maximumComponents"), "maximum cycle components"
    )
    observed = max(
        (
            len(platform["components"])
            for scope in expected["targetCycleBaseline"]["scopes"]
            for platform in scope["platforms"]
        ),
        default=0,
    )
    if observed > maximum:
        raise StructuralBaselineError(
            f"target cycles exceed approved maximum: {observed} > {maximum}"
        )
    if observed < maximum:
        raise StructuralBaselineError(
            f"target cycle baseline can be reduced: {observed} < {maximum}"
        )
    if cycle != expected["targetCycleBaseline"]:
        raise StructuralBaselineError(
            "target cycle evidence is stale; run "
            "scripts/architecture/update_structural_dependency_baseline.py"
        )

    entries = _require_array(
        baseline.get("internalHeaderBaseline"), "internal header baseline"
    )
    expected_by_owner = {
        entry["ownerTarget"]: entry
        for entry in expected["internalHeaderBaseline"]
    }
    owners: list[str] = []
    for index, item in enumerate(entries):
        entry = _require_object(item, f"internal header baseline {index}")
        _require_fields(
            entry,
            MANUAL_INTERNAL_FIELDS | DERIVED_INTERNAL_FIELDS,
            f"internal header baseline {index}",
        )
        manual = _manual_internal(entry, index)
        owner = manual["ownerTarget"]
        owners.append(owner)
        if owner not in expected_by_owner:
            raise StructuralBaselineError(f"unexpected internal header owner: {owner}")
        evidence = expected_by_owner[owner]
        header_count = len(evidence["headers"])
        reference_count = sum(
            len(header["consumerPaths"]) for header in evidence["headers"]
        )
        if (
            header_count > manual["maximumHeaders"]
            or reference_count > manual["maximumDirectReferences"]
        ):
            raise StructuralBaselineError(
                f"internal header references exceed approved maximum for {owner}"
            )
        if (
            header_count < manual["maximumHeaders"]
            or reference_count < manual["maximumDirectReferences"]
        ):
            raise StructuralBaselineError(
                f"internal header reference baseline can be reduced for {owner}"
            )
        if entry != evidence:
            raise StructuralBaselineError(
                "internal header evidence is stale; run "
                "scripts/architecture/update_structural_dependency_baseline.py"
            )
    if owners != sorted(set(owners)) or set(owners) != set(expected_by_owner):
        raise StructuralBaselineError(
            "internal header baselines must be complete, sorted, and unique"
        )


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Validate shared-target projection resolutions, target cycles, "
            "and de-facto internal header references."
        )
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/structural-dependency-baseline.json"
        ),
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        validate_baseline(
            load_baseline(options.baseline), repository_root=REPO_ROOT
        )
    except (OSError, StructuralBaselineError) as error:
        print(f"check-structural-dependency-baseline: {error}", file=sys.stderr)
        return 1
    print(f"structural dependency baseline verified: {options.baseline}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
