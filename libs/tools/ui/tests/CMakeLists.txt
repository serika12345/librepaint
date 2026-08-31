kis_add_tests(
    TestToolSettingsUiContract.cpp
    LINK_LIBRARIES kritatoolsui Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisCompositeOpsModelContractTest.cpp
    LINK_LIBRARIES
        kritatoolsuicategorizedmodelobjects
        kritatoolsuicompositeopsmodelobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_compile_definitions(KisCompositeOpsModelContractTest PRIVATE
    kritaglobal_EXPORTS
    kritatoolsui_EXPORTS
)

kis_add_test(
    KisPaintOpOptionsModelContractTest.cpp
    LINK_LIBRARIES
        kritatoolsuicategorizedmodelobjects
        kritatoolsuipaintopoptionsmodelobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisPaintOpOptionsModelContractTest PRIVATE
    ${kritatoolsuipaintopmodel_INCLUDE_DIRS}
)
target_compile_definitions(KisPaintOpOptionsModelContractTest PRIVATE
    kritaglobal_EXPORTS
    kritatoolsui_EXPORTS
)

kis_add_test(
    KisPaintOpsModelContractTest.cpp
    LINK_LIBRARIES
        kritatoolsuicategorizedmodelobjects
        kritatoolsuipaintopsmodelobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisPaintOpsModelContractTest PRIVATE
    kritatoolsui_EXPORTS
)

kis_add_test(
    KisCategoriesMapperContractTest.cpp
    LINK_LIBRARIES
        kritatoolsuicategorizedmodelobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisCategoriesMapperContractTest PRIVATE
    kritatoolsui_EXPORTS
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
