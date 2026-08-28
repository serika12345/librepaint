kis_add_tests(
    kis_coordinates_converter_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_test(
    KisInputActionGroupContractTest.cpp
    LINK_LIBRARIES
        kritacanvasinputactiongroupobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisInputActionGroupContractTest PRIVATE kritacanvas_EXPORTS)

kis_add_tests(
    kis_prescaled_projection_contract_test.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_tests(
    kis_display_color_transform_test.cpp
    KisSurfaceColorSpaceWrapperTest.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)

kis_add_tests(
    kis_animation_frame_cache_index_test.cpp
    kis_frame_cache_store_test.cpp
    KisFrameSerializerTest.cpp
    LINK_LIBRARIES kritacanvas kritatestsdk
)
