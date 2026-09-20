#!/usr/bin/env python3

from __future__ import annotations

import json
import posixpath
import re
import sys
from pathlib import Path, PurePosixPath


REPO_ROOT = Path(__file__).resolve().parents[2]
HEADER_SUFFIXES = frozenset({".h", ".hh", ".hpp"})
SOURCE_SUFFIXES = HEADER_SUFFIXES | frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".m", ".mm"}
)
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
PLUGIN_REGISTRATION_PATTERN = re.compile(
    r"\b(K_PLUGIN_(?:FACTORY|CLASS)_WITH_JSON)\s*\("
    r'[^,]+,\s*"([^"]+\.json)"',
    re.DOTALL,
)
PUBLIC_EXPORT_PATTERN = re.compile(
    r"\b(?:[A-Z][A-Z0-9]*_)*[A-Z][A-Z0-9]*_EXPORT"
    r"(?:_TEMPLATE|_INSTANCE)?\b"
)

PUBLIC_HEADER_OWNERS = (
    (
        "libs/application",
        ("libs/application",),
        ("libs/application/tests", "libs/application/ui"),
        "KRITAAPPLICATION_EXPORT",
    ),
    (
        "libs/application",
        ("libs/application/ui", "libs/ui"),
        (),
        "KRITAUI_EXPORT",
    ),
    (
        "libs/widgets",
        ("libs/widgets",),
        ("libs/widgets/tests",),
        "KRITAWIDGETS_EXPORT",
    ),
    (
        "libs/canvas",
        ("libs/canvas",),
        ("libs/canvas/tests", "libs/canvas/workspace"),
        "KRITACANVAS_EXPORT",
    ),
    (
        "libs/canvas",
        ("libs/canvas/workspace",),
        (),
        "KRITAUI_EXPORT",
    ),
    (
        "libs/document",
        ("libs/document",),
        ("libs/document/files", "libs/document/tests", "libs/document/ui"),
        "KRITADOCUMENT_EXPORT",
    ),
    (
        "libs/document/files",
        ("libs/document/files",),
        ("libs/document/files/tests",),
        "KRITADOCUMENTFILES_EXPORT",
    ),
    (
        "libs/document/ui",
        ("libs/document/ui",),
        ("libs/document/ui/tests",),
        "KRITADOCUMENTUI_EXPORT",
    ),
    ("libs/image", ("libs/image",), (), "KRITAIMAGE_EXPORT"),
    (
        "libs/impex",
        ("libs/impex",),
        ("libs/impex/animation", "libs/impex/tests", "libs/impex/ui"),
        "KRITAIMPEX_EXPORT",
    ),
    (
        "libs/impex",
        ("libs/impex/animation", "libs/impex/ui"),
        (),
        "KRITAUI_EXPORT",
    ),
    (
        "libs/input",
        ("libs/input",),
        ("libs/input/tests", "libs/input/ui"),
        "KRITAINPUT_EXPORT",
    ),
    (
        "libs/input/ui",
        ("libs/input/ui",),
        ("libs/input/ui/tests",),
        "KRITAINPUTUI_EXPORT",
    ),
    (
        "libs/painting",
        ("libs/painting",),
        ("libs/painting/metadata", "libs/painting/tests", "libs/painting/undo"),
        "KRITAPAINTING_EXPORT",
    ),
    (
        "libs/tools",
        ("libs/tools",),
        ("libs/tools/tests", "libs/tools/ui"),
        "KRITATOOLS_EXPORT",
    ),
)

PUBLIC_HEADER_COMPILE_CONTRACTS = {
    "libs/application": (
        "libs/application/tests/TestApplicationConfiguration.cpp",
        "libs/ui/tests/TestApplicationWorkspaceToolUiPublicHeaders.cpp",
        "libs/ui/tests/TestCanvasUiPublicHeaders.cpp",
        "libs/ui/tests/TestDocumentStateUiPublicHeaders.cpp",
        "libs/ui/tests/TestRemainingUiRootPublicHeaders.cpp",
    ),
    "libs/canvas": (
        "libs/ui/tests/TestApplicationWorkspaceToolUiPublicHeaders.cpp",
    ),
    "libs/document/ui": (
        "libs/document/ui/tests/kis_document_autosave_recovery_dialog_test.cpp",
        "libs/document/ui/tests/kis_document_undo_store_test.cpp",
        "libs/document/ui/tests/kis_document_undo_ui_test.cpp",
    ),
    "libs/image": ("libs/painting/tests/TestPublicImageHeaders.cpp",),
    "libs/impex": ("libs/impex/tests/TestImportExportPublicHeaders.cpp",),
    "libs/input": ("libs/input/tests/TestInputShortcutMatcher.cpp",),
    "libs/painting": ("libs/painting/tests/TestPaintingBoundary.cpp",),
    "libs/tools": ("libs/tools/tests/TestToolCoreContract.cpp",),
}

PLUGIN_OWNER_TARGET_OVERRIDES = {
    "plugins/dockers/touchdocker/kritatouchdocker.json": "kritatouchdocker",
    "plugins/extensions/qmic/kritaqmic.json": "kritaqmic",
    "plugins/filters/gaussianhighpass/kritagaussianhighpassfilter.json": (
        "kritagaussianhighpassfilter"
    ),
    "plugins/filters/gradientmap/KritaGradientMapFilter.json": "kritagradientmap",
    "plugins/filters/halftone/KritaHalftone.json": "kritahalftone",
    "plugins/filters/propagatecolors/kritapropagatecolorsfilter.json": (
        "kritapropagatecolors"
    ),
    "plugins/generators/gradient/KritaGradientGenerator.json": (
        "kritagradientgenerator"
    ),
    "plugins/generators/screentone/KritaScreentoneGenerator.json": (
        "kritascreentonegenerator"
    ),
    "plugins/impex/brush/krita_brush_export.json": "kritabrushexport",
    "plugins/impex/brush/krita_brush_import.json": "kritabrushimport",
    "plugins/platforms/wayland/kritaplatformwayland.json": (
        "kritaplatformpluginwayland"
    ),
    "plugins/platforms/xcb/kritaplatformxcb.json": "kritaplatformpluginxcb",
    "plugins/tools/karbonplugins/tools/karbon_tools.json": "krita_karbontools",
    "plugins/tools/tool_knife/kritatoolknife.json": "kritatoolKnife",
    "plugins/tools/tool_smart_patch/kritatoolsmartpatch.json": (
        "kritatoolSmartPatch"
    ),
}

PLUGIN_SERVICE_TYPES = frozenset(
    {
        "Krita/ApplicationPlugin",
        "Krita/ColorSpace",
        "Krita/ColorSpaceExtension",
        "Krita/Dock",
        "Krita/FileFilter",
        "Krita/Filter",
        "Krita/Flake",
        "Krita/Generator",
        "Krita/Metadata",
        "Krita/Paintop",
        "Krita/PlatformPlugin",
        "Krita/Shape",
        "Krita/Tool",
        "Krita/ViewPlugin",
    }
)


class PublicContractError(RuntimeError):
    """Raised when a declared header or plugin contract is invalid."""


def _path_is_within(path: str, directory: str) -> bool:
    value = PurePosixPath(path)
    parent = PurePosixPath(directory)
    return value == parent or parent in value.parents


def _is_production_source(path: PurePosixPath) -> bool:
    return not any(part in TEST_PATH_PARTS for part in path.parts) and not path.name.endswith(
        ("_test.cpp", "_test.cc", "_test.cxx")
    )


def _source_files(repository_root: Path) -> list[tuple[str, Path]]:
    result: list[tuple[str, Path]] = []
    for directory in PRODUCTION_SOURCE_DIRECTORIES:
        root = repository_root / directory
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repository_root).as_posix()
            if _is_production_source(PurePosixPath(relative)):
                result.append((relative, path))
    return sorted(result)


def _declared_public_header_names(
    repository_root: Path,
    source_directory: str,
    headers_by_name: dict[str, tuple[str, Path]],
) -> set[str]:
    declared: set[str] = set()
    for relative in PUBLIC_HEADER_COMPILE_CONTRACTS.get(source_directory, ()):
        contract = repository_root / relative
        if not contract.is_file():
            raise PublicContractError(
                f"public header compile contract is missing: {relative}"
            )
        for include in INCLUDE_PATTERN.findall(contract.read_text(encoding="utf-8")):
            name = PurePosixPath(include).name
            if name in headers_by_name:
                declared.add(name)
    return declared


def discover_public_headers(repository_root: Path) -> list[str]:
    source_files = _source_files(repository_root)
    headers_by_name: dict[str, list[str]] = {}
    paths = {relative for relative, _path in source_files}
    result: set[str] = set()
    for relative, path in source_files:
        if path.suffix not in HEADER_SUFFIXES:
            continue
        headers_by_name.setdefault(path.name, []).append(relative)
        if PUBLIC_EXPORT_PATTERN.search(path.read_text(encoding="utf-8")):
            result.add(relative)

    def component(path: str) -> str:
        parts = PurePosixPath(path).parts
        if not parts:
            return ""
        depth = 3 if parts[0] == "plugins" else 2
        return "/".join(parts[: min(depth, len(parts))])

    def resolve_include(source: str, include: str) -> str | None:
        candidates: set[str] = set()
        if include in paths:
            candidates.add(include)
        local = posixpath.normpath(
            str(PurePosixPath(source).parent / include)
        )
        if local in paths:
            candidates.add(local)
        suffix_matches = {
            path
            for path in headers_by_name.get(PurePosixPath(include).name, ())
            if path == include or path.endswith(f"/{include}")
        }
        candidates.update(suffix_matches)
        basename_matches = headers_by_name.get(PurePosixPath(include).name, ())
        if not candidates and len(basename_matches) == 1:
            candidates.add(basename_matches[0])
        return next(iter(candidates)) if len(candidates) == 1 else None

    for source, path in source_files:
        source_component = component(source)
        for include in INCLUDE_PATTERN.findall(path.read_text(encoding="utf-8")):
            header = resolve_include(source, include)
            if header is not None and component(header) != source_component:
                result.add(header)

    for contracts in PUBLIC_HEADER_COMPILE_CONTRACTS.values():
        for relative in contracts:
            contract = repository_root / relative
            if not contract.is_file():
                raise PublicContractError(
                    f"public header compile contract is missing: {relative}"
                )
            for include in INCLUDE_PATTERN.findall(
                contract.read_text(encoding="utf-8")
            ):
                matches = headers_by_name.get(PurePosixPath(include).name, ())
                if len(matches) == 1:
                    result.add(matches[0])
    return sorted(result)


def validate_public_headers(repository_root: Path) -> int:
    source_files = _source_files(repository_root)
    includes_by_source = {
        relative: INCLUDE_PATTERN.findall(path.read_text(encoding="utf-8"))
        for relative, path in source_files
    }
    repository_headers: dict[str, list[str]] = {}
    for relative, path in source_files:
        if path.suffix in HEADER_SUFFIXES:
            repository_headers.setdefault(path.name, []).append(relative)

    checked = 0
    leaks: list[str] = []
    for source_directory, header_directories, exclusions, export_macro in (
        PUBLIC_HEADER_OWNERS
    ):
        def is_owned(relative: str) -> bool:
            return any(
                _path_is_within(relative, directory)
                for directory in header_directories
            ) and not any(
                _path_is_within(relative, directory) for directory in exclusions
            )

        owner_headers = {
            path.name: (relative, path)
            for relative, path in source_files
            if path.suffix in HEADER_SUFFIXES and is_owned(relative)
        }
        ambiguous = {
            name: repository_headers[name]
            for name in owner_headers
            if len(repository_headers[name]) != 1
        }
        if ambiguous:
            raise PublicContractError(
                f"ambiguous header basenames for {header_directories}: {ambiguous}"
            )

        consumers: dict[str, set[str]] = {name: set() for name in owner_headers}
        for relative, _path in source_files:
            if is_owned(relative):
                continue
            for include in includes_by_source[relative]:
                name = PurePosixPath(include).name
                if name in consumers:
                    consumers[name].add(relative)

        compile_contracts = _declared_public_header_names(
            repository_root, source_directory, owner_headers
        )
        export_macros = (
            export_macro,
            f"{export_macro}_TEMPLATE",
            f"{export_macro}_INSTANCE",
        )
        for name, (relative, path) in owner_headers.items():
            if not consumers[name]:
                continue
            checked += 1
            text = path.read_text(encoding="utf-8")
            exported = any(
                re.search(rf"\b{re.escape(macro)}\b", text)
                for macro in export_macros
            )
            if not exported and name not in compile_contracts:
                leaks.append(
                    f"{relative} used by {sorted(consumers[name])[:3]}"
                )
    if leaks:
        raise PublicContractError(
            "external internal-header references must be declared: "
            f"{leaks}"
        )
    return checked


def _load_metadata(path: Path) -> dict[str, object]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise PublicContractError(f"plugin metadata does not exist: {path}") from error
    except json.JSONDecodeError as error:
        raise PublicContractError(
            f"invalid plugin metadata {path}: line {error.lineno}, column {error.colno}"
        ) from error
    if not isinstance(value, dict):
        raise PublicContractError(f"plugin metadata must be an object: {path}")
    return value


def _cmake_definitions(repository_root: Path) -> list[tuple[str, str]]:
    plugin_root = repository_root / "plugins"
    if not plugin_root.is_dir():
        return []
    return [
        (
            path.relative_to(repository_root).as_posix(),
            path.read_text(encoding="utf-8"),
        )
        for path in sorted(plugin_root.rglob("CMakeLists.txt"))
        if not any(part in TEST_PATH_PARTS for part in path.parts)
    ]


def _has_cmake_membership(
    definitions: list[tuple[str, str]], owner_target: str, source_name: str
) -> bool:
    target_pattern = re.compile(
        rf"(?<![A-Za-z0-9_]){re.escape(owner_target)}(?![A-Za-z0-9_])"
    )
    source_pattern = re.compile(
        rf"(?<![A-Za-z0-9_]){re.escape(source_name)}(?![A-Za-z0-9_])"
    )
    return any(
        target_pattern.search(text) and source_pattern.search(text)
        for _relative, text in definitions
    )


def validate_plugins(repository_root: Path) -> int:
    identifiers: set[str] = set()
    metadata_paths: set[str] = set()
    cmake_definitions = _cmake_definitions(repository_root)
    count = 0
    for implementation, implementation_file in _source_files(repository_root):
        if not _path_is_within(implementation, "plugins"):
            continue
        if implementation_file.suffix in HEADER_SUFFIXES:
            continue
        text = implementation_file.read_text(encoding="utf-8")
        for registration_macro, metadata_name in PLUGIN_REGISTRATION_PATTERN.findall(text):
            metadata_file = implementation_file.parent / metadata_name
            metadata = metadata_file.relative_to(repository_root).as_posix()
            value = _load_metadata(metadata_file)
            identifier = value.get("Id")
            if not isinstance(identifier, str) or not identifier:
                raise PublicContractError(f"plugin id is missing: {metadata}")
            if identifier in identifiers:
                raise PublicContractError(f"duplicate plugin id: {identifier}")
            if metadata in metadata_paths:
                raise PublicContractError(
                    f"plugin metadata has multiple registrations: {metadata}"
                )
            service_types = value.get("X-KDE-ServiceTypes")
            if (
                not isinstance(service_types, list)
                or len(service_types) != 1
                or not isinstance(service_types[0], str)
                or service_types[0] not in PLUGIN_SERVICE_TYPES
            ):
                raise PublicContractError(
                    f"plugin metadata has an invalid service type: {metadata}"
                )
            metadata_library = value.get("X-KDE-Library")
            if metadata_library is not None and (
                not isinstance(metadata_library, str) or not metadata_library
            ):
                raise PublicContractError(
                    f"plugin metadata has an invalid library: {metadata}"
                )
            owner_target = PLUGIN_OWNER_TARGET_OVERRIDES.get(
                metadata, metadata_library
            )
            if owner_target is None:
                raise PublicContractError(
                    f"plugin owner target cannot be resolved: {metadata}"
                )
            if not _has_cmake_membership(
                cmake_definitions, owner_target, implementation_file.name
            ):
                raise PublicContractError(
                    "plugin owner does not match CMake source membership: "
                    f"{metadata} -> {owner_target}"
                )
            if registration_macro not in text or metadata_name not in text:
                raise PublicContractError(
                    f"plugin registration does not bind its metadata: {implementation}"
                )
            identifiers.add(identifier)
            metadata_paths.add(metadata)
            count += 1

    unused_overrides = sorted(set(PLUGIN_OWNER_TARGET_OVERRIDES) - metadata_paths)
    if unused_overrides:
        raise PublicContractError(
            f"unused plugin owner overrides: {unused_overrides}"
        )
    return count


def main() -> int:
    try:
        header_count = validate_public_headers(REPO_ROOT)
        plugin_count = validate_plugins(REPO_ROOT)
    except (OSError, PublicContractError) as error:
        print(f"check-public-contracts: {error}", file=sys.stderr)
        return 1
    print(
        f"public contracts verified: {header_count} externally used headers, "
        f"{plugin_count} plugin registrations"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
