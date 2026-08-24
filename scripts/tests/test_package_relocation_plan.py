#!/usr/bin/env python3

from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_package_relocation_plan.py"
PLAN_PATH = REPO_ROOT / "docs/architecture/package-relocation-plan.json"
SPEC = importlib.util.spec_from_file_location(
    "check_package_relocation_plan", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_package_relocation_plan = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_package_relocation_plan
SPEC.loader.exec_module(check_package_relocation_plan)


class PackageRelocationPlanTests(unittest.TestCase):
    def load_plan(self):
        return check_package_relocation_plan.load_plan(PLAN_PATH)

    def validate(self, plan) -> None:
        check_package_relocation_plan.validate_plan(
            plan, repository_root=REPO_ROOT
        )

    def test_complete_relocation_plan_is_valid(self) -> None:
        plan = self.load_plan()

        self.validate(plan)

        self.assertEqual(
            plan["scope"], "r1-g5-complete-package-relocation-plan"
        )
        self.assertEqual(len(plan["packages"]), 9)
        self.assertEqual(len(plan["migrationWaves"]), 8)
        self.assertEqual(plan["firstImplementationWave"], "R1-G6a")
        first = plan["migrationWaves"][0]
        self.assertEqual(first["responsibilities"], ["resource-management"])
        self.assertIn(
            {
                "from": "libs/store",
                "to": "libs/resources/storage",
                "target": "kritaresourcestorage",
            },
            first["sourceMoves"],
        )
        self.assertEqual(
            sum(
                reduction["from"] - reduction["to"]
                for wave in plan["migrationWaves"]
                for reduction in wave["reverseBaselineReductions"]
            ),
            338,
        )
        self.assertEqual(
            plan["finalState"],
            {
                "reverseDirectIncludes": 0,
                "unresolvedProjections": 0,
                "targetCycleComponents": 0,
                "internalHeaderDirectReferences": 0,
                "temporaryCompatibilityRoutes": 0,
            },
        )

    def test_wave_order_must_follow_allowed_dependencies(self) -> None:
        plan = copy.deepcopy(self.load_plan())
        plan["packages"][0]["migrationWave"] = "R1-G6a"

        with self.assertRaisesRegex(
            check_package_relocation_plan.RelocationPlanError,
            "migration wave order violates allowed dependency order",
        ):
            self.validate(plan)

    def test_internal_header_destination_cannot_be_dropped(self) -> None:
        plan = copy.deepcopy(self.load_plan())
        plan["reviewedInternalHeaderDestinations"].pop()

        with self.assertRaisesRegex(
            check_package_relocation_plan.RelocationPlanError,
            "internal header destinations do not cover the baseline",
        ):
            self.validate(plan)

    def test_zero_reference_public_owner_does_not_require_a_migration(self) -> None:
        structural_path = REPO_ROOT / (
            "docs/architecture/structural-dependency-baseline.json"
        )
        structural = check_package_relocation_plan._load_json(
            structural_path, "structural dependency baseline"
        )

        self.assertIn(
            "kritaimpex",
            {
                item["ownerTarget"]
                for item in structural["internalHeaderBaseline"]
                if item["maximumDirectReferences"] == 0
            },
        )
        self.validate(self.load_plan())

    def test_reverse_baseline_must_reach_each_wave_maximum(self) -> None:
        plan = copy.deepcopy(self.load_plan())
        plan["migrationWaves"][0][
            "maximumReverseDirectIncludesAfterWave"
        ] += 1

        with self.assertRaisesRegex(
            check_package_relocation_plan.RelocationPlanError,
            "reverse dependency reduction does not match wave maximum",
        ):
            self.validate(plan)

    def test_implemented_target_must_exist(self) -> None:
        plan = copy.deepcopy(self.load_plan())
        resource_package = next(
            item
            for item in plan["packages"]
            if item["responsibility"] == "resource-management"
        )
        storage_target = next(
            item
            for item in resource_package["target"]["cmakeTargets"]
            if item["name"] == "kritaresourcestorage"
        )
        storage_target["name"] = "kritamissingstorage"

        with self.assertRaisesRegex(
            check_package_relocation_plan.RelocationPlanError,
            "implemented target is missing",
        ):
            self.validate(plan)

    def test_compatibility_route_requires_removal_condition(self) -> None:
        plan = copy.deepcopy(self.load_plan())
        plan["compatibilityRoutes"][0]["removalCondition"] = ""

        with self.assertRaisesRegex(
            check_package_relocation_plan.RelocationPlanError,
            "expected a non-empty string for removal condition",
        ):
            self.validate(plan)

    def test_quick_verification_runs_the_plan_checker(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "python3 scripts/architecture/check_package_relocation_plan.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
