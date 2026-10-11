# SPDX-License-Identifier: GPL-2.0-or-later
kis_add_test(KisGpuDocumentWindowTest.cpp LINK_LIBRARIES kritagpucanvaswindow Qt${QT_MAJOR_VERSION}::Test)
set_tests_properties(plugins-extensions-gpucanvas-KisGpuDocumentWindowTest PROPERTIES
    TIMEOUT 60 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
if(APPLE)
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuDocumentWindowTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
else()
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuDocumentWindowTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
