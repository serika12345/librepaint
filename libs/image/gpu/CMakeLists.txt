find_package(ZLIB 1.2.9 REQUIRED)
add_library(kritaimagegpuoperations OBJECT
    KisGpuOperationDocument.cpp
    KisGpuOperationRecovery.cpp
    KisGpuRasterPixels.cpp
    KisGpuRasterOperations.cpp
    KisGpuPaintGeometry.cpp
)
target_compile_definitions(kritaimagegpuoperations PRIVATE kritaimage_EXPORTS)
target_include_directories(kritaimagegpuoperations PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritaimagegpuoperations PUBLIC Qt${QT_MAJOR_VERSION}::Core)
target_link_libraries(kritaimagegpuoperations PRIVATE ZLIB::ZLIB)
set_target_properties(kritaimagegpuoperations PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

find_package(Qt6 6.6 REQUIRED COMPONENTS GuiPrivate)
find_package(Qt6ShaderToolsTools ${Qt6_VERSION} EXACT REQUIRED)
add_library(kritaimagegputiles OBJECT
    KisGpuDevice.cpp
    KisGpuExecution.cpp
    KisGpuPixelReadback.cpp
    KisGpuPaintJob.cpp
    KisGpuTileRenderer.cpp
    KisGpuTileProjection.cpp
)
target_compile_definitions(kritaimagegputiles PRIVATE kritaimage_EXPORTS)
target_link_libraries(kritaimagegputiles PUBLIC kritaimagegpuoperations Qt6::Gui PRIVATE Qt6::GuiPrivate)
set_target_properties(kritaimagegputiles PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
qt6_add_shaders(kritaimagegputiles krita_gpu_tiles
    PREFIX "/librepaint/gpu"
    GLSL "310es,430"
    NOHLSL
    FILES shaders/paint.comp shaders/over.comp
)

add_library(kritaimagegpupaint OBJECT KisGpuPaintImage.cpp KisGpuPaintImage.h)
target_compile_definitions(kritaimagegpupaint PRIVATE kritaimage_EXPORTS)
target_link_libraries(kritaimagegpupaint PUBLIC kritaimagegpuoperations Qt6::Core PRIVATE kritaimagegputiles)
set_target_properties(kritaimagegpupaint PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_subdirectory(tests)
