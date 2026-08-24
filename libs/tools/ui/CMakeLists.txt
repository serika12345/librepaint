set(kritatoolsui_LIB_SRCS
    KisPaintResourceServerProvider.cpp
    KisPaintopPropertiesBase.cpp
    KisPaletteEditor.cpp
    kis_categories_mapper.cpp
    kis_categorized_item_delegate.cpp
    kis_categorized_list_model.cpp
    kis_categorized_list_view.cpp
    kis_cmb_composite.cc
    kis_composite_ops_model.cc
    kis_paint_ops_model.cpp
    kis_paintop_list_widget.cpp
    kis_paintop_option.cpp
    kis_paintop_options_model.cpp
    kis_paintop_settings_widget.cpp
    kis_rectangle_constraint_widget.cpp
    kis_selection_options.cc
    kis_selection_tool_config_widget_helper.cpp
    kis_tool_options_popup.cpp
)

ki18n_wrap_ui(kritatoolsui_LIB_SRCS
    forms/wdgrectangleconstraints.ui
)

kis_add_library(kritatoolsui SHARED ${kritatoolsui_LIB_SRCS})
generate_export_header(kritatoolsui BASE_NAME kritatoolsui)

target_link_libraries(kritatoolsui
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        kritaglobal
        kritawidgetutils
        kritapigment
        kritaimage
        kritawidgets
        kritaresources
        kritaresourceui
    PRIVATE
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
)

set_target_properties(kritatoolsui PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)
install(TARGETS kritatoolsui ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
