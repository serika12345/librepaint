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


class PublicSurfaceError(RuntimeError):
    """Raised when the public-surface inventory violates its contract."""


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


def _validate_public_headers(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    targets_by_platform: dict[str, dict[str, dict[str, Any]]],
) -> dict[str, dict[str, Any]]:
    entries = _require_array(inventory.get("publicHeaders"), "publicHeaders")
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
        raise PublicSurfaceError("publicHeaders must be sorted and unique by path")
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
            "publicHeaders",
            "majorClasses",
            "plugins",
        },
        "public-surface inventory",
    )
    if inventory.get("schemaVersion") != 1:
        raise PublicSurfaceError("public-surface inventory schemaVersion must be 1")
    if inventory.get("scope") not in {"representative", "complete"}:
        raise PublicSurfaceError(
            "public-surface inventory scope must be representative or complete"
        )
    if _platforms(inventory.get("platforms"), "inventory platforms") != list(
        PLATFORMS
    ):
        raise PublicSurfaceError(
            f"inventory platforms must cover every supported platform: {list(PLATFORMS)}"
        )

    targets_by_platform = _load_target_graphs(graph_directory)
    public_headers = _validate_public_headers(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
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
            "Validate the representative public headers, major classes, and "
            "plugins against every recorded CMake target graph."
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
