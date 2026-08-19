#!/usr/bin/env python3

from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_public_surface_inventory.py"
INVENTORY_PATH = REPO_ROOT / "docs/architecture/public-surface-inventory.json"
UI_CLASS_INVENTORY_PATH = (
    REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
)
UI_TOOL_CLASS_INVENTORY_PATH = (
    REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
)
GRAPH_DIRECTORY = REPO_ROOT / "docs/architecture"
SPEC = importlib.util.spec_from_file_location(
    "check_public_surface_inventory", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_public_surface_inventory = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_public_surface_inventory
SPEC.loader.exec_module(check_public_surface_inventory)


class PublicSurfaceInventoryTests(unittest.TestCase):
    def load_inventory(self):
        return check_public_surface_inventory.load_inventory(INVENTORY_PATH)

    def load_ui_class_inventory(self):
        return check_public_surface_inventory.load_ui_class_inventory(
            UI_CLASS_INVENTORY_PATH
        )

    def load_ui_tool_class_inventory(self):
        return check_public_surface_inventory.load_ui_tool_class_inventory(
            UI_TOOL_CLASS_INVENTORY_PATH
        )

    def validate(self, inventory) -> None:
        check_public_surface_inventory.validate_inventory(
            inventory,
            repository_root=REPO_ROOT,
            graph_directory=GRAPH_DIRECTORY,
        )

    def validate_ui_classes(self, inventory) -> None:
        check_public_surface_inventory.validate_ui_class_inventory(
            inventory,
            public_surface_inventory=self.load_inventory(),
            repository_root=REPO_ROOT,
            graph_directory=GRAPH_DIRECTORY,
        )

    def validate_ui_tool_classes(self, inventory) -> None:
        check_public_surface_inventory.validate_ui_tool_class_inventory(
            inventory,
            public_surface_inventory=self.load_inventory(),
            repository_root=REPO_ROOT,
            graph_directory=GRAPH_DIRECTORY,
        )

    def test_public_header_discovery_combines_declared_and_used_surfaces(
        self,
    ) -> None:
        ui_headers = check_public_surface_inventory.discover_public_headers(
            repository_root=REPO_ROOT,
            source_directory="libs/ui",
            export_macro="KRITAUI_EXPORT",
        )
        canvas_headers = check_public_surface_inventory.discover_public_headers(
            repository_root=REPO_ROOT,
            source_directory="libs/canvas",
            export_macro="KRITACANVAS_EXPORT",
        )
        document_headers = check_public_surface_inventory.discover_public_headers(
            repository_root=REPO_ROOT,
            source_directory="libs/document",
            export_macro="KRITADOCUMENT_EXPORT",
        )
        image_headers = check_public_surface_inventory.discover_public_headers(
            repository_root=REPO_ROOT,
            source_directory="libs/image",
            export_macro="KRITAIMAGE_EXPORT",
        )
        impex_ui_headers = check_public_surface_inventory.discover_public_headers(
            repository_root=REPO_ROOT,
            source_directory="libs/impex",
            export_macro="KRITAUI_EXPORT",
            header_directories=["libs/impex/animation", "libs/impex/ui"],
        )
        ui_by_path = {entry["path"]: entry for entry in ui_headers}
        canvas_by_path = {entry["path"]: entry for entry in canvas_headers}
        document_by_path = {
            entry["path"]: entry for entry in document_headers
        }
        image_by_path = {entry["path"]: entry for entry in image_headers}
        impex_ui_by_path = {entry["path"]: entry for entry in impex_ui_headers}

        self.assertEqual(len(canvas_headers), 17)
        self.assertEqual(len(document_headers), 2)
        self.assertEqual(len(ui_headers), 248)
        self.assertEqual(len(image_headers), 332)
        self.assertEqual(len(impex_ui_headers), 23)
        self.assertEqual(
            document_by_path[
                "libs/document/undo/kis_document_undo_store.h"
            ]["publicationEvidence"],
            ["export-macro", "compile-contract", "external-include"],
        )
        self.assertEqual(
            document_by_path[
                "libs/document/session/kis_document_identity.h"
            ]["publicationEvidence"],
            ["export-macro", "external-include"],
        )
        self.assertEqual(
            canvas_by_path["libs/canvas/animation/kis_frame_cache_store.h"],
            {
                "path": "libs/canvas/animation/kis_frame_cache_store.h",
                "publicationEvidence": ["export-macro", "external-include"],
                "consumerPaths": [
                    "libs/ui/animation/cache/KisFrameCacheSwapper.cpp"
                ],
            },
        )
        self.assertEqual(
            impex_ui_by_path["libs/impex/ui/KisImportUserFeedbackInterface.h"][
                "publicationEvidence"
            ],
            ["export-macro", "external-include"],
        )
        self.assertIn(
            "plugins/impex/psd/psd_loader.cpp",
            impex_ui_by_path["libs/impex/ui/KisImportUserFeedbackInterface.h"][
                "consumerPaths"
            ],
        )
        self.assertEqual(
            image_by_path["libs/image/kis_image.h"]["publicationEvidence"],
            ["export-macro", "external-include"],
        )
        self.assertIn(
            "plugins/impex/png/kis_png_import.cc",
            image_by_path["libs/image/kis_image.h"]["consumerPaths"],
        )
        self.assertNotIn("libs/ui/tests/util.h", ui_by_path)

    def test_ui_top_level_class_discovery_is_complete(self) -> None:
        inventory = self.load_inventory()
        classes = check_public_surface_inventory.discover_ui_top_level_classes(
            repository_root=REPO_ROOT,
            public_surface_inventory=inventory,
        )
        by_name = {entry["name"]: entry for entry in classes}

        self.assertEqual(len(classes), 84)
        self.assertEqual(
            by_name["KisApplication"],
            {
                "name": "KisApplication",
                "declarationKind": "class",
                "header": "libs/ui/KisApplication.h",
                "implementationPaths": ["libs/ui/KisApplication.cpp"],
            },
        )
        self.assertNotIn("KisImportExportComplexError", by_name)
        self.assertNotIn("KisImportExportManager", by_name)
        self.assertNotIn("KisDocumentUndoStore", by_name)
        self.assertEqual(
            by_name["KisAbstractPreferenceSetFactory"]["implementationPaths"],
            [],
        )
        self.assertNotIn("KisCanvas2", by_name)

    def test_ui_tool_class_discovery_is_complete(self) -> None:
        inventory = self.load_inventory()
        classes = check_public_surface_inventory.discover_ui_tool_classes(
            repository_root=REPO_ROOT,
            public_surface_inventory=inventory,
        )
        by_name = {entry["name"]: entry for entry in classes}

        self.assertEqual(len(classes), 33)
        self.assertNotIn("Data", by_name)
        self.assertNotIn("FreehandStrokeStrategy", by_name)
        self.assertEqual(
            by_name["NoopActivationPolicy"]["implementationPaths"], []
        )
        self.assertIn(
            "plugins/tools/basictools/kis_tool_brush.cc",
            by_name["KisTool"]["consumerPaths"],
        )

    def test_recorded_ui_tool_class_responsibilities_are_complete(self) -> None:
        inventory = self.load_ui_tool_class_inventory()

        self.validate_ui_tool_classes(inventory)

        self.assertEqual(inventory["scope"], "libs/ui/tool-public-classes")
        self.assertEqual(len(inventory["classes"]), 33)
        by_name = {entry["name"]: entry for entry in inventory["classes"]}
        self.assertEqual(
            by_name["KisPaintingInformationBuilder"]["responsibilityArea"],
            "input-interpretation",
        )
        self.assertEqual(
            by_name["KisTool"]["responsibilityArea"], "tool-invocation"
        )
        self.assertEqual(
            by_name["KisToolFreehandHelper"]["responsibilityArea"],
            "stroke-generation",
        )
        self.assertNotIn("FreehandStrokeStrategy", by_name)
        self.assertEqual(
            by_name["KisRectangleConstraintWidget"]["responsibilityArea"],
            "settings-presentation",
        )

    def test_missing_ui_tool_class_responsibility_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_ui_tool_class_inventory())
        inventory["classes"].pop(0)

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            r"missing=\['ColorSamplerConfig'\]",
        ):
            self.validate_ui_tool_classes(inventory)

    def test_unknown_ui_tool_class_responsibility_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_ui_tool_class_inventory())
        inventory["classes"][0]["responsibilityArea"] = "unknown-area"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "unknown responsibility area unknown-area",
        ):
            self.validate_ui_tool_classes(inventory)

    def test_ui_tool_class_consumer_paths_must_match_discovery(self) -> None:
        inventory = copy.deepcopy(self.load_ui_tool_class_inventory())
        entry = next(
            item for item in inventory["classes"] if item["name"] == "KisTool"
        )
        entry["consumerPaths"].pop(0)

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "recorded source evidence for KisTool does not match source discovery",
        ):
            self.validate_ui_tool_classes(inventory)

    def test_recorded_ui_class_responsibilities_are_complete(self) -> None:
        inventory = self.load_ui_class_inventory()

        self.validate_ui_classes(inventory)

        self.assertEqual(inventory["scope"], "libs/ui-top-level-public-classes")
        self.assertEqual(len(inventory["classes"]), 84)
        by_name = {entry["name"]: entry for entry in inventory["classes"]}
        self.assertEqual(
            by_name["KisApplication"]["responsibilityArea"],
            "application-orchestration",
        )
        self.assertEqual(
            by_name["KisDocument"]["responsibilityArea"], "document-state"
        )
        self.assertNotIn("KisImportExportManager", by_name)
        self.assertNotIn(
            "import-export",
            {entry["responsibilityArea"] for entry in inventory["classes"]},
        )

    def test_missing_ui_class_responsibility_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_ui_class_inventory())
        inventory["classes"].pop(0)

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            r"missing=\['KisAbstractPerspectiveGrid'\]",
        ):
            self.validate_ui_classes(inventory)

    def test_unknown_ui_class_responsibility_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_ui_class_inventory())
        inventory["classes"][0]["responsibilityArea"] = "unknown-area"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "unknown responsibility area unknown-area",
        ):
            self.validate_ui_classes(inventory)

    def test_recorded_inventory_has_complete_public_header_scope(self) -> None:
        inventory = self.load_inventory()

        self.validate(inventory)

        self.assertEqual(inventory["schemaVersion"], 3)
        self.assertEqual(
            inventory["scope"],
            {
                "publicHeaders": "complete",
                "majorClasses": "representative",
                "plugins": "complete",
            },
        )
        self.assertEqual(
            {
                entry["ownerTarget"]: len(entry["headers"])
                for entry in inventory["publicHeaderSets"]
            },
            {
                "kritacanvas": 17,
                "kritadocument": 2,
                "kritaimage": 332,
                "kritaimpex": 11,
                "kritaimpexui": 23,
                "kritaui": 248,
            },
        )
        self.assertEqual(
            [entry["path"] for entry in inventory["publicHeaderDetails"]],
            [
                "libs/image/kis_image.h",
                "libs/impex/ui/KisImportExportManager.h",
                "libs/ui/KisDocument.h",
            ],
        )
        self.assertEqual(
            [entry["name"] for entry in inventory["majorClasses"]],
            ["KisDocument", "KisImage", "KisImportExportManager"],
        )
        self.assertEqual(len(inventory["plugins"]), 172)
        plugin_by_id = {
            entry["id"]: entry for entry in inventory["plugins"]
        }
        self.assertEqual(
            plugin_by_id["Krita PNG Import Filter"]["featureOwner"],
            "import-export",
        )
        self.assertEqual(len(inventory["pluginServiceTypeOwners"]), 14)

    def test_plugin_discovery_is_complete(self) -> None:
        plugins = check_public_surface_inventory.discover_plugins(
            repository_root=REPO_ROOT,
            graph_directory=GRAPH_DIRECTORY,
        )
        by_id = {entry["id"]: entry for entry in plugins}

        self.assertEqual(len(plugins), 172)
        self.assertEqual(
            by_id["Krita PNG Import Filter"],
            {
                "id": "Krita PNG Import Filter",
                "metadata": "plugins/impex/png/krita_png_import.json",
                "implementation": "plugins/impex/png/kis_png_import.cc",
                "ownerTarget": "kritapngimport",
                "ownerEvidence": "metadata-library",
                "metadataLibrary": "kritapngimport",
                "platforms": ["macos", "linux", "ios", "android", "windows"],
                "serviceTypes": ["Krita/FileFilter"],
                "registrationMacro": "K_PLUGIN_CLASS_WITH_JSON",
                "featureOwner": "import-export",
                "runtimeConsumer": "KisImportExportFilterRegistry",
            },
        )
        self.assertEqual(
            by_id["Krita Brush Export Filter"]["ownerEvidence"],
            "cmake-source-override",
        )
        self.assertIsNone(
            by_id["Krita Brush Export Filter"]["metadataLibrary"]
        )
        self.assertEqual(
            by_id["kritaplatformwayland"]["platforms"], ["linux"]
        )
        self.assertEqual(
            sum(
                entry["ownerEvidence"] == "cmake-source-override"
                for entry in plugins
            ),
            15,
        )

    def test_unknown_owner_target_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        inventory["publicHeaderDetails"][0]["ownerTarget"] = "missing-target"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "unknown owner target missing-target",
        ):
            self.validate(inventory)

    def test_consumer_evidence_must_include_the_public_header(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        inventory["publicHeaderDetails"][1]["consumerEvidence"][0]["path"] = (
            "plugins/impex/png/kis_png_import.h"
        )

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "does not include KisImportExportManager.h",
        ):
            self.validate(inventory)

    def test_missing_complete_public_header_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        image_headers = next(
            item["headers"]
            for item in inventory["publicHeaderSets"]
            if item["ownerTarget"] == "kritaimage"
        )
        image_headers.pop(0)

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            r"missing=\['libs/image/KisAnimAutoKey.h'\]",
        ):
            self.validate(inventory)

    def test_publication_evidence_must_match_source_discovery(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        impex_ui_headers = next(
            item["headers"]
            for item in inventory["publicHeaderSets"]
            if item["ownerTarget"] == "kritaimpexui"
        )
        entry = next(
            header
            for header in impex_ui_headers
            if header["path"]
            == "libs/impex/ui/KisImportUserFeedbackInterface.h"
        )
        entry["publicationEvidence"] = ["external-include"]

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "do not match source discovery",
        ):
            self.validate(inventory)

    def test_plugin_metadata_library_must_match_metadata(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        entry = next(
            item
            for item in inventory["plugins"]
            if item["id"] == "Krita PNG Import Filter"
        )
        entry["metadataLibrary"] = "kritapngexport"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "metadata library does not match plugin Krita PNG Import Filter",
        ):
            self.validate(inventory)

    def test_missing_plugin_registration_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        inventory["plugins"].pop(0)

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            r"missing=\['ASC CDL Color Balance'\]",
        ):
            self.validate(inventory)

    def test_plugin_feature_owner_must_match_service_type(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        entry = next(
            item
            for item in inventory["plugins"]
            if item["id"] == "Krita PNG Import Filter"
        )
        entry["featureOwner"] = "unknown-owner"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "feature owner or runtime consumer does not match its service type",
        ):
            self.validate(inventory)

    def test_quick_verification_runs_the_inventory_checker(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "python3 scripts/architecture/check_public_surface_inventory.py",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/update_public_header_inventory.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/update_plugin_inventory.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/update_ui_class_responsibilities.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/update_ui_tool_class_responsibilities.py --check",
            verify_quick,
        )
        self.assertIn("bash scripts/docs/check-architecture.sh", verify_quick)

    def test_nix_governance_uses_the_nix_bash_interpreter(self) -> None:
        flake = (REPO_ROOT / "flake.nix").read_text(encoding="utf-8")

        self.assertIn(
            "${packageSet.bash}/bin/bash ./scripts/verify-quick",
            flake,
        )

    def test_nix_policy_tools_provide_ca_certificates(self) -> None:
        flake = (REPO_ROOT / "flake.nix").read_text(encoding="utf-8")
        policy_tools = flake.split("policyTools =", 1)[1].split(
            "mkDocsShell =", 1
        )[0]

        self.assertIn("cacert", policy_tools)


if __name__ == "__main__":
    unittest.main()
