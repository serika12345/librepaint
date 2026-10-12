find_package(Qt6 6.6 REQUIRED COMPONENTS GuiPrivate)
find_package(Qt6ShaderToolsTools ${Qt6_VERSION} EXACT REQUIRED)
add_library(kritacanvasgputilepresentation OBJECT KisGpuTilePresentation.cpp)
target_compile_definitions(kritacanvasgputilepresentation PRIVATE kritacanvas_EXPORTS)
target_include_directories(kritacanvasgputilepresentation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritacanvasgputilepresentation PUBLIC kritaimagegputiles PRIVATE Qt6::GuiPrivate)
set_target_properties(kritacanvasgputilepresentation PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
qt6_add_shaders(kritacanvasgputilepresentation krita_gpu_tile_presentation
    PREFIX "/librepaint/canvas"
    GLSL "310es,430"
    NOHLSL
    FILES shaders/tile.vert shaders/tile.frag
)

add_subdirectory(tests)
