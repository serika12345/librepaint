kis_add_test(KisGpuTilePresentationTest.cpp
    TEST_NAME_VAR gpu_tile_presentation_test
    LINK_LIBRARIES kritacanvasgputilepresentation kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuTilePresentationTest PRIVATE kritacanvas_EXPORTS kritaimage_EXPORTS)

kis_add_test(KisGpuCanvasWindowTest.cpp
    TEST_NAME_VAR gpu_canvas_window_test
    LINK_LIBRARIES kritacanvasgputilepresentation kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuCanvasWindowTest PRIVATE kritacanvas_EXPORTS kritaimage_EXPORTS)

add_executable(KisGpuInteractiveFrameBenchmark EXCLUDE_FROM_ALL KisGpuInteractiveFrameBenchmark.cpp)
target_link_libraries(KisGpuInteractiveFrameBenchmark PRIVATE
    kritacanvasgputilepresentation kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuInteractiveFrameBenchmark PRIVATE kritacanvas_EXPORTS kritaimage_EXPORTS)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_tests_properties(${gpu_canvas_window_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_tile_presentation_test} ${gpu_canvas_window_test}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
