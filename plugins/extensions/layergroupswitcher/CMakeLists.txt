set(kritalayergroupswitcher_SOURCES
    layergroupswitcher.cpp
)

kis_add_library(kritalayergroupswitcher MODULE ${kritalayergroupswitcher_SOURCES})
target_link_libraries(kritalayergroupswitcher kritaapplicationui)
install(TARGETS kritalayergroupswitcher  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
