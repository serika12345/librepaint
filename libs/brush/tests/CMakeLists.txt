########### next target ###############

kis_add_tests(
    kis_auto_brush_test.cpp
    kis_auto_brush_factory_test.cpp
    kis_gbr_brush_test.cpp
    kis_png_brush_test.cpp
    kis_boundary_test.cpp
    kis_imagepipe_brush_test.cpp
    TestAbrStorage.cpp
    KisBrushModelTest.cpp
    LINK_LIBRARIES kritaimage kritalibbrush kritatestsdk
    )

kis_add_test(
    KisDabShapeContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDabShapeContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
