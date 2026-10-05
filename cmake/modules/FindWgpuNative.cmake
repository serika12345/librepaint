# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: BSD-3-Clause

find_path(WgpuNative_INCLUDE_DIR NAMES webgpu/wgpu.h)
find_library(WgpuNative_LIBRARY NAMES wgpu_native)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WgpuNative REQUIRED_VARS
    WgpuNative_INCLUDE_DIR WgpuNative_LIBRARY)
mark_as_advanced(WgpuNative_INCLUDE_DIR WgpuNative_LIBRARY)

if(WgpuNative_FOUND AND NOT TARGET WgpuNative::WgpuNative)
    add_library(WgpuNative::WgpuNative UNKNOWN IMPORTED)
    set_target_properties(WgpuNative::WgpuNative PROPERTIES
        IMPORTED_LOCATION "${WgpuNative_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WgpuNative_INCLUDE_DIR}")
endif()
