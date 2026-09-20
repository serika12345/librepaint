set(kritagamutmask_SOURCES
    gamutmask_plugin.cpp
    gamutmask_dock.cpp
    KisGamutMaskChooser.cpp
)

ki18n_wrap_ui(kritagamutmask_SOURCES 
    forms/wdgGamutMaskChooser.ui
)

kis_add_library(kritagamutmask MODULE ${kritagamutmask_SOURCES})
target_link_libraries(kritagamutmask kritaapplication kritaapplicationui)
install(TARGETS kritagamutmask DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
