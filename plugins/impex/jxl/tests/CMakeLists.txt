kis_add_test(
    kis_jpegxl_test.cpp
    TEST_NAME kis_jpegxl_test
    LINK_LIBRARIES kritapaintingmetadata kritaapplicationui kritatestsdk
)

# File conversion and fill-layer capabilities are registered by these plugins.
add_dependencies(kis_jpegxl_test
    kritajxlimport kritajxlexport kritakraimport
    kritacolorgenerator kritapatterngenerator kritagradientgenerator)
