# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuStrokeInputTest.cpp LINK_LIBRARIES kritatoolsgpustrokeinput Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuStrokeInputTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuStrokeInputTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
set_tests_properties(libs-tools-gpu-KisGpuStrokeInputTest PROPERTIES TIMEOUT 60 LABELS gpu
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
kis_add_test(KisGpuPointerInputTest.cpp LINK_LIBRARIES kritatoolsgpupointerinput Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuPointerInputTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuPointerInputTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
set_tests_properties(libs-tools-gpu-KisGpuPointerInputTest PROPERTIES TIMEOUT 60 LABELS gpu
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
