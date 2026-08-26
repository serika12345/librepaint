#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_structural_dependencies.py"
SPEC = importlib.util.spec_from_file_location(
    "check_structural_dependencies", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_structural_dependencies = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_structural_dependencies
SPEC.loader.exec_module(check_structural_dependencies)


class StructuralDependencyTests(unittest.TestCase):
    def test_structural_dependencies_are_clean(self) -> None:
        check_structural_dependencies.validate_structural_dependencies(
            repository_root=REPO_ROOT
        )

    def test_responsibility_projection_is_rejected(self) -> None:
        with patch.object(
            check_structural_dependencies,
            "_candidate_links",
            return_value={
                ("canvas-presentation", "application-orchestration"): {
                    ("kritacanvas", "kritaapplicationui")
                }
            },
        ):
            with self.assertRaisesRegex(
                check_structural_dependencies.StructuralDependencyError,
                "projections must remain empty",
            ):
                check_structural_dependencies.validate_structural_dependencies(
                    repository_root=REPO_ROOT
                )

    def test_cycle_detection_finds_a_component(self) -> None:
        components = check_structural_dependencies.strongly_connected_components(
            {"a", "b", "c"}, {"a": {"b"}, "b": {"a"}, "c": set()}
        )
        self.assertEqual(components, [["a", "b"]])

    def test_target_cycle_is_rejected(self) -> None:
        cycles = [
            {
                "scope": "all-production-build-targets",
                "platforms": [
                    {
                        "platform": "macos",
                        "targetCount": 2,
                        "components": [["a", "b"]],
                    }
                ],
            }
        ]
        with patch.object(
            check_structural_dependencies,
            "discover_target_cycles",
            return_value=cycles,
        ):
            with self.assertRaisesRegex(
                check_structural_dependencies.StructuralDependencyError,
                "cycles must remain empty",
            ):
                check_structural_dependencies.validate_structural_dependencies(
                    repository_root=REPO_ROOT
                )

    def test_external_internal_header_reference_is_rejected(self) -> None:
        with patch.object(
            check_structural_dependencies,
            "discover_internal_headers",
            return_value={
                "kritaexample": {
                    "headers": [
                        {
                            "path": "libs/example/private.h",
                            "consumerPaths": ["plugins/example.cpp"],
                        }
                    ]
                }
            },
        ):
            with self.assertRaisesRegex(
                check_structural_dependencies.StructuralDependencyError,
                "internal-header references must remain empty",
            ):
                check_structural_dependencies.validate_structural_dependencies(
                    repository_root=REPO_ROOT
                )

    def test_quick_verification_runs_the_direct_check(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            "python3 scripts/architecture/check_structural_dependencies.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
