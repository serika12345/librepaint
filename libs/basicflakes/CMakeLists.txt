include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
)

set(kritabasicflakes_LIB_SRCS
    tools/KoCreatePathTool.cpp
    tools/KoPencilTool.cpp
)

ki18n_wrap_ui( kritabasicflakes_LIB_SRCS
)


kis_add_library(kritabasicflakes SHARED ${kritabasicflakes_LIB_SRCS})
generate_export_header(kritabasicflakes)

target_include_directories(kritabasicflakes
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tools>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plugin>
)

target_link_libraries(kritabasicflakes 
    PUBLIC
        kritawidgets
        kritaflake 
        kritapigment
    PRIVATE
        KF${KF_MAJOR}::ConfigCore
)

set_target_properties(kritabasicflakes PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritabasicflakes  ${INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_TESTING)
    kis_add_test(
        tests/KoPathCreationToolSchemaContractTest.cpp
        LINK_LIBRARIES
            Qt${QT_MAJOR_VERSION}::Core
            Qt${QT_MAJOR_VERSION}::Test
    )
    target_include_directories(KoPathCreationToolSchemaContractTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tools
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/libs/flake
        ${CMAKE_BINARY_DIR}/libs/flake
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:KF${KF_MAJOR}::ConfigCore,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(KoPathCreationToolSchemaContractTest PRIVATE
        kritabasicflakes_EXPORTS
        kritaflake_EXPORTS
    )
endif()
