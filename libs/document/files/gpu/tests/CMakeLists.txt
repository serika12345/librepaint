# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuDocumentRecoveryFileTest.cpp LINK_LIBRARIES kritadocumentgpurecovery Qt${QT_MAJOR_VERSION}::Test)
target_sources(KisGpuDocumentRecoveryFileTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests/KisGpuTestDevice.cpp)
target_include_directories(KisGpuDocumentRecoveryFileTest PRIVATE ${CMAKE_SOURCE_DIR}/libs/image/gpu/tests)
set_tests_properties(libs-document-files-gpu-KisGpuDocumentRecoveryFileTest PROPERTIES
    TIMEOUT 30 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
