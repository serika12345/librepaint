#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_package_boundaries.py"
SPEC = importlib.util.spec_from_file_location("check_package_boundaries", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_package_boundaries = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_package_boundaries
SPEC.loader.exec_module(check_package_boundaries)


class PackageBoundaryTests(unittest.TestCase):
    def test_forbidden_owner_dependency_is_rejected(self) -> None:
        policy = {
            "schemaVersion": 1,
            "responsibilities": [
                {
                    "id": "lower",
                    "ownerTargets": ["lower-target"],
                    "allowedDependencies": [],
                },
                {
                    "id": "upper",
                    "ownerTargets": ["upper-target"],
                    "allowedDependencies": ["lower"],
                },
            ],
        }
        graph = {
            "schemaVersion": 1,
            "platform": "fixture",
            "targets": [
                {
                    "name": "lower-target",
                    "type": "STATIC_LIBRARY",
                    "sourceDirectory": "libs/lower",
                    "dependencies": ["upper-target"],
                },
                {
                    "name": "upper-target",
                    "type": "STATIC_LIBRARY",
                    "sourceDirectory": "libs/upper",
                    "dependencies": ["lower-target"],
                },
            ],
        }

        with self.assertRaisesRegex(
            check_package_boundaries.PackageBoundaryError,
            "forbidden package dependencies",
        ):
            check_package_boundaries.validate_graph(policy, graph)

    def test_product_target_cycle_is_rejected(self) -> None:
        policy = {
            "schemaVersion": 1,
            "responsibilities": [
                {
                    "id": "owner",
                    "ownerTargets": ["a", "b"],
                    "allowedDependencies": [],
                }
            ],
        }
        graph = {
            "schemaVersion": 1,
            "platform": "fixture",
            "targets": [
                {
                    "name": "a",
                    "type": "STATIC_LIBRARY",
                    "sourceDirectory": "libs/a",
                    "dependencies": ["b"],
                },
                {
                    "name": "b",
                    "type": "STATIC_LIBRARY",
                    "sourceDirectory": "libs/b",
                    "dependencies": ["a"],
                },
            ],
        }

        with self.assertRaisesRegex(
            check_package_boundaries.PackageBoundaryError,
            "target cycles",
        ):
            check_package_boundaries.validate_graph(policy, graph)


if __name__ == "__main__":
    unittest.main()
