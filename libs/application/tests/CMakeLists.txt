kis_add_test(
    TestApplicationConfiguration.cpp
    TEST_NAME TestApplicationConfiguration
    LINK_LIBRARIES kritaapplication Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisConfigEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisConfigEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/compositeops
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
)
target_compile_definitions(KisConfigEnumContractTest PRIVATE
    kritaapplication_EXPORTS
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)
