set(PSD_TEST_LIBS kritapsdutils kritatestsdk)

kis_add_tests(
    psd_utils_test.cpp
    psd_cos_parser_test.cpp
    LINK_LIBRARIES ${PSD_TEST_LIBS})

kis_add_test(
    compression_test.cpp
    LINK_LIBRARIES
        kritapsdcompressionobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(compression_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(compression_test PRIVATE kritapsdutils_EXPORTS)

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

kis_add_test(
    KisAslObjectCatcherContractTest.cpp
    LINK_LIBRARIES
        kritaaslobjectcatcherobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisAslObjectCatcherContractTest PRIVATE kritapsdutils_EXPORTS)

kis_add_test(
    KisOffsetKeeperContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisOffsetKeeperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisOffsetKeeperContractTest PRIVATE kritapsdutils_EXPORTS)

kis_add_test(
    KisOffsetOnExitVerifierContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisOffsetOnExitVerifierContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisOffsetOnExitVerifierContractTest PRIVATE kritapsdutils_EXPORTS)

kis_add_test(
    KisCosParserContractTest.cpp
    LINK_LIBRARIES
        kritapsdcosparserobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
if (TARGET Qt6::Core5Compat)
    target_link_libraries(KisCosParserContractTest Qt6::Core5Compat)
endif()
target_include_directories(KisCosParserContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisCosParserContractTest PRIVATE kritapsdutils_EXPORTS)
