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
    @staticmethod
    def violation_evidence(count: int):
        return {
            "targetLinks": [
                {
                    "sourceTarget": "example_source",
                    "dependencyTarget": "example_dependency",
                    "platforms": ["macos"],
                }
            ],
            "directIncludes": [
                {
                    "sourceTarget": "example_source",
                    "dependencyTarget": "example_dependency",
                    "sourcePath": f"libs/ui/example_{index}.cpp",
                    "include": "example.h",
                    "headerPath": "libs/image/example.h",
                    "sourceAttribution": "unique-owner-target",
                    "dependencyAttribution": "unique-owner-target",
                }
                for index in range(count)
            ],
        }

    @classmethod
    def reviewed_violation(cls, maximum: int, observed: int | None = None):
        evidence = cls.violation_evidence(
            maximum if observed is None else observed
        )
        return {
            "sourceResponsibility": "application-orchestration",
            "dependencyResponsibility": "painting-rendering",
            "owner": "R1-G6",
            "reason": "Synthetic reviewed violation for governance checks.",
            "maximumDirectIncludes": maximum,
            "removalCondition": "The synthetic dependency is removed.",
            **evidence,
        }

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
        self.assertEqual(len(baseline["violations"]), 0)
        self.assertEqual(
            sum(
                len(entry["directIncludes"])
                for entry in baseline["violations"]
            ),
            0,
        )
        self.assertEqual(len(baseline["unresolvedProjections"]), 0)
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
            self.assertIn(entry["owner"], {"R1-G6", "R1-G6g"})
            self.assertTrue(entry["removalCondition"])
        self.assertNotIn(("canvas-presentation", "input-interpretation"), by_pair)
        self.assertNotIn(("input-interpretation", "painting-rendering"), by_pair)
        self.assertNotIn(("input-interpretation", "resource-management"), by_pair)
        self.assertNotIn(("tool-invocation", "input-interpretation"), by_pair)

    def test_missing_violation_pair_is_rejected(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        evidence = self.violation_evidence(1)

        with patch.object(
            check_dependency_violation_baseline,
            "discover_baseline_evidence",
            return_value=(
                {("application-orchestration", "painting-rendering"): evidence},
                {},
            ),
        ):
            with self.assertRaisesRegex(
                check_dependency_violation_baseline.DependencyBaselineError,
                "confirmed violation pairs do not match discovery",
            ):
                self.validate(baseline)

    def test_expanded_violation_exceeds_the_reviewed_maximum(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"] = [self.reviewed_violation(1)]
        evidence = self.violation_evidence(2)

        with patch.object(
            check_dependency_violation_baseline,
            "discover_baseline_evidence",
            return_value=(
                {("application-orchestration", "painting-rendering"): evidence},
                {},
            ),
        ):
            with self.assertRaisesRegex(
                check_dependency_violation_baseline.DependencyBaselineError,
                "exceeds approved maximum",
            ):
                self.validate(baseline)

    def test_reduced_violation_requires_baseline_reduction(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"] = [self.reviewed_violation(2)]
        evidence = self.violation_evidence(1)

        with patch.object(
            check_dependency_violation_baseline,
            "discover_baseline_evidence",
            return_value=(
                {("application-orchestration", "painting-rendering"): evidence},
                {},
            ),
        ):
            with self.assertRaisesRegex(
                check_dependency_violation_baseline.DependencyBaselineError,
                "baseline can be reduced",
            ):
                self.validate(baseline)

    def test_updater_prunes_a_resolved_violation_pair(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"] = [self.reviewed_violation(1)]

        with patch.object(
            check_dependency_violation_baseline,
            "discover_baseline_evidence",
            return_value=({}, {}),
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
            [],
        )

    def test_stale_direct_include_evidence_is_rejected(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["violations"] = [self.reviewed_violation(1)]
        confirmed = self.violation_evidence(1)
        baseline["violations"][0]["directIncludes"][0]["include"] = "stale.h"

        with patch.object(
            check_dependency_violation_baseline,
            "discover_baseline_evidence",
            return_value=(
                {("application-orchestration", "painting-rendering"): confirmed},
                {},
            ),
        ):
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
