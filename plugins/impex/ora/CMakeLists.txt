add_subdirectory(tests)

set(libkritaconverter_LIB_SRCS
    ora_converter.cpp
    kis_open_raster_load_context.cpp
    kis_open_raster_save_context.cpp
    kis_open_raster_stack_load_visitor.cpp
    kis_open_raster_stack_save_visitor.cpp
    )

if(IOS)
    kis_add_library(kritaoracommon STATIC ${libkritaconverter_LIB_SRCS})
    target_link_libraries(kritaoracommon PUBLIC kritaapplicationui kritaresourcestorage)
    set(kritaoracommon_TARGET kritaoracommon)
    set(libkritaconverter_PLUGIN_SRCS)
else()
    set(libkritaconverter_PLUGIN_SRCS ${libkritaconverter_LIB_SRCS})
endif()

set(kritaoraimport_SOURCES
    ora_import.cc
    ${libkritaconverter_PLUGIN_SRCS}
    )

kis_add_library(kritaoraimport MODULE ${kritaoraimport_SOURCES})

target_link_libraries(kritaoraimport kritaapplicationui kritaresourcestorage ${kritaoracommon_TARGET})

install(TARGETS kritaoraimport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

set(kritaoraexport_SOURCES
    ora_export.cc
    ${libkritaconverter_PLUGIN_SRCS}
    )

kis_add_library(kritaoraexport MODULE ${kritaoraexport_SOURCES})

target_link_libraries(kritaoraexport kritaapplicationui kritaimpex kritaresourcestorage ${kritaoracommon_TARGET})

install(TARGETS kritaoraexport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install(FILES krita_ora.desktop DESTINATION ${KDE_INSTALL_APPDIR})
