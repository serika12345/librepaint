add_library(kritainputshortcutconfigurationobjects OBJECT
    kis_shortcut_configuration.cpp
)
target_compile_definitions(kritainputshortcutconfigurationobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputshortcutconfigurationobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritainputshortcutconfigurationobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritainputshortcutconfigurationobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritainputprofileobjects OBJECT
    kis_input_profile.cpp
)
target_compile_definitions(kritainputprofileobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputprofileobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritainputprofileobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritainputprofileobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritainputabstractshortcutobjects OBJECT
    kis_abstract_shortcut.cpp
)
target_compile_definitions(kritainputabstractshortcutobjects PRIVATE
    kritacanvas_EXPORTS
    kritainput_EXPORTS
)
target_include_directories(kritainputabstractshortcutobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
)
target_link_libraries(kritainputabstractshortcutobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritainputabstractshortcutobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritainputtimedsignalthresholdobjects OBJECT
    KisTimedSignalThreshold.cpp
)
target_compile_definitions(kritainputtimedsignalthresholdobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputtimedsignalthresholdobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritainputtimedsignalthresholdobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritainputtimedsignalthresholdobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritainputeventsuppressorobjects OBJECT
    KisInputEventSuppressor.cpp
)
target_compile_definitions(kritainputeventsuppressorobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputeventsuppressorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
set_target_properties(kritainputeventsuppressorobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritainput_LIB_SRCS
    kis_native_gesture_shortcut.cpp
    kis_shortcut_matcher.cpp
    kis_single_action_shortcut.cpp
    kis_stroke_shortcut.cpp
    kis_touch_shortcut.cpp
)

kis_add_library(kritainput SHARED
    ${kritainput_LIB_SRCS}
    $<TARGET_OBJECTS:kritainputshortcutconfigurationobjects>
    $<TARGET_OBJECTS:kritainputprofileobjects>
    $<TARGET_OBJECTS:kritainputabstractshortcutobjects>
    $<TARGET_OBJECTS:kritainputtimedsignalthresholdobjects>
    $<TARGET_OBJECTS:kritainputeventsuppressorobjects>
)
generate_export_header(kritainput BASE_NAME kritainput)

target_link_libraries(kritainput
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritacanvas
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
