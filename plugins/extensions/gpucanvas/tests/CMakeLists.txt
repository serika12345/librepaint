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

kis_add_test(KisGpuCanvasEntryTest.cpp ${CMAKE_SOURCE_DIR}/krita/krita.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/Breeze-dark/breeze-dark-icons.qrc
    ${CMAKE_SOURCE_DIR}/krita/pics/Breeze-light/breeze-light-icons.qrc
    TEST_NAME KisGpuCanvasEntryTest
    LINK_LIBRARIES kritagpucanvaswindow kritaapplicationui kritatestsdk KF${KF_MAJOR}::I18n)
add_dependencies(KisGpuCanvasEntryTest kritagpucanvas kritalcmsengine krita_shape_image krita_shape_paths
    kritadefaultpaintops kritadefaulttools)
target_compile_definitions(KisGpuCanvasEntryTest PRIVATE
    GPU_ACTION_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/.."
    GPU_DEFAULT_RESOURCE_BUNDLE="${CMAKE_SOURCE_DIR}/krita/data/bundles/Krita_4_Default_Resources.bundle")
set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasEntryTest PROPERTIES
    TIMEOUT 90 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
if(APPLE)
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasEntryTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
else()
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasEntryTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()

kis_add_test(KisGpuCanvasWorkspaceTest.cpp LINK_LIBRARIES kritagpucanvasworkspace kritaapplicationui kritatestsdk)
add_dependencies(KisGpuCanvasWorkspaceTest kritakraexport kritakraimport kritalcmsengine krita_shape_image krita_shape_paths)
set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasWorkspaceTest PROPERTIES
    TIMEOUT 90 LABELS "gpu;gpu-display" RESOURCE_LOCK gpu_native_window
    FAIL_REGULAR_EXPRESSION "Validation Error;VUID-")
if(APPLE)
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasWorkspaceTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
else()
    set_tests_properties(plugins-extensions-gpucanvas-KisGpuCanvasWorkspaceTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
