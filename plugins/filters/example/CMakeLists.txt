set(kritaexample_SOURCES example.cpp )
kis_add_library(kritaexample MODULE ${kritaexample_SOURCES})
target_link_libraries(kritaexample kritaapplicationui)
install(TARGETS kritaexample  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
