kis_add_tests(
    TestPerspectiveBasedAssistantHelper.cpp
    TestAssistants.cpp
    LINK_LIBRARIES kritaassistanttool_static kritatestsdk
    )

kis_add_test(
    PerspectiveBasedAssistantHelperContractTest.cpp
    LINK_LIBRARIES
        kritaassistantperspectivehelperobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(PerspectiveBasedAssistantHelperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/ui
    ${CMAKE_BINARY_DIR}/libs/ui
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
)
target_compile_definitions(PerspectiveBasedAssistantHelperContractTest PRIVATE
    KRITAASSISTANTTOOL_STATIC_DEFINE
    kritaglobal_EXPORTS
)
