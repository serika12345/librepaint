#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_package_dependencies.py"
POLICY_PATH = REPO_ROOT / "docs/architecture/allowed-package-dependencies.json"
RESPONSIBILITY_MAP_PATH = REPO_ROOT / "docs/architecture/package-responsibilities.json"
UI_CLASS_INVENTORY_PATH = REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
UI_TOOL_CLASS_INVENTORY_PATH = (
    REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
)
GRAPH_DIRECTORY = REPO_ROOT / "docs/architecture"
SPEC = importlib.util.spec_from_file_location("check_package_dependencies", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_package_dependencies = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_package_dependencies
SPEC.loader.exec_module(check_package_dependencies)


class PackageDependencyTests(unittest.TestCase):
    def validate(self) -> None:
        check_package_dependencies.validate_package_dependencies(
            repository_root=REPO_ROOT,
            policy_path=POLICY_PATH,
            responsibility_map_path=RESPONSIBILITY_MAP_PATH,
            ui_class_inventory_path=UI_CLASS_INVENTORY_PATH,
            ui_tool_class_inventory_path=UI_TOOL_CLASS_INVENTORY_PATH,
            graph_directory=GRAPH_DIRECTORY,
        )

    def test_package_dependencies_follow_the_policy(self) -> None:
        self.validate()

    def test_reverse_dependency_is_rejected(self) -> None:
        evidence = {
            "targetLinks": [],
            "directIncludes": [{"sourcePath": "libs/example.cpp"}],
        }
        with patch.object(
            check_package_dependencies,
            "discover_dependency_evidence",
            return_value=(
                {("application-orchestration", "painting-rendering"): evidence},
                {},
            ),
        ):
            with self.assertRaisesRegex(
                check_package_dependencies.PackageDependencyError,
                "violations must remain empty",
            ):
                self.validate()

    def test_unattributed_projection_is_rejected(self) -> None:
        with patch.object(
            check_package_dependencies,
            "discover_dependency_evidence",
            return_value=(
                {},
                {
                    ("canvas-presentation", "application-orchestration"): {
                        "targetLinks": [],
                        "ambiguousDirectIncludes": [],
                    }
                },
            ),
        ):
            with self.assertRaisesRegex(
                check_package_dependencies.PackageDependencyError,
                "projections must be attributable",
            ):
                self.validate()

    def test_quick_verification_runs_the_direct_check(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            "python3 scripts/architecture/check_package_dependencies.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
