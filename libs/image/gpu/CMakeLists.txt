# SPDX-License-Identifier: GPL-2.0-or-later
find_path(LIBREPAINT_WGPU_NATIVE_INCLUDE_DIR webgpu/wgpu.h REQUIRED)
find_library(LIBREPAINT_WGPU_NATIVE_LIBRARY NAMES wgpu_native REQUIRED)
add_library(LibrePaintWgpuNative UNKNOWN IMPORTED)
set_target_properties(LibrePaintWgpuNative PROPERTIES
    IMPORTED_LOCATION "${LIBREPAINT_WGPU_NATIVE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBREPAINT_WGPU_NATIVE_INCLUDE_DIR}"
)

qt_add_resources(gpu_document_resources gpu_document.qrc)
add_library(kritaimagegpudocument STATIC KisGpuDevice.cpp KisGpuTileStore.cpp KisGpuTileTransfer.cpp KisGpuTileTexture.cpp KisGpuTileComposite.cpp KisGpuProjection.cpp KisGpuEditSession.cpp KisGpuEditRecovery.cpp KisGpuSubmissionTiming.cpp ${gpu_document_resources})
target_include_directories(kritaimagegpudocument PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kritaimagegpudocument PUBLIC Qt${QT_MAJOR_VERSION}::Core LibrePaintWgpuNative)
set_target_properties(kritaimagegpudocument PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
