find_package(ZLIB 1.2.9 REQUIRED)
kis_add_library(kritaimagegpuoperations SHARED
    KisGpuOperationDocument.cpp
    KisGpuLayerTranslation.cpp
    KisGpuCanvasResize.cpp
    KisGpuLayerProjectionValues.cpp
    KisGpuOperationRecovery.cpp
    KisGpuRasterPixels.cpp
    KisGpuRasterOperations.cpp
)
generate_export_header(kritaimagegpuoperations BASE_NAME kritaimagegpuoperations
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/../kritaimagegpuoperations_export.h)
target_include_directories(kritaimagegpuoperations PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
)
target_link_libraries(kritaimagegpuoperations PUBLIC Qt${QT_MAJOR_VERSION}::Core)
target_link_libraries(kritaimagegpuoperations PRIVATE ZLIB::ZLIB)
set_target_properties(kritaimagegpuoperations PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritaimagegpuoperations ${INSTALL_TARGETS_DEFAULT_ARGS})

find_package(WGPUNative REQUIRED)
add_library(kritaimagegputiles OBJECT
    KisGpuPaintGeometry.cpp
    KisGpuDevice.cpp
    KisGpuPresentationSurface.cpp
    KisGpuExecution.cpp
    KisGpuPixelReadback.cpp
    KisGpuColorReadback.cpp
    KisGpuThumbnailReadback.cpp
    KisGpuProjectionResize.cpp
    KisGpuPaintJob.cpp
    KisGpuTileRenderer.cpp
    KisGpuTileProjection.cpp
    KisGpuTileTranslation.cpp
    KisGpuTileCrop.cpp
)
target_compile_definitions(kritaimagegputiles PRIVATE kritagpuimage_EXPORTS)
target_link_libraries(kritaimagegputiles PUBLIC kritaimagegpuoperations Qt6::Gui)
target_link_libraries(kritaimagegputiles PRIVATE WGPUNative::WGPUNative)
set_target_properties(kritaimagegputiles PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
qt6_add_resources(kritaimagegputiles krita_gpu_tiles
    PREFIX "/librepaint/gpu"
    FILES shaders/rgba.wgsl shaders/paint.wgsl shaders/over.wgsl shaders/transparency.wgsl shaders/bounds.wgsl shaders/translate.wgsl shaders/crop.wgsl shaders/thumbnail.wgsl shaders/resize.wgsl shaders/sample-color.wgsl
)

add_library(kritaimagegpupaint OBJECT KisGpuPaintImage.cpp KisGpuPaintImage.h KisGpuExternalPixels.cpp)
target_compile_definitions(kritaimagegpupaint PRIVATE kritagpuimage_EXPORTS)
target_link_libraries(kritaimagegpupaint PUBLIC kritaimagegpuoperations Qt6::Core PRIVATE kritaimagegputiles)
set_target_properties(kritaimagegpupaint PROPERTIES POSITION_INDEPENDENT_CODE ON)

kis_add_library(kritagpuimage SHARED)
generate_export_header(kritagpuimage BASE_NAME kritagpuimage
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/../kritagpuimage_export.h)
target_include_directories(kritagpuimage PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>)
target_link_libraries(kritagpuimage
    PRIVATE kritaimagegputiles kritaimagegpupaint
    PUBLIC kritaimagegpuoperations Qt6::Core Qt6::Gui)
set_target_properties(kritagpuimage PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION})
install(TARGETS kritagpuimage ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
