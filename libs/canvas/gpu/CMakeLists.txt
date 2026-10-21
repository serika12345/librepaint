find_package(WGPUNative REQUIRED)
add_library(kritacanvasgputilepresentation OBJECT
    KisGpuWindowSurface.cpp
    KisGpuTilePresentation.cpp
    KisGpuCropPreview.cpp
    KisGpuCanvasWindow.cpp KisGpuCanvasWindow.h
)
if(APPLE)
    target_sources(kritacanvasgputilepresentation PRIVATE KisGpuWindowSurfaceApple.mm)
    target_link_libraries(kritacanvasgputilepresentation PRIVATE "-framework QuartzCore")
endif()
target_compile_definitions(kritacanvasgputilepresentation PRIVATE kritacanvas_EXPORTS)
target_include_directories(kritacanvasgputilepresentation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritacanvasgputilepresentation PUBLIC kritagpuimage PRIVATE WGPUNative::WGPUNative)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
    find_package(Qt6 6.6 REQUIRED COMPONENTS GuiPrivate)
    target_link_libraries(kritacanvasgputilepresentation PRIVATE Qt6::GuiPrivate)
endif()
if(ANDROID)
    target_link_libraries(kritacanvasgputilepresentation PRIVATE android)
endif()
set_target_properties(kritacanvasgputilepresentation PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
qt6_add_resources(kritacanvasgputilepresentation krita_gpu_tile_presentation
    PREFIX "/librepaint/canvas"
    FILES shaders/tile.wgsl shaders/tool_outline.wgsl shaders/crop_preview.wgsl
)

add_subdirectory(tests)
