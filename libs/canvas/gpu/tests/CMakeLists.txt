# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuCanvasRendererTest.cpp
    LINK_LIBRARIES kritacanvasgpurenderer Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuCanvasRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuCanvasRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
set_tests_properties(libs-canvas-gpu-KisGpuCanvasRendererTest PROPERTIES
    TIMEOUT 30 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")

if(TARGET kritacanvasgpusurface)
    kis_add_test(KisGpuSurfaceRendererTest.cpp
        LINK_LIBRARIES kritacanvasgpusurface Qt${QT_MAJOR_VERSION}::Test)
    target_sources(KisGpuSurfaceRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
    target_include_directories(KisGpuSurfaceRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
    set_tests_properties(libs-canvas-gpu-KisGpuSurfaceRendererTest PROPERTIES
        TIMEOUT 90 LABELS "gpu;gpu-display" FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
    if(APPLE)
        set_tests_properties(libs-canvas-gpu-KisGpuSurfaceRendererTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    else()
        set_tests_properties(libs-canvas-gpu-KisGpuSurfaceRendererTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
    endif()
endif()
