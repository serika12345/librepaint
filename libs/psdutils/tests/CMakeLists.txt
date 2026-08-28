set(PSD_TEST_LIBS kritapsdutils kritatestsdk)

kis_add_tests(
    psd_utils_test.cpp
    compression_test.cpp
    psd_cos_parser_test.cpp
    LINK_LIBRARIES ${PSD_TEST_LIBS})

kis_add_test(
    PsdFormatValuesContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(PsdFormatValuesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(PsdFormatValuesContractTest PRIVATE kritapsdutils_EXPORTS)
