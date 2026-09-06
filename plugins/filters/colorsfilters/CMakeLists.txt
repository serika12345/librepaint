add_library(kritafiltersvirtualchannelinfoobjects OBJECT
    virtual_channel_info.cpp
)
target_include_directories(kritafiltersvirtualchannelinfoobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritafiltersvirtualchannelinfoobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
        Boost::boost
)
set_target_properties(kritafiltersvirtualchannelinfoobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritacolorsfilters_SOURCES
    colorsfilters.cpp
    kis_hsv_adjustment_filter.cpp
    kis_multichannel_filter_base.cpp
    kis_multichannel_utils.cpp
    kis_perchannel_filter.cpp
    kis_cross_channel_filter.cpp
    kis_color_balance_filter.cpp
    kis_desaturate_filter.cpp
    )

ki18n_wrap_ui(kritacolorsfilters_SOURCES
    wdg_perchannel.ui
    wdg_color_balance.ui
    wdg_hsv_adjustment.ui
    wdg_desaturate.ui
)

kis_add_library(kritacolorsfilters MODULE
    ${kritacolorsfilters_SOURCES}
    $<TARGET_OBJECTS:kritafiltersvirtualchannelinfoobjects>
)
target_link_libraries(kritacolorsfilters kritaapplicationui)
install(TARGETS kritacolorsfilters  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
install( FILES
    colorsfilters.action
DESTINATION  ${KDE_INSTALL_DATADIR}/krita/actions)
