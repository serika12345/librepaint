add_subdirectory(tests)

set(kritawebpexport_SOURCES
    kis_wdg_options_webp.cpp
    kis_webp_export.cpp
)
ki18n_wrap_ui(kritawebpexport_SOURCES kis_wdg_options_webp.ui)

kis_add_library(kritawebpexport MODULE ${kritawebpexport_SOURCES})
target_link_libraries(kritawebpexport kritaapplicationui kritalibkra kritaimpex WebP::webp WebP::libwebpmux)

install(TARGETS kritawebpexport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})


set(kritawebpimport_SOURCES
    kis_webp_import.cpp
)

kis_add_library(kritawebpimport MODULE ${kritawebpimport_SOURCES})
target_link_libraries(kritawebpimport kritaapplicationui WebP::webp WebP::webpdemux)

install(TARGETS kritawebpimport DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install(FILES krita_webp.desktop DESTINATION ${KDE_INSTALL_APPDIR})
