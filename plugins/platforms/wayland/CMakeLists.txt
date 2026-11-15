if (NOT QT_MAJOR_VERSION STREQUAL "6")
    message(FATAL_ERROR "kritaplatformpluginwayland library is supported with Qt6 only")
endif()

project( kritaplatformpluginwayland )

##
## Define the kritaplatformpluginwayland library
##

set(kritaplatformpluginwayland_LIB_SRCS
    KisWaylandKeyboardWatcher.cpp
    KisExtendedModifiersMapperWayland.cpp
    KritaPlatformPluginWayland.cpp
    KisWaylandRegistry.cpp
)

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    list(APPEND kritaplatformpluginwayland_LIB_SRCS
         waylandcolormanagement/KisWaylandAPIColorManager.cpp
         waylandcolormanagement/KisWaylandAPIImageDescription.cpp
         waylandcolormanagement/KisWaylandAPIImageDescriptionCreatorParams.cpp
         waylandcolormanagement/KisWaylandAPIOutput.cpp
         waylandcolormanagement/KisWaylandAPISurface.cpp
         waylandcolormanagement/KisWaylandOutputColorInfo.cpp
         waylandcolormanagement/KisWaylandSurfaceColorimetry.cpp
         waylandcolormanagement/KisWaylandSurfaceColorManager.cpp
         waylandcolormanagement/KisWaylandDebugInfoFetcher.cpp)
endif()

kis_add_library(kritaplatformpluginwayland MODULE ${kritaplatformpluginwayland_LIB_SRCS})

set(kritaplatformpluginwayland_WAYLAND_PROTOCOLS
    "${CMAKE_CURRENT_SOURCE_DIR}/protocols/wayland.xml"
)

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    list(APPEND kritaplatformpluginwayland_WAYLAND_PROTOCOLS
         "${CMAKE_CURRENT_SOURCE_DIR}/protocols/color-management-v1.xml")
endif()

qt6_generate_wayland_protocol_client_sources(kritaplatformpluginwayland
    FILES
    ${kritaplatformpluginwayland_WAYLAND_PROTOCOLS}
)

set_source_files_properties(
    KisWaylandKeyboardWatcher.cpp
    PROPERTIES
    INCLUDE_DIRECTORIES
    "${Qt6WaylandGlobalPrivate_INCLUDE_DIRS};${Qt6WaylandClient_PRIVATE_INCLUDE_DIRS};${Qt6Gui_PRIVATE_INCLUDE_DIRS}"
    )

target_link_libraries(kritaplatformpluginwayland
    PRIVATE
        Qt::Gui
        Qt::GuiPrivate
        Qt::WaylandClient
        kritaglobal
        kritainputui
        kritaapplicationui
)

if (KRITA_USE_SURFACE_COLOR_MANAGEMENT_API)
    target_link_libraries(kritaplatformpluginwayland
        PRIVATE
        kritasurfacecolormanagementapi
    )
endif()

install(TARGETS kritaplatformpluginwayland DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
