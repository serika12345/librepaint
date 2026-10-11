# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuDocumentDeviceTest.cpp LINK_LIBRARIES kritadocumentgpudevice Qt${QT_MAJOR_VERSION}::Test)
target_include_directories(KisGpuDocumentDeviceTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
set_tests_properties(libs-document-gpu-KisGpuDocumentDeviceTest PROPERTIES
    TIMEOUT 60 LABELS gpu FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
