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

kis_add_test(
    ToolTransformArgsGeometrySchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(ToolTransformArgsGeometrySchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/tools
    ${CMAKE_BINARY_DIR}/libs/tools
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(ToolTransformArgsGeometrySchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritatools_EXPORTS
    kritatooltransform_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    ToolTransformStateSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(ToolTransformStateSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/tools
    ${CMAKE_BINARY_DIR}/libs/tools
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(ToolTransformStateSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritatools_EXPORTS
    kritatooltransform_EXPORTS
    kritawidgetutils_EXPORTS
)
