kis_add_test(
    TestApplicationConfiguration.cpp
    TEST_NAME TestApplicationConfiguration
    LINK_LIBRARIES kritaapplication Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisSnapConfigContractTest.cpp
    TEST_NAME KisSnapConfigContractTest
    LINK_LIBRARIES
        kritaapplicationsnapconfigobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSnapConfigContractTest PRIVATE
    kritaapplication_EXPORTS
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisConfigCompatibilityTest.cpp
    LINK_LIBRARIES
        kritaapplication
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisActionCompatibilityTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisActionCompatibilityTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisActionCompatibilityTest PRIVATE
    kritaglobal_EXPORTS
    kritaui_EXPORTS
)

kis_add_test(
    KisPreferenceSetRegistryContractTest.cpp
    TEST_NAME KisPreferenceSetRegistryContractTest
    LINK_LIBRARIES
        kritaapplicationpreferencesetregistryobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisPreferenceSetRegistryContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaglobal_EXPORTS
)
