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

kis_add_test(KisGpuRasterImportTest.cpp
    TEST_NAME_VAR gpu_raster_import_test
    LINK_LIBRARIES kritaapplicationui kritapaintingmetadata kritatestsdk)
add_dependencies(KisGpuRasterImportTest
    kritalcmsengine kritapngimport kritapngexport kritajpegimport kritajpegexport
    kritakraimport kritakraexport kritaexif kritaiptc kritaxmp)
set_tests_properties(${gpu_raster_import_test} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
