kis_add_test(KisGpuOperationDocumentTest.cpp
    LINK_LIBRARIES kritaimagegpuoperations Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(KisGpuPixelDataTest.cpp
    LINK_LIBRARIES kritaimagegpuoperations Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(KisGpuPaintImageTest.cpp
    TEST_NAME_VAR gpu_paint_image_test
    LINK_LIBRARIES kritaimagegpupaint kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuPaintImageTest PRIVATE kritagpuimage_EXPORTS)

kis_add_test(KisGpuImageBrushTest.cpp
    TEST_NAME_VAR gpu_image_brush_test
    LINK_LIBRARIES kritaimagegpupaint kritaimagegputiles kritaimagegpuoperations
        kritabrushqimagepyramidobjects kritaglobal kritapigment Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuImageBrushTest PRIVATE kritagpuimage_EXPORTS kritalibbrush_EXPORTS)
set_tests_properties(${gpu_image_brush_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

kis_add_test(KisGpuTileRendererTest.cpp KisGpuTileRendererTest2.cpp KisGpuTileRendererTest3.cpp KisGpuTileRendererTest4.cpp
    TEST_NAME KisGpuTileRendererTest
    TEST_NAME_VAR gpu_tile_renderer_test
    LINK_LIBRARIES kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative
)
target_compile_definitions(KisGpuTileRendererTest PRIVATE kritagpuimage_EXPORTS)
target_include_directories(KisGpuTileRendererTest PRIVATE ${CMAKE_SOURCE_DIR}/sdk/tests)

kis_add_test(KisGpuPixelToleranceTest.cpp
    LINK_LIBRARIES Qt6::Test)
target_include_directories(KisGpuPixelToleranceTest PRIVATE ${CMAKE_SOURCE_DIR}/sdk/tests)

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
    set_tests_properties(${gpu_tile_renderer_test} ${gpu_execution_test} ${gpu_paint_image_test}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb")
    set_tests_properties(${gpu_pixel_readback_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

kis_add_test(KisGpuRegionColourTest.cpp
    TEST_NAME_VAR gpu_region_colour_test
    LINK_LIBRARIES kritaimagegpupaint kritaimagegputiles kritaimagegpuoperations kritapigment Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuRegionColourTest PRIVATE kritagpuimage_EXPORTS)
add_dependencies(KisGpuRegionColourTest kritalcmsengine)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_tests_properties(${gpu_region_colour_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=xcb" TIMEOUT 300)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_tests_properties(${gpu_region_colour_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=cocoa" TIMEOUT 300)
endif()

kis_add_test(KisGpuPipelineBuildTest.cpp
    TEST_NAME_VAR gpu_pipeline_build_test
    LINK_LIBRARIES kritaimagegputiles kritaimagegpuoperations Qt6::Gui Qt6::Test WGPUNative::WGPUNative)
target_compile_definitions(KisGpuPipelineBuildTest PRIVATE kritagpuimage_EXPORTS)
set_tests_properties(${gpu_pipeline_build_test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;__GL_SHADER_DISK_CACHE=0;MESA_SHADER_CACHE_DISABLE=true" TIMEOUT 30 RUN_SERIAL TRUE)
