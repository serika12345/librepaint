# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuApplicationStateTest.cpp LINK_LIBRARIES kritaapplicationgpustate Qt${QT_MAJOR_VERSION}::Test)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET Qt${QT_MAJOR_VERSION}::DBus)
    target_link_libraries(KisGpuApplicationStateTest PRIVATE Qt${QT_MAJOR_VERSION}::DBus)
    target_compile_definitions(KisGpuApplicationStateTest PRIVATE KIS_GPU_TEST_MEMORY_DBUS)
endif()
set_tests_properties(libs-application-gpu-KisGpuApplicationStateTest PROPERTIES
    TIMEOUT 60 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
