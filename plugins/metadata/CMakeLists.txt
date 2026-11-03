include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

# This helper is used by the TIFF and RAW importers.  The codec image
# import/export profile includes TIFF, so it needs the same helper on iOS.
if(NOT IOS OR KRITA_IOS_PLUGIN_CODEC_IMPEX)
    add_subdirectory(common)
endif()
add_subdirectory(exif)
add_subdirectory(iptc)
add_subdirectory(xmp)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
