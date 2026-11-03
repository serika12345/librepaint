kis_add_test(KisGpuTilePresentationTest.cpp
    TEST_NAME_VAR gpu_tile_presentation_test
    LINK_LIBRARIES kritacanvasgputilepresentation kritagpuimage Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuTilePresentationTest PRIVATE kritacanvas_EXPORTS)

kis_add_test(KisGpuCanvasWindowTest.cpp
    TEST_NAME_VAR gpu_canvas_window_test
    LINK_LIBRARIES kritacanvasgputilepresentation kritagpuimage Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuCanvasWindowTest PRIVATE kritacanvas_EXPORTS)
if(BUILD_TESTING AND NOT IOS AND NOT ANDROID)
    add_test(NAME libs-canvas-gpu-offscreen-KisGpuCanvasWindowTest
        COMMAND KisGpuCanvasWindowTest unconfiguredSurfacesAreRejectedBeforeNativeSubmission)
    set_tests_properties(libs-canvas-gpu-offscreen-KisGpuCanvasWindowTest PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

add_executable(KisGpuInteractiveFrameBenchmark EXCLUDE_FROM_ALL KisGpuInteractiveFrameBenchmark.cpp)
target_link_libraries(KisGpuInteractiveFrameBenchmark PRIVATE
    kritacanvasgputilepresentation kritagpuimage Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuInteractiveFrameBenchmark PRIVATE kritacanvas_EXPORTS)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_tests_properties(${gpu_canvas_window_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_tile_presentation_test} ${gpu_canvas_window_test}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
