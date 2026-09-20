kis_add_test(
    KisMultiArchPublicApiTest.cpp
    TEST_NAME KisMultiArchPublicApiTest
    LINK_LIBRARIES
        kritamultiarch
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
)
