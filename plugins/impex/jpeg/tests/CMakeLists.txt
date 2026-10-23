kis_add_test(
    kis_jpeg_test.cpp
    TEST_NAME kis_jpeg_test
    LINK_LIBRARIES kritapaintingmetadata kritaapplicationui kritatestsdk
    )

if(IOS)
    target_link_libraries(kis_jpeg_test kritajpegcommon)
else()
    target_sources(kis_jpeg_test PRIVATE ${libkritaconverter_LIB_SRCS})
    target_include_directories(kis_jpeg_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${ICCJPEG_SOURCE_DIR})
    target_link_libraries(kis_jpeg_test ${kritajpeg_LIBRARIES} ${LCMS2_LIBRARIES} LibExiv2::LibExiv2)
endif()
target_link_libraries(kis_jpeg_test kritaimpex kritagpuimage)
add_dependencies(kis_jpeg_test
    kritajpegimport kritajpegexport kritalcmsengine kritaexif kritaiptc kritaxmp)

kis_add_test(
    KisJpegCoefficientStoreTest.cpp
    ../KisJpegCoefficientStore.cpp
    ../kis_jpeg_destination.cpp
    TEST_NAME KisJpegCoefficientStoreTest
    LINK_LIBRARIES Qt6::Core Qt6::Test ${kritajpeg_LIBRARIES}
)
if(IOS)
    target_include_directories(KisJpegCoefficientStoreTest BEFORE PRIVATE ${KRITA_IOS_QT_JPEG_INCLUDE_DIR})
endif()
