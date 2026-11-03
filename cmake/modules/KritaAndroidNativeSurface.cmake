# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later
function(kis_use_android_native_surface_plugin)
    if(NOT EXISTS "${LIBREPAINT_ANDROID_NATIVE_SURFACE_PLUGIN}")
        message(FATAL_ERROR "The Qt Android native-surface plugin is required for APK packaging")
    endif()
    if(NOT TARGET Qt6::QAndroidIntegrationPlugin)
        message(FATAL_ERROR "The Qt Android platform plugin target is unavailable")
    endif()
    set_property(TARGET Qt6::QAndroidIntegrationPlugin PROPERTY
        IMPORTED_LOCATION "${LIBREPAINT_ANDROID_NATIVE_SURFACE_PLUGIN}")
    get_target_property(configurations Qt6::QAndroidIntegrationPlugin IMPORTED_CONFIGURATIONS)
    foreach(configuration IN LISTS configurations)
        string(TOUPPER "${configuration}" configuration)
        set_property(TARGET Qt6::QAndroidIntegrationPlugin PROPERTY
            "IMPORTED_LOCATION_${configuration}" "${LIBREPAINT_ANDROID_NATIVE_SURFACE_PLUGIN}")
    endforeach()
endfunction()
