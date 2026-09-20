kis_add_test(
    kis_xcf_test.cpp
    TEST_NAME kis_xcf_test
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test(
    XcfToolsValueContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(XcfToolsValueContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/xcftools
)
