#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]


class CanvasPackageBoundaryTest(unittest.TestCase):
    def test_coordinate_boundary_has_one_owner(self) -> None:
        canvas_cmake = REPOSITORY_ROOT / "libs/canvas/CMakeLists.txt"
        self.assertTrue(canvas_cmake.is_file(), "libs/canvas must own a CMake target")

        cmake_text = canvas_cmake.read_text(encoding="utf-8")
        self.assertIn("kis_add_library(kritacanvas SHARED", cmake_text)
        self.assertNotIn("kritaui", cmake_text)

        expected_files = (
            "libs/canvas/kis_coordinates_converter.h",
            "libs/canvas/kis_coordinates_converter.cpp",
            "libs/canvas/KisCanvasState.h",
            "libs/canvas/KisCanvasState.cpp",
            "libs/canvas/tests/kis_coordinates_converter_test.h",
            "libs/canvas/tests/kis_coordinates_converter_test.cpp",
        )
        for relative_path in expected_files:
            with self.subTest(path=relative_path):
                self.assertTrue((REPOSITORY_ROOT / relative_path).is_file())

        removed_paths = (
            "libs/ui/canvas/kis_coordinates_converter.h",
            "libs/ui/canvas/kis_coordinates_converter.cpp",
            "libs/ui/canvas/KisCanvasState.h",
            "libs/ui/canvas/KisCanvasState.cpp",
            "libs/ui/tests/kis_coordinates_converter_test.h",
            "libs/ui/tests/kis_coordinates_converter_test.cpp",
        )
        for relative_path in removed_paths:
            with self.subTest(path=relative_path):
                self.assertFalse((REPOSITORY_ROOT / relative_path).exists())

    def test_coordinate_boundary_does_not_read_ui_configuration(self) -> None:
        source = (
            REPOSITORY_ROOT / "libs/canvas/kis_coordinates_converter.cpp"
        ).read_text(encoding="utf-8")
        self.assertNotIn("kis_config.h", source)
        self.assertNotIn("KisDocument.h", source)
        self.assertNotIn("KisView.h", source)


if __name__ == "__main__":
    unittest.main()
