# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuKraSaveJobTest.cpp LINK_LIBRARIES kritagpukrasave kritaapplicationui kritalibkra kritatestsdk)
target_sources(KisGpuKraSaveJobTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuKraSaveJobTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
add_dependencies(KisGpuKraSaveJobTest kritakraexport)
set_tests_properties(plugins-impex-libkra-gpu-KisGpuKraSaveJobTest PROPERTIES
    TIMEOUT 60 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
