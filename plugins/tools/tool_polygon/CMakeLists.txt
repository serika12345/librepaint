set(kritatoolpolygon_SOURCES
    tool_polygon.cc
    kis_tool_polygon.cc
    )

qt_add_resources(kritatoolpolygon_SOURCES tool_polygon.qrc)
krita_ios_add_static_resource(tool_polygon)
kis_add_library(kritatoolpolygon MODULE ${kritatoolpolygon_SOURCES})

target_link_libraries(kritatoolpolygon kritaapplicationui kritapainting)

install(TARGETS kritatoolpolygon  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install( FILES
    KisToolPolygon.action
DESTINATION  ${KDE_INSTALL_DATADIR}/krita/actions)
