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
        canvas_target_definition = cmake_text.split(
            "set_target_properties(kritacanvas", 1
        )[0]
        self.assertNotIn("kritaui", canvas_target_definition)

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

    def test_workspace_presentation_resource_has_canvas_source_ownership(self) -> None:
        cmake_text = (
            REPOSITORY_ROOT / "libs/canvas/CMakeLists.txt"
        ).read_text(encoding="utf-8")
        self.assertIn(
            "kis_add_library(kritaworkspacepresentation OBJECT", cmake_text
        )
        self.assertIn("workspace/kis_workspace_resource.cpp", cmake_text)
        self.assertIn("kritaui_EXPORTS", cmake_text)
        self.assertTrue(
            (
                REPOSITORY_ROOT
                / "libs/canvas/workspace/kis_workspace_resource.cpp"
            ).is_file()
        )
        self.assertTrue(
            (
                REPOSITORY_ROOT
                / "libs/canvas/workspace/kis_workspace_resource.h"
            ).is_file()
        )
        self.assertFalse(
            (REPOSITORY_ROOT / "libs/ui/kis_workspace_resource.cpp").exists()
        )
        self.assertFalse(
            (REPOSITORY_ROOT / "libs/ui/kis_workspace_resource.h").exists()
        )
        self.assertFalse(
            (
                REPOSITORY_ROOT
                / "libs/ui/workspace/kis_workspace_resource.cpp"
            ).exists()
        )
        self.assertFalse(
            (
                REPOSITORY_ROOT
                / "libs/ui/workspace/kis_workspace_resource.h"
            ).exists()
        )

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

    def test_display_color_boundary_has_one_owner(self) -> None:
        canvas_cmake = (
            REPOSITORY_ROOT / "libs/canvas/CMakeLists.txt"
        ).read_text(encoding="utf-8")
        expected_files = (
            "libs/canvas/color/KisOcioConfiguration.h",
            "libs/canvas/color/KisSurfaceColorSpaceWrapper.h",
            "libs/canvas/color/kis_display_color_filter.h",
            "libs/canvas/color/kis_display_color_transform.h",
            "libs/canvas/color/kis_display_color_transform.cpp",
            "libs/canvas/tests/kis_display_color_transform_test.h",
            "libs/canvas/tests/kis_display_color_transform_test.cpp",
            "libs/canvas/tests/KisSurfaceColorSpaceWrapperTest.h",
            "libs/canvas/tests/KisSurfaceColorSpaceWrapperTest.cpp",
        )
        for relative_path in expected_files:
            with self.subTest(path=relative_path):
                self.assertTrue((REPOSITORY_ROOT / relative_path).is_file())
                if "/tests/" not in relative_path:
                    self.assertIn(Path(relative_path).name, canvas_cmake)

        removed_paths = (
            "libs/ui/KisOcioConfiguration.h",
            "libs/ui/KisOcioConfiguration.cpp",
            "libs/ui/KisSurfaceColorSpaceWrapper.h",
            "libs/ui/tests/KisSurfaceColorSpaceWrapperTest.h",
            "libs/ui/tests/KisSurfaceColorSpaceWrapperTest.cpp",
        )
        for relative_path in removed_paths:
            with self.subTest(path=relative_path):
                self.assertFalse((REPOSITORY_ROOT / relative_path).exists())

    def test_display_color_transform_does_not_read_ui_state(self) -> None:
        production_sources = (
            "libs/canvas/color/KisOcioConfiguration.h",
            "libs/canvas/color/KisSurfaceColorSpaceWrapper.h",
            "libs/canvas/color/kis_display_color_filter.h",
            "libs/canvas/color/kis_display_color_transform.h",
            "libs/canvas/color/kis_display_color_transform.cpp",
        )
        forbidden_fragments = (
            "kritaui",
            "kis_config.h",
            "kis_config_notifier.h",
            "KoCanvasResourceProvider",
            "KisDisplayConfig",
            "KisDocument",
            "KisMainWindow",
            "KisNode",
            "KisPart",
            "QApplication",
            "QPalette",
        )
        for relative_path in production_sources:
            source = (REPOSITORY_ROOT / relative_path).read_text(encoding="utf-8")
            for fragment in forbidden_fragments:
                with self.subTest(path=relative_path, fragment=fragment):
                    self.assertNotIn(fragment, source)

    def test_ui_display_color_converter_only_coordinates_external_state(self) -> None:
        source = (
            REPOSITORY_ROOT
            / "libs/ui/canvas/kis_display_color_converter.cpp"
        ).read_text(encoding="utf-8")
        self.assertIn("KisDisplayColorTransform transform", source)
        self.assertIn("KisConfig config", source)
        self.assertIn("KisHandlePalette", source)
        self.assertNotIn("convertPixelsTo", source)
        self.assertNotIn("KisSequentialIterator", source)


if __name__ == "__main__":
    unittest.main()
