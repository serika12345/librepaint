# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuApplicationStateTest.cpp LINK_LIBRARIES kritaapplicationgpustate Qt${QT_MAJOR_VERSION}::Test)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET Qt${QT_MAJOR_VERSION}::DBus)
    target_link_libraries(KisGpuApplicationStateTest Qt${QT_MAJOR_VERSION}::DBus)
    target_compile_definitions(KisGpuApplicationStateTest PRIVATE KIS_GPU_TEST_MEMORY_DBUS)
endif()
set_tests_properties(libs-application-gpu-KisGpuApplicationStateTest PROPERTIES
    TIMEOUT 60 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
if(TARGET kritaapplicationgpucanvas)
    add_executable(KisGpuCanvasInputBenchmark EXCLUDE_FROM_ALL KisGpuCanvasInputBenchmark.cpp)
    target_link_libraries(KisGpuCanvasInputBenchmark PRIVATE kritaapplicationgpucanvas)
    target_include_directories(KisGpuCanvasInputBenchmark PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/canvas/gpu/tests ${CMAKE_SOURCE_DIR}/libs/painting/gpu/tests)
    set_target_properties(KisGpuCanvasInputBenchmark PROPERTIES AUTOMOC OFF MACOSX_BUNDLE OFF)
    kis_add_test(KisGpuCanvasSessionTest.cpp LINK_LIBRARIES kritaapplicationgpucanvas Qt${QT_MAJOR_VERSION}::Test)
    set_tests_properties(libs-application-gpu-KisGpuCanvasSessionTest PROPERTIES
        TIMEOUT 60 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
        FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
    if(APPLE)
        set_tests_properties(libs-application-gpu-KisGpuCanvasSessionTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
    else()
        set_tests_properties(libs-application-gpu-KisGpuCanvasSessionTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
    endif()
endif()
