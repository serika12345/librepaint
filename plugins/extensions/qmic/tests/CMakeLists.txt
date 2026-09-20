kis_add_test(kis_qmic_tests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../kis_qmic_simple_convertor.cpp
    TEST_NAME kis_qmic_tests
    LINK_LIBRARIES kritaimage kritaqmicinterface kritatestsdk
    )

kis_add_test(
    KisQMicInterfaceContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisQMicInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisQMicInterfaceContractTest PRIVATE
    kritaqmicinterface_EXPORTS
)
