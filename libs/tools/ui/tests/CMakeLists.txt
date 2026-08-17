kis_add_tests(
    TestToolSettingsUiContract.cpp
    LINK_LIBRARIES kritatoolsui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    kis_categorized_list_model_test.cpp
    ../../../ui/tests/modeltest.cpp
    TEST_NAME KisCategorizedListModelTest
    LINK_LIBRARIES kritatoolsui kritatestsdk
)

krita_add_broken_unit_test(
    kis_categories_mapper_test.cpp
    testing_categories_mapper.cpp
    TEST_NAME KisCategoriesMapperTest
    LINK_LIBRARIES kritatoolsui kritatestsdk
)
