kis_add_test(
    KisMacosValuesContractTest.cpp
    TEST_NAME KisMacosValuesContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisMacosValuesContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
