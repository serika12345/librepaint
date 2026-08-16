#!/usr/bin/env python3

from __future__ import annotations

import copy
import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = (
    REPO_ROOT / "scripts/architecture/check_allowed_package_dependencies.py"
)
POLICY_PATH = REPO_ROOT / "docs/architecture/allowed-package-dependencies.json"
RESPONSIBILITY_MAP_PATH = (
    REPO_ROOT / "docs/architecture/package-responsibilities.json"
)
SPEC = importlib.util.spec_from_file_location(
    "check_allowed_package_dependencies", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_allowed_package_dependencies = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_allowed_package_dependencies
SPEC.loader.exec_module(check_allowed_package_dependencies)


class AllowedPackageDependencyTests(unittest.TestCase):
    def load_policy(self):
        return check_allowed_package_dependencies.load_policy(POLICY_PATH)

    def validate(self, policy) -> None:
        check_allowed_package_dependencies.validate_policy(
            policy,
            responsibility_map_path=RESPONSIBILITY_MAP_PATH,
        )

    def test_recorded_policy_is_complete_and_acyclic(self) -> None:
        policy = self.load_policy()

        self.validate(policy)

        self.assertEqual(
            policy["scope"], "r1-package-responsibility-dependency-policy"
        )
        self.assertEqual(len(policy["responsibilities"]), 9)
        self.assertEqual(len(policy["currentTargetEdges"]), 27)
        status_counts = {
            status: sum(
                projection["status"] == status
                for edge in policy["currentTargetEdges"]
                for projection in edge["projections"]
            )
            for status in (
                "internal",
                "allowed",
                "requires-r1-g4-baseline",
            )
        }
        self.assertEqual(
            status_counts,
            {
                "internal": 8,
                "allowed": 35,
                "requires-r1-g4-baseline": 26,
            },
        )
        by_id = {
            entry["id"]: entry for entry in policy["responsibilities"]
        }
        self.assertEqual(
            by_id["plugin-infrastructure"]["allowedDependencies"], []
        )
        self.assertIn(
            {
                "responsibility": "import-export",
                "surface": "document-serialization",
            },
            by_id["document-lifecycle"]["allowedDependencies"],
        )
        self.assertGreater(
            by_id["application-orchestration"]["layer"],
            max(
                entry["layer"]
                for entry in policy["responsibilities"]
                if entry["id"] != "application-orchestration"
            ),
        )

    def test_unknown_responsibility_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        policy["responsibilities"][0]["id"] = "unknown-owner"
        policy["responsibilities"].sort(key=lambda entry: entry["id"])

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            r"unexpected=\['unknown-owner'\]",
        ):
            self.validate(policy)

    def test_self_dependency_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        entry = policy["responsibilities"][0]
        entry["allowedDependencies"].append(
            {
                "responsibility": entry["id"],
                "surface": entry["publicSurfaces"][0]["id"],
            }
        )
        entry["allowedDependencies"].sort(
            key=lambda dependency: (
                dependency["responsibility"], dependency["surface"]
            )
        )

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            "self dependency",
        ):
            self.validate(policy)

    def test_dependency_cycle_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        plugin = next(
            entry
            for entry in policy["responsibilities"]
            if entry["id"] == "plugin-infrastructure"
        )
        plugin["allowedDependencies"].append(
            {
                "responsibility": "application-orchestration",
                "surface": "application-lifecycle",
            }
        )

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            "dependency policy contains a cycle",
        ):
            self.validate(policy)

    def test_dependency_to_the_same_layer_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        import_export = next(
            entry
            for entry in policy["responsibilities"]
            if entry["id"] == "import-export"
        )
        import_export["allowedDependencies"].append(
            {
                "responsibility": "canvas-presentation",
                "surface": "canvas-view",
            }
        )
        import_export["allowedDependencies"].sort(
            key=lambda dependency: (
                dependency["responsibility"], dependency["surface"]
            )
        )

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            "must point to a lower layer",
        ):
            self.validate(policy)

    def test_unknown_public_surface_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        dependency = policy["responsibilities"][0]["allowedDependencies"][0]
        dependency["surface"] = "unknown-surface"

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            "unknown public surface",
        ):
            self.validate(policy)

    def test_unclassified_current_projection_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        policy["currentTargetEdges"][0]["projections"][0]["status"] = (
            "unclassified"
        )

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            "unknown current projection status",
        ):
            self.validate(policy)

    def test_stale_current_target_edge_is_rejected(self) -> None:
        policy = copy.deepcopy(self.load_policy())
        policy["currentTargetEdges"].pop()

        with self.assertRaisesRegex(
            check_allowed_package_dependencies.DependencyPolicyError,
            "current target edge projection is stale",
        ):
            self.validate(policy)

    def test_quick_verification_runs_the_dependency_policy_tools(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn(
            "python3 scripts/architecture/"
            "update_allowed_package_dependencies.py --check",
            verify_quick,
        )
        self.assertIn(
            "python3 scripts/architecture/"
            "check_allowed_package_dependencies.py",
            verify_quick,
        )


if __name__ == "__main__":
    unittest.main()
