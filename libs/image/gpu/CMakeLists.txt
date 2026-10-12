add_library(kritaimagegpuoperations OBJECT
    KisGpuOperationDocument.cpp
    KisGpuOperationRecovery.cpp
)
target_compile_definitions(kritaimagegpuoperations PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagegpuoperations PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritaimagegpuoperations PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaimagegpuoperations PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

find_package(Qt6 6.6 REQUIRED COMPONENTS GuiPrivate)
find_package(Qt6ShaderToolsTools ${Qt6_VERSION} EXACT REQUIRED)
add_library(kritaimagegputiles OBJECT KisGpuTileRenderer.cpp)
target_compile_definitions(kritaimagegputiles PRIVATE kritaimage_EXPORTS)
target_link_libraries(kritaimagegputiles PUBLIC kritaimagegpuoperations PRIVATE Qt6::GuiPrivate)
set_target_properties(kritaimagegputiles PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
qt6_add_shaders(kritaimagegputiles krita_gpu_tiles
    PREFIX "/librepaint/gpu"
    GLSL "310es,430"
    NOHLSL
    FILES shaders/paint.comp
)

add_subdirectory(tests)
