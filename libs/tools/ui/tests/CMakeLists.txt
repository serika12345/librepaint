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

kis_add_test(KisPaintOpOptionSchemaContractTest.cpp LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test)
target_include_directories(KisPaintOpOptionSchemaContractTest PRIVATE
 ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..
 ${CMAKE_SOURCE_DIR}/libs/global ${CMAKE_BINARY_DIR}/libs/global
 ${CMAKE_SOURCE_DIR}/libs/image ${CMAKE_BINARY_DIR}/libs/image
 ${CMAKE_SOURCE_DIR}/libs/image/brushengine ${CMAKE_BINARY_DIR}/libs/image/brushengine
 ${CMAKE_SOURCE_DIR}/libs/resources ${CMAKE_BINARY_DIR}/libs/resources
 ${CMAKE_SOURCE_DIR}/libs/pigment ${CMAKE_BINARY_DIR}/libs/pigment
 ${CMAKE_SOURCE_DIR}/libs/application ${CMAKE_BINARY_DIR}/libs/application
 $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
 $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
 $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
 $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(KisPaintOpOptionSchemaContractTest PRIVATE kritatoolsui_EXPORTS kritaglobal_EXPORTS kritaimage_EXPORTS kritaresources_EXPORTS kritapigment_EXPORTS)

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
    KisCategorizedListModelSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisCategorizedListModelSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisCategorizedListModelSchemaContractTest PRIVATE
    kritatoolsui_EXPORTS
)

kis_add_test(
    KisCompositeOpSelectorSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisCompositeOpSelectorSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisCompositeOpSelectorSchemaContractTest PRIVATE
    kritatoolsui_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisSelectionOptionsSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSelectionOptionsSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisSelectionOptionsSchemaContractTest PRIVATE
    kritatoolsui_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisPaletteEditorSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test Boost::boost
)
target_include_directories(KisPaletteEditorSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisPaletteEditorSchemaContractTest PRIVATE
    kritatoolsui_EXPORTS
    kritapigment_EXPORTS
    kritaresources_EXPORTS
    kritaglobal_EXPORTS
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
