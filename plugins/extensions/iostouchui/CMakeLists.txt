set(kritaiostouchui_SOURCES
    iostouchui.cpp
    iostouchui.h
)

kis_add_library(kritaiostouchui MODULE ${kritaiostouchui_SOURCES})
target_link_libraries(kritaiostouchui PRIVATE kritainputui kritaui)
install(TARGETS kritaiostouchui DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
install(FILES iostouchui.action DESTINATION ${KDE_INSTALL_DATADIR}/krita/actions)
