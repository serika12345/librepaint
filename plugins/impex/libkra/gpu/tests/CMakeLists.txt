# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuKraSaveJobTest.cpp LINK_LIBRARIES kritagpukrasave kritaapplicationui kritalibkra kritatestsdk)
target_sources(KisGpuKraSaveJobTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuKraSaveJobTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
add_dependencies(KisGpuKraSaveJobTest kritakraexport kritakraimport kritalcmsengine krita_shape_image krita_shape_paths)
set_tests_properties(plugins-impex-libkra-gpu-KisGpuKraSaveJobTest PROPERTIES
    TIMEOUT 60 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")

kis_add_test(KisGpuKraLoadJobTest.cpp LINK_LIBRARIES kritagpukraload kritagpukrasave kritadocumentgpudevice kritaapplicationui kritatestsdk)
target_sources(KisGpuKraLoadJobTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuKraLoadJobTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
add_dependencies(KisGpuKraLoadJobTest kritakraexport kritakraimport kritalcmsengine krita_shape_image krita_shape_paths)
set_tests_properties(plugins-impex-libkra-gpu-KisGpuKraLoadJobTest PROPERTIES
    TIMEOUT 60 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
