# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: BSD-3-Clause

find_path(WGPUNative_INCLUDE_DIR NAMES webgpu/wgpu.h)
find_library(WGPUNative_LIBRARY NAMES wgpu_native)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WGPUNative
    REQUIRED_VARS WGPUNative_LIBRARY WGPUNative_INCLUDE_DIR)

if(WGPUNative_FOUND AND NOT TARGET WGPUNative::WGPUNative)
    add_library(WGPUNative::WGPUNative UNKNOWN IMPORTED)
    set_target_properties(WGPUNative::WGPUNative PROPERTIES
        IMPORTED_LOCATION "${WGPUNative_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WGPUNative_INCLUDE_DIR}")
    if(IOS)
        set_property(TARGET WGPUNative::WGPUNative PROPERTY INTERFACE_LINK_LIBRARIES
            "-framework Metal;-framework Foundation;-framework QuartzCore;-lobjc")
    endif()
endif()

mark_as_advanced(WGPUNative_INCLUDE_DIR WGPUNative_LIBRARY)
