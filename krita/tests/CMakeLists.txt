kis_add_test(
    KisIOSApplicationIntegrationSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisIOSApplicationIntegrationSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
