if(IOS)
    # Keep the iOS runtime-data boundary deliberately small while the
    # provenance of the remaining non-code assets is being reviewed.
    add_subdirectory( actions )
    add_subdirectory( bundles )
    add_subdirectory( profiles )
    add_subdirectory( workspaces )
    add_subdirectory( themes )
    add_subdirectory( predefined_image_sizes )
    add_subdirectory( input )
    add_subdirectory( shortcuts )
    add_subdirectory( metadata/schemas )

    set(librepaint_NON_CODE_LICENSE_DIR
        "${KDE_INSTALL_DATADIR}/doc/librepaint/non-code-licenses")
    install(FILES
        "${CMAKE_SOURCE_DIR}/LICENSES/CC-BY-3.0.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/CC-BY-SA-3.0.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/CC-BY-SA-4.0.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/CC0-1.0.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/GPL-2.0-or-later.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0-only.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0-or-later.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/LGPL-2.0-or-later.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/LGPL-3.0-only.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/LGPL-3.0-or-later.txt"
        "${CMAKE_SOURCE_DIR}/LICENSES/LicenseRef-ICC-License.txt"
        "${CMAKE_SOURCE_DIR}/packaging/ios/manifests/default-resource-bundle-licenses.json"
        "${CMAKE_SOURCE_DIR}/packaging/ios/manifests/non-code-licenses.md"
        "${CMAKE_SOURCE_DIR}/packaging/ios/manifests/static-dependency-resources.json"
        "${CMAKE_SOURCE_DIR}/packaging/ios/manifests/ios-image-licenses.json"
        "${CMAKE_SOURCE_DIR}/packaging/ios/notices/librepaint-brand-assets.md"
        "${CMAKE_SOURCE_DIR}/packaging/ios/notices/qtbase-icc-attribution.json"
        "${CMAKE_SOURCE_DIR}/packaging/ios/notices/retained-functional-assets.md"
        DESTINATION "${librepaint_NON_CODE_LICENSE_DIR}")
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/README"
        DESTINATION "${librepaint_NON_CODE_LICENSE_DIR}/bundles")
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/elles-icc-profiles/README"
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/elles-icc-profiles/gpl-2.0.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/elles-icc-profiles/GNU General Public License v2.0 - GNU Project - Free Software Foundation (FSF).html"
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/elles-icc-profiles/html-README-for-elles-well-behaved-icc-profiles.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/elles-icc-profiles/plain-text-README-for-elles-well-behaved-icc-profiles.txt"
        DESTINATION "${librepaint_NON_CODE_LICENSE_DIR}/profiles/elles-icc-profiles")
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/ycbcr-icc-profiles/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/profiles/ycbcr-icc-profiles/LICENSE-PROFILES.txt"
        DESTINATION "${librepaint_NON_CODE_LICENSE_DIR}/profiles/ycbcr-icc-profiles")
else()
    add_subdirectory( actions )
    add_subdirectory( brushes )
    add_subdirectory( bundles )
    add_subdirectory( patterns )
    add_subdirectory( gradients )
    add_subdirectory( profiles )
    add_subdirectory( templates )
    add_subdirectory( workspaces )
    add_subdirectory( themes )
    add_subdirectory( predefined_image_sizes )
    add_subdirectory( input )
    add_subdirectory( shortcuts )
    add_subdirectory( paintoppresets )
    add_subdirectory( palettes )
    add_subdirectory( symbols )
    add_subdirectory( preset_icons )
    add_subdirectory( metadata/schemas )
    add_subdirectory( gamutmasks )
    if(HAVE_SEEXPR)
        add_subdirectory( seexpr_scripts )
    endif()

    if(NOT ANDROID)
        # Both layouts are unsupported since they detach/create a new window
        add_subdirectory( windowlayouts )
    endif()
endif()
