#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path, PurePosixPath
from typing import Any


SCRIPT_DIRECTORY = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIRECTORY.parents[1]
PLATFORMS = ("macos", "linux", "ios", "android", "windows")
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
RESPONSIBILITY_IDS = (
    "application-orchestration",
    "canvas-presentation",
    "document-lifecycle",
    "import-export",
    "input-interpretation",
    "painting-rendering",
    "plugin-infrastructure",
    "resource-management",
    "tool-invocation",
)
MANUAL_RESPONSIBILITY_FIELDS = {
    "id",
    "responsibility",
    "sourceDirectories",
    "ownerTargets",
    "publicHeaderOwnerTargets",
    "reviewedPublicHeaderPaths",
    "reviewedSourcePaths",
    "uiClassAreas",
    "uiToolClassAreas",
    "majorClasses",
    "pluginScope",
    "pluginFeatureOwners",
}
DERIVED_RESPONSIBILITY_FIELDS = {
    "publicHeaderPaths",
    "classNames",
    "pluginIds",
    "pluginServiceTypes",
}


class ResponsibilityMapError(RuntimeError):
    """Raised when the current package responsibility map is inconsistent."""


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise ResponsibilityMapError(
            f"{description} does not exist: {path}"
        ) from error
    except json.JSONDecodeError as error:
        raise ResponsibilityMapError(
            f"invalid JSON in {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise ResponsibilityMapError(f"expected a JSON object in {path}")
    return value


def load_map(path: Path) -> dict[str, Any]:
    return _load_json(path, "package responsibility map")


def relation_policy() -> dict[str, Any]:
    return {
        "graphFiles": "docs/architecture/cmake-targets-<platform>.json",
        "dependencyKind": "direct-link-library",
        "platformAggregation": "union",
        "targetScope": "production-source-directories-excluding-tests",
    }


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ResponsibilityMapError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise ResponsibilityMapError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ResponsibilityMapError(
            f"expected a non-empty string for {description}"
        )
    return value


def _require_fields(
    value: dict[str, Any], expected: set[str], description: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise ResponsibilityMapError(
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
        raise ResponsibilityMapError(
            f"{description} must be sorted and unique"
        )
    return entries


def _load_target_graphs(
    graph_directory: Path,
) -> dict[str, dict[str, dict[str, Any]]]:
    result: dict[str, dict[str, dict[str, Any]]] = {}
    for platform in PLATFORMS:
        path = graph_directory / f"cmake-targets-{platform}.json"
        graph = _load_json(path, f"{platform} CMake target graph")
        if graph.get("schemaVersion") != 1 or graph.get("platform") != platform:
            raise ResponsibilityMapError(f"invalid CMake target graph: {path}")
        targets = _require_array(graph.get("targets"), f"targets in {path}")
        result[platform] = {
            _require_string(
                _require_object(target, f"target in {path}").get("name"),
                f"target name in {path}",
            ): target
            for target in targets
        }
    return result


def _is_production_target(target: dict[str, Any]) -> bool:
    source_directory = target.get("sourceDirectory")
    if not isinstance(source_directory, str):
        return False
    path = PurePosixPath(source_directory)
    return (
        bool(path.parts)
        and path.parts[0] in PRODUCTION_SOURCE_DIRECTORIES
        and not any(part in TEST_PATH_PARTS for part in path.parts)
    )


def build_target_relations(
    *,
    owner_targets: set[str],
    graph_directory: Path,
) -> list[dict[str, Any]]:
    graphs = _load_target_graphs(graph_directory)
    production_targets = {
        name
        for graph in graphs.values()
        for name, target in graph.items()
        if _is_production_target(target)
    }
    relations: list[dict[str, Any]] = []
    for owner_target in sorted(owner_targets):
        platforms = [
            platform for platform in PLATFORMS if owner_target in graphs[platform]
        ]
        if not platforms:
            raise ResponsibilityMapError(
                f"owner target is absent from every graph: {owner_target}"
            )
        source_directories = {
            _require_string(
                graphs[platform][owner_target].get("sourceDirectory"),
                f"source directory for {owner_target} on {platform}",
            )
            for platform in platforms
        }
        if len(source_directories) != 1:
            raise ResponsibilityMapError(
                f"owner target changes source directory: {owner_target}"
            )
        target_types = sorted(
            {
                _require_string(
                    graphs[platform][owner_target].get("type"),
                    f"target type for {owner_target} on {platform}",
                )
                for platform in platforms
            }
        )
        dependencies = sorted(
            {
                dependency
                for platform in platforms
                for dependency in _string_list(
                    graphs[platform][owner_target].get("dependencies"),
                    f"dependencies for {owner_target} on {platform}",
                )
                if dependency in production_targets
            }
        )
        consumers = sorted(
            {
                target_name
                for graph in graphs.values()
                for target_name, target in graph.items()
                if _is_production_target(target)
                and owner_target
                in _require_array(
                    target.get("dependencies"),
                    f"dependencies for target {target_name}",
                )
            }
        )
        relations.append(
            {
                "name": owner_target,
                "sourceDirectory": next(iter(source_directories)),
                "platforms": platforms,
                "types": target_types,
                "repositoryDependencies": dependencies,
                "repositoryConsumers": consumers,
            }
        )
    return relations


def _manual_responsibility(entry: dict[str, Any], index: int) -> dict[str, Any]:
    identifier = _require_string(
        entry.get("id"), f"responsibility id {index}"
    )
    return {
        "id": identifier,
        "responsibility": _require_string(
            entry.get("responsibility"), f"responsibility for {identifier}"
        ),
        "sourceDirectories": _string_list(
            entry.get("sourceDirectories"),
            f"source directories for {identifier}",
        ),
        "ownerTargets": _string_list(
            entry.get("ownerTargets"), f"owner targets for {identifier}"
        ),
        "publicHeaderOwnerTargets": _string_list(
            entry.get("publicHeaderOwnerTargets"),
            f"public header owners for {identifier}",
        ),
        "reviewedPublicHeaderPaths": _string_list(
            entry.get("reviewedPublicHeaderPaths"),
            f"reviewed public header paths for {identifier}",
        ),
        "reviewedSourcePaths": _string_list(
            entry.get("reviewedSourcePaths"),
            f"reviewed source paths for {identifier}",
        ),
        "uiClassAreas": _string_list(
            entry.get("uiClassAreas"), f"UI class areas for {identifier}"
        ),
        "uiToolClassAreas": _string_list(
            entry.get("uiToolClassAreas"),
            f"UI tool class areas for {identifier}",
        ),
        "majorClasses": _string_list(
            entry.get("majorClasses"), f"major classes for {identifier}"
        ),
        "pluginScope": _require_string(
            entry.get("pluginScope"), f"plugin scope for {identifier}"
        ),
        "pluginFeatureOwners": _string_list(
            entry.get("pluginFeatureOwners"),
            f"plugin feature owners for {identifier}",
        ),
    }


def updated_map(
    responsibility_map: dict[str, Any],
    *,
    graph_directory: Path,
    public_surface_inventory_path: Path,
    ui_class_inventory_path: Path,
    ui_tool_class_inventory_path: Path,
) -> dict[str, Any]:
    public_surface = _load_json(
        public_surface_inventory_path, "public-surface inventory"
    )
    ui_classes = _load_json(ui_class_inventory_path, "UI class inventory")
    ui_tool_classes = _load_json(
        ui_tool_class_inventory_path, "UI tool class inventory"
    )

    public_headers_by_owner = {
        _require_string(entry.get("ownerTarget"), "public header owner"): [
            _require_string(header.get("path"), "public header path")
            for header in _require_array(entry.get("headers"), "public headers")
        ]
        for item in _require_array(
            public_surface.get("publicHeaderSets"), "publicHeaderSets"
        )
        for entry in [_require_object(item, "public header set")]
    }
    major_classes = {
        _require_string(entry.get("name"), "major class name"): entry
        for item in _require_array(
            public_surface.get("majorClasses"), "majorClasses"
        )
        for entry in [_require_object(item, "major class")]
    }
    top_level_classes = [
        _require_object(item, "UI class")
        for item in _require_array(ui_classes.get("classes"), "UI classes")
    ]
    tool_classes = [
        _require_object(item, "UI tool class")
        for item in _require_array(
            ui_tool_classes.get("classes"), "UI tool classes"
        )
    ]
    plugins = [
        _require_object(item, "plugin")
        for item in _require_array(public_surface.get("plugins"), "plugins")
    ]

    responsibility_entries = _require_array(
        responsibility_map.get("responsibilities"), "responsibilities"
    )
    responsibilities: list[dict[str, Any]] = []
    owner_targets: set[str] = set()
    for index, item in enumerate(responsibility_entries):
        entry = _require_object(item, f"responsibility {index}")
        manual = _manual_responsibility(entry, index)
        owner_targets.update(manual["ownerTargets"])

        selected_top_level = [
            candidate
            for candidate in top_level_classes
            if candidate.get("responsibilityArea") in manual["uiClassAreas"]
        ]
        selected_tool = [
            candidate
            for candidate in tool_classes
            if candidate.get("responsibilityArea")
            in manual["uiToolClassAreas"]
        ]
        selected_major = [major_classes[name] for name in manual["majorClasses"]]
        class_names = sorted(
            {
                _require_string(candidate.get("name"), "classified class name")
                for candidate in [
                    *selected_top_level,
                    *selected_tool,
                    *selected_major,
                ]
            }
        )
        public_header_paths = {
            _require_string(candidate.get("header"), "classified class header")
            for candidate in [
                *selected_top_level,
                *selected_tool,
                *selected_major,
            ]
        }
        for owner in manual["publicHeaderOwnerTargets"]:
            public_header_paths.update(public_headers_by_owner[owner])
        public_header_paths.update(manual["reviewedPublicHeaderPaths"])

        if manual["pluginScope"] == "all-registrations":
            selected_plugins = plugins
        else:
            selected_plugins = [
                plugin
                for plugin in plugins
                if plugin.get("featureOwner")
                in manual["pluginFeatureOwners"]
            ]
        plugin_ids = sorted(
            _require_string(plugin.get("id"), "plugin id")
            for plugin in selected_plugins
        )
        plugin_service_types = sorted(
            {
                _require_string(service_type, "plugin service type")
                for plugin in selected_plugins
                for service_type in _require_array(
                    plugin.get("serviceTypes"), "plugin service types"
                )
            }
        )
        responsibilities.append(
            {
                **manual,
                "publicHeaderPaths": sorted(public_header_paths),
                "classNames": class_names,
                "pluginIds": plugin_ids,
                "pluginServiceTypes": plugin_service_types,
            }
        )

    return {
        "schemaVersion": 1,
        "scope": "current-production-package-responsibilities",
        "platforms": list(PLATFORMS),
        "relationPolicy": relation_policy(),
        "responsibilities": sorted(
            responsibilities, key=lambda entry: entry["id"]
        ),
        "targetRelations": build_target_relations(
            owner_targets=owner_targets,
            graph_directory=graph_directory,
        ),
    }


def validate_map(
    responsibility_map: dict[str, Any],
    *,
    repository_root: Path,
    graph_directory: Path,
    public_surface_inventory_path: Path,
    ui_class_inventory_path: Path,
    ui_tool_class_inventory_path: Path,
) -> None:
    _require_fields(
        responsibility_map,
        {
            "schemaVersion",
            "scope",
            "platforms",
            "relationPolicy",
            "responsibilities",
            "targetRelations",
        },
        "package responsibility map",
    )
    if responsibility_map.get("schemaVersion") != 1:
        raise ResponsibilityMapError(
            "package responsibility map schemaVersion must be 1"
        )
    if (
        responsibility_map.get("scope")
        != "current-production-package-responsibilities"
    ):
        raise ResponsibilityMapError("package responsibility map has invalid scope")
    if responsibility_map.get("platforms") != list(PLATFORMS):
        raise ResponsibilityMapError(
            "package responsibility map must cover all platforms"
        )
    if responsibility_map.get("relationPolicy") != relation_policy():
        raise ResponsibilityMapError(
            "relationPolicy must match the CMake relation policy"
        )

    public_surface = _load_json(
        public_surface_inventory_path, "public-surface inventory"
    )
    ui_classes = _load_json(ui_class_inventory_path, "UI class inventory")
    ui_tool_classes = _load_json(
        ui_tool_class_inventory_path, "UI tool class inventory"
    )
    known_public_header_owners = {
        _require_string(entry.get("ownerTarget"), "public header owner")
        for item in _require_array(
            public_surface.get("publicHeaderSets"), "publicHeaderSets"
        )
        for entry in [_require_object(item, "public header set")]
    }
    known_public_headers = {
        _require_string(header.get("path"), "public header path")
        for item in _require_array(
            public_surface.get("publicHeaderSets"), "publicHeaderSets"
        )
        for header_set in [_require_object(item, "public header set")]
        for header_item in _require_array(header_set.get("headers"), "headers")
        for header in [_require_object(header_item, "public header")]
    }
    known_ui_areas = {
        _require_string(entry.get("id"), "UI class area")
        for item in _require_array(
            ui_classes.get("responsibilityAreas"), "UI responsibility areas"
        )
        for entry in [_require_object(item, "UI responsibility area")]
    }
    known_tool_areas = {
        _require_string(entry.get("id"), "UI tool class area")
        for item in _require_array(
            ui_tool_classes.get("responsibilityAreas"),
            "UI tool responsibility areas",
        )
        for entry in [_require_object(item, "UI tool responsibility area")]
    }
    known_major_classes = {
        _require_string(entry.get("name"), "major class name")
        for item in _require_array(
            public_surface.get("majorClasses"), "majorClasses"
        )
        for entry in [_require_object(item, "major class")]
    }
    known_feature_owners = {
        _require_string(plugin.get("featureOwner"), "plugin feature owner")
        for item in _require_array(public_surface.get("plugins"), "plugins")
        for plugin in [_require_object(item, "plugin")]
    }

    entries = _require_array(
        responsibility_map.get("responsibilities"), "responsibilities"
    )
    identifiers: list[str] = []
    ui_area_assignments: list[str] = []
    tool_area_assignments: list[str] = []
    major_class_assignments: list[str] = []
    feature_owner_assignments: list[str] = []
    reviewed_header_assignments: list[str] = []
    reviewed_source_assignments: list[str] = []
    all_plugin_scopes = 0
    owner_targets: set[str] = set()
    for index, item in enumerate(entries):
        entry = _require_object(item, f"responsibility {index}")
        _require_fields(
            entry,
            MANUAL_RESPONSIBILITY_FIELDS | DERIVED_RESPONSIBILITY_FIELDS,
            f"responsibility {index}",
        )
        manual = _manual_responsibility(entry, index)
        identifier = manual["id"]
        identifiers.append(identifier)
        owner_targets.update(manual["ownerTargets"])
        if not manual["ownerTargets"]:
            raise ResponsibilityMapError(
                f"responsibility has no owner target: {identifier}"
            )
        for source_directory in manual["sourceDirectories"]:
            path = PurePosixPath(source_directory)
            if (
                path.is_absolute()
                or ".." in path.parts
                or path.as_posix() != source_directory
                or not (repository_root / source_directory).is_dir()
            ):
                raise ResponsibilityMapError(
                    f"invalid source directory for {identifier}: {source_directory}"
                )
        for source_path in manual["reviewedSourcePaths"]:
            path = PurePosixPath(source_path)
            if (
                path.is_absolute()
                or ".." in path.parts
                or path.as_posix() != source_path
                or not (repository_root / source_path).is_file()
                or not any(
                    path == root or root in path.parents
                    for root in map(PurePosixPath, PRODUCTION_SOURCE_DIRECTORIES)
                )
                or any(part in TEST_PATH_PARTS for part in path.parts)
            ):
                raise ResponsibilityMapError(
                    f"invalid reviewed source path for {identifier}: {source_path}"
                )
        unknown_header_owners = sorted(
            set(manual["publicHeaderOwnerTargets"])
            - known_public_header_owners
        )
        unknown_ui_areas = sorted(set(manual["uiClassAreas"]) - known_ui_areas)
        unknown_tool_areas = sorted(
            set(manual["uiToolClassAreas"]) - known_tool_areas
        )
        unknown_major_classes = sorted(
            set(manual["majorClasses"]) - known_major_classes
        )
        unknown_feature_owners = sorted(
            set(manual["pluginFeatureOwners"]) - known_feature_owners
        )
        unknown_reviewed_headers = sorted(
            set(manual["reviewedPublicHeaderPaths"]) - known_public_headers
        )
        if any(
            (
                unknown_header_owners,
                unknown_ui_areas,
                unknown_tool_areas,
                unknown_major_classes,
                unknown_feature_owners,
                unknown_reviewed_headers,
            )
        ):
            raise ResponsibilityMapError(
                f"unknown inventory reference for {identifier}"
            )
        if manual["pluginScope"] not in {
            "all-registrations",
            "feature-owners",
        }:
            raise ResponsibilityMapError(
                f"unknown plugin scope for {identifier}"
            )
        if manual["pluginScope"] == "all-registrations":
            all_plugin_scopes += 1
            if manual["pluginFeatureOwners"]:
                raise ResponsibilityMapError(
                    "all-registrations scope must not select feature owners"
                )
        else:
            feature_owner_assignments.extend(manual["pluginFeatureOwners"])
        ui_area_assignments.extend(manual["uiClassAreas"])
        tool_area_assignments.extend(manual["uiToolClassAreas"])
        major_class_assignments.extend(manual["majorClasses"])
        reviewed_header_assignments.extend(
            manual["reviewedPublicHeaderPaths"]
        )
        reviewed_source_assignments.extend(manual["reviewedSourcePaths"])

        public_header_paths = _string_list(
            entry.get("publicHeaderPaths"),
            f"public header paths for {identifier}",
        )
        if not set(public_header_paths).issubset(known_public_headers):
            raise ResponsibilityMapError(
                f"unknown public header path for {identifier}"
            )
        _string_list(entry.get("classNames"), f"class names for {identifier}")
        _string_list(entry.get("pluginIds"), f"plugin ids for {identifier}")
        _string_list(
            entry.get("pluginServiceTypes"),
            f"plugin service types for {identifier}",
        )

    if identifiers != sorted(set(identifiers)):
        raise ResponsibilityMapError(
            "responsibilities must be sorted and unique by id"
        )
    if len(reviewed_header_assignments) != len(
        set(reviewed_header_assignments)
    ):
        raise ResponsibilityMapError(
            "reviewed public header paths must have one responsibility"
        )
    if len(reviewed_source_assignments) != len(
        set(reviewed_source_assignments)
    ):
        raise ResponsibilityMapError(
            "reviewed source paths must have one responsibility"
        )
    missing = sorted(set(RESPONSIBILITY_IDS) - set(identifiers))
    unexpected = sorted(set(identifiers) - set(RESPONSIBILITY_IDS))
    if missing or unexpected:
        raise ResponsibilityMapError(
            "responsibility map does not match the R1-G3a scope; "
            f"missing={missing}, unexpected={unexpected}"
        )
    coverage_checks = (
        (ui_area_assignments, known_ui_areas, "UI class areas"),
        (tool_area_assignments, known_tool_areas, "UI tool class areas"),
        (major_class_assignments, known_major_classes, "major classes"),
        (feature_owner_assignments, known_feature_owners, "plugin feature owners"),
    )
    for assignments, known, description in coverage_checks:
        if set(assignments) != known or len(assignments) != len(set(assignments)):
            raise ResponsibilityMapError(
                f"{description} must be assigned exactly once"
            )
    if all_plugin_scopes != 1:
        raise ResponsibilityMapError(
            "exactly one responsibility must own all plugin registrations"
        )

    expected = updated_map(
        responsibility_map,
        graph_directory=graph_directory,
        public_surface_inventory_path=public_surface_inventory_path,
        ui_class_inventory_path=ui_class_inventory_path,
        ui_tool_class_inventory_path=ui_tool_class_inventory_path,
    )
    if responsibility_map.get("targetRelations") != expected["targetRelations"]:
        raise ResponsibilityMapError(
            "target relations do not match the recorded CMake graphs"
        )
    if responsibility_map.get("responsibilities") != expected["responsibilities"]:
        raise ResponsibilityMapError(
            "responsibility evidence does not match the recorded inventories"
        )


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Validate the current package responsibility map against CMake, "
            "public-surface, class, and plugin inventories."
        )
    )
    parser.add_argument(
        "--map",
        type=Path,
        default=REPO_ROOT / "docs/architecture/package-responsibilities.json",
    )
    parser.add_argument(
        "--graph-directory",
        type=Path,
        default=REPO_ROOT / "docs/architecture",
    )
    parser.add_argument(
        "--public-surface-inventory",
        type=Path,
        default=REPO_ROOT / "docs/architecture/public-surface-inventory.json",
    )
    parser.add_argument(
        "--ui-class-inventory",
        type=Path,
        default=REPO_ROOT / "docs/architecture/ui-class-responsibilities.json",
    )
    parser.add_argument(
        "--ui-tool-class-inventory",
        type=Path,
        default=(
            REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
        ),
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        responsibility_map = load_map(options.map)
        validate_map(
            responsibility_map,
            repository_root=REPO_ROOT,
            graph_directory=options.graph_directory,
            public_surface_inventory_path=options.public_surface_inventory,
            ui_class_inventory_path=options.ui_class_inventory,
            ui_tool_class_inventory_path=options.ui_tool_class_inventory,
        )
    except (OSError, ResponsibilityMapError) as error:
        print(f"check-package-responsibility-map: {error}", file=sys.stderr)
        return 1
    print(f"package responsibility map verified: {options.map}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
