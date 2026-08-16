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
HEADER_SUFFIXES = frozenset({".h", ".hh", ".hpp"})
SOURCE_SUFFIXES = HEADER_SUFFIXES | frozenset({".c", ".cc", ".cpp", ".cxx", ".m", ".mm"})
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
PUBLICATION_EVIDENCE = ("export-macro", "external-include")
INCLUDE_PATTERN = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.MULTILINE
)
PUBLIC_HEADER_SET_SPECS = (
    {
        "ownerTarget": "kritaimage",
        "sourceDirectory": "libs/image",
        "exportMacro": "KRITAIMAGE_EXPORT",
        "responsibility": (
            "Records the declared and de facto inter-package header surface "
            "for image, layer, tile, projection, brush, and stroke state."
        ),
        "evidence": ["libs/image/CMakeLists.txt", "libs/image/kis_image_export.h"],
    },
    {
        "ownerTarget": "kritaui",
        "sourceDirectory": "libs/ui",
        "exportMacro": "KRITAUI_EXPORT",
        "responsibility": (
            "Records the declared and de facto inter-package header surface "
            "for application, document, canvas, input, tool, and UI coordination."
        ),
        "evidence": ["libs/ui/CMakeLists.txt", "libs/ui/kritaui_export_instance.h"],
    },
)


class PublicSurfaceError(RuntimeError):
    """Raised when the public-surface inventory violates its contract."""


def _is_production_source_path(path: PurePosixPath) -> bool:
    if any(part in TEST_PATH_PARTS for part in path.parts):
        return False
    return not path.name.endswith(("_test.cpp", "_test.cc", "_test.cxx"))


def _source_files(repository_root: Path) -> list[tuple[str, Path]]:
    files: list[tuple[str, Path]] = []
    for directory in PRODUCTION_SOURCE_DIRECTORIES:
        root = repository_root / directory
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repository_root).as_posix()
            if _is_production_source_path(PurePosixPath(relative)):
                files.append((relative, path))
    return sorted(files)


def discover_public_headers(
    *,
    repository_root: Path,
    source_directory: str,
    export_macro: str,
) -> list[dict[str, Any]]:
    """Return the complete declared or used production header surface."""

    source_files = _source_files(repository_root)
    owner_headers = [
        (relative, path)
        for relative, path in source_files
        if path.suffix in HEADER_SUFFIXES
        and _path_is_within(relative, source_directory)
    ]
    headers_by_name: dict[str, list[str]] = {}
    for relative, _path in owner_headers:
        headers_by_name.setdefault(PurePosixPath(relative).name, []).append(relative)
    repository_headers_by_name: dict[str, list[str]] = {}
    for relative, path in source_files:
        if path.suffix in HEADER_SUFFIXES:
            repository_headers_by_name.setdefault(
                PurePosixPath(relative).name, []
            ).append(relative)
    ambiguous = {
        name: repository_headers_by_name[name]
        for name in headers_by_name
        if len(repository_headers_by_name[name]) != 1
    }
    if ambiguous:
        raise PublicSurfaceError(
            f"ambiguous header basenames below {source_directory}: {ambiguous}"
        )

    consumers_by_name: dict[str, set[str]] = {
        name: set() for name in headers_by_name
    }
    for relative, path in source_files:
        if _path_is_within(relative, source_directory):
            continue
        text = path.read_text(encoding="utf-8")
        for include in INCLUDE_PATTERN.findall(text):
            name = PurePosixPath(include).name
            if name in consumers_by_name:
                consumers_by_name[name].add(relative)

    entries: list[dict[str, Any]] = []
    export_macros = (
        export_macro,
        f"{export_macro}_TEMPLATE",
        f"{export_macro}_INSTANCE",
    )
    for relative, path in owner_headers:
        name = PurePosixPath(relative).name
        text = path.read_text(encoding="utf-8")
        consumers = sorted(consumers_by_name[name])
        publication_evidence = []
        if any(
            re.search(rf"\b{re.escape(macro)}\b", text)
            for macro in export_macros
        ):
            publication_evidence.append("export-macro")
        if consumers:
            publication_evidence.append("external-include")
        if publication_evidence:
            entries.append(
                {
                    "path": relative,
                    "publicationEvidence": publication_evidence,
                    "consumerPaths": consumers,
                }
            )
    return entries


def public_header_policy() -> dict[str, Any]:
    return {
        "headerSuffixes": sorted(HEADER_SUFFIXES),
        "sourceSuffixes": sorted(SOURCE_SUFFIXES),
        "productionSourceDirectories": list(PRODUCTION_SOURCE_DIRECTORIES),
        "excludedPathParts": sorted(TEST_PATH_PARTS),
        "excludedFileSuffixes": ["_test.cc", "_test.cpp", "_test.cxx"],
        "publicationEvidence": list(PUBLICATION_EVIDENCE),
    }


def build_public_header_sets(repository_root: Path) -> list[dict[str, Any]]:
    sets: list[dict[str, Any]] = []
    for spec in PUBLIC_HEADER_SET_SPECS:
        sets.append(
            {
                **spec,
                "platforms": list(PLATFORMS),
                "headers": discover_public_headers(
                    repository_root=repository_root,
                    source_directory=spec["sourceDirectory"],
                    export_macro=spec["exportMacro"],
                ),
            }
        )
    return sets


def _load_json(path: Path, description: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise PublicSurfaceError(f"{description} does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise PublicSurfaceError(
            f"invalid JSON in {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise PublicSurfaceError(f"expected a JSON object in {path}")
    return value


def load_inventory(path: Path) -> dict[str, Any]:
    return _load_json(path, "public-surface inventory")


def _require_object(value: Any, description: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise PublicSurfaceError(f"expected an object for {description}")
    return value


def _require_array(value: Any, description: str) -> list[Any]:
    if not isinstance(value, list):
        raise PublicSurfaceError(f"expected an array for {description}")
    return value


def _require_string(value: Any, description: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise PublicSurfaceError(f"expected a non-empty string for {description}")
    return value


def _require_fields(
    value: dict[str, Any], expected: set[str], description: str
) -> None:
    actual = set(value)
    if actual != expected:
        missing = sorted(expected - actual)
        unexpected = sorted(actual - expected)
        raise PublicSurfaceError(
            f"unexpected fields for {description}; missing={missing}, "
            f"unexpected={unexpected}"
        )


def _repository_file(
    repository_root: Path, value: Any, description: str
) -> tuple[str, Path]:
    relative = _require_string(value, description)
    path = PurePosixPath(relative)
    if path.is_absolute() or ".." in path.parts or path.as_posix() != relative:
        raise PublicSurfaceError(
            f"{description} must be a normalized repository-relative path: {relative}"
        )
    resolved = repository_root / relative
    if not resolved.is_file():
        raise PublicSurfaceError(f"{description} does not exist: {relative}")
    return relative, resolved


def _platforms(value: Any, description: str) -> list[str]:
    platforms = _require_array(value, description)
    if not platforms or not all(isinstance(item, str) for item in platforms):
        raise PublicSurfaceError(f"{description} must contain platform names")
    unknown = [item for item in platforms if item not in PLATFORMS]
    if unknown:
        raise PublicSurfaceError(f"{description} contains unknown platforms: {unknown}")
    expected_order = [platform for platform in PLATFORMS if platform in platforms]
    if platforms != expected_order:
        raise PublicSurfaceError(
            f"{description} must be unique and ordered as {list(PLATFORMS)}"
        )
    return platforms


def _load_target_graphs(graph_directory: Path) -> dict[str, dict[str, dict[str, Any]]]:
    targets_by_platform: dict[str, dict[str, dict[str, Any]]] = {}
    for platform in PLATFORMS:
        path = graph_directory / f"cmake-targets-{platform}.json"
        graph = _load_json(path, f"{platform} CMake target graph")
        if graph.get("schemaVersion") != 1 or graph.get("platform") != platform:
            raise PublicSurfaceError(f"invalid {platform} CMake target graph: {path}")
        targets = _require_array(graph.get("targets"), f"targets in {path}")
        targets_by_platform[platform] = {
            _require_string(
                _require_object(target, f"target in {path}").get("name"),
                f"target name in {path}",
            ): target
            for target in targets
        }
    return targets_by_platform


def _available_platforms(
    target_name: str,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
) -> list[str]:
    return [
        platform
        for platform in PLATFORMS
        if target_name in targets_by_platform[platform]
    ]


def _path_is_within(relative_path: str, source_directory: str) -> bool:
    path = PurePosixPath(relative_path)
    source = PurePosixPath(source_directory)
    try:
        path.relative_to(source)
    except ValueError:
        return False
    return True


def _validate_owner(
    *,
    owner_target: str,
    platforms: list[str],
    owned_paths: list[str],
    description: str,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
) -> None:
    available = _available_platforms(owner_target, targets_by_platform)
    if not available:
        raise PublicSurfaceError(
            f"{description}: unknown owner target {owner_target}"
        )
    if platforms != available:
        raise PublicSurfaceError(
            f"{description}: owner target {owner_target} is available on "
            f"{available}, not {platforms}"
        )
    for platform in platforms:
        source_directory = _require_string(
            targets_by_platform[platform][owner_target].get("sourceDirectory"),
            f"source directory for {owner_target} on {platform}",
        )
        for owned_path in owned_paths:
            if not _path_is_within(owned_path, source_directory):
                raise PublicSurfaceError(
                    f"{description}: {owned_path} is outside {owner_target} "
                    f"source directory {source_directory} on {platform}"
                )


def _validate_evidence(
    value: Any,
    *,
    repository_root: Path,
    description: str,
    required_paths: set[str],
) -> None:
    evidence = _require_array(value, f"evidence for {description}")
    paths = [
        _repository_file(
            repository_root,
            item,
            f"evidence path for {description}",
        )[0]
        for item in evidence
    ]
    if paths != sorted(set(paths)):
        raise PublicSurfaceError(
            f"evidence for {description} must be sorted and unique"
        )
    missing = sorted(required_paths - set(paths))
    if missing:
        raise PublicSurfaceError(
            f"evidence for {description} is missing required paths: {missing}"
        )


def _validate_consumer_evidence(
    value: Any,
    *,
    header_path: str,
    repository_root: Path,
    description: str,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
) -> None:
    entries = _require_array(value, f"consumer evidence for {description}")
    if not entries:
        raise PublicSurfaceError(
            f"consumer evidence for {description} must not be empty"
        )
    normalized: list[tuple[str, str]] = []
    header_name = PurePosixPath(header_path).name
    include_pattern = re.compile(
        rf"#[ \t]*include[ \t]*[<\"]{re.escape(header_name)}[>\"]"
    )
    for index, item in enumerate(entries):
        entry = _require_object(item, f"consumer evidence {index} for {description}")
        _require_fields(
            entry,
            {"target", "path", "platforms"},
            f"consumer evidence {index} for {description}",
        )
        target = _require_string(
            entry.get("target"), f"consumer target for {description}"
        )
        path, resolved = _repository_file(
            repository_root,
            entry.get("path"),
            f"consumer path for {description}",
        )
        platforms = _platforms(
            entry.get("platforms"), f"consumer platforms for {target}"
        )
        available = _available_platforms(target, targets_by_platform)
        if not available:
            raise PublicSurfaceError(
                f"{description}: unknown consumer target {target}"
            )
        if any(platform not in available for platform in platforms):
            raise PublicSurfaceError(
                f"{description}: consumer target {target} is available on "
                f"{available}, not {platforms}"
            )
        for platform in platforms:
            source_directory = _require_string(
                targets_by_platform[platform][target].get("sourceDirectory"),
                f"source directory for {target} on {platform}",
            )
            if not _path_is_within(path, source_directory):
                raise PublicSurfaceError(
                    f"{description}: consumer path {path} is outside {target} "
                    f"source directory {source_directory} on {platform}"
                )
        if include_pattern.search(resolved.read_text(encoding="utf-8")) is None:
            raise PublicSurfaceError(
                f"consumer path {path} does not include {header_name}"
            )
        normalized.append((target, path))
    if normalized != sorted(set(normalized)):
        raise PublicSurfaceError(
            f"consumer evidence for {description} must be sorted and unique"
        )


def _validate_public_header_sets(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
) -> dict[str, dict[str, Any]]:
    if inventory.get("publicHeaderPolicy") != public_header_policy():
        raise PublicSurfaceError(
            "publicHeaderPolicy must match the repository discovery policy"
        )

    entries = _require_array(
        inventory.get("publicHeaderSets"), "publicHeaderSets"
    )
    expected_specs = {spec["ownerTarget"]: spec for spec in PUBLIC_HEADER_SET_SPECS}
    owners: list[str] = []
    by_path: dict[str, dict[str, Any]] = {}
    expected_fields = {
        "ownerTarget",
        "sourceDirectory",
        "exportMacro",
        "platforms",
        "headers",
        "responsibility",
        "evidence",
    }
    for index, item in enumerate(entries):
        entry = _require_object(item, f"public header set {index}")
        _require_fields(entry, expected_fields, f"public header set {index}")
        owner_target = _require_string(
            entry.get("ownerTarget"), f"owner target for public header set {index}"
        )
        description = f"public header set {owner_target}"
        spec = expected_specs.get(owner_target)
        if spec is None:
            raise PublicSurfaceError(
                f"{description}: target is outside the complete R1-G2b scope"
            )
        source_directory = _require_string(
            entry.get("sourceDirectory"), f"source directory for {description}"
        )
        export_macro = _require_string(
            entry.get("exportMacro"), f"export macro for {description}"
        )
        if source_directory != spec["sourceDirectory"]:
            raise PublicSurfaceError(
                f"{description}: source directory must be {spec['sourceDirectory']}"
            )
        if export_macro != spec["exportMacro"]:
            raise PublicSurfaceError(
                f"{description}: export macro must be {spec['exportMacro']}"
            )
        source_path = PurePosixPath(source_directory)
        if (
            source_path.is_absolute()
            or ".." in source_path.parts
            or source_path.as_posix() != source_directory
            or not (repository_root / source_directory).is_dir()
        ):
            raise PublicSurfaceError(
                f"{description}: invalid source directory {source_directory}"
            )
        platforms = _platforms(
            entry.get("platforms"), f"platforms for {description}"
        )
        _require_string(
            entry.get("responsibility"), f"responsibility for {description}"
        )
        cmake_path = f"{source_directory}/CMakeLists.txt"
        _validate_evidence(
            entry.get("evidence"),
            repository_root=repository_root,
            description=description,
            required_paths={cmake_path},
        )

        headers = _require_array(entry.get("headers"), f"headers for {description}")
        header_paths: list[str] = []
        recorded_by_path: dict[str, dict[str, Any]] = {}
        for header_index, header_item in enumerate(headers):
            header = _require_object(
                header_item, f"header {header_index} for {description}"
            )
            _require_fields(
                header,
                {"path", "publicationEvidence", "consumerPaths"},
                f"header {header_index} for {description}",
            )
            path, _resolved = _repository_file(
                repository_root,
                header.get("path"),
                f"header path {header_index} for {description}",
            )
            if (
                PurePosixPath(path).suffix not in HEADER_SUFFIXES
                or not _path_is_within(path, source_directory)
                or not _is_production_source_path(PurePosixPath(path))
            ):
                raise PublicSurfaceError(
                    f"{description}: invalid production header path {path}"
                )
            publication_evidence = _require_array(
                header.get("publicationEvidence"),
                f"publication evidence for {path}",
            )
            expected_order = [
                value for value in PUBLICATION_EVIDENCE if value in publication_evidence
            ]
            if not publication_evidence or publication_evidence != expected_order:
                raise PublicSurfaceError(
                    f"publication evidence for {path} must be unique and ordered as "
                    f"{list(PUBLICATION_EVIDENCE)}"
                )
            consumer_values = _require_array(
                header.get("consumerPaths"), f"consumer paths for {path}"
            )
            consumer_paths = [
                _repository_file(
                    repository_root,
                    consumer,
                    f"consumer path for {path}",
                )[0]
                for consumer in consumer_values
            ]
            if consumer_paths != sorted(set(consumer_paths)):
                raise PublicSurfaceError(
                    f"consumer paths for {path} must be sorted and unique"
                )
            for consumer in consumer_paths:
                consumer_path = PurePosixPath(consumer)
                if (
                    consumer_path.suffix not in SOURCE_SUFFIXES
                    or _path_is_within(consumer, source_directory)
                    or not _is_production_source_path(consumer_path)
                ):
                    raise PublicSurfaceError(
                        f"{description}: invalid external consumer path {consumer} "
                        f"for {path}"
                    )
            header_paths.append(path)
            recorded_by_path[path] = header
        if header_paths != sorted(set(header_paths)):
            raise PublicSurfaceError(
                f"headers for {description} must be sorted and unique by path"
            )

        expected_headers = discover_public_headers(
            repository_root=repository_root,
            source_directory=source_directory,
            export_macro=export_macro,
        )
        expected_by_path = {header["path"]: header for header in expected_headers}
        missing = sorted(set(expected_by_path) - set(recorded_by_path))
        unexpected = sorted(set(recorded_by_path) - set(expected_by_path))
        if missing or unexpected:
            raise PublicSurfaceError(
                f"{description} does not match discovered candidates; "
                f"missing={missing}, unexpected={unexpected}"
            )
        for path in header_paths:
            expected = expected_by_path[path]
            if recorded_by_path[path] != expected:
                raise PublicSurfaceError(
                    f"recorded publication evidence or consumers for {path} "
                    "do not match source discovery"
                )

        _validate_owner(
            owner_target=owner_target,
            platforms=platforms,
            owned_paths=[cmake_path, *header_paths],
            description=description,
            targets_by_platform=targets_by_platform,
        )
        for platform in platforms:
            target_source = _require_string(
                targets_by_platform[platform][owner_target].get("sourceDirectory"),
                f"source directory for {owner_target} on {platform}",
            )
            if target_source != source_directory:
                raise PublicSurfaceError(
                    f"{description}: {platform} target source directory is "
                    f"{target_source}, not {source_directory}"
                )
        for path in header_paths:
            if path in by_path:
                raise PublicSurfaceError(
                    f"public header is owned by multiple sets: {path}"
                )
            by_path[path] = {
                "ownerTarget": owner_target,
                "exportMacro": export_macro,
                "platforms": platforms,
            }
        owners.append(owner_target)

    if owners != sorted(expected_specs):
        raise PublicSurfaceError(
            f"publicHeaderSets must contain {sorted(expected_specs)} in owner order"
        )
    return by_path


def _validate_public_headers(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
    complete_public_headers: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    entries = _require_array(
        inventory.get("publicHeaderDetails"), "publicHeaderDetails"
    )
    normalized: list[str] = []
    by_path: dict[str, dict[str, Any]] = {}
    expected_fields = {
        "path",
        "ownerTarget",
        "exportMacro",
        "platforms",
        "consumerEvidence",
        "responsibility",
        "evidence",
    }
    for index, item in enumerate(entries):
        entry = _require_object(item, f"public header {index}")
        _require_fields(entry, expected_fields, f"public header {index}")
        path, resolved = _repository_file(
            repository_root, entry.get("path"), f"public header path {index}"
        )
        description = f"public header {path}"
        owner_target = _require_string(
            entry.get("ownerTarget"), f"owner target for {path}"
        )
        export_macro = _require_string(
            entry.get("exportMacro"), f"export macro for {path}"
        )
        platforms = _platforms(
            entry.get("platforms"), f"platforms for {description}"
        )
        _require_string(
            entry.get("responsibility"), f"responsibility for {description}"
        )
        _validate_owner(
            owner_target=owner_target,
            platforms=platforms,
            owned_paths=[path],
            description=description,
            targets_by_platform=targets_by_platform,
        )
        complete_header = complete_public_headers.get(path)
        if complete_header is None:
            raise PublicSurfaceError(
                f"{description}: header is absent from the complete public header sets"
            )
        for field in ("ownerTarget", "exportMacro", "platforms"):
            if entry[field] != complete_header[field]:
                raise PublicSurfaceError(
                    f"{description}: {field} does not match its complete header set"
                )
        if re.search(rf"\b{re.escape(export_macro)}\b", resolved.read_text(encoding="utf-8")) is None:
            raise PublicSurfaceError(
                f"{description}: export macro {export_macro} was not found"
            )
        _validate_consumer_evidence(
            entry.get("consumerEvidence"),
            header_path=path,
            repository_root=repository_root,
            description=description,
            targets_by_platform=targets_by_platform,
        )
        _validate_evidence(
            entry.get("evidence"),
            repository_root=repository_root,
            description=description,
            required_paths={path},
        )
        normalized.append(path)
        by_path[path] = entry
    if normalized != sorted(set(normalized)):
        raise PublicSurfaceError(
            "publicHeaderDetails must be sorted and unique by path"
        )
    return by_path


def _validate_major_classes(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
    public_headers: dict[str, dict[str, Any]],
) -> set[str]:
    entries = _require_array(inventory.get("majorClasses"), "majorClasses")
    names: list[str] = []
    expected_fields = {
        "name",
        "header",
        "implementation",
        "ownerTarget",
        "exportMacro",
        "platforms",
        "consumerEvidence",
        "responsibility",
        "evidence",
    }
    for index, item in enumerate(entries):
        entry = _require_object(item, f"major class {index}")
        _require_fields(entry, expected_fields, f"major class {index}")
        name = _require_string(entry.get("name"), f"major class name {index}")
        description = f"major class {name}"
        header, header_file = _repository_file(
            repository_root, entry.get("header"), f"header for {description}"
        )
        implementation, implementation_file = _repository_file(
            repository_root,
            entry.get("implementation"),
            f"implementation for {description}",
        )
        owner_target = _require_string(
            entry.get("ownerTarget"), f"owner target for {description}"
        )
        export_macro = _require_string(
            entry.get("exportMacro"), f"export macro for {description}"
        )
        platforms = _platforms(
            entry.get("platforms"), f"platforms for {description}"
        )
        _require_string(
            entry.get("responsibility"), f"responsibility for {description}"
        )
        _validate_owner(
            owner_target=owner_target,
            platforms=platforms,
            owned_paths=[header, implementation],
            description=description,
            targets_by_platform=targets_by_platform,
        )
        public_header = public_headers.get(header)
        if public_header is None:
            raise PublicSurfaceError(
                f"{description}: header is not recorded in publicHeaders: {header}"
            )
        for field in ("ownerTarget", "exportMacro", "platforms"):
            if entry[field] != public_header[field]:
                raise PublicSurfaceError(
                    f"{description}: {field} does not match public header {header}"
                )
        declaration_pattern = re.compile(
            rf"\bclass\s+{re.escape(export_macro)}\s+{re.escape(name)}\b"
        )
        if declaration_pattern.search(header_file.read_text(encoding="utf-8")) is None:
            raise PublicSurfaceError(
                f"{description}: exported class declaration was not found in {header}"
            )
        if f"{name}::" not in implementation_file.read_text(encoding="utf-8"):
            raise PublicSurfaceError(
                f"{description}: implementation symbols were not found in {implementation}"
            )
        _validate_consumer_evidence(
            entry.get("consumerEvidence"),
            header_path=header,
            repository_root=repository_root,
            description=description,
            targets_by_platform=targets_by_platform,
        )
        _validate_evidence(
            entry.get("evidence"),
            repository_root=repository_root,
            description=description,
            required_paths={header, implementation},
        )
        names.append(name)
    if names != sorted(set(names)):
        raise PublicSurfaceError("majorClasses must be sorted and unique by name")
    return set(names)


def _validate_plugins(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
    major_classes: set[str],
) -> None:
    entries = _require_array(inventory.get("plugins"), "plugins")
    identifiers: list[str] = []
    expected_fields = {
        "id",
        "metadata",
        "implementation",
        "ownerTarget",
        "platforms",
        "serviceTypes",
        "registrationMacro",
        "runtimeConsumer",
        "responsibility",
        "evidence",
    }
    for index, item in enumerate(entries):
        entry = _require_object(item, f"plugin {index}")
        _require_fields(entry, expected_fields, f"plugin {index}")
        identifier = _require_string(entry.get("id"), f"plugin id {index}")
        description = f"plugin {identifier}"
        metadata, metadata_file = _repository_file(
            repository_root, entry.get("metadata"), f"metadata for {description}"
        )
        implementation, implementation_file = _repository_file(
            repository_root,
            entry.get("implementation"),
            f"implementation for {description}",
        )
        owner_target = _require_string(
            entry.get("ownerTarget"), f"owner target for {description}"
        )
        platforms = _platforms(
            entry.get("platforms"), f"platforms for {description}"
        )
        registration_macro = _require_string(
            entry.get("registrationMacro"), f"registration macro for {description}"
        )
        runtime_consumer = _require_string(
            entry.get("runtimeConsumer"), f"runtime consumer for {description}"
        )
        _require_string(
            entry.get("responsibility"), f"responsibility for {description}"
        )
        _validate_owner(
            owner_target=owner_target,
            platforms=platforms,
            owned_paths=[metadata, implementation],
            description=description,
            targets_by_platform=targets_by_platform,
        )
        service_types = _require_array(
            entry.get("serviceTypes"), f"service types for {description}"
        )
        if (
            not service_types
            or not all(isinstance(value, str) and value for value in service_types)
            or service_types != sorted(set(service_types))
        ):
            raise PublicSurfaceError(
                f"service types for {description} must be sorted non-empty strings"
            )
        metadata_value = _load_json(metadata_file, f"metadata for {description}")
        if metadata_value.get("Id") != identifier:
            raise PublicSurfaceError(
                f"metadata id {metadata_value.get('Id')} does not match plugin id {identifier}"
            )
        metadata_library = metadata_value.get("X-KDE-Library")
        if metadata_library != owner_target:
            raise PublicSurfaceError(
                f"metadata library {metadata_library} does not match owner target "
                f"{owner_target}"
            )
        if metadata_value.get("X-KDE-ServiceTypes") != service_types:
            raise PublicSurfaceError(
                f"metadata service types do not match {description}"
            )
        implementation_text = implementation_file.read_text(encoding="utf-8")
        if registration_macro not in implementation_text or PurePosixPath(
            metadata
        ).name not in implementation_text:
            raise PublicSurfaceError(
                f"{description}: registration does not bind {metadata} with "
                f"{registration_macro}"
            )
        if runtime_consumer not in major_classes:
            raise PublicSurfaceError(
                f"{description}: runtime consumer is not a recorded major class: "
                f"{runtime_consumer}"
            )
        _validate_evidence(
            entry.get("evidence"),
            repository_root=repository_root,
            description=description,
            required_paths={metadata, implementation},
        )
        identifiers.append(identifier)
    if identifiers != sorted(set(identifiers)):
        raise PublicSurfaceError("plugins must be sorted and unique by id")


def validate_inventory(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    graph_directory: Path,
) -> None:
    _require_fields(
        inventory,
        {
            "schemaVersion",
            "scope",
            "platforms",
            "publicHeaderPolicy",
            "publicHeaderSets",
            "publicHeaderDetails",
            "majorClasses",
            "plugins",
        },
        "public-surface inventory",
    )
    if inventory.get("schemaVersion") != 2:
        raise PublicSurfaceError("public-surface inventory schemaVersion must be 2")
    scope = _require_object(inventory.get("scope"), "inventory scope")
    _require_fields(
        scope,
        {"publicHeaders", "majorClasses", "plugins"},
        "inventory scope",
    )
    if scope.get("publicHeaders") != "complete":
        raise PublicSurfaceError(
            "public header inventory scope must be complete"
        )
    for section in ("majorClasses", "plugins"):
        if scope.get(section) not in {"representative", "complete"}:
            raise PublicSurfaceError(
                f"inventory scope for {section} must be representative or complete"
            )
    if _platforms(inventory.get("platforms"), "inventory platforms") != list(
        PLATFORMS
    ):
        raise PublicSurfaceError(
            f"inventory platforms must cover every supported platform: {list(PLATFORMS)}"
        )

    targets_by_platform = _load_target_graphs(graph_directory)
    complete_public_headers = _validate_public_header_sets(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
    )
    public_headers = _validate_public_headers(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
        complete_public_headers=complete_public_headers,
    )
    major_classes = _validate_major_classes(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
        public_headers=public_headers,
    )
    _validate_plugins(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
        major_classes=major_classes,
    )


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Validate complete public-header sets and representative major "
            "classes and plugins against every recorded CMake target graph."
        )
    )
    parser.add_argument(
        "--inventory",
        type=Path,
        default=REPO_ROOT / "docs/architecture/public-surface-inventory.json",
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
        inventory = load_inventory(options.inventory)
        validate_inventory(
            inventory,
            repository_root=REPO_ROOT,
            graph_directory=options.graph_directory,
        )
    except (OSError, PublicSurfaceError) as error:
        print(f"check-public-surface-inventory: {error}", file=sys.stderr)
        return 1
    print(f"public surface inventory verified: {options.inventory}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
