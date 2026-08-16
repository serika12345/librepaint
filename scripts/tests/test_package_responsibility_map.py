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
        self.assertEqual(len(responsibility_map["targetRelations"]), 15)
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
            7,
        )
        target_by_name = {
            entry["name"]: entry
            for entry in responsibility_map["targetRelations"]
        }
        self.assertIn(
            "kritaimage", target_by_name["kritaui"]["repositoryDependencies"]
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
