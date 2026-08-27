#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
import shutil
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/extract_cmake_graph.py"
FIXTURE_DIRECTORY = REPO_ROOT / "scripts/tests/fixtures/cmake-file-api"
SPEC = importlib.util.spec_from_file_location("extract_cmake_graph", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
extract_cmake_graph = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = extract_cmake_graph
SPEC.loader.exec_module(extract_cmake_graph)


class ExtractCMakeGraphTests(unittest.TestCase):
    def test_extracts_sorted_direct_link_dependencies(self) -> None:
        graph = extract_cmake_graph.extract_graph(
            FIXTURE_DIRECTORY,
            platform="macos",
            build_profile="tdd-macos",
        )

        self.assertEqual(
            graph,
            {
                "schemaVersion": 1,
                "platform": "macos",
                "buildProfile": "tdd-macos",
                "configuration": "RelWithDebInfo",
                "targets": [
                    {
                        "name": "foundation",
                        "type": "STATIC_LIBRARY",
                        "sourceDirectory": "libs/foundation",
                        "dependencies": [],
                    },
                    {
                        "name": "librepaint",
                        "type": "EXECUTABLE",
                        "sourceDirectory": "app",
                        "dependencies": ["paint", "sampleplugin"],
                    },
                    {
                        "name": "paint",
                        "type": "SHARED_LIBRARY",
                        "sourceDirectory": "libs/paint",
                        "dependencies": ["Qt6::Core", "foundation"],
                    },
                    {
                        "name": "sampleplugin",
                        "type": "MODULE_LIBRARY",
                        "sourceDirectory": "plugins/sample",
                        "dependencies": ["paint"],
                    },
                ],
            },
        )

    def test_serialization_is_byte_reproducible(self) -> None:
        graph = extract_cmake_graph.extract_graph(
            FIXTURE_DIRECTORY,
            platform="macos",
            build_profile="tdd-macos",
        )

        first = extract_cmake_graph.serialize_graph(graph)
        second = extract_cmake_graph.serialize_graph(graph)

        self.assertEqual(first, second)
        self.assertTrue(first.endswith("\n"))
        self.assertEqual(json.loads(first), graph)

    def test_missing_codemodel_reports_the_reply_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            reply_directory = Path(temporary_directory)

            with self.assertRaisesRegex(
                extract_cmake_graph.GraphExtractionError,
                f"codemodel-v2 reply was not found in {reply_directory}",
            ):
                extract_cmake_graph.extract_graph(
                    reply_directory,
                    platform="macos",
                    build_profile="tdd-macos",
                )

    def test_codemodel_without_direct_link_data_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            reply_directory = Path(temporary_directory)
            shutil.copytree(FIXTURE_DIRECTORY, reply_directory, dirs_exist_ok=True)
            codemodel_path = reply_directory / "codemodel-v2-test.json"
            codemodel = json.loads(codemodel_path.read_text(encoding="utf-8"))
            codemodel["version"]["minor"] = 8
            codemodel_path.write_text(json.dumps(codemodel), encoding="utf-8")

            with self.assertRaisesRegex(
                extract_cmake_graph.GraphExtractionError,
                "requires codemodel version 2.9 or newer",
            ):
                extract_cmake_graph.extract_graph(
                    reply_directory,
                    platform="macos",
                    build_profile="tdd-macos",
                )

if __name__ == "__main__":
    unittest.main()
