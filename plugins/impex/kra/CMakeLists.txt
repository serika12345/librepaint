set(kritakraimport_SOURCES
    kra_import.cpp
)

kis_add_library(kritakraimport MODULE ${kritakraimport_SOURCES})

target_link_libraries(kritakraimport kritaapplicationui kritalibkra)

install(TARGETS kritakraimport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

set(kritakraexport_SOURCES
    kra_export.cpp
)

kis_add_library(kritakraexport MODULE ${kritakraexport_SOURCES})

target_link_libraries(kritakraexport kritaapplicationui kritalibkra kritaimpex)

install(TARGETS kritakraexport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install(FILES krita_kra.desktop DESTINATION ${KDE_INSTALL_APPDIR})
