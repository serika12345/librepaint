kis_add_tests(
    TestNodeView.cpp
    LINK_LIBRARIES kritaapplicationui kritalayerdocker_static kritatestsdk
    )

kis_add_test(
    LayerBoxSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(LayerBoxSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/ui
    ${CMAKE_BINARY_DIR}/libs/ui
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(LayerBoxSchemaContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalayerdocker_static_EXPORTS
    kritaui_EXPORTS
)
