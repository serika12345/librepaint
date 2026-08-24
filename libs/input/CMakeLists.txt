set(kritainput_LIB_SRCS
    KisInputActionGroup.cpp
    KisInputEventSuppressor.cpp
    kis_input_profile.cpp
    kis_abstract_shortcut.cpp
    kis_native_gesture_shortcut.cpp
    kis_shortcut_matcher.cpp
    kis_shortcut_configuration.cpp
    kis_single_action_shortcut.cpp
    kis_stroke_shortcut.cpp
    kis_touch_shortcut.cpp
)

kis_add_library(kritainput SHARED ${kritainput_LIB_SRCS})
generate_export_header(kritainput BASE_NAME kritainput)

target_link_libraries(kritainput
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        kritaflake
        kritaglobal
)

target_include_directories(kritainput
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(kritainput
    PROPERTIES VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritainput ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(ui)
add_subdirectory(tests)
