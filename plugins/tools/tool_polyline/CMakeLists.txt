set(kritatoolpolyline_SOURCES
    tool_polyline.cc
    kis_tool_polyline.cc
    )

qt_add_resources(kritatoolpolyline_SOURCES polyline.qrc)
krita_ios_add_static_resource(polyline)

kis_add_library(kritatoolpolyline MODULE ${kritatoolpolyline_SOURCES})

target_link_libraries(kritatoolpolyline kritaui kritapainting)

install(TARGETS kritatoolpolyline  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})

install( FILES
    KisToolPolyline.action
DESTINATION  ${KDE_INSTALL_DATADIR}/krita/actions)
