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
