########### next target ###############


kis_add_tests(KisCurveOptionDataTest.cpp
    KisCurveOptionModelTest.cpp
    LINK_LIBRARIES kritaimage kritalibpaintop kritatestsdk)

kis_add_tests(kis_linked_pattern_manager_test.cpp
    LINK_LIBRARIES kritaimage kritalibpaintop kritatestsdk)

kis_add_test(
    KisPaintOpOptionUtilsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPaintOpOptionUtilsContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
