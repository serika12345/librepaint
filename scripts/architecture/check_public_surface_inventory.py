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
PUBLICATION_EVIDENCE = ("export-macro", "compile-contract", "external-include")
PUBLIC_HEADER_COMPILE_CONTRACTS = {
    "libs/document/ui": (
        "libs/document/ui/tests/kis_document_autosave_recovery_dialog_test.cpp",
        "libs/document/ui/tests/kis_document_undo_store_test.cpp",
        "libs/document/ui/tests/kis_document_undo_ui_test.cpp",
    ),
    "libs/image": ("libs/painting/tests/TestPublicImageHeaders.cpp",),
    "libs/impex": ("libs/impex/tests/TestImportExportBoundary.cpp",),
    "libs/input": ("libs/input/tests/TestInputShortcutMatcher.cpp",),
    "libs/painting": ("libs/painting/tests/TestPaintingBoundary.cpp",),
    "libs/tools": ("libs/tools/tests/TestToolCoreContract.cpp",),
    "libs/ui": ("libs/ui/tests/TestCanvasUiPublicHeaders.cpp",),
}
INCLUDE_PATTERN = re.compile(
    r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.MULTILINE
)
UI_CLASS_DECLARATION_PATTERN = re.compile(
    r"\b(class|struct)\s+KRITAUI_EXPORT(?:_TEMPLATE)?\s+"
    r"([A-Za-z_][A-Za-z0-9_]*)"
)
PLUGIN_REGISTRATION_PATTERN = re.compile(
    r"\b(K_PLUGIN_(?:FACTORY|CLASS)_WITH_JSON)\s*\("
    r"[^,]+,\s*\"([^\"]+\.json)\"",
    re.DOTALL,
)
PUBLIC_HEADER_SET_SPECS = (
    {
        "ownerTarget": "kritacanvas",
        "sourceDirectory": "libs/canvas",
        "headerDirectories": ["libs/canvas"],
        "excludedHeaderDirectories": ["libs/canvas/tests"],
        "exportMacro": "KRITACANVAS_EXPORT",
        "responsibility": (
            "Records the declared canvas-view surface for coordinate mapping, "
            "display projection and color, animation cache storage, and immutable "
            "snapshots of canvas screen state."
        ),
        "evidence": [
            "libs/canvas/CMakeLists.txt",
            "libs/canvas/KisToolCanvas.h",
            "libs/canvas/kis_coordinates_converter.h",
        ],
    },
    {
        "ownerTarget": "kritadocument",
        "sourceDirectory": "libs/document",
        "headerDirectories": ["libs/document"],
        "excludedHeaderDirectories": [
            "libs/document/files",
            "libs/document/tests",
            "libs/document/ui",
        ],
        "exportMacro": "KRITADOCUMENT_EXPORT",
        "responsibility": (
            "Records the declared document-domain surface for document lifetime "
            "and session state without presentation ownership."
        ),
        "evidence": [
            "libs/document/CMakeLists.txt",
            "libs/document/session/kis_document_identity.h",
        ],
    },
    {
        "ownerTarget": "kritadocumentfiles",
        "sourceDirectory": "libs/document/files",
        "headerDirectories": ["libs/document/files"],
        "excludedHeaderDirectories": ["libs/document/files/tests"],
        "exportMacro": "KRITADOCUMENTFILES_EXPORT",
        "responsibility": (
            "Records the concrete document-file surface for save-target "
            "inspection, backup creation, autosave paths, and recovery files."
        ),
        "evidence": [
            "libs/document/files/CMakeLists.txt",
            "libs/document/files/kis_document_autosave_files.h",
        ],
    },
    {
        "ownerTarget": "kritadocumentui",
        "sourceDirectory": "libs/document/ui",
        "headerDirectories": ["libs/document/ui"],
        "excludedHeaderDirectories": ["libs/document/ui/tests"],
        "exportMacro": "KRITADOCUMENTUI_EXPORT",
        "responsibility": (
            "Records the declared document UI surface for connecting document "
            "history to undo stacks, actions, history presentation, and "
            "autosave recovery selection."
        ),
        "evidence": [
            "libs/document/ui/CMakeLists.txt",
            "libs/document/ui/recovery/KisAutoSaveRecoveryDialog.h",
            "libs/document/ui/undo/kis_document_undo_store.h",
        ],
    },
    {
        "ownerTarget": "kritaimage",
        "sourceDirectory": "libs/image",
        "headerDirectories": ["libs/image"],
        "excludedHeaderDirectories": [],
        "exportMacro": "KRITAIMAGE_EXPORT",
        "responsibility": (
            "Records the declared and de facto inter-package header surface "
            "for image, layer, tile, projection, brush, and stroke state."
        ),
        "evidence": ["libs/image/CMakeLists.txt", "libs/image/kis_image_export.h"],
    },
    {
        "ownerTarget": "kritaimpex",
        "sourceDirectory": "libs/impex",
        "headerDirectories": ["libs/impex"],
        "excludedHeaderDirectories": [
            "libs/impex/animation",
            "libs/impex/tests",
            "libs/impex/ui",
        ],
        "exportMacro": "KRITAIMPEX_EXPORT",
        "responsibility": (
            "Records the declared and de facto inter-package header surface "
            "for format discovery, codec selection, validation, and conversion results."
        ),
        "evidence": ["libs/impex/CMakeLists.txt", "libs/impex/KisImportExportFilter.h"],
    },
    {
        "ownerTarget": "kritaimpexui",
        "sourceDirectory": "libs/impex",
        "headerDirectories": ["libs/impex/animation", "libs/impex/ui"],
        "excludedHeaderDirectories": [],
        "exportMacro": "KRITAUI_EXPORT",
        "responsibility": (
            "Records the declared and de facto inter-package header surface "
            "for document-facing conversion coordination, feedback, and format options."
        ),
        "evidence": ["libs/impex/CMakeLists.txt", "libs/impex/ui/KisImportExportManager.h"],
    },
    {
        "ownerTarget": "kritainput",
        "sourceDirectory": "libs/input",
        "headerDirectories": ["libs/input"],
        "excludedHeaderDirectories": ["libs/input/tests", "libs/input/ui"],
        "exportMacro": "KRITAINPUT_EXPORT",
        "responsibility": (
            "Records the declared and externally consumed input-sequence surface "
            "for shortcut matching, normalized gestures, cancellation, and "
            "borrowed input-action commands."
        ),
        "evidence": [
            "libs/input/CMakeLists.txt",
            "libs/input/KisInputAction.h",
            "libs/input/kis_shortcut_matcher.h",
        ],
    },
    {
        "ownerTarget": "kritainputui",
        "sourceDirectory": "libs/input/ui",
        "headerDirectories": ["libs/input/ui"],
        "excludedHeaderDirectories": ["libs/input/ui/tests"],
        "exportMacro": "KRITAUI_EXPORT",
        "responsibility": (
            "Records the declared and externally consumed input UI surface for "
            "Qt event wiring, shortcut configuration presentation, diagnostics, "
            "and platform integration."
        ),
        "evidence": [
            "libs/input/ui/CMakeLists.txt",
            "libs/input/ui/kis_input_manager.h",
            "libs/input/ui/kis_input_profile_manager.h",
        ],
    },
    {
        "ownerTarget": "kritapainting",
        "sourceDirectory": "libs/painting",
        "headerDirectories": ["libs/painting"],
        "excludedHeaderDirectories": [
            "libs/painting/metadata",
            "libs/painting/tests",
            "libs/painting/undo",
        ],
        "exportMacro": "KRITAPAINTING_EXPORT",
        "responsibility": (
            "Records the declared and externally consumed painting-execution surface "
            "for resource snapshots, figure strokes, and painting jobs."
        ),
        "evidence": [
            "libs/painting/CMakeLists.txt",
            "libs/painting/kis_figure_painting_stroke.h",
        ],
    },
    {
        "ownerTarget": "kritatools",
        "sourceDirectory": "libs/tools",
        "headerDirectories": ["libs/tools"],
        "excludedHeaderDirectories": ["libs/tools/tests", "libs/tools/ui"],
        "exportMacro": "KRITATOOLS_EXPORT",
        "responsibility": (
            "Records the declared and externally consumed tool-command surface "
            "for activation, interaction geometry, settings state, canvas access, "
            "and shared tool policies."
        ),
        "evidence": [
            "libs/tools/CMakeLists.txt",
            "libs/tools/kis_outline_interaction.h",
            "libs/tools/kis_polyline_interaction.h",
            "libs/tools/kis_rectangle_interaction.h",
            "libs/tools/kis_tool.h",
        ],
    },
    {
        "ownerTarget": "kritaui",
        "sourceDirectory": "libs/ui",
        "headerDirectories": ["libs/ui"],
        "excludedHeaderDirectories": [],
        "exportMacro": "KRITAUI_EXPORT",
        "responsibility": (
            "Records the declared and de facto inter-package header surface "
            "for application, document, canvas, input, tool, and UI coordination."
        ),
        "evidence": ["libs/ui/CMakeLists.txt", "libs/ui/kritaui_export_instance.h"],
    },
)
UI_CLASS_RESPONSIBILITY_AREAS = (
    {
        "id": "application-orchestration",
        "responsibility": (
            "Coordinates process startup, application services, actions, plugins, "
            "and the shared application object graph."
        ),
    },
    {
        "id": "canvas-display",
        "responsibility": (
            "Coordinates canvas presentation, display configuration, animation "
            "playback and caches, reference overlays, and view transforms."
        ),
    },
    {
        "id": "document-state",
        "responsibility": (
            "Coordinates document metadata, image and node operations, selections, "
            "undo state, and document-facing feature managers."
        ),
    },
    {
        "id": "tool-invocation",
        "responsibility": (
            "Coordinates toolbar-level tool selection, filter settings, and "
            "invocation while reusable settings presentation remains with tools."
        ),
    },
    {
        "id": "window-workspace",
        "responsibility": (
            "Coordinates windows, views, workspace and session layouts, templates, "
            "preferences, status presentation, and startup surfaces."
        ),
    },
)
UI_TOOL_CLASS_RESPONSIBILITY_AREAS = (
    {
        "id": "input-interpretation",
        "responsibility": (
            "Transforms pointer events, coordinates, timing, pressure, and sampled "
            "canvas state into normalized painting input."
        ),
    },
    {
        "id": "tool-invocation",
        "responsibility": (
            "Coordinates tool factories, activation, interaction state, geometry, "
            "and invocation of document or painting operations."
        ),
    },
    {
        "id": "stroke-generation",
        "responsibility": (
            "Combines interpreted input and resource snapshots into stroke jobs and "
            "submits their lifecycle to the stroke facade."
        ),
    },
    {
        "id": "painting-execution",
        "responsibility": (
            "Executes painting, compositing, selection, shape, and image mutation "
            "work within stroke or command processing."
        ),
    },
    {
        "id": "settings-presentation",
        "responsibility": (
            "Owns tool option widgets and UI-facing configuration, brush-size, and "
            "stroke-status state."
        ),
    },
)
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
PLUGIN_SERVICE_TYPE_OWNERS = (
    {
        "serviceType": "Krita/ApplicationPlugin",
        "featureOwner": "application-extension",
        "runtimeConsumer": "KisMainWindow",
        "evidence": "libs/ui/KisMainWindow.cpp",
    },
    {
        "serviceType": "Krita/ColorSpace",
        "featureOwner": "color-processing",
        "runtimeConsumer": "KoColorSpaceRegistry",
        "evidence": "libs/pigment/KoColorSpaceRegistry.cpp",
    },
    {
        "serviceType": "Krita/ColorSpaceExtension",
        "featureOwner": "color-processing",
        "runtimeConsumer": "KoColorSpaceRegistry",
        "evidence": "libs/pigment/KoColorSpaceRegistry.cpp",
    },
    {
        "serviceType": "Krita/Dock",
        "featureOwner": "docker-presentation",
        "runtimeConsumer": "KoDockRegistry",
        "evidence": "libs/flake/KoDockRegistry.cpp",
    },
    {
        "serviceType": "Krita/FileFilter",
        "featureOwner": "import-export",
        "runtimeConsumer": "KisImportExportFilterRegistry",
        "evidence": "libs/impex/KisImportExportFilterRegistry.cpp",
    },
    {
        "serviceType": "Krita/Filter",
        "featureOwner": "image-filtering",
        "runtimeConsumer": "KisFilterRegistry",
        "evidence": "libs/image/filter/kis_filter_registry.cc",
    },
    {
        "serviceType": "Krita/Flake",
        "featureOwner": "vector-shape",
        "runtimeConsumer": "KoShapeRegistry",
        "evidence": "libs/flake/KoShapeRegistry.cpp",
    },
    {
        "serviceType": "Krita/Generator",
        "featureOwner": "image-generation",
        "runtimeConsumer": "KisGeneratorRegistry",
        "evidence": "libs/image/generator/kis_generator_registry.cpp",
    },
    {
        "serviceType": "Krita/Metadata",
        "featureOwner": "document-metadata",
        "runtimeConsumer": "KisMetadataBackendRegistry",
        "evidence": "libs/painting/metadata/kis_meta_data_backend_registry.cpp",
    },
    {
        "serviceType": "Krita/Paintop",
        "featureOwner": "painting-execution",
        "runtimeConsumer": "KisPaintOpRegistry",
        "evidence": "libs/image/brushengine/kis_paintop_registry.cc",
    },
    {
        "serviceType": "Krita/PlatformPlugin",
        "featureOwner": "platform-adapter",
        "runtimeConsumer": "KisPlatformPluginInterfaceFactory",
        "evidence": "libs/ui/KisPlatformPluginInterfaceFactory.cpp",
    },
    {
        "serviceType": "Krita/Shape",
        "featureOwner": "vector-shape",
        "runtimeConsumer": "KoShapeRegistry",
        "evidence": "libs/flake/KoShapeRegistry.cpp",
    },
    {
        "serviceType": "Krita/Tool",
        "featureOwner": "tool-invocation",
        "runtimeConsumer": "KoToolRegistry",
        "evidence": "libs/flake/KoToolRegistry.cpp",
    },
    {
        "serviceType": "Krita/ViewPlugin",
        "featureOwner": "view-extension",
        "runtimeConsumer": "KisMainWindow",
        "evidence": "libs/ui/KisMainWindow.cpp",
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
    header_directories: list[str] | None = None,
    excluded_header_directories: list[str] | None = None,
) -> list[dict[str, Any]]:
    """Return the complete declared or used production header surface."""

    header_directories = header_directories or [source_directory]
    excluded_header_directories = excluded_header_directories or []

    def is_owned_path(relative: str) -> bool:
        return any(
            _path_is_within(relative, directory) for directory in header_directories
        ) and not any(
            _path_is_within(relative, directory)
            for directory in excluded_header_directories
        )

    source_files = _source_files(repository_root)
    owner_headers = [
        (relative, path)
        for relative, path in source_files
        if path.suffix in HEADER_SUFFIXES
        and is_owned_path(relative)
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
            f"ambiguous header basenames for {header_directories}: {ambiguous}"
        )

    consumers_by_name: dict[str, set[str]] = {
        name: set() for name in headers_by_name
    }
    for relative, path in source_files:
        if is_owned_path(relative):
            continue
        text = path.read_text(encoding="utf-8")
        for include in INCLUDE_PATTERN.findall(text):
            name = PurePosixPath(include).name
            if name in consumers_by_name:
                consumers_by_name[name].add(relative)

    compile_contract_headers: set[str] = set()
    for relative in PUBLIC_HEADER_COMPILE_CONTRACTS.get(source_directory, ()):
        path = repository_root / relative
        if not path.is_file():
            raise PublicSurfaceError(f"public header compile contract is missing: {relative}")
        for include in INCLUDE_PATTERN.findall(path.read_text(encoding="utf-8")):
            name = PurePosixPath(include).name
            if name in headers_by_name:
                compile_contract_headers.add(name)

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
        if name in compile_contract_headers:
            publication_evidence.append("compile-contract")
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


def _discover_ui_classes(
    *,
    repository_root: Path,
    public_surface_inventory: dict[str, Any],
    source_directory: str,
    recursive: bool,
    include_consumer_paths: bool,
) -> list[dict[str, Any]]:
    ui_header_set = next(
        (
            entry
            for entry in _require_array(
                public_surface_inventory.get("publicHeaderSets"),
                "publicHeaderSets",
            )
            if _require_object(entry, "public header set").get("ownerTarget")
            == "kritaui"
        ),
        None,
    )
    if ui_header_set is None:
        raise PublicSurfaceError("publicHeaderSets does not contain kritaui")

    all_source_files = _source_files(repository_root)
    source_files = {
        relative: path
        for relative, path in all_source_files
        if _path_is_within(relative, source_directory)
        and path.suffix in SOURCE_SUFFIXES - HEADER_SUFFIXES
    }
    header_entries = [
        _require_object(entry, "kritaui public header")
        for entry in _require_array(
            ui_header_set.get("headers"), "headers for public header set kritaui"
        )
        if _path_is_within(
            _require_string(
                _require_object(entry, "kritaui public header").get("path"),
                "path for kritaui public header",
            ),
            source_directory,
        )
    ]
    consumer_paths_by_header: dict[str, set[str]] = {}
    if include_consumer_paths:
        headers_by_name = {
            PurePosixPath(
                _require_string(entry.get("path"), "path for kritaui public header")
            ).name: _require_string(
                entry.get("path"), "path for kritaui public header"
            )
            for entry in header_entries
        }
        consumer_paths_by_header = {
            header_path: set() for header_path in headers_by_name.values()
        }
        for relative, source in all_source_files:
            if _path_is_within(relative, source_directory):
                continue
            for include in INCLUDE_PATTERN.findall(
                source.read_text(encoding="utf-8")
            ):
                header_path = headers_by_name.get(PurePosixPath(include).name)
                if header_path is not None:
                    consumer_paths_by_header[header_path].add(relative)

    classes: list[dict[str, Any]] = []
    names: set[str] = set()
    for header in header_entries:
        header_path = _require_string(
            header.get("path"), "path for kritaui public header"
        )
        path = PurePosixPath(header_path)
        if not _path_is_within(header_path, source_directory):
            continue
        relative_path = path.relative_to(PurePosixPath(source_directory))
        if not relative_path.parts or (
            not recursive and len(relative_path.parts) != 1
        ):
            continue
        consumer_paths = sorted(consumer_paths_by_header.get(header_path, set()))
        header_text = (repository_root / header_path).read_text(encoding="utf-8")
        for declaration_kind, name in UI_CLASS_DECLARATION_PATTERN.findall(
            header_text
        ):
            if name in names:
                raise PublicSurfaceError(
                    "duplicate exported UI class name below "
                    f"{source_directory}: {name}"
                )
            implementation_stems = [path.with_suffix("").as_posix()]
            if path.stem.endswith("_p"):
                implementation_stems.append(
                    path.with_name(path.stem.removesuffix("_p")).as_posix()
                )
            implementation_paths = []
            symbol_pattern = re.compile(rf"\b{re.escape(name)}\s*::")
            for stem in implementation_stems:
                for suffix in sorted(SOURCE_SUFFIXES - HEADER_SUFFIXES):
                    implementation = f"{stem}{suffix}"
                    source = source_files.get(implementation)
                    if source is not None and symbol_pattern.search(
                        source.read_text(encoding="utf-8")
                    ):
                        implementation_paths.append(implementation)
            entry = {
                "name": name,
                "declarationKind": declaration_kind,
                "header": header_path,
                "implementationPaths": sorted(set(implementation_paths)),
            }
            if include_consumer_paths:
                entry["consumerPaths"] = consumer_paths
            classes.append(entry)
            names.add(name)
    return sorted(classes, key=lambda entry: entry["name"])


def discover_ui_top_level_classes(
    *,
    repository_root: Path,
    public_surface_inventory: dict[str, Any],
) -> list[dict[str, Any]]:
    return _discover_ui_classes(
        repository_root=repository_root,
        public_surface_inventory=public_surface_inventory,
        source_directory="libs/ui",
        recursive=False,
        include_consumer_paths=False,
    )


def discover_ui_tool_classes(
    *,
    repository_root: Path,
    public_surface_inventory: dict[str, Any],
) -> list[dict[str, Any]]:
    return _discover_ui_classes(
        repository_root=repository_root,
        public_surface_inventory=public_surface_inventory,
        source_directory="libs/ui/tool",
        recursive=True,
        include_consumer_paths=True,
    )


def discover_plugins(
    *,
    repository_root: Path,
    graph_directory: Path,
) -> list[dict[str, Any]]:
    targets_by_platform = _load_target_graphs(graph_directory)
    service_type_owners = {
        entry["serviceType"]: entry for entry in PLUGIN_SERVICE_TYPE_OWNERS
    }
    plugins: list[dict[str, Any]] = []
    identifiers: set[str] = set()
    discovered_metadata: set[str] = set()

    for implementation, implementation_file in _source_files(repository_root):
        if (
            not _path_is_within(implementation, "plugins")
            or implementation_file.suffix
            not in SOURCE_SUFFIXES - HEADER_SUFFIXES
        ):
            continue
        implementation_text = implementation_file.read_text(encoding="utf-8")
        for registration_macro, metadata_name in PLUGIN_REGISTRATION_PATTERN.findall(
            implementation_text
        ):
            metadata_path = implementation_file.parent / metadata_name
            metadata = metadata_path.relative_to(repository_root).as_posix()
            metadata_value = _load_json(metadata_path, f"plugin metadata {metadata}")
            identifier = _require_string(
                metadata_value.get("Id"), f"plugin id in {metadata}"
            )
            if identifier in identifiers:
                raise PublicSurfaceError(f"duplicate plugin id: {identifier}")
            if metadata in discovered_metadata:
                raise PublicSurfaceError(
                    f"plugin metadata has multiple registrations: {metadata}"
                )

            service_types = [
                _require_string(value, f"service type in {metadata}")
                for value in _require_array(
                    metadata_value.get("X-KDE-ServiceTypes"),
                    f"service types in {metadata}",
                )
            ]
            if len(service_types) != 1:
                raise PublicSurfaceError(
                    f"plugin metadata must declare one service type: {metadata}"
                )
            service_owner = service_type_owners.get(service_types[0])
            if service_owner is None:
                raise PublicSurfaceError(
                    f"plugin metadata has an unknown service type: {metadata}"
                )

            metadata_library = metadata_value.get("X-KDE-Library")
            if metadata_library is not None and (
                not isinstance(metadata_library, str) or not metadata_library
            ):
                raise PublicSurfaceError(
                    f"plugin metadata has an invalid library name: {metadata}"
                )
            owner_target = PLUGIN_OWNER_TARGET_OVERRIDES.get(
                metadata, metadata_library
            )
            if owner_target is None:
                raise PublicSurfaceError(
                    f"plugin owner target cannot be resolved: {metadata}"
                )
            owner_evidence = (
                "cmake-source-override"
                if metadata in PLUGIN_OWNER_TARGET_OVERRIDES
                else "metadata-library"
            )
            if owner_evidence == "cmake-source-override":
                cmake_path = metadata_path.parent / "CMakeLists.txt"
                cmake_text = cmake_path.read_text(encoding="utf-8")
                if (
                    owner_target not in cmake_text
                    or implementation_file.name not in cmake_text
                ):
                    raise PublicSurfaceError(
                        "plugin owner override does not match CMake source "
                        f"membership: {metadata}"
                    )

            platforms = _available_platforms(owner_target, targets_by_platform)
            if not platforms:
                raise PublicSurfaceError(
                    f"plugin owner target is absent from every target graph: "
                    f"{owner_target}"
                )
            plugins.append(
                {
                    "id": identifier,
                    "metadata": metadata,
                    "implementation": implementation,
                    "ownerTarget": owner_target,
                    "ownerEvidence": owner_evidence,
                    "metadataLibrary": metadata_library,
                    "platforms": platforms,
                    "serviceTypes": service_types,
                    "registrationMacro": registration_macro,
                    "featureOwner": service_owner["featureOwner"],
                    "runtimeConsumer": service_owner["runtimeConsumer"],
                }
            )
            identifiers.add(identifier)
            discovered_metadata.add(metadata)

    unused_overrides = sorted(
        set(PLUGIN_OWNER_TARGET_OVERRIDES) - discovered_metadata
    )
    if unused_overrides:
        raise PublicSurfaceError(
            f"plugin owner overrides do not match registrations: {unused_overrides}"
        )
    return sorted(plugins, key=lambda entry: entry["id"])


def public_header_policy() -> dict[str, Any]:
    return {
        "headerSuffixes": sorted(HEADER_SUFFIXES),
        "sourceSuffixes": sorted(SOURCE_SUFFIXES),
        "productionSourceDirectories": list(PRODUCTION_SOURCE_DIRECTORIES),
        "excludedPathParts": sorted(TEST_PATH_PARTS),
        "excludedFileSuffixes": ["_test.cc", "_test.cpp", "_test.cxx"],
        "publicationEvidence": list(PUBLICATION_EVIDENCE),
    }


def ui_class_policy() -> dict[str, Any]:
    return {
        "publicHeaderOwner": "kritaui",
        "sourceDirectory": "libs/ui",
        "headerDepth": 1,
        "declarationKinds": ["class", "struct"],
        "exportMacros": ["KRITAUI_EXPORT", "KRITAUI_EXPORT_TEMPLATE"],
        "implementationSuffixes": sorted(SOURCE_SUFFIXES - HEADER_SUFFIXES),
        "privateHeaderImplementationSuffix": "_p",
    }


def ui_tool_class_policy() -> dict[str, Any]:
    return {
        "publicHeaderOwner": "kritaui",
        "sourceDirectory": "libs/ui/tool",
        "recursive": True,
        "declarationKinds": ["class", "struct"],
        "exportMacros": ["KRITAUI_EXPORT", "KRITAUI_EXPORT_TEMPLATE"],
        "implementationSuffixes": sorted(SOURCE_SUFFIXES - HEADER_SUFFIXES),
        "privateHeaderImplementationSuffix": "_p",
        "consumerPathsSource": "production-includes-outside-source-directory",
    }


def plugin_policy() -> dict[str, Any]:
    return {
        "sourceDirectory": "plugins",
        "excludedPathParts": sorted(TEST_PATH_PARTS),
        "registrationMacros": [
            "K_PLUGIN_CLASS_WITH_JSON",
            "K_PLUGIN_FACTORY_WITH_JSON",
        ],
        "metadataArgument": "sibling-json-literal",
        "ownerEvidence": ["cmake-source-override", "metadata-library"],
        "ownerTargetOverrideReason": (
            "CMake source membership is authoritative when plugin metadata omits "
            "X-KDE-Library or names a target absent from the recorded graphs."
        ),
        "ownerTargetOverrides": [
            {
                "metadata": metadata,
                "ownerTarget": owner_target,
                "cmake": str(PurePosixPath(metadata).parent / "CMakeLists.txt"),
            }
            for metadata, owner_target in sorted(
                PLUGIN_OWNER_TARGET_OVERRIDES.items()
            )
        ],
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
                    header_directories=spec["headerDirectories"],
                    excluded_header_directories=spec["excludedHeaderDirectories"],
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


def load_ui_class_inventory(path: Path) -> dict[str, Any]:
    return _load_json(path, "UI class responsibility inventory")


def load_ui_tool_class_inventory(path: Path) -> dict[str, Any]:
    return _load_json(path, "UI tool class responsibility inventory")


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
        "headerDirectories",
        "excludedHeaderDirectories",
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
        header_directories = _require_array(
            entry.get("headerDirectories"), f"header directories for {description}"
        )
        excluded_header_directories = _require_array(
            entry.get("excludedHeaderDirectories"),
            f"excluded header directories for {description}",
        )
        if header_directories != spec["headerDirectories"]:
            raise PublicSurfaceError(
                f"{description}: header directories must be {spec['headerDirectories']}"
            )
        if excluded_header_directories != spec["excludedHeaderDirectories"]:
            raise PublicSurfaceError(
                f"{description}: excluded header directories must be "
                f"{spec['excludedHeaderDirectories']}"
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
                    or (
                        any(
                            _path_is_within(consumer, directory)
                            for directory in header_directories
                        )
                        and not any(
                            _path_is_within(consumer, directory)
                            for directory in excluded_header_directories
                        )
                    )
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
            header_directories=header_directories,
            excluded_header_directories=excluded_header_directories,
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
    graph_directory: Path,
) -> None:
    if inventory.get("pluginPolicy") != plugin_policy():
        raise PublicSurfaceError(
            "pluginPolicy must match the repository plugin discovery policy"
        )
    service_owner_entries = _require_array(
        inventory.get("pluginServiceTypeOwners"), "pluginServiceTypeOwners"
    )
    if service_owner_entries != list(PLUGIN_SERVICE_TYPE_OWNERS):
        raise PublicSurfaceError(
            "pluginServiceTypeOwners must match the service ownership policy"
        )
    service_owners: dict[str, dict[str, Any]] = {}
    for index, item in enumerate(service_owner_entries):
        entry = _require_object(item, f"plugin service owner {index}")
        _require_fields(
            entry,
            {"serviceType", "featureOwner", "runtimeConsumer", "evidence"},
            f"plugin service owner {index}",
        )
        service_type = _require_string(
            entry.get("serviceType"), f"plugin service type {index}"
        )
        feature_owner = _require_string(
            entry.get("featureOwner"), f"feature owner for {service_type}"
        )
        runtime_consumer = _require_string(
            entry.get("runtimeConsumer"),
            f"runtime consumer for {service_type}",
        )
        _evidence, evidence_file = _repository_file(
            repository_root,
            entry.get("evidence"),
            f"runtime evidence for {service_type}",
        )
        evidence_text = evidence_file.read_text(encoding="utf-8")
        if (
            service_type not in evidence_text
            or runtime_consumer not in evidence_text
        ):
            raise PublicSurfaceError(
                f"runtime evidence does not bind {service_type} to "
                f"{runtime_consumer}"
            )
        service_owners[service_type] = {
            "featureOwner": feature_owner,
            "runtimeConsumer": runtime_consumer,
        }

    expected_plugins = discover_plugins(
        repository_root=repository_root,
        graph_directory=graph_directory,
    )
    expected_by_id = {entry["id"]: entry for entry in expected_plugins}
    entries = _require_array(inventory.get("plugins"), "plugins")
    identifiers: list[str] = []
    recorded_by_id: dict[str, dict[str, Any]] = {}
    expected_fields = {
        "id",
        "metadata",
        "implementation",
        "ownerTarget",
        "ownerEvidence",
        "metadataLibrary",
        "platforms",
        "serviceTypes",
        "registrationMacro",
        "featureOwner",
        "runtimeConsumer",
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
        owner_evidence = _require_string(
            entry.get("ownerEvidence"), f"owner evidence for {description}"
        )
        if owner_evidence not in {"cmake-source-override", "metadata-library"}:
            raise PublicSurfaceError(
                f"unknown owner evidence for {description}: {owner_evidence}"
            )
        metadata_library = entry.get("metadataLibrary")
        if metadata_library is not None and (
            not isinstance(metadata_library, str) or not metadata_library
        ):
            raise PublicSurfaceError(
                f"metadata library for {description} must be a string or null"
            )
        feature_owner = _require_string(
            entry.get("featureOwner"), f"feature owner for {description}"
        )
        runtime_consumer = _require_string(
            entry.get("runtimeConsumer"), f"runtime consumer for {description}"
        )
        _validate_owner(
            owner_target=owner_target,
            platforms=platforms,
            owned_paths=[metadata, implementation],
            description=description,
            targets_by_platform=targets_by_platform,
        )
        for platform in platforms:
            target_type = _require_string(
                targets_by_platform[platform][owner_target].get("type"),
                f"target type for {owner_target} on {platform}",
            )
            if target_type not in {"MODULE_LIBRARY", "STATIC_LIBRARY"}:
                raise PublicSurfaceError(
                    f"{description}: owner target type is {target_type} on "
                    f"{platform}"
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
        if metadata_value.get("X-KDE-Library") != metadata_library:
            raise PublicSurfaceError(
                f"metadata library does not match {description}"
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
        if len(service_types) != 1 or service_types[0] not in service_owners:
            raise PublicSurfaceError(
                f"{description}: service type has no recorded feature owner"
            )
        service_owner = service_owners[service_types[0]]
        if (
            feature_owner != service_owner["featureOwner"]
            or runtime_consumer != service_owner["runtimeConsumer"]
        ):
            raise PublicSurfaceError(
                f"{description}: feature owner or runtime consumer does not "
                "match its service type"
            )
        recorded_by_id[identifier] = {
            "id": identifier,
            "metadata": metadata,
            "implementation": implementation,
            "ownerTarget": owner_target,
            "ownerEvidence": owner_evidence,
            "metadataLibrary": metadata_library,
            "platforms": platforms,
            "serviceTypes": service_types,
            "registrationMacro": registration_macro,
            "featureOwner": feature_owner,
            "runtimeConsumer": runtime_consumer,
        }
        identifiers.append(identifier)
    if identifiers != sorted(set(identifiers)):
        raise PublicSurfaceError("plugins must be sorted and unique by id")

    missing = sorted(set(expected_by_id) - set(recorded_by_id))
    unexpected = sorted(set(recorded_by_id) - set(expected_by_id))
    if missing or unexpected:
        raise PublicSurfaceError(
            "plugin inventory does not match discovered registrations; "
            f"missing={missing}, unexpected={unexpected}"
        )
    for identifier in identifiers:
        if recorded_by_id[identifier] != expected_by_id[identifier]:
            raise PublicSurfaceError(
                f"recorded plugin evidence for {identifier} does not match "
                "source discovery"
            )


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
            "pluginPolicy",
            "pluginServiceTypeOwners",
            "plugins",
        },
        "public-surface inventory",
    )
    if inventory.get("schemaVersion") != 3:
        raise PublicSurfaceError("public-surface inventory schemaVersion must be 3")
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
    if scope.get("majorClasses") not in {"representative", "complete"}:
        raise PublicSurfaceError(
            "inventory scope for majorClasses must be representative or complete"
        )
    if scope.get("plugins") != "complete":
        raise PublicSurfaceError("plugin inventory scope must be complete")
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
    _validate_major_classes(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
        public_headers=public_headers,
    )
    _validate_plugins(
        inventory,
        repository_root=repository_root,
        targets_by_platform=targets_by_platform,
        graph_directory=graph_directory,
    )


def _validate_classified_ui_inventory(
    inventory: dict[str, Any],
    *,
    repository_root: Path,
    graph_directory: Path,
    inventory_description: str,
    expected_scope: str,
    expected_policy: dict[str, Any],
    responsibility_areas: tuple[dict[str, str], ...],
    expected_classes: list[dict[str, Any]],
    include_consumer_paths: bool,
    ownership_description: str,
) -> None:
    _require_fields(
        inventory,
        {
            "schemaVersion",
            "scope",
            "ownerTarget",
            "platforms",
            "classPolicy",
            "responsibilityAreas",
            "classes",
        },
        inventory_description,
    )
    if inventory.get("schemaVersion") != 1:
        raise PublicSurfaceError(
            f"{inventory_description} schemaVersion must be 1"
        )
    if inventory.get("scope") != expected_scope:
        raise PublicSurfaceError(
            f"{inventory_description} has an invalid scope"
        )
    owner_target = _require_string(
        inventory.get("ownerTarget"), "UI class owner target"
    )
    if owner_target != "kritaui":
        raise PublicSurfaceError("UI class owner target must be kritaui")
    platforms = _platforms(
        inventory.get("platforms"), "UI class inventory platforms"
    )
    if inventory.get("classPolicy") != expected_policy:
        raise PublicSurfaceError(
            f"classPolicy must match the {inventory_description} discovery policy"
        )
    if inventory.get("responsibilityAreas") != list(responsibility_areas):
        raise PublicSurfaceError(
            f"responsibilityAreas must match the recorded {inventory_description} "
            "policy"
        )

    expected_by_name = {entry["name"]: entry for entry in expected_classes}
    known_areas = {entry["id"] for entry in responsibility_areas}
    entries = _require_array(inventory.get("classes"), "UI classes")
    names: list[str] = []
    recorded_by_name: dict[str, dict[str, Any]] = {}
    owned_paths: set[str] = set()
    used_areas: set[str] = set()
    for index, item in enumerate(entries):
        entry = _require_object(item, f"UI class {index}")
        expected_fields = {
            "name",
            "declarationKind",
            "header",
            "implementationPaths",
            "responsibilityArea",
        }
        if include_consumer_paths:
            expected_fields.add("consumerPaths")
        _require_fields(entry, expected_fields, f"UI class {index}")
        name = _require_string(entry.get("name"), f"UI class name {index}")
        area = _require_string(
            entry.get("responsibilityArea"), f"responsibility area for {name}"
        )
        if area not in known_areas:
            raise PublicSurfaceError(
                f"UI class {name} has unknown responsibility area {area}"
            )
        implementation_values = _require_array(
            entry.get("implementationPaths"), f"implementation paths for {name}"
        )
        implementation_paths = [
            _repository_file(
                repository_root,
                value,
                f"implementation path for {name}",
            )[0]
            for value in implementation_values
        ]
        if implementation_paths != sorted(set(implementation_paths)):
            raise PublicSurfaceError(
                f"implementation paths for {name} must be sorted and unique"
            )
        header, _header_file = _repository_file(
            repository_root, entry.get("header"), f"header for UI class {name}"
        )
        recorded = {
            "name": name,
            "declarationKind": _require_string(
                entry.get("declarationKind"), f"declaration kind for {name}"
            ),
            "header": header,
            "implementationPaths": implementation_paths,
        }
        if include_consumer_paths:
            consumer_values = _require_array(
                entry.get("consumerPaths"), f"consumer paths for {name}"
            )
            consumer_paths = [
                _repository_file(
                    repository_root,
                    value,
                    f"consumer path for {name}",
                )[0]
                for value in consumer_values
            ]
            if consumer_paths != sorted(set(consumer_paths)):
                raise PublicSurfaceError(
                    f"consumer paths for {name} must be sorted and unique"
                )
            recorded["consumerPaths"] = consumer_paths
        recorded_by_name[name] = recorded
        names.append(name)
        owned_paths.update([header, *implementation_paths])
        used_areas.add(area)
    if names != sorted(set(names)):
        raise PublicSurfaceError("UI classes must be sorted and unique by name")

    missing = sorted(set(expected_by_name) - set(recorded_by_name))
    unexpected = sorted(set(recorded_by_name) - set(expected_by_name))
    if missing or unexpected:
        raise PublicSurfaceError(
            "UI class inventory does not match discovered candidates; "
            f"missing={missing}, unexpected={unexpected}"
        )
    for name in names:
        if recorded_by_name[name] != expected_by_name[name]:
            raise PublicSurfaceError(
                f"recorded source evidence for {name} does not "
                "match source discovery"
            )
    if used_areas != known_areas:
        raise PublicSurfaceError(
            "every UI responsibility area must own at least one recorded class"
        )

    targets_by_platform = _load_target_graphs(graph_directory)
    _validate_owner(
        owner_target=owner_target,
        platforms=platforms,
        owned_paths=sorted(owned_paths),
        description=ownership_description,
        targets_by_platform=targets_by_platform,
    )


def validate_ui_class_inventory(
    inventory: dict[str, Any],
    *,
    public_surface_inventory: dict[str, Any],
    repository_root: Path,
    graph_directory: Path,
) -> None:
    _validate_classified_ui_inventory(
        inventory,
        repository_root=repository_root,
        graph_directory=graph_directory,
        inventory_description="UI class responsibility inventory",
        expected_scope="libs/ui-top-level-public-classes",
        expected_policy=ui_class_policy(),
        responsibility_areas=UI_CLASS_RESPONSIBILITY_AREAS,
        expected_classes=discover_ui_top_level_classes(
            repository_root=repository_root,
            public_surface_inventory=public_surface_inventory,
        ),
        include_consumer_paths=False,
        ownership_description="UI top-level public classes",
    )


def validate_ui_tool_class_inventory(
    inventory: dict[str, Any],
    *,
    public_surface_inventory: dict[str, Any],
    repository_root: Path,
    graph_directory: Path,
) -> None:
    _validate_classified_ui_inventory(
        inventory,
        repository_root=repository_root,
        graph_directory=graph_directory,
        inventory_description="UI tool class responsibility inventory",
        expected_scope="libs/ui/tool-public-classes",
        expected_policy=ui_tool_class_policy(),
        responsibility_areas=UI_TOOL_CLASS_RESPONSIBILITY_AREAS,
        expected_classes=discover_ui_tool_classes(
            repository_root=repository_root,
            public_surface_inventory=public_surface_inventory,
        ),
        include_consumer_paths=True,
        ownership_description="UI tool public classes",
    )


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Validate complete public-header, top-level UI class, and UI tool "
            "class inventories, complete product plugins, and representative "
            "major classes against every recorded CMake target graph."
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
    parser.add_argument(
        "--ui-class-inventory",
        type=Path,
        default=REPO_ROOT / "docs/architecture/ui-class-responsibilities.json",
    )
    parser.add_argument(
        "--ui-tool-class-inventory",
        type=Path,
        default=(
            REPO_ROOT
            / "docs/architecture/ui-tool-class-responsibilities.json"
        ),
    )
    return parser


def main(arguments: list[str] | None = None) -> int:
    options = _argument_parser().parse_args(arguments)
    try:
        inventory = load_inventory(options.inventory)
        ui_class_inventory = load_ui_class_inventory(options.ui_class_inventory)
        ui_tool_class_inventory = load_ui_tool_class_inventory(
            options.ui_tool_class_inventory
        )
        validate_inventory(
            inventory,
            repository_root=REPO_ROOT,
            graph_directory=options.graph_directory,
        )
        validate_ui_class_inventory(
            ui_class_inventory,
            public_surface_inventory=inventory,
            repository_root=REPO_ROOT,
            graph_directory=options.graph_directory,
        )
        validate_ui_tool_class_inventory(
            ui_tool_class_inventory,
            public_surface_inventory=inventory,
            repository_root=REPO_ROOT,
            graph_directory=options.graph_directory,
        )
    except (OSError, PublicSurfaceError) as error:
        print(f"check-public-surface-inventory: {error}", file=sys.stderr)
        return 1
    print(
        "public surface and UI class responsibility inventories verified: "
        f"{options.inventory}, {options.ui_class_inventory}, "
        f"{options.ui_tool_class_inventory}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
