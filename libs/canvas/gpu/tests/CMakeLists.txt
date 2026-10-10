# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuCanvasRendererTest.cpp
    LINK_LIBRARIES kritacanvasgpurenderer Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuCanvasRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuCanvasRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
set_tests_properties(libs-canvas-gpu-KisGpuCanvasRendererTest PROPERTIES
    TIMEOUT 30 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
