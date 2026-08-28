add_library(kritatoolsuicategorizedmodelobjects OBJECT
    kis_categories_mapper.cpp
    kis_categorized_list_model.cpp
)
target_compile_definitions(kritatoolsuicategorizedmodelobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuicategorizedmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritatoolsuicategorizedmodelobjects
    PUBLIC Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritatoolsuicategorizedmodelobjects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatoolsuicompositeopsmodelobjects OBJECT
    kis_composite_ops_model.cc
)
target_compile_definitions(kritatoolsuicompositeopsmodelobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuicompositeopsmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritatoolsuicompositeopsmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
)
set_target_properties(kritatoolsuicompositeopsmodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatoolsuicompositeopsmodelsourceobjects OBJECT
    kis_composite_ops_model_source.cpp
)
target_compile_definitions(kritatoolsuicompositeopsmodelsourceobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuicompositeopsmodelsourceobjects
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/compositeops>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/widgetutils>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/widgetutils>
)
target_link_libraries(kritatoolsuicompositeopsmodelsourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
)
set_target_properties(kritatoolsuicompositeopsmodelsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritatoolsui_LIB_SRCS
    $<TARGET_OBJECTS:kritatoolsuicategorizedmodelobjects>
    $<TARGET_OBJECTS:kritatoolsuicompositeopsmodelobjects>
    $<TARGET_OBJECTS:kritatoolsuicompositeopsmodelsourceobjects>

    KisPaintResourceServerProvider.cpp
    KisPaletteEditor.cpp
    kis_categorized_item_delegate.cpp
    kis_categorized_list_view.cpp
    kis_cmb_composite.cc
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
        kritalibbrush
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
