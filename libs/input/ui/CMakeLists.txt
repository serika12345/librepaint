add_library(kritainputuiextendedmodifiersinterfaceobjects OBJECT
    KisExtendedModifiersMapperPluginInterface.cpp
)
target_compile_definitions(kritainputuiextendedmodifiersinterfaceobjects
    PRIVATE kritainputui_EXPORTS
)
target_include_directories(kritainputuiextendedmodifiersinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritainputuiextendedmodifiersinterfaceobjects
    PUBLIC Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritainputuiextendedmodifiersinterfaceobjects
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

set(kritainputuiextendedmodifiersmapper_SRCS
    kis_extended_modifiers_mapper.cpp
)
if(APPLE AND NOT IOS)
    list(APPEND kritainputuiextendedmodifiersmapper_SRCS
        kis_extended_modifiers_mapper_osx.mm
    )
endif()

add_library(kritainputuiextendedmodifiersmapperobjects OBJECT
    ${kritainputuiextendedmodifiersmapper_SRCS}
)
target_compile_definitions(kritainputuiextendedmodifiersmapperobjects
    PRIVATE kritainputui_EXPORTS
)
target_include_directories(kritainputuiextendedmodifiersmapperobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritainputuiextendedmodifiersmapperobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
)
if(APPLE AND NOT IOS)
    target_link_libraries(kritainputuiextendedmodifiersmapperobjects
        PUBLIC
            ${FOUNDATION_LIBRARY}
            ${APPKIT_LIBRARY}
    )
endif()
set_target_properties(kritainputuiextendedmodifiersmapperobjects
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

set(kritainputui_LIB_SRCS
    KisInputProfileMigrator.cpp
    KisPopupWidgetAction.cpp
    kis_abstract_input_action.cpp
    kis_alternate_invocation_action.cpp
    kis_change_primary_setting_action.cpp
    kis_input_config.cpp
    kis_input_manager.cpp
    kis_input_manager_canvas.cpp
    kis_input_manager_p.cpp
    kis_input_profile_manager.cpp
    kis_shortcut_configuration_text.cpp
    kis_tablet_debugger.cpp
    kis_tool_proxy.cpp
    kis_tool_invocation_action.cpp
    config/KisGestureSelector.cpp
    config/kis_action_shortcuts_model.cpp
    config/kis_edit_profiles_dialog.cpp
    config/kis_input_button.cpp
    config/kis_input_configuration_page.cpp
    config/kis_input_configuration_page_item.cpp
    config/kis_input_editor_delegate.cpp
    config/kis_input_mode_delegate.cpp
    config/kis_input_profile_model.cpp
    config/kis_input_type_delegate.cpp
    config/kis_key_input_editor.cpp
    config/kis_mouse_input_editor.cpp
    config/kis_wheel_input_editor.cpp
    wintab/drawpile_tablettester/tablettest.cpp
    wintab/drawpile_tablettester/tablettester.cpp
)

ki18n_wrap_ui(kritainputui_LIB_SRCS
    config/kis_edit_profiles_dialog.ui
    config/kis_input_configuration_page.ui
    config/kis_input_configuration_page_item.ui
    config/kis_key_input_editor.ui
    config/kis_mouse_input_editor.ui
    config/kis_wheel_input_editor.ui
    wintab/drawpile_tablettester/tablettest.ui
)

kis_add_library(kritainputui SHARED
    ${kritainputui_LIB_SRCS}
    $<TARGET_OBJECTS:kritainputuiextendedmodifiersinterfaceobjects>
    $<TARGET_OBJECTS:kritainputuiextendedmodifiersmapperobjects>
)
generate_export_header(kritainputui BASE_NAME kritainputui)

target_include_directories(kritainputui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wintab>
)

target_link_libraries(kritainputui
    PUBLIC
        kritacanvas
        kritaflake
        kritainput
        kritatools
        kritawidgetutils
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        kritaversion
        kritawidgets
        kritatoolsui
        kritaxmlserialization
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews
        Qt${QT_MAJOR_VERSION}::Concurrent
        Qt${QT_MAJOR_VERSION}::Network
        ${PNG_LIBRARIES}
)

set_target_properties(kritainputui
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritainputui ${INSTALL_TARGETS_DEFAULT_ARGS})

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(kritainputui PRIVATE Qt6::OpenGLWidgets)
endif()

if(APPLE AND NOT IOS)
    target_link_libraries(kritainputui
        PRIVATE
            kritamacosutils
            ${FOUNDATION_LIBRARY}
            ${APPKIT_LIBRARY}
    )
endif()

add_subdirectory(tests)
