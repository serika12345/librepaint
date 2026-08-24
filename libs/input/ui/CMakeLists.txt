set(kritainputui_LIB_SRCS
    KisCanvasOnlyAction.cpp
    KisExtendedModifiersMapperPluginInterface.cpp
    KisInputProfileMigrator.cpp
    KisPopupWidgetAction.cpp
    KisQtWidgetsTweaker.cpp
    KisTouchGestureAction.cpp
    kis_abstract_input_action.cpp
    kis_alternate_invocation_action.cpp
    kis_change_frame_action.cpp
    kis_change_primary_setting_action.cpp
    kis_extended_modifiers_mapper.cpp
    kis_gamma_exposure_action.cpp
    kis_input_manager.cpp
    kis_input_manager_canvas.cpp
    kis_input_manager_p.cpp
    kis_input_profile_manager.cpp
    kis_pan_action.cpp
    kis_rotate_canvas_action.cpp
    kis_select_layer_action.cpp
    kis_shortcut_configuration_text.cpp
    kis_tablet_debugger.cpp
    kis_tool_invocation_action.cpp
    kis_zoom_action.cpp
    kis_zoom_and_rotate_action.cpp
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

if(APPLE AND NOT IOS)
    list(APPEND kritainputui_LIB_SRCS
        kis_extended_modifiers_mapper_osx.mm
    )
endif()

ki18n_wrap_ui(kritainputui_LIB_SRCS
    config/kis_edit_profiles_dialog.ui
    config/kis_input_configuration_page.ui
    config/kis_input_configuration_page_item.ui
    config/kis_key_input_editor.ui
    config/kis_mouse_input_editor.ui
    config/kis_wheel_input_editor.ui
    wintab/drawpile_tablettester/tablettest.ui
)

# This object target keeps input event wiring, configuration presentation,
# diagnostics, and platform integration in one input-owned build unit while
# kritaui continues to publish the established ABI.
kis_add_library(kritainputui OBJECT ${kritainputui_LIB_SRCS})
target_compile_definitions(kritainputui PRIVATE kritaui_EXPORTS)

target_include_directories(kritainputui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wintab>
    PRIVATE
        ${CMAKE_BINARY_DIR}/libs/ui
        ${CMAKE_BINARY_DIR}/libs/impex
        ${CMAKE_SOURCE_DIR}/libs/impex
        ${CMAKE_SOURCE_DIR}/libs/impex/ui
        ${CMAKE_SOURCE_DIR}/libs/impex/animation
        ${CMAKE_SOURCE_DIR}/libs/ui
        ${CMAKE_SOURCE_DIR}/libs/ui/canvas
        ${CMAKE_SOURCE_DIR}/libs/ui/flake
        ${CMAKE_SOURCE_DIR}/libs/ui/ora
        ${CMAKE_SOURCE_DIR}/libs/ui/tool
        ${CMAKE_SOURCE_DIR}/libs/ui/utils
        ${CMAKE_SOURCE_DIR}/libs/ui/widgets
        ${CMAKE_SOURCE_DIR}/libs/ui/widgets/gradient
)

target_link_libraries(kritainputui
    PRIVATE
        kritaversion
        kritawidgets
        kritawidgetutils
        kritatools
        kritatoolsui
        kritacanvas
        kritaflake
        kritainput
        kritaxmlserialization
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ItemViews
        Qt${QT_MAJOR_VERSION}::Concurrent
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Widgets
        ${PNG_LIBRARIES}
)

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
