add_subdirectory(tests)

set(ICCJPEG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lcms")

include_directories(
    ${ICCJPEG_SOURCE_DIR}
)

include_directories(SYSTEM
    ${LCMS2_INCLUDE_DIR}
)

if(IOS)
    if(NOT TARGET Qt6::BundledLibjpeg)
        find_package(Qt6BundledLibjpeg REQUIRED)
    endif()
    get_target_property(_qt_bundled_jpeg_location Qt6::BundledLibjpeg IMPORTED_LOCATION)
    if(NOT _qt_bundled_jpeg_location)
        get_target_property(_qt_bundled_jpeg_location Qt6::BundledLibjpeg IMPORTED_LOCATION_RELEASE)
    endif()
    get_filename_component(_qt_bundled_jpeg_lib_dir "${_qt_bundled_jpeg_location}" DIRECTORY)
    find_path(KRITA_IOS_QT_JPEG_INCLUDE_DIR
        NAMES jpeglib.h
        HINTS "${_qt_bundled_jpeg_lib_dir}/../include/QtJpeg"
        NO_DEFAULT_PATH
        REQUIRED
    )
    set(kritajpeg_LIBRARIES Qt6::BundledLibjpeg)
else()
    set(kritajpeg_LIBRARIES ${JPEG_LIBRARIES})
endif()

set(libkritaconverter_LIB_SRCS
    kis_jpeg_converter.cc
    kis_jpeg_source.cpp
    kis_jpeg_destination.cpp
    ${ICCJPEG_SOURCE_DIR}/iccjpeg.c
    )

if(IOS)
    kis_add_library(kritajpegcommon STATIC ${libkritaconverter_LIB_SRCS})
    target_include_directories(kritajpegcommon BEFORE PUBLIC ${KRITA_IOS_QT_JPEG_INCLUDE_DIR})
    target_link_libraries(kritajpegcommon PUBLIC kritaapplicationui ${kritajpeg_LIBRARIES} ${LCMS2_LIBRARIES} LibExiv2::LibExiv2)
    set(kritajpegcommon_TARGET kritajpegcommon)
    set(libkritaconverter_PLUGIN_SRCS)
else()
    set(libkritaconverter_PLUGIN_SRCS ${libkritaconverter_LIB_SRCS})
endif()

set(kritajpegimport_SOURCES
    kis_jpeg_import.cc
    ${libkritaconverter_PLUGIN_SRCS}
    )

kis_add_library(kritajpegimport MODULE ${kritajpegimport_SOURCES})

target_link_libraries(kritajpegimport kritaapplicationui ${kritajpegcommon_TARGET} ${kritajpeg_LIBRARIES} ${LCMS2_LIBRARIES} LibExiv2::LibExiv2 )

install(TARGETS kritajpegimport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

set(kritajpegexport_SOURCES
    kis_jpeg_export.cc
    ${libkritaconverter_PLUGIN_SRCS}
    )

ki18n_wrap_ui(kritajpegexport_SOURCES kis_wdg_options_jpeg.ui )

kis_add_library(kritajpegexport MODULE ${kritajpegexport_SOURCES})

target_link_libraries(kritajpegexport kritaapplication kritaapplicationui kritaimpex ${kritajpegcommon_TARGET} ${kritajpeg_LIBRARIES} ${LCMS2_LIBRARIES} LibExiv2::LibExiv2 )

if (MSVC AND "${CMAKE_CXX_FLAGS}" MATCHES "-EHsc") 
    message(WARNING "The JPEG error catcher requires crossing the C ABI on MSVC. Exceptions need to be patched in.")
    # It needs to be done at the file level to override OpenEXR sneaking /EHsc.
    # See https://github.com/AcademySoftwareFoundation/openexr/issues/1326
    set_source_files_properties(kis_jpeg_converter.cc PROPERTIES COMPILE_OPTIONS "/EHsc-")
endif()

install(TARGETS kritajpegexport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install(FILES krita_jpeg.desktop DESTINATION ${KDE_INSTALL_APPDIR})
