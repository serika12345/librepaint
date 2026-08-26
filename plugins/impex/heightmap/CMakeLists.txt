add_subdirectory(tests)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

if(IOS)
    # Import and export are distinct shared modules on desktop, so each can
    # own these implementation files.  iOS links both into one executable;
    # keep the QWidget metadata and heightmap utilities in one static helper.
    set(kritaheightmapcommon_SOURCES
        kis_wdg_options_heightmap.cpp
        kis_heightmap_utils.cpp
        )
    ki18n_wrap_ui(kritaheightmapcommon_SOURCES kis_wdg_options_heightmap.ui )
    kis_add_library(kritaheightmapcommon STATIC ${kritaheightmapcommon_SOURCES})
    target_link_libraries(kritaheightmapcommon PRIVATE kritaapplicationui)

    kis_add_library(kritaheightmapimport MODULE kis_heightmap_import.cpp)
    target_link_libraries(kritaheightmapimport PRIVATE kritaapplication kritaapplicationui kritaheightmapcommon)

    kis_add_library(kritaheightmapexport MODULE kis_heightmap_export.cpp)
    target_link_libraries(kritaheightmapexport PRIVATE kritaapplication kritaapplicationui kritaimpex kritaheightmapcommon)
else()
    set(kritaheightmapimport_SOURCES
        kis_heightmap_import.cpp
        kis_wdg_options_heightmap.cpp
        kis_heightmap_utils.cpp
        )
    ki18n_wrap_ui(kritaheightmapimport_SOURCES kis_wdg_options_heightmap.ui )
    kis_add_library(kritaheightmapimport MODULE ${kritaheightmapimport_SOURCES})
    target_link_libraries(kritaheightmapimport kritaapplication kritaapplicationui )

    set(kritaheightmapexport_SOURCES
        kis_heightmap_export.cpp
        kis_wdg_options_heightmap.cpp
        kis_heightmap_utils.cpp
        )
    ki18n_wrap_ui(kritaheightmapexport_SOURCES kis_wdg_options_heightmap.ui )
    kis_add_library(kritaheightmapexport MODULE ${kritaheightmapexport_SOURCES})
    target_link_libraries(kritaheightmapexport kritaapplication kritaapplicationui kritaimpex)
endif()

install(TARGETS kritaheightmapimport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
install(TARGETS kritaheightmapexport  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install(FILES krita_heightmap.desktop DESTINATION ${KDE_INSTALL_APPDIR})
