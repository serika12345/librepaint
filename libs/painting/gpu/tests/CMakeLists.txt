# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuBrushStrokeTest.cpp LINK_LIBRARIES kritapaintinggpubrushstroke Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuBrushStrokeTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuBrushStrokeTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests
    ${CMAKE_SOURCE_DIR}/libs/image/dabspacing)
target_compile_definitions(KisGpuBrushStrokeTest PRIVATE
    SPACING_FIXTURE="${CMAKE_SOURCE_DIR}/libs/image/tests/data/brush_spacing_contract.json")
set_tests_properties(libs-painting-gpu-KisGpuBrushStrokeTest PROPERTIES TIMEOUT 30 LABELS gpu
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
