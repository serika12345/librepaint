add_library(kritapaintingundokundo2coreobjects OBJECT
    kundo2stack.cpp
    kundo2group.cpp
    kundo2magicstring.cpp
    kundo2commandextradata.cpp
)
target_compile_definitions(kritapaintingundokundo2coreobjects PRIVATE kritapaintingundo_EXPORTS)
target_include_directories(kritapaintingundokundo2coreobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintingundokundo2coreobjects
    PUBLIC
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
)
set_target_properties(kritapaintingundokundo2coreobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritapaintingundostoreobjects OBJECT
    kis_undo_store.cpp
    kis_undo_stores.cpp
)
target_compile_definitions(kritapaintingundostoreobjects PRIVATE kritapaintingundo_EXPORTS)
target_include_directories(kritapaintingundostoreobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintingundostoreobjects
    PUBLIC
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
)
set_target_properties(kritapaintingundostoreobjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(kritapaintingundostackactionobjects OBJECT
    kundo2stack_actions.cpp
)
target_compile_definitions(kritapaintingundostackactionobjects PRIVATE kritapaintingundo_EXPORTS)
target_include_directories(kritapaintingundostackactionobjects
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/widgetutils
        ${CMAKE_BINARY_DIR}/libs/widgetutils
        ${CMAKE_SOURCE_DIR}/libs/widgetutils/config
        ${CMAKE_SOURCE_DIR}/libs/widgetutils/xmlgui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintingundostackactionobjects
    PUBLIC
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigGui
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
)
set_target_properties(kritapaintingundostackactionobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintingundocommandutilsobjects OBJECT
    kis_command_utils.cpp
)
target_compile_definitions(kritapaintingundocommandutilsobjects PRIVATE kritapaintingundo_EXPORTS)
target_include_directories(kritapaintingundocommandutilsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintingundocommandutilsobjects
    PUBLIC
        kritapaintingundokundo2coreobjects
        kritapaintingundostoreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
)
set_target_properties(kritapaintingundocommandutilsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

set(kritapaintingundo_LIB_SRCS
    $<TARGET_OBJECTS:kritapaintingundokundo2coreobjects>
    $<TARGET_OBJECTS:kritapaintingundostoreobjects>
    $<TARGET_OBJECTS:kritapaintingundostackactionobjects>
    $<TARGET_OBJECTS:kritapaintingundocommandutilsobjects>
)

kis_add_library(kritapaintingundo SHARED ${kritapaintingundo_LIB_SRCS})
generate_export_header(kritapaintingundo BASE_NAME kritapaintingundo)

target_link_libraries(kritapaintingundo
    PUBLIC
        kritawidgetutils
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::ConfigGui
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Boost::boost
)

target_include_directories(kritapaintingundo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

set_target_properties(kritapaintingundo PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION}
    SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritapaintingundo ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
