#!/usr/bin/env python3

from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_package_responsibility_map.py"
MAP_PATH = REPO_ROOT / "docs/architecture/package-responsibilities.json"
GRAPH_DIRECTORY = REPO_ROOT / "docs/architecture"
PUBLIC_SURFACE_INVENTORY = (
    REPO_ROOT / "docs/architecture/public-surface-inventory.json"
)
UI_CLASS_INVENTORY = REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
UI_TOOL_CLASS_INVENTORY = (
    REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
)
SPEC = importlib.util.spec_from_file_location(
    "check_package_responsibility_map", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_package_responsibility_map = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_package_responsibility_map
SPEC.loader.exec_module(check_package_responsibility_map)


class PackageResponsibilityMapTests(unittest.TestCase):
    def load_map(self):
        return check_package_responsibility_map.load_map(MAP_PATH)

    def validate(self, responsibility_map) -> None:
        check_package_responsibility_map.validate_map(
            responsibility_map,
            repository_root=REPO_ROOT,
            graph_directory=GRAPH_DIRECTORY,
            public_surface_inventory_path=PUBLIC_SURFACE_INVENTORY,
            ui_class_inventory_path=UI_CLASS_INVENTORY,
            ui_tool_class_inventory_path=UI_TOOL_CLASS_INVENTORY,
        )

    def test_recorded_current_responsibilities_are_complete(self) -> None:
        responsibility_map = self.load_map()

        self.validate(responsibility_map)

        self.assertEqual(
            responsibility_map["scope"],
            "current-production-package-responsibilities",
        )
        self.assertEqual(len(responsibility_map["responsibilities"]), 9)
        self.assertEqual(len(responsibility_map["targetRelations"]), 23)
        by_id = {
            entry["id"]: entry
            for entry in responsibility_map["responsibilities"]
        }
        self.assertIn(
            "KisApplication",
            by_id["application-orchestration"]["classNames"],
        )
        self.assertIn(
            "Krita PNG Import Filter",
            by_id["import-export"]["pluginIds"],
        )
        self.assertIn(
            "kritadocumentui",
            by_id["document-lifecycle"]["ownerTargets"],
        )
        self.assertIn(
            "libs/document/ui/undo/kis_document_undo_store.h",
            by_id["document-lifecycle"]["publicHeaderPaths"],
        )
        self.assertIn(
            "libs/document/ui/recovery/KisAutoSaveRecoveryDialog.h",
            by_id["document-lifecycle"]["publicHeaderPaths"],
        )
        self.assertIn(
            "libs/document/ui/io/kis_document_io_presentation.h",
            by_id["document-lifecycle"]["publicHeaderPaths"],
        )
        self.assertIn(
            "libs/impex/metadata/KoDocumentInfo.h",
            by_id["import-export"]["publicHeaderPaths"],
        )
        self.assertNotIn(
            "kritacommand",
            by_id["document-lifecycle"]["ownerTargets"],
        )
        self.assertEqual(
            len(by_id["plugin-infrastructure"]["pluginIds"]), 172
        )
        self.assertEqual(
            len(by_id["plugin-infrastructure"]["pluginServiceTypes"]), 14
        )
        self.assertEqual(
            sum(
                "kritaui" in entry["ownerTargets"]
                for entry in responsibility_map["responsibilities"]
            ),
            5,
        )
        self.assertEqual(
            by_id["canvas-presentation"]["reviewedPublicHeaderPaths"],
            ["libs/ui/widgets/KoStrokeConfigWidget.h"],
        )
        self.assertEqual(
            by_id["tool-invocation"]["reviewedSourcePaths"],
            [
                "libs/ui/canvas/kis_tool_proxy.h",
                "libs/ui/dialogs/KisDlgPaletteEditor.cpp",
                "libs/ui/dialogs/kis_dlg_layer_properties.cc",
                "libs/ui/dialogs/kis_dlg_layer_style.cpp",
                "libs/ui/dialogs/kis_dlg_stroke_selection_properties.h",
                "libs/ui/kis_favorite_resource_manager.cpp",
                "libs/ui/tool/kis_delegated_tool.h",
                "libs/ui/tool/kis_figure_painting_tool_helper.h",
                "libs/ui/tool/kis_tool_freehand_helper.h",
                "libs/ui/tool/kis_tool_select_base.h",
                "libs/ui/widgets/KisCompositeOpListConnectionHelper.cpp",
                "libs/ui/widgets/kis_paintop_presets_editor.cpp",
                "libs/ui/widgets/kis_paintop_presets_editor.h",
                "libs/ui/widgets/kis_zoom_scrollbar.cpp",
            ],
        )
        target_by_name = {
            entry["name"]: entry
            for entry in responsibility_map["targetRelations"]
        }
        self.assertIn(
            "kritaimage", target_by_name["kritaui"]["repositoryDependencies"]
        )
        self.assertIn(
            "kritatools", target_by_name["kritaui"]["repositoryDependencies"]
        )
        self.assertEqual(
            target_by_name["kritadocument"]["repositoryDependencies"], []
        )
        self.assertEqual(
            target_by_name["kritadocumentfiles"]["repositoryDependencies"],
            ["kritaglobal", "kritaresourcestorage"],
        )
        self.assertEqual(
            target_by_name["kritadocumentui"]["repositoryDependencies"],
            [
                "kritadocument",
                "kritadocumentfiles",
                "kritaimpex",
                "kritapaintingundo",
                "kritaplugin",
                "kritaresourcestorage",
                "kritawidgets",
                "kritawidgetutils",
            ],
        )
        self.assertIn(
            "kritadocumentui",
            target_by_name["kritaui"]["repositoryDependencies"],
        )
        self.assertIn(
            "kritapngimport", target_by_name["kritaui"]["repositoryConsumers"]
        )

    def test_missing_responsibility_is_rejected(self) -> None:
        responsibility_map = copy.deepcopy(self.load_map())
        responsibility_map["responsibilities"].pop(0)

        with self.assertRaisesRegex(
            check_package_responsibility_map.ResponsibilityMapError,
            r"missing=\['application-orchestration'\]",
        ):
            self.validate(responsibility_map)

    def test_unknown_responsibility_is_rejected(self) -> None:
        responsibility_map = copy.deepcopy(self.load_map())
        responsibility_map["responsibilities"][0]["id"] = "unknown-owner"
        responsibility_map["responsibilities"].sort(key=lambda entry: entry["id"])

        with self.assertRaisesRegex(
            check_package_responsibility_map.ResponsibilityMapError,
            r"unexpected=\['unknown-owner'\]",
        ):
            self.validate(responsibility_map)

    def test_responsibility_without_owner_target_is_rejected(self) -> None:
        responsibility_map = copy.deepcopy(self.load_map())
        responsibility_map["responsibilities"][0]["ownerTargets"] = []

        with self.assertRaisesRegex(
            check_package_responsibility_map.ResponsibilityMapError,
            "responsibility has no owner target: application-orchestration",
        ):
            self.validate(responsibility_map)

    def test_stale_target_relation_is_rejected(self) -> None:
        responsibility_map = copy.deepcopy(self.load_map())
        relation = next(
            entry
            for entry in responsibility_map["targetRelations"]
            if entry["name"] == "kritaui"
        )
        relation["repositoryDependencies"].remove("kritaimage")

        with self.assertRaisesRegex(
            check_package_responsibility_map.ResponsibilityMapError,
            "target relations do not match the recorded CMake graphs",
        ):
            self.validate(responsibility_map)

    def test_duplicate_reviewed_source_path_is_rejected(self) -> None:
        responsibility_map = copy.deepcopy(self.load_map())
        path = responsibility_map["responsibilities"][-1][
            "reviewedSourcePaths"
        ][0]
        responsibility_map["responsibilities"][0][
            "reviewedSourcePaths"
        ].append(path)

        with self.assertRaisesRegex(
            check_package_responsibility_map.ResponsibilityMapError,
            "reviewed source paths must have one responsibility",
        ):
            self.validate(responsibility_map)

    def test_quick_verification_runs_the_responsibility_map_tools(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "python3 scripts/architecture/"
            "update_package_responsibility_map.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/check_package_responsibility_map.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
