kis_add_tests(
    StoryboardModelTest.cpp
    StoryboardTimelineSyncTest.cpp
    LINK_LIBRARIES kritastoryboarddocker_static kritaapplicationui kritatestsdk
    )

kis_add_test(
    StoryboardModelSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(StoryboardModelSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/ui
    ${CMAKE_BINARY_DIR}/libs/ui
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(StoryboardModelSchemaContractTest PRIVATE
    kritastoryboarddocker_static_EXPORTS
    kritaui_EXPORTS
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
)
