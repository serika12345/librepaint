#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/regenerate_cmake_graph.py"
FIXTURE_DIRECTORY = REPO_ROOT / "scripts/tests/fixtures/cmake-file-api"
EXPECTED_GRAPH = FIXTURE_DIRECTORY / "expected-cmake-targets-macos.json"
SPEC = importlib.util.spec_from_file_location("regenerate_cmake_graph", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
regenerate_cmake_graph = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = regenerate_cmake_graph
SPEC.loader.exec_module(regenerate_cmake_graph)


class RegenerateCMakeGraphTests(unittest.TestCase):
    def test_prepare_query_creates_the_stateless_codemodel_request(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            build_directory = Path(temporary_directory)

            query_path = regenerate_cmake_graph.prepare_query(build_directory)

            self.assertEqual(
                query_path,
                build_directory / ".cmake/api/v1/query/codemodel-v2",
            )
            self.assertEqual(query_path.read_bytes(), b"")

    def test_fixture_matches_the_recorded_graph(self) -> None:
        regenerate_cmake_graph.regenerate(
            platform="macos",
            reply_directory=FIXTURE_DIRECTORY,
            output_path=EXPECTED_GRAPH,
            check=True,
        )

    def test_check_reports_the_command_for_a_stale_graph(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            output_path = Path(temporary_directory) / "graph.json"
            output_path.write_text("{}\n", encoding="utf-8")

            with self.assertRaisesRegex(
                regenerate_cmake_graph.RegenerationError,
                "regenerate_cmake_graph.py macos",
            ):
                regenerate_cmake_graph.regenerate(
                    platform="macos",
                    reply_directory=FIXTURE_DIRECTORY,
                    output_path=output_path,
                    check=True,
                )


if __name__ == "__main__":
    unittest.main()
