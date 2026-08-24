#!/usr/bin/env python3

from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = (
    REPO_ROOT / "scripts/architecture/check_dependency_violation_baseline.py"
)
BASELINE_PATH = (
    REPO_ROOT / "docs/architecture/dependency-violation-baseline.json"
)
POLICY_PATH = (
    REPO_ROOT / "docs/architecture/allowed-package-dependencies.json"
)
RESPONSIBILITY_MAP_PATH = (
    REPO_ROOT / "docs/architecture/package-responsibilities.json"
)
UI_CLASS_INVENTORY_PATH = (
    REPO_ROOT / "docs/architecture/ui-class-responsibilities.json"
)
UI_TOOL_CLASS_INVENTORY_PATH = (
    REPO_ROOT / "docs/architecture/ui-tool-class-responsibilities.json"
)
GRAPH_DIRECTORY = REPO_ROOT / "docs/architecture"
SPEC = importlib.util.spec_from_file_location(
    "check_dependency_violation_baseline", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_dependency_violation_baseline = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_dependency_violation_baseline
SPEC.loader.exec_module(check_dependency_violation_baseline)


class DependencyViolationBaselineTests(unittest.TestCase):
    def load_baseline(self):
        return check_dependency_violation_baseline.load_baseline(BASELINE_PATH)

    def validate(self, baseline) -> None:
        check_dependency_violation_baseline.validate_baseline(
            baseline,
            repository_root=REPO_ROOT,
            policy_path=POLICY_PATH,
            responsibility_map_path=RESPONSIBILITY_MAP_PATH,
            ui_class_inventory_path=UI_CLASS_INVENTORY_PATH,
            ui_tool_class_inventory_path=UI_TOOL_CLASS_INVENTORY_PATH,
            graph_directory=GRAPH_DIRECTORY,
        )

    def test_recorded_confirmed_violations_are_complete(self) -> None:
        baseline = self.load_baseline()

        self.validate(baseline)

        self.assertEqual(
            baseline["scope"],
            "r1-g4a-confirmed-reverse-dependency-baseline",
        )
        self.assertEqual(len(baseline["violations"]), 3)
        self.assertEqual(
            sum(
                len(entry["directIncludes"])
                for entry in baseline["violations"]
            ),
            104,
        )
        self.assertEqual(baseline["unresolvedProjections"], [])
        by_pair = {
            (entry["sourceResponsibility"], entry["dependencyResponsibility"]): (
                entry
            )
            for entry in baseline["violations"]
        }
        self.assertNotIn(
            ("painting-rendering", "document-lifecycle"),
            by_pair,
        )
        for entry in baseline["violations"]:
            self.assertEqual(
                entry["maximumDirectIncludes"],
                len(entry["directIncludes"]),
            )
            self.assertEqual(entry["owner"], "R1-G6")
            self.assertTrue(entry["removalCondition"])

    def test_missing_violation_pair_is_rejected(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"].pop(0)

        with self.assertRaisesRegex(
            check_dependency_violation_baseline.DependencyBaselineError,
            "confirmed violation pairs do not match discovery",
        ):
            self.validate(baseline)

    def test_expanded_violation_exceeds_the_reviewed_maximum(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"][0]["maximumDirectIncludes"] -= 1

        with self.assertRaisesRegex(
            check_dependency_violation_baseline.DependencyBaselineError,
            "exceeds approved maximum",
        ):
            self.validate(baseline)

    def test_reduced_violation_requires_baseline_reduction(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"][0]["maximumDirectIncludes"] += 1

        with self.assertRaisesRegex(
            check_dependency_violation_baseline.DependencyBaselineError,
            "baseline can be reduced",
        ):
            self.validate(baseline)

    def test_updater_prunes_a_resolved_violation_pair(self) -> None:
        baseline = self.load_baseline()
        confirmed = {
            (
                entry["sourceResponsibility"],
                entry["dependencyResponsibility"],
            ): {
                "targetLinks": entry["targetLinks"],
                "directIncludes": entry["directIncludes"],
            }
            for entry in baseline["violations"][:-1]
        }

        with patch.object(
            check_dependency_violation_baseline,
            "discover_baseline_evidence",
            return_value=(confirmed, {}),
        ):
            updated = check_dependency_violation_baseline.updated_baseline(
                baseline,
                repository_root=REPO_ROOT,
                policy={},
                responsibility_map={},
                ui_class_inventory={},
                ui_tool_class_inventory={},
                graph_directory=GRAPH_DIRECTORY,
            )

        self.assertEqual(
            updated["violations"],
            baseline["violations"][:-1],
        )

    def test_stale_direct_include_evidence_is_rejected(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"][0]["directIncludes"].pop()

        with self.assertRaisesRegex(
            check_dependency_violation_baseline.DependencyBaselineError,
            "dependency violation evidence is stale",
        ):
            self.validate(baseline)

    def test_resolved_projection_cannot_return_to_unresolved(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["unresolvedProjections"].append(
            {
                "sourceResponsibility": "canvas-presentation",
                "dependencyResponsibility": "application-orchestration",
                "status": "no-attributed-direct-include",
                "targetLinks": [],
                "ambiguousDirectIncludes": [],
            }
        )

        with self.assertRaisesRegex(
            check_dependency_violation_baseline.DependencyBaselineError,
            "unresolved dependency projections are stale",
        ):
            self.validate(baseline)

    def test_quick_verification_runs_the_baseline_tools(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "python3 scripts/architecture/"
            "update_dependency_violation_baseline.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/"
            "check_dependency_violation_baseline.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
