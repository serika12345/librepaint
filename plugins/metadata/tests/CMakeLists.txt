if (WIN32)
    # The EXIF test is not yet supported on Windows.
    krita_add_broken_unit_test( kis_exif_test.cpp
        TEST_NAME KisExifTest
        LINK_LIBRARIES kritapaintingmetadata kritaapplicationui kritatestsdk
    )
else()
    kis_add_test(
        kis_exif_test.cpp
        TEST_NAME KisExifTest
        LINK_LIBRARIES kritapaintingmetadata kritaapplicationui kritatestsdk
    )
endif()
add_dependencies(KisExifTest kritaexif)
