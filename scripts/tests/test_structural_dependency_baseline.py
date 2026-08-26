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
    REPO_ROOT / "scripts/architecture/check_structural_dependency_baseline.py"
)
BASELINE_PATH = (
    REPO_ROOT / "docs/architecture/structural-dependency-baseline.json"
)
SPEC = importlib.util.spec_from_file_location(
    "check_structural_dependency_baseline", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_structural_dependency_baseline = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_structural_dependency_baseline
SPEC.loader.exec_module(check_structural_dependency_baseline)


class StructuralDependencyBaselineTests(unittest.TestCase):
    def load_baseline(self):
        return check_structural_dependency_baseline.load_baseline(BASELINE_PATH)

    def validate(self, baseline) -> None:
        check_structural_dependency_baseline.validate_baseline(
            baseline,
            repository_root=REPO_ROOT,
        )

    def test_structural_baseline_is_complete(self) -> None:
        baseline = self.load_baseline()

        self.validate(baseline)

        self.assertEqual(
            baseline["scope"], "r1-g4b-structural-dependency-baseline"
        )
        self.assertEqual(len(baseline["projectionResolutions"]), 0)
        self.assertEqual(
            baseline["targetCycleBaseline"]["maximumComponents"], 0
        )
        self.assertTrue(
            all(
                not entry["components"]
                for scope in baseline["targetCycleBaseline"]["scopes"]
                for entry in scope["platforms"]
            )
        )
        internal = baseline["internalHeaderBaseline"]
        self.assertEqual(len(internal), 14)
        self.assertEqual(
            sum(len(entry["headers"]) for entry in internal), 0
        )
        self.assertEqual(
            sum(
                len(header["consumerPaths"])
                for entry in internal
                for header in entry["headers"]
            ),
            0,
        )

    def test_resolved_projection_cannot_return(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["projectionResolutions"].append({"stale": True})

        with self.assertRaisesRegex(
            check_structural_dependency_baseline.StructuralBaselineError,
            "projection resolutions are stale",
        ):
            self.validate(baseline)

    def test_cycle_detection_finds_a_component(self) -> None:
        components = (
            check_structural_dependency_baseline.strongly_connected_components(
                {"a", "b", "c"},
                {"a": {"b"}, "b": {"a"}, "c": set()},
            )
        )

        self.assertEqual(components, [["a", "b"]])

    def test_internal_header_growth_exceeds_reviewed_maximum(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        expected = copy.deepcopy(baseline)
        expected["internalHeaderBaseline"][0]["headers"] = [
            {"consumerPaths": ["libs/example.cpp"]}
        ]

        with patch.object(
            check_structural_dependency_baseline,
            "updated_baseline",
            return_value=expected,
        ):
            with self.assertRaisesRegex(
                check_structural_dependency_baseline.StructuralBaselineError,
                "internal header references exceed approved maximum",
            ):
                self.validate(baseline)

    def test_internal_header_reduction_requires_baseline_reduction(self) -> None:
        baseline = copy.deepcopy(self.load_baseline())
        baseline["internalHeaderBaseline"][0][
            "maximumDirectReferences"
        ] += 1

        with self.assertRaisesRegex(
            check_structural_dependency_baseline.StructuralBaselineError,
            "internal header reference baseline can be reduced",
        ):
            self.validate(baseline)

    def test_quick_verification_runs_the_structural_tools(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "python3 scripts/architecture/"
            "update_structural_dependency_baseline.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/"
            "check_structural_dependency_baseline.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
