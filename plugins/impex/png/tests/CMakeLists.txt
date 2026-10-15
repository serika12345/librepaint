krita_add_broken_unit_test(
    kis_png_test.cpp
    TEST_NAME kis_png_test
    LINK_LIBRARIES kritaapplicationui kritatestsdk
)

add_test(
    NAME plugins-impex-png-KisPngHdrAndColorProfileContractTest
    COMMAND kis_png_test
        testSaveHDR
        testRoundtripCicpIccProfile
        testLoadPngWithLegacyProfile
)

add_test(
    NAME plugins-impex-png-KisPngRowWritingContractTest
    COMMAND kis_png_test testFileWritingPreservesExistingData testWriteCancellation testRowEncodingPreservesPixels testRowEncodingPreservesColorOptions testRowReadFailureStopsEncoding testWriteFailureReleasesSourcePixels
)

set_tests_properties(
    plugins-impex-png-KisPngHdrAndColorProfileContractTest
    plugins-impex-png-KisPngRowWritingContractTest
    PROPERTIES RESOURCE_LOCK kis_png_test_resources
)
