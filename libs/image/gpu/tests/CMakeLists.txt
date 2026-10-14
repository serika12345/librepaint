kis_add_test(KisGpuOperationDocumentTest.cpp
    LINK_LIBRARIES kritaimagegpuoperations Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(KisGpuRasterPixelsTest.cpp
    LINK_LIBRARIES kritaimagegpuoperations Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(KisGpuPaintImageTest.cpp
    TEST_NAME_VAR gpu_paint_image_test
    LINK_LIBRARIES kritaimagegpupaint kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuPaintImageTest PRIVATE kritagpuimage_EXPORTS)

kis_add_test(KisGpuTileRendererTest.cpp
    TEST_NAME_VAR gpu_tile_renderer_test
    LINK_LIBRARIES kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuTileRendererTest PRIVATE kritagpuimage_EXPORTS)

kis_add_test(KisGpuExecutionTest.cpp
    TEST_NAME_VAR gpu_execution_test
    LINK_LIBRARIES kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuExecutionTest PRIVATE kritagpuimage_EXPORTS)

kis_add_test(KisGpuPixelReadbackTest.cpp
    TEST_NAME_VAR gpu_pixel_readback_test
    LINK_LIBRARIES kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuPixelReadbackTest PRIVATE kritagpuimage_EXPORTS)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_tests_properties(${gpu_execution_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_tile_renderer_test} ${gpu_execution_test} ${gpu_pixel_readback_test} ${gpu_paint_image_test}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
endif()
