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

add_library(kritainputnativegestureshortcutobjects OBJECT
    kis_native_gesture_shortcut.cpp
)
target_compile_definitions(kritainputnativegestureshortcutobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputnativegestureshortcutobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
)
target_link_libraries(kritainputnativegestureshortcutobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritainputnativegestureshortcutobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritainputstrokeshortcutobjects OBJECT
    kis_stroke_shortcut.cpp
)
target_compile_definitions(kritainputstrokeshortcutobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputstrokeshortcutobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
)
target_link_libraries(kritainputstrokeshortcutobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritainputstrokeshortcutobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritainputtouchshortcutobjects OBJECT
    kis_touch_shortcut.cpp
)
target_compile_definitions(kritainputtouchshortcutobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputtouchshortcutobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
)
target_link_libraries(kritainputtouchshortcutobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritainputtouchshortcutobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritainputsingleactionshortcutobjects OBJECT
    kis_single_action_shortcut.cpp
)
target_compile_definitions(kritainputsingleactionshortcutobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputsingleactionshortcutobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
)
target_link_libraries(kritainputsingleactionshortcutobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritainputsingleactionshortcutobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritainputshortcutmatcherobjects OBJECT
    kis_shortcut_matcher.cpp
)
target_compile_definitions(kritainputshortcutmatcherobjects PRIVATE kritainput_EXPORTS)
target_include_directories(kritainputshortcutmatcherobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/canvas>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/flake>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/flake>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritainputshortcutmatcherobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaglobal
)
set_target_properties(kritainputshortcutmatcherobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritainput SHARED
    $<TARGET_OBJECTS:kritainputshortcutconfigurationobjects>
    $<TARGET_OBJECTS:kritainputprofileobjects>
    $<TARGET_OBJECTS:kritainputabstractshortcutobjects>
    $<TARGET_OBJECTS:kritainputtimedsignalthresholdobjects>
    $<TARGET_OBJECTS:kritainputeventsuppressorobjects>
    $<TARGET_OBJECTS:kritainputnativegestureshortcutobjects>
    $<TARGET_OBJECTS:kritainputstrokeshortcutobjects>
    $<TARGET_OBJECTS:kritainputtouchshortcutobjects>
    $<TARGET_OBJECTS:kritainputsingleactionshortcutobjects>
    $<TARGET_OBJECTS:kritainputshortcutmatcherobjects>
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
