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

set(kritatoolsuipaintopmodel_INCLUDE_DIRS
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/flake>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/compositeops>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)

add_library(kritatoolsuipaintopoptionsmodelobjects OBJECT
    kis_paintop_options_model.cpp
)
target_compile_definitions(kritatoolsuipaintopoptionsmodelobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuipaintopoptionsmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritatoolsuipaintopmodel_INCLUDE_DIRS}
)
target_link_libraries(kritatoolsuipaintopoptionsmodelobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritatoolsuipaintopoptionsmodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatoolsuipaintopoptionsmodelsourceobjects OBJECT
    kis_paintop_options_model_source.cpp
)
target_compile_definitions(kritatoolsuipaintopoptionsmodelsourceobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuipaintopoptionsmodelsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritatoolsuipaintopmodel_INCLUDE_DIRS}
)
target_link_libraries(kritatoolsuipaintopoptionsmodelsourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritatoolsuipaintopoptionsmodelsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatoolsuipaintopsmodelobjects OBJECT
    kis_paint_ops_model.cpp
)
target_compile_definitions(kritatoolsuipaintopsmodelobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuipaintopsmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritatoolsuipaintopsmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
)
set_target_properties(kritatoolsuipaintopsmodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritatoolsuipaintopsmodelsourceobjects OBJECT
    kis_paint_ops_model_source.cpp
)
target_compile_definitions(kritatoolsuipaintopsmodelsourceobjects PRIVATE
    kritatoolsui_EXPORTS
)
target_include_directories(kritatoolsuipaintopsmodelsourceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${kritatoolsuipaintopmodel_INCLUDE_DIRS}
)
target_link_libraries(kritatoolsuipaintopsmodelsourceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritatoolsuipaintopsmodelsourceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritatoolsui_LIB_SRCS
    $<TARGET_OBJECTS:kritatoolsuicategorizedmodelobjects>
    $<TARGET_OBJECTS:kritatoolsuicompositeopsmodelobjects>
    $<TARGET_OBJECTS:kritatoolsuicompositeopsmodelsourceobjects>
    $<TARGET_OBJECTS:kritatoolsuipaintopoptionsmodelobjects>
    $<TARGET_OBJECTS:kritatoolsuipaintopoptionsmodelsourceobjects>
    $<TARGET_OBJECTS:kritatoolsuipaintopsmodelobjects>
    $<TARGET_OBJECTS:kritatoolsuipaintopsmodelsourceobjects>

    KisPaintResourceServerProvider.cpp
    KisPaletteEditor.cpp
    kis_categorized_item_delegate.cpp
    kis_categorized_list_view.cpp
    kis_cmb_composite.cc
    kis_paintop_list_widget.cpp
    kis_paintop_option.cpp
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
