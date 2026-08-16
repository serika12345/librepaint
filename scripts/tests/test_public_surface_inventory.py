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

    def validate(self, inventory) -> None:
        check_public_surface_inventory.validate_inventory(
            inventory,
            repository_root=REPO_ROOT,
            graph_directory=GRAPH_DIRECTORY,
        )

    def test_recorded_representative_inventory_is_valid(self) -> None:
        inventory = self.load_inventory()

        self.validate(inventory)

        self.assertEqual(inventory["scope"], "representative")
        self.assertEqual(
            [entry["path"] for entry in inventory["publicHeaders"]],
            [
                "libs/image/kis_image.h",
                "libs/ui/KisDocument.h",
                "libs/ui/KisImportExportManager.h",
            ],
        )
        self.assertEqual(
            [entry["name"] for entry in inventory["majorClasses"]],
            ["KisDocument", "KisImage", "KisImportExportManager"],
        )
        self.assertEqual(
            [entry["id"] for entry in inventory["plugins"]],
            ["Krita PNG Import Filter"],
        )

    def test_unknown_owner_target_is_rejected(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        inventory["publicHeaders"][0]["ownerTarget"] = "missing-target"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "unknown owner target missing-target",
        ):
            self.validate(inventory)

    def test_consumer_evidence_must_include_the_public_header(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        inventory["publicHeaders"][1]["consumerEvidence"][0]["path"] = (
            "krita/windows_stub_main.cpp"
        )

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "does not include KisDocument.h",
        ):
            self.validate(inventory)

    def test_plugin_metadata_library_must_match_owner_target(self) -> None:
        inventory = copy.deepcopy(self.load_inventory())
        inventory["plugins"][0]["ownerTarget"] = "kritapngexport"

        with self.assertRaisesRegex(
            check_public_surface_inventory.PublicSurfaceError,
            "metadata library kritapngimport does not match owner target kritapngexport",
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
        self.assertIn("bash scripts/docs/check-architecture.sh", verify_quick)

    def test_nix_governance_uses_the_nix_bash_interpreter(self) -> None:
        flake = (REPO_ROOT / "flake.nix").read_text(encoding="utf-8")

        self.assertIn(
            "${packageSet.bash}/bin/bash ./scripts/verify-quick",
            flake,
        )


if __name__ == "__main__":
    unittest.main()
