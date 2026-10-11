kis_add_test(KisRgb8DisplayLookupTest.cpp
    LINK_LIBRARIES kritacanvasrgb8displaylookup Qt${QT_MAJOR_VERSION}::Test)
add_dependencies(KisRgb8DisplayLookupTest kritalcmsengine)
set_tests_properties(libs-canvas-gpu-KisRgb8DisplayLookupTest PROPERTIES TIMEOUT 120)

# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuCanvasRendererTest.cpp
    LINK_LIBRARIES kritacanvasgpurenderer Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuCanvasRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuCanvasRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
set_tests_properties(libs-canvas-gpu-KisGpuCanvasRendererTest PROPERTIES
    TIMEOUT 30 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")

if(TARGET kritacanvasgpusurface)
    kis_add_test(KisGpuDocumentCanvasTest.cpp LINK_LIBRARIES kritacanvasgpudocument Qt${QT_MAJOR_VERSION}::Test)
    target_sources(KisGpuDocumentCanvasTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
    target_include_directories(KisGpuDocumentCanvasTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
    set_tests_properties(libs-canvas-gpu-KisGpuDocumentCanvasTest PROPERTIES
        TIMEOUT 60 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
        FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
    if(APPLE)
        set_tests_properties(libs-canvas-gpu-KisGpuDocumentCanvasTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    else()
        set_tests_properties(libs-canvas-gpu-KisGpuDocumentCanvasTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
    endif()
    add_executable(KisGpuInputToDisplayBenchmark EXCLUDE_FROM_ALL KisGpuInputToDisplayBenchmark.cpp
        ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
    target_link_libraries(KisGpuInputToDisplayBenchmark PRIVATE kritacanvasgpusurface kritapaintinggpubrushstroke)
    target_include_directories(KisGpuInputToDisplayBenchmark PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests
        ${CMAKE_SOURCE_DIR}/libs/painting/gpu/tests)
    set_target_properties(KisGpuInputToDisplayBenchmark PROPERTIES AUTOMOC OFF MACOSX_BUNDLE OFF)
    kis_add_test(KisGpuSurfaceLifetimeTest.cpp
        LINK_LIBRARIES kritacanvasgpusurface Qt${QT_MAJOR_VERSION}::Test)
    target_sources(KisGpuSurfaceLifetimeTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
    target_include_directories(KisGpuSurfaceLifetimeTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
    set_tests_properties(libs-canvas-gpu-KisGpuSurfaceLifetimeTest PROPERTIES
        TIMEOUT 30 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
        FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
    if(APPLE)
        set_tests_properties(libs-canvas-gpu-KisGpuSurfaceLifetimeTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    else()
        set_tests_properties(libs-canvas-gpu-KisGpuSurfaceLifetimeTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
    endif()
    kis_add_test(KisGpuSurfaceRendererTest.cpp
        LINK_LIBRARIES kritacanvasgpusurface Qt${QT_MAJOR_VERSION}::Test)
    target_sources(KisGpuSurfaceRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
    target_include_directories(KisGpuSurfaceRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
    set_tests_properties(libs-canvas-gpu-KisGpuSurfaceRendererTest PROPERTIES
        TIMEOUT 90 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
        FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
    if(APPLE)
        set_tests_properties(libs-canvas-gpu-KisGpuSurfaceRendererTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    else()
        set_tests_properties(libs-canvas-gpu-KisGpuSurfaceRendererTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
    endif()
endif()
