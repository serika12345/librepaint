#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path, PurePosixPath
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
PLATFORMS = ("macos", "linux", "ios", "android", "windows")
BASELINE_SCOPE = "r1-g4a-confirmed-reverse-dependency-baseline"
SOURCE_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".m", ".mm"}
)
HEADER_SUFFIXES = frozenset({".h", ".hh", ".hpp"})
PRODUCTION_SOURCE_DIRECTORIES = (
    "krita",
    "libs",
    "packaging",
    "pch",
    "plugins",
    "qmlmodules",
    "sdk",
    "winquirks",
)
TEST_PATH_PARTS = frozenset({"benchmarks", "test", "tests"})
INCLUDE_PATTERN = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.MULTILINE
)
AREA_TO_RESPONSIBILITY = {
    "application-configuration": "application-configuration",
    "application-orchestration": "application-orchestration",
    "window-workspace": "application-orchestration",
    "canvas-display": "canvas-presentation",
    "document-state": "document-lifecycle",
    "import-export": "import-export",
    "input-interpretation": "input-interpretation",
    "tool-invocation": "tool-invocation",
    "settings-presentation": "tool-invocation",
    "stroke-generation": "painting-rendering",
    "painting-execution": "painting-rendering",
}
MANUAL_VIOLATION_FIELDS = {
    "sourceResponsibility",
    "dependencyResponsibility",
    "owner",
    "reason",
    "maximumDirectIncludes",
    "removalCondition",
}
DERIVED_VIOLATION_FIELDS = {"targetLinks", "directIncludes"}
TARGET_LINK_FIELDS = {"sourceTarget", "dependencyTarget", "platforms"}
DIRECT_INCLUDE_FIELDS = {
    "sourceTarget",
    "dependencyTarget",
    "sourcePath",
    "include",
    "headerPath",
    "sourceAttribution",
    "dependencyAttribution",
}
UNRESOLVED_FIELDS = {
    "sourceResponsibility",
    "dependencyResponsibility",
    "status",
    "targetLinks",
    "ambiguousDirectIncludes",
}
AMBIGUOUS_INCLUDE_FIELDS = DIRECT_INCLUDE_FIELDS | {
    "sourceResponsibilities",
    "dependencyResponsibilities",
}


class DependencyBaselineError(RuntimeError):
    """Raised when the R1-G4 dependency baseline is inconsistent."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise DependencyBaselineError(
            f"{description} does not exist: {path}"
        ) from error
    except json.JSONDecodeError as error:
        raise DependencyBaselineError(
            f"invalid JSON in {path}: line {error.lineno}, "
            f"column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise DependencyBaselineError(f"expected a JSON object in {path}")
    return value


def load_baseline(path: Path) -> dict[str, Any]:
    return _load_json(path, "dependency violation baseline")


def load_input(path: Path, description: str) -> dict[str, Any]:
    return _load_json(path, description)


def baseline_inputs() -> dict[str, str]:
    return {
        "allowedDependencyPolicy": (
            "docs/architecture/allowed-package-dependencies.json"
        ),
        "packageResponsibilityMap": (
            "docs/architecture/package-responsibilities.json"
        ),
        "uiClassInventory": (
            "docs/architecture/ui-class-responsibilities.json"
        ),
        "uiToolClassInventory": (
            "docs/architecture/ui-tool-class-responsibilities.json"
        ),
        "cmakeGraphs": "docs/architecture/cmake-targets-<platform>.json",
    }


def attribution_policy() -> dict[str, Any]:
    return {
        "sourceScope": "production-files-below-selected-target-source-directory",
        "excludedPathParts": sorted(TEST_PATH_PARTS),
        "includeResolution": [
            "dependency-header-path-suffix",
            "repository-unique-header-basename",
        ],
        "responsibilityAttribution": [
            "unique-target-owner",
            "classified-public-or-class-path",
            "longest-responsibility-source-directory",
        ],
        "ambiguousHandling": "record-unresolved-projection",
        "noEvidenceHandling": "structural-projection-resolution-baseline",
    }


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise DependencyBaselineError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise DependencyBaselineError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise DependencyBaselineError(
            f"expected a non-empty string for {description}"
        )
    return value


def _require_fields(
    value: dict[str, Any], expected: set[str], description: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise DependencyBaselineError(
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
        raise DependencyBaselineError(
            f"{description} must be sorted and unique"
        )
    return entries


def _manual_violation(value: dict[str, Any], index: int) -> dict[str, Any]:
    expected = MANUAL_VIOLATION_FIELDS
    actual = set(value)
    if not expected.issubset(actual):
        raise DependencyBaselineError(
            f"missing manual fields for violation {index}: "
            f"{sorted(expected - actual)}"
        )
    source = _require_string(
        value.get("sourceResponsibility"),
        f"source responsibility for violation {index}",
    )
    dependency = _require_string(
        value.get("dependencyResponsibility"),
        f"dependency responsibility for violation {index}",
    )
    maximum = value.get("maximumDirectIncludes")
    if not isinstance(maximum, int) or isinstance(maximum, bool) or maximum < 1:
        raise DependencyBaselineError(
            f"maximumDirectIncludes must be positive for {source} -> {dependency}"
        )
    return {
        "sourceResponsibility": source,
        "dependencyResponsibility": dependency,
        "owner": _require_string(
            value.get("owner"), f"owner for {source} -> {dependency}"
        ),
        "reason": _require_string(
            value.get("reason"), f"reason for {source} -> {dependency}"
        ),
        "maximumDirectIncludes": maximum,
        "removalCondition": _require_string(
            value.get("removalCondition"),
            f"removal condition for {source} -> {dependency}",
        ),
    }


def _is_production_source(path: PurePosixPath) -> bool:
    return not any(part in TEST_PATH_PARTS for part in path.parts) and not (
        path.name.endswith(("_test.cpp", "_test.cc", "_test.cxx"))
    )


def _files_below(
    repository_root: Path,
    source_directory: str,
    suffixes: frozenset[str],
) -> list[tuple[str, Path]]:
    root = repository_root / source_directory
    if not root.is_dir():
        raise DependencyBaselineError(
            f"target source directory does not exist: {source_directory}"
        )
    result: list[tuple[str, Path]] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in suffixes:
            continue
        relative = path.relative_to(repository_root).as_posix()
        if _is_production_source(PurePosixPath(relative)):
            result.append((relative, path))
    return sorted(result)


def _all_production_headers(
    repository_root: Path,
) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for directory in PRODUCTION_SOURCE_DIRECTORIES:
        if not (repository_root / directory).is_dir():
            continue
        for relative, _path in _files_below(
            repository_root, directory, HEADER_SUFFIXES
        ):
            result.setdefault(PurePosixPath(relative).name, []).append(relative)
    return result


def _path_is_below(path: str, directory: str) -> bool:
    value = PurePosixPath(path)
    parent = PurePosixPath(directory)
    return value == parent or parent in value.parents


def _path_classifications(
    responsibility_map: dict[str, Any],
    ui_class_inventory: dict[str, Any],
    ui_tool_class_inventory: dict[str, Any],
) -> tuple[dict[str, set[str]], dict[str, set[str]]]:
    public_paths: dict[str, set[str]] = {}
    class_paths: dict[str, set[str]] = {}
    for item in _require_array(
        responsibility_map.get("responsibilities"),
        "responsibility map responsibilities",
    ):
        entry = _require_object(item, "mapped responsibility")
        identifier = _require_string(entry.get("id"), "mapped responsibility id")
        for path in _string_list(
            entry.get("publicHeaderPaths"),
            f"public header paths for {identifier}",
        ):
            public_paths.setdefault(path, set()).add(identifier)

        for path in _string_list(
            entry.get("reviewedSourcePaths"),
            f"reviewed source paths for {identifier}",
        ):
            class_paths.setdefault(path, set()).add(identifier)

    for inventory, description in (
        (ui_class_inventory, "UI class inventory"),
        (ui_tool_class_inventory, "UI tool class inventory"),
    ):
        for item in _require_array(inventory.get("classes"), description):
            entry = _require_object(item, f"class in {description}")
            area = _require_string(
                entry.get("responsibilityArea"),
                f"responsibility area in {description}",
            )
            try:
                responsibility = AREA_TO_RESPONSIBILITY[area]
            except KeyError as error:
                raise DependencyBaselineError(
                    f"unknown class responsibility area: {area}"
                ) from error
            paths = [
                _require_string(entry.get("header"), "classified class header"),
                *_string_list(
                    entry.get("implementationPaths"),
                    "classified class implementation paths",
                ),
            ]
            for path in paths:
                class_paths.setdefault(path, set()).add(responsibility)
    return public_paths, class_paths


def _responsibility_data(
    responsibility_map: dict[str, Any],
) -> tuple[
    dict[str, list[str]],
    dict[str, list[str]],
    dict[str, str],
]:
    target_owners: dict[str, list[str]] = {}
    source_directories: dict[str, list[str]] = {}
    for item in _require_array(
        responsibility_map.get("responsibilities"),
        "responsibility map responsibilities",
    ):
        entry = _require_object(item, "mapped responsibility")
        identifier = _require_string(entry.get("id"), "mapped responsibility id")
        source_directories[identifier] = _string_list(
            entry.get("sourceDirectories"),
            f"source directories for {identifier}",
        )
        for target in _string_list(
            entry.get("ownerTargets"), f"owner targets for {identifier}"
        ):
            target_owners.setdefault(target, []).append(identifier)
    for owners in target_owners.values():
        owners.sort()

    target_sources: dict[str, str] = {}
    for item in _require_array(
        responsibility_map.get("targetRelations"), "target relations"
    ):
        entry = _require_object(item, "target relation")
        target_sources[_require_string(entry.get("name"), "target name")] = (
            _require_string(entry.get("sourceDirectory"), "target source directory")
        )
    return target_owners, source_directories, target_sources


def _classify_path(
    *,
    path: str,
    target: str,
    target_owners: dict[str, list[str]],
    responsibility_source_directories: dict[str, list[str]],
    public_paths: dict[str, set[str]],
    class_paths: dict[str, set[str]],
) -> tuple[list[str], str]:
    owners = target_owners[target]
    if len(owners) == 1:
        return owners, "unique-owner-target"
    public_matches = sorted(set(owners) & public_paths.get(path, set()))
    if len(public_matches) == 1:
        return public_matches, "classified-public-path"
    class_matches = sorted(set(owners) & class_paths.get(path, set()))
    if len(class_matches) == 1:
        return class_matches, "classified-class-path"

    directory_matches: list[tuple[int, str]] = []
    for owner in owners:
        for directory in responsibility_source_directories[owner]:
            if _path_is_below(path, directory):
                directory_matches.append(
                    (len(PurePosixPath(directory).parts), owner)
                )
    if directory_matches:
        longest = max(length for length, _owner in directory_matches)
        matched_owners = sorted(
            {
                owner
                for length, owner in directory_matches
                if length == longest
            }
        )
        if len(matched_owners) == 1:
            return matched_owners, "longest-responsibility-source-directory"
    return owners, "shared-owner-target"


def _target_link_platforms(
    *,
    source_target: str,
    dependency_target: str,
    graphs: dict[str, dict[str, dict[str, Any]]],
) -> list[str]:
    result = []
    for platform in PLATFORMS:
        target = graphs[platform].get(source_target)
        if target is None:
            continue
        dependencies = _require_array(
            target.get("dependencies"),
            f"dependencies for {source_target} on {platform}",
        )
        if dependency_target in dependencies:
            result.append(platform)
    if not result:
        raise DependencyBaselineError(
            f"target link is absent from every graph: "
            f"{source_target} -> {dependency_target}"
        )
    return result


def _load_graphs(
    graph_directory: Path,
) -> dict[str, dict[str, dict[str, Any]]]:
    result: dict[str, dict[str, dict[str, Any]]] = {}
    for platform in PLATFORMS:
        path = graph_directory / f"cmake-targets-{platform}.json"
        graph = _load_json(path, f"{platform} CMake target graph")
        if graph.get("schemaVersion") != 1 or graph.get("platform") != platform:
            raise DependencyBaselineError(f"invalid CMake target graph: {path}")
        result[platform] = {
            _require_string(entry.get("name"), f"target name in {path}"): entry
            for item in _require_array(graph.get("targets"), f"targets in {path}")
            for entry in [_require_object(item, f"target in {path}")]
        }
    return result


def discover_baseline_evidence(
    *,
    repository_root: Path,
    policy: dict[str, Any],
    responsibility_map: dict[str, Any],
    ui_class_inventory: dict[str, Any],
    ui_tool_class_inventory: dict[str, Any],
    graph_directory: Path,
) -> tuple[
    dict[tuple[str, str], dict[str, list[dict[str, Any]]]],
    dict[tuple[str, str], dict[str, list[dict[str, Any]]]],
]:
    if (
        policy.get("schemaVersion") != 1
        or policy.get("scope")
        != "r1-package-responsibility-dependency-policy"
    ):
        raise DependencyBaselineError("invalid allowed dependency policy")
    if (
        responsibility_map.get("schemaVersion") != 1
        or responsibility_map.get("scope")
        != "current-production-package-responsibilities"
    ):
        raise DependencyBaselineError("invalid package responsibility map")
    target_owners, responsibility_directories, target_sources = (
        _responsibility_data(responsibility_map)
    )
    public_paths, class_paths = _path_classifications(
        responsibility_map, ui_class_inventory, ui_tool_class_inventory
    )
    graphs = _load_graphs(graph_directory)
    repository_headers_by_name = _all_production_headers(repository_root)
    allowed_pairs = {
        (
            _require_string(entry.get("id"), "policy responsibility id"),
            _require_string(
                dependency.get("responsibility"),
                "allowed dependency responsibility",
            ),
        )
        for item in _require_array(
            policy.get("responsibilities"), "policy responsibilities"
        )
        for entry in [_require_object(item, "policy responsibility")]
        for dependency_item in _require_array(
            entry.get("allowedDependencies"), "allowed dependencies"
        )
        for dependency in [
            _require_object(dependency_item, "allowed dependency")
        ]
    }

    candidate_links: dict[tuple[str, str], set[tuple[str, str]]] = {}
    relevant_target_edges: set[tuple[str, str]] = set()
    for edge_item in _require_array(
        policy.get("currentTargetEdges"), "current target edges"
    ):
        edge = _require_object(edge_item, "current target edge")
        source_target = _require_string(
            edge.get("sourceTarget"), "current edge source target"
        )
        dependency_target = _require_string(
            edge.get("dependencyTarget"), "current edge dependency target"
        )
        for projection_item in _require_array(
            edge.get("projections"), "current edge projections"
        ):
            projection = _require_object(
                projection_item, "current edge projection"
            )
            if projection.get("status") != "requires-r1-g4-baseline":
                continue
            pair = (
                _require_string(
                    projection.get("sourceResponsibility"),
                    "projection source responsibility",
                ),
                _require_string(
                    projection.get("dependencyResponsibility"),
                    "projection dependency responsibility",
                ),
            )
            candidate_links.setdefault(pair, set()).add(
                (source_target, dependency_target)
            )
            relevant_target_edges.add((source_target, dependency_target))

    target_links_by_pair: dict[
        tuple[str, str], list[dict[str, Any]]
    ] = {
        pair: [
            {
                "sourceTarget": source_target,
                "dependencyTarget": dependency_target,
                "platforms": _target_link_platforms(
                    source_target=source_target,
                    dependency_target=dependency_target,
                    graphs=graphs,
                ),
            }
            for source_target, dependency_target in sorted(links)
        ]
        for pair, links in candidate_links.items()
    }
    direct_includes: dict[tuple[str, str], set[tuple[str, ...]]] = {}
    ambiguous_includes: dict[tuple[str, str], set[tuple[Any, ...]]] = {}
    source_cache: dict[str, list[tuple[str, Path]]] = {}
    header_cache: dict[str, dict[str, list[str]]] = {}

    for source_target, dependency_target in sorted(relevant_target_edges):
        source_directory = target_sources[source_target]
        dependency_directory = target_sources[dependency_target]
        if source_directory not in source_cache:
            source_cache[source_directory] = _files_below(
                repository_root, source_directory, SOURCE_SUFFIXES
            )
        source_files = source_cache[source_directory]
        if dependency_directory not in header_cache:
            headers_by_name: dict[str, list[str]] = {}
            for header_path, _path in _files_below(
                repository_root, dependency_directory, HEADER_SUFFIXES
            ):
                headers_by_name.setdefault(
                    PurePosixPath(header_path).name, []
                ).append(header_path)
            header_cache[dependency_directory] = headers_by_name
        headers_by_name = header_cache[dependency_directory]

        resolved: set[tuple[str, str, str]] = set()
        for source_path, path in source_files:
            text = path.read_text(encoding="utf-8")
            for include in INCLUDE_PATTERN.findall(text):
                name = PurePosixPath(include).name
                candidates = headers_by_name.get(name, [])
                suffix_matches = [
                    candidate
                    for candidate in candidates
                    if candidate == include or candidate.endswith(f"/{include}")
                ]
                if len(suffix_matches) == 1:
                    header_path = suffix_matches[0]
                elif (
                    len(candidates) == 1
                    and len(repository_headers_by_name.get(name, [])) == 1
                ):
                    header_path = candidates[0]
                else:
                    continue
                resolved.add((source_path, include, header_path))

        for source_path, include, header_path in sorted(resolved):
            source_responsibilities, source_method = _classify_path(
                path=source_path,
                target=source_target,
                target_owners=target_owners,
                responsibility_source_directories=responsibility_directories,
                public_paths=public_paths,
                class_paths=class_paths,
            )
            dependency_responsibilities, dependency_method = _classify_path(
                path=header_path,
                target=dependency_target,
                target_owners=target_owners,
                responsibility_source_directories=responsibility_directories,
                public_paths=public_paths,
                class_paths=class_paths,
            )
            for source_responsibility in source_responsibilities:
                for dependency_responsibility in dependency_responsibilities:
                    pair = (source_responsibility, dependency_responsibility)
                    if (
                        source_responsibility == dependency_responsibility
                        or pair in allowed_pairs
                        or pair not in candidate_links
                    ):
                        continue
                    if (
                        len(source_responsibilities) == 1
                        and len(dependency_responsibilities) == 1
                    ):
                        direct_includes.setdefault(pair, set()).add(
                            (
                                source_target,
                                dependency_target,
                                source_path,
                                include,
                                header_path,
                                source_method,
                                dependency_method,
                            )
                        )
                    else:
                        ambiguous_includes.setdefault(pair, set()).add(
                            (
                                source_target,
                                dependency_target,
                                source_path,
                                include,
                                header_path,
                                source_method,
                                dependency_method,
                                tuple(source_responsibilities),
                                tuple(dependency_responsibilities),
                            )
                        )

    confirmed: dict[tuple[str, str], dict[str, list[dict[str, Any]]]] = {}
    unresolved: dict[tuple[str, str], dict[str, list[dict[str, Any]]]] = {}
    for pair in sorted(candidate_links):
        evidence = [
            {
                "sourceTarget": value[0],
                "dependencyTarget": value[1],
                "sourcePath": value[2],
                "include": value[3],
                "headerPath": value[4],
                "sourceAttribution": value[5],
                "dependencyAttribution": value[6],
            }
            for value in sorted(direct_includes.get(pair, set()))
        ]
        if evidence:
            confirmed[pair] = {
                "targetLinks": target_links_by_pair[pair],
                "directIncludes": evidence,
            }
            continue
        ambiguous = [
            {
                "sourceTarget": value[0],
                "dependencyTarget": value[1],
                "sourcePath": value[2],
                "include": value[3],
                "headerPath": value[4],
                "sourceAttribution": value[5],
                "dependencyAttribution": value[6],
                "sourceResponsibilities": list(value[7]),
                "dependencyResponsibilities": list(value[8]),
            }
            for value in sorted(ambiguous_includes.get(pair, set()))
        ]
        if not ambiguous:
            continue
        unresolved[pair] = {
            "targetLinks": target_links_by_pair[pair],
            "ambiguousDirectIncludes": ambiguous,
        }
    return confirmed, unresolved


def updated_baseline(
    baseline: dict[str, Any],
    *,
    repository_root: Path,
    policy: dict[str, Any],
    responsibility_map: dict[str, Any],
    ui_class_inventory: dict[str, Any],
    ui_tool_class_inventory: dict[str, Any],
    graph_directory: Path,
) -> dict[str, Any]:
    manual_entries = [
        _manual_violation(
            _require_object(item, f"violation {index}"), index
        )
        for index, item in enumerate(
            _require_array(baseline.get("violations"), "violations")
        )
    ]
    manual_entries.sort(
        key=lambda entry: (
            entry["sourceResponsibility"],
            entry["dependencyResponsibility"],
        )
    )
    confirmed, unresolved = discover_baseline_evidence(
        repository_root=repository_root,
        policy=policy,
        responsibility_map=responsibility_map,
        ui_class_inventory=ui_class_inventory,
        ui_tool_class_inventory=ui_tool_class_inventory,
        graph_directory=graph_directory,
    )
    manual_pairs = {
        (entry["sourceResponsibility"], entry["dependencyResponsibility"])
        for entry in manual_entries
    }
    missing_pairs = set(confirmed) - manual_pairs
    if missing_pairs:
        raise DependencyBaselineError(
            "confirmed violation pairs are missing manual review data; "
            f"missing={sorted(missing_pairs)}"
        )
    violations = []
    for manual in manual_entries:
        pair = (
            manual["sourceResponsibility"],
            manual["dependencyResponsibility"],
        )
        if pair not in confirmed:
            continue
        violations.append({**manual, **confirmed[pair]})
    unresolved_entries = [
        {
            "sourceResponsibility": pair[0],
            "dependencyResponsibility": pair[1],
            "status": (
                "ambiguous-direct-include"
                if evidence["ambiguousDirectIncludes"]
                else "no-attributed-direct-include"
            ),
            **evidence,
        }
        for pair, evidence in sorted(unresolved.items())
    ]
    return {
        "schemaVersion": 1,
        "scope": BASELINE_SCOPE,
        "inputs": baseline_inputs(),
        "attributionPolicy": attribution_policy(),
        "violations": violations,
        "unresolvedProjections": unresolved_entries,
    }


def validate_baseline(
    baseline: dict[str, Any],
    *,
    repository_root: Path,
    policy_path: Path,
    responsibility_map_path: Path,
    ui_class_inventory_path: Path,
    ui_tool_class_inventory_path: Path,
    graph_directory: Path,
) -> None:
    _require_fields(
        baseline,
        {
            "schemaVersion",
            "scope",
            "inputs",
            "attributionPolicy",
            "violations",
            "unresolvedProjections",
        },
        "dependency violation baseline",
    )
    if baseline.get("schemaVersion") != 1:
        raise DependencyBaselineError("baseline schemaVersion must be 1")
    if baseline.get("scope") != BASELINE_SCOPE:
        raise DependencyBaselineError("dependency baseline has invalid scope")
    if baseline.get("inputs") != baseline_inputs():
        raise DependencyBaselineError("dependency baseline inputs are invalid")
    if baseline.get("attributionPolicy") != attribution_policy():
        raise DependencyBaselineError("dependency attribution policy is invalid")

    policy = load_input(policy_path, "allowed dependency policy")
    responsibility_map = load_input(
        responsibility_map_path, "package responsibility map"
    )
    ui_classes = load_input(ui_class_inventory_path, "UI class inventory")
    ui_tool_classes = load_input(
        ui_tool_class_inventory_path, "UI tool class inventory"
    )
    entries = _require_array(baseline.get("violations"), "violations")
    manual_entries: list[dict[str, Any]] = []
    pairs: list[tuple[str, str]] = []
    for index, item in enumerate(entries):
        entry = _require_object(item, f"violation {index}")
        _require_fields(
            entry,
            MANUAL_VIOLATION_FIELDS | DERIVED_VIOLATION_FIELDS,
            f"violation {index}",
        )
        manual = _manual_violation(entry, index)
        manual_entries.append(manual)
        pairs.append(
            (
                manual["sourceResponsibility"],
                manual["dependencyResponsibility"],
            )
        )
    if pairs != sorted(set(pairs)):
        raise DependencyBaselineError(
            "violation pairs must be sorted and unique"
        )

    confirmed, unresolved = discover_baseline_evidence(
        repository_root=repository_root,
        policy=policy,
        responsibility_map=responsibility_map,
        ui_class_inventory=ui_classes,
        ui_tool_class_inventory=ui_tool_classes,
        graph_directory=graph_directory,
    )
    if set(pairs) != set(confirmed):
        raise DependencyBaselineError(
            "confirmed violation pairs do not match discovery"
        )
    for entry, manual in zip(entries, manual_entries):
        pair = (
            manual["sourceResponsibility"],
            manual["dependencyResponsibility"],
        )
        observed_count = len(confirmed[pair]["directIncludes"])
        maximum = manual["maximumDirectIncludes"]
        if observed_count > maximum:
            raise DependencyBaselineError(
                f"confirmed violation exceeds approved maximum for "
                f"{pair[0]} -> {pair[1]}: {observed_count} > {maximum}"
            )
        if observed_count < maximum:
            raise DependencyBaselineError(
                f"confirmed violation baseline can be reduced for "
                f"{pair[0]} -> {pair[1]}: {observed_count} < {maximum}"
            )
        if (
            entry.get("targetLinks") != confirmed[pair]["targetLinks"]
            or entry.get("directIncludes") != confirmed[pair]["directIncludes"]
        ):
            raise DependencyBaselineError(
                "dependency violation evidence is stale; run "
                "scripts/architecture/update_dependency_violation_baseline.py"
            )

    expected_unresolved = updated_baseline(
        baseline,
        repository_root=repository_root,
        policy=policy,
        responsibility_map=responsibility_map,
        ui_class_inventory=ui_classes,
        ui_tool_class_inventory=ui_tool_classes,
        graph_directory=graph_directory,
    )["unresolvedProjections"]
    unresolved_entries = _require_array(
        baseline.get("unresolvedProjections"), "unresolved projections"
    )
    for index, item in enumerate(unresolved_entries):
        entry = _require_object(item, f"unresolved projection {index}")
        _require_fields(
            entry, UNRESOLVED_FIELDS, f"unresolved projection {index}"
        )
        for ambiguous_index, ambiguous_item in enumerate(
            _require_array(
                entry.get("ambiguousDirectIncludes"),
                f"ambiguous direct includes for unresolved projection {index}",
            )
        ):
            _require_fields(
                _require_object(
                    ambiguous_item,
                    f"ambiguous include {index}/{ambiguous_index}",
                ),
                AMBIGUOUS_INCLUDE_FIELDS,
                f"ambiguous include {index}/{ambiguous_index}",
            )
    if unresolved_entries != expected_unresolved:
        raise DependencyBaselineError(
            "unresolved dependency projections are stale; run "
            "scripts/architecture/update_dependency_violation_baseline.py"
        )


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Validate confirmed reverse-dependency evidence and reviewed "
            "maximum scopes."
        )
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/dependency-violation-baseline.json"
        ),
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
    parser.add_argument(
        "--ui-class-inventory",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
        ),
    )
    parser.add_argument(
        "--ui-tool-class-inventory",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
        ),
    )
    parser.add_argument(
        "--graph-directory",
        type=Path,
        default=REPO_ROOT / "docs/architecture",
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        validate_baseline(
            load_baseline(options.baseline),
            repository_root=REPO_ROOT,
            policy_path=options.policy,
            responsibility_map_path=options.responsibility_map,
            ui_class_inventory_path=options.ui_class_inventory,
            ui_tool_class_inventory_path=options.ui_tool_class_inventory,
            graph_directory=options.graph_directory,
        )
    except (OSError, DependencyBaselineError) as error:
        print(f"check-dependency-violation-baseline: {error}", file=sys.stderr)
        return 1
    print(f"dependency violation baseline verified: {options.baseline}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
