kis_add_tests(
    kis_coordinates_converter_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_tests(
    kis_prescaled_projection_contract_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_tests(
    kis_display_color_transform_test.cpp
    KisSurfaceColorSpaceWrapperTest.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)
