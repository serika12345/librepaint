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

    def test_projection_boundary_has_one_owner(self) -> None:
        canvas_cmake = (
            REPOSITORY_ROOT / "libs/canvas/CMakeLists.txt"
        ).read_text(encoding="utf-8")

        expected_files = (
            "libs/canvas/kis_prescaled_projection.h",
            "libs/canvas/kis_prescaled_projection.cpp",
            "libs/canvas/kis_projection_backend.h",
            "libs/canvas/kis_projection_backend.cpp",
            "libs/canvas/kis_projection_update_info.h",
            "libs/canvas/kis_projection_update_info.cpp",
            "libs/canvas/kis_image_patch.h",
            "libs/canvas/kis_image_patch.cpp",
            "libs/canvas/kis_update_info.h",
            "libs/canvas/kis_update_info.cpp",
            "libs/canvas/tests/kis_prescaled_projection_contract_test.h",
            "libs/canvas/tests/kis_prescaled_projection_contract_test.cpp",
        )
        for relative_path in expected_files:
            with self.subTest(path=relative_path):
                self.assertTrue((REPOSITORY_ROOT / relative_path).is_file())
                if not relative_path.endswith(("_test.h", "_test.cpp")):
                    self.assertIn(Path(relative_path).name, canvas_cmake)

        removed_paths = (
            "libs/ui/canvas/kis_prescaled_projection.h",
            "libs/ui/canvas/kis_prescaled_projection.cpp",
            "libs/ui/canvas/kis_projection_backend.h",
            "libs/ui/canvas/kis_projection_backend.cpp",
            "libs/ui/canvas/kis_image_patch.h",
            "libs/ui/canvas/kis_image_patch.cpp",
            "libs/ui/canvas/kis_update_info.h",
            "libs/ui/canvas/kis_update_info.cpp",
        )
        for relative_path in removed_paths:
            with self.subTest(path=relative_path):
                self.assertFalse((REPOSITORY_ROOT / relative_path).exists())

    def test_projection_boundary_does_not_read_ui_state(self) -> None:
        production_sources = (
            "libs/canvas/kis_prescaled_projection.h",
            "libs/canvas/kis_prescaled_projection.cpp",
            "libs/canvas/kis_projection_backend.h",
            "libs/canvas/kis_projection_backend.cpp",
            "libs/canvas/kis_projection_update_info.h",
            "libs/canvas/kis_projection_update_info.cpp",
        )
        forbidden_fragments = (
            "kritaui",
            "kis_config.h",
            "kis_config_notifier.h",
            "kis_image_config.h",
            "KisDisplayConfig.h",
            "kis_display_filter.h",
            "kis_texture_tile_update_info.h",
            "KisDocument.h",
            "KisView.h",
        )
        for relative_path in production_sources:
            source = (REPOSITORY_ROOT / relative_path).read_text(encoding="utf-8")
            for fragment in forbidden_fragments:
                with self.subTest(path=relative_path, fragment=fragment):
                    self.assertNotIn(fragment, source)


if __name__ == "__main__":
    unittest.main()
