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

kis_add_test(KisGpuCanvasWorkspaceTest.cpp LINK_LIBRARIES kritagpucanvasworkspace kritaapplicationui kritatestsdk)
add_dependencies(KisGpuCanvasWorkspaceTest kritakraexport)
set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasWorkspaceTest PROPERTIES
    TIMEOUT 90 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
if(APPLE)
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasWorkspaceTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
else()
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasWorkspaceTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
