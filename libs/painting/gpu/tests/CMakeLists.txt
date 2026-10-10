# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuLayerBrushImageTest.cpp LINK_LIBRARIES kritapaintinggpubrushstroke Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuLayerBrushImageTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuLayerBrushImageTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
target_compile_definitions(KisGpuLayerBrushImageTest PRIVATE
    BRUSH_INPUT_FIXTURE="${CMAKE_SOURCE_DIR}/libs/painting/tests/data/gpu_brush_input_contract.json")
set_tests_properties(libs-painting-gpu-KisGpuLayerBrushImageTest PROPERTIES TIMEOUT 60 LABELS gpu
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")

kis_add_test(KisGpuBrushStrokeTest.cpp LINK_LIBRARIES kritapaintinggpubrushstroke Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuBrushStrokeTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuBrushStrokeTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests
    ${CMAKE_SOURCE_DIR}/libs/image/dabspacing)
target_compile_definitions(KisGpuBrushStrokeTest PRIVATE
    SPACING_FIXTURE="${CMAKE_SOURCE_DIR}/libs/image/tests/data/brush_spacing_contract.json")
set_tests_properties(libs-painting-gpu-KisGpuBrushStrokeTest PROPERTIES TIMEOUT 30 LABELS gpu
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
