########### next target ###############

kis_add_test(test_animated_transform_parameters.cpp
    LINK_LIBRARIES kritatooltransform_static kritaapplicationui kritaimage kritatestsdk)

krita_add_broken_unit_test(TransformStrokeStrategyTest.cpp
    ../../../../sdk/tests/stroke_testing_utils.cpp
    TEST_NAME TransformStrokeStrategyTest
    LINK_LIBRARIES kritatooltransform_static kritaapplicationui kritaimage kritatestsdk)

kis_add_test(
    KisLiquifyPropertiesContractTest.cpp
    LINK_LIBRARIES
        kritatooltransformliquifypropertiesobjects
        kritaglobaldomutilsobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_compile_definitions(KisLiquifyPropertiesContractTest PRIVATE
    kritaglobal_EXPORTS
    kritatooltransform_EXPORTS
)
