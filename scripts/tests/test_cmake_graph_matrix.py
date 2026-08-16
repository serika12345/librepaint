#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/regenerate_cmake_graph_matrix.py"
RECORDED_MATRIX = REPO_ROOT / "docs/architecture/cmake-target-matrix.json"
SPEC = importlib.util.spec_from_file_location("regenerate_cmake_graph_matrix", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
regenerate_cmake_graph_matrix = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = regenerate_cmake_graph_matrix
SPEC.loader.exec_module(regenerate_cmake_graph_matrix)


class CMakeGraphMatrixTests(unittest.TestCase):
    def test_recorded_matrix_matches_all_platform_ledgers(self) -> None:
        matrix = regenerate_cmake_graph_matrix.build_matrix(
            regenerate_cmake_graph_matrix.load_graphs(
                REPO_ROOT / "docs/architecture"
            )
        )

        self.assertEqual(
            regenerate_cmake_graph_matrix.serialize_matrix(matrix),
            RECORDED_MATRIX.read_text(encoding="utf-8"),
        )
        self.assertEqual(
            matrix["platforms"],
            ["macos", "linux", "ios", "android", "windows"],
        )
        self.assertEqual(
            list(matrix["targetCounts"]),
            ["macos", "linux", "ios", "android", "windows"],
        )
        self.assertEqual(matrix["commonTargets"], sorted(matrix["commonTargets"]))
        self.assertEqual(
            [target["name"] for target in matrix["conditionalTargets"]],
            sorted(target["name"] for target in matrix["conditionalTargets"]),
        )
        self.assertEqual(matrix["variantTargets"], sorted(matrix["variantTargets"]))

    def test_recorded_matrix_is_valid_json(self) -> None:
        matrix = json.loads(RECORDED_MATRIX.read_text(encoding="utf-8"))

        self.assertEqual(matrix["schemaVersion"], 1)


if __name__ == "__main__":
    unittest.main()
