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
PLATFORM_PROFILES = {
    "macos": "tdd-macos",
    "linux": "tdd-linux",
    "ios": "ios-device-incremental",
    "android": "android-arm64-v8a-incremental",
    "windows": "windows-x86_64-incremental",
}
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

    def test_recorded_graphs_cover_every_platform(self) -> None:
        for platform, build_profile in PLATFORM_PROFILES.items():
            with self.subTest(platform=platform):
                graph_path = (
                    REPO_ROOT
                    / "docs/architecture"
                    / f"cmake-targets-{platform}.json"
                )
                graph = json.loads(graph_path.read_text(encoding="utf-8"))
                targets = graph["targets"]

                self.assertEqual(graph["platform"], platform)
                self.assertEqual(graph["buildProfile"], build_profile)
                self.assertEqual(
                    [target["name"] for target in targets],
                    sorted(target["name"] for target in targets),
                )
                for target in targets:
                    self.assertNotRegex(
                        target["name"], r"^(?:pofiles|tsfiles)-[0-9a-f]{32}$"
                    )
                    self.assertEqual(
                        set(target),
                        {"name", "type", "sourceDirectory", "dependencies"},
                    )
                    self.assertEqual(
                        target["dependencies"], sorted(target["dependencies"])
                    )

    def test_recorded_macos_graph_contains_the_architecture_entry_points(self) -> None:
        graph_path = REPO_ROOT / "docs/architecture/cmake-targets-macos.json"
        graph = json.loads(graph_path.read_text(encoding="utf-8"))
        targets = graph["targets"]
        targets_by_name = {target["name"]: target for target in targets}

        self.assertEqual(graph["platform"], "macos")
        self.assertEqual(graph["buildProfile"], "tdd-macos")
        self.assertEqual(
            [target["name"] for target in targets],
            sorted(target["name"] for target in targets),
        )
        self.assertEqual(targets_by_name["krita"]["type"], "EXECUTABLE")
        self.assertEqual(targets_by_name["kritaui"]["sourceDirectory"], "libs/ui")
        self.assertIn("kritaimage", targets_by_name["kritaui"]["dependencies"])
        self.assertEqual(
            targets_by_name["kritaimage"]["sourceDirectory"], "libs/image"
        )
        self.assertEqual(
            targets_by_name["krita_colorspaces_extensions"]["type"],
            "MODULE_LIBRARY",
        )


if __name__ == "__main__":
    unittest.main()
