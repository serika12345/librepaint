kis_add_tests(
    TestPosition.cpp
    TestSelection.cpp
    TestPathTool.cpp
    TestShapeAt.cpp
    TestShapePainting.cpp
    TestKoShapeFactory.cpp
    TestShapeContainer.cpp
    TestShapeGroupCommand.cpp
    TestShapeReorderCommand.cpp
    TestResourceManager.cpp
    TestShapeBackgroundCommand.cpp
    TestShapeStrokeCommand.cpp
    TestSnapStrategy.cpp
    TestPathShape.cpp
    TestControlPointMoveCommand.cpp
    TestPointTypeCommand.cpp
    TestPointRemoveCommand.cpp
    TestRemoveSubpathCommand.cpp
    TestPathSegment.cpp
    TestSegmentTypeCommand.cpp
    TestKoDrag.cpp
    TestKoMarkerCollection.cpp
    TestSvgSavingContext.cpp
    TestXsimdPainting.cpp
    TestFontLibraryResourceUtils.cpp
    TestSvgTextShape.cpp
    TestDependentShapes.cpp

    LINK_LIBRARIES kritaflake kritatestsdk kritaxmlserialization
    )

kis_add_test(
    TestInputDevice.cpp
    LINK_LIBRARIES
        kritaflakeinputdeviceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestInputDevice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(TestInputDevice PRIVATE kritaflake_EXPORTS)

kis_add_test(
    TestSvgParser.cpp
    TEST_NAME TestSvgParser
    LINK_LIBRARIES kritaflake kritatestsdk
    )

kis_add_test(
    TestSvgParser.cpp
    TEST_NAME TestSvgParserCloned
    LINK_LIBRARIES kritaflake kritatestsdk
    )
target_compile_definitions(TestSvgParserCloned PRIVATE USE_CLONED_SHAPES)

kis_add_test(
    TestSvgParser.cpp
    TEST_NAME TestSvgParserRoundTrip
    LINK_LIBRARIES kritaflake kritatestsdk
    )
target_compile_definitions(TestSvgParserRoundTrip PRIVATE USE_ROUND_TRIP)

kis_add_test(
    KoFlakeTypesContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoFlakeTypesContractTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

kis_add_test(
    KoShapeAnchorEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeAnchorEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeAnchorEnumContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoShapeEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeEnumContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoSvgTextEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoSvgTextEnumContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoSvgTextEnumContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoWritingSystemUtilsContractTest.cpp
    LINK_LIBRARIES
        kritaflakewritingsystemutilsobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoWritingSystemUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoWritingSystemUtilsContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoOpenTypeFeatureInfoFactoryContractTest.cpp
    LINK_LIBRARIES
        kritaflakeopentypefeatureinfoobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoOpenTypeFeatureInfoFactoryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoOpenTypeFeatureInfoFactoryContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoInsetsContractTest.cpp
    LINK_LIBRARIES
        kritaflakeinsetsobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KoZoomStateContractTest.cpp
    LINK_LIBRARIES
        kritaflakezoomstateobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoZoomStateContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoZoomStateContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoViewTransformStillPointContractTest.cpp
    LINK_LIBRARIES
        kritaflakeviewtransformstillpointobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoViewTransformStillPointContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoViewTransformStillPointContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoViewConverterContractTest.cpp
    LINK_LIBRARIES
        kritaflakeviewconverterobjects
        kritaflakeviewtransformstillpointobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoViewConverterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoViewConverterContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoPathPointDataContractTest.cpp
    LINK_LIBRARIES
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoPathPointDataContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoPathPointDataContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoFlakeUtilsContractTest.cpp
    LINK_LIBRARIES
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoFlakeUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoFlakeUtilsContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    SvgTransformParserContractTest.cpp
    LINK_LIBRARIES
        kritaflakesvgtransformparserobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(SvgTransformParserContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(SvgTransformParserContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    SvgMeshPatchContractTest.cpp
    LINK_LIBRARIES
        kritaflakesvgmeshpatchobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(SvgMeshPatchContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(SvgMeshPatchContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoBehaviorInterfacesContractTest.cpp
    LINK_LIBRARIES
        kritaflakebehaviorinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoBehaviorInterfacesContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoGradientHelperContractTest.cpp
    LINK_LIBRARIES
        kritaflakegradienthelperobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KoGradientHelperContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoCurveFitContractTest.cpp
    LINK_LIBRARIES
        kritaflakecurvefitobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCurveFitContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoCurveFitContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    FlakeDebugContractTest.cpp
    LINK_LIBRARIES
        kritaflakedebugobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(FlakeDebugContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(FlakeDebugContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoPathFillRuleCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakepathfillrulecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoPathFillRuleCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoPathFillRuleCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

############## broken tests ###############

kis_add_test(TestPointMergeCommand.cpp
    TEST_NAME TestPointMergeCommand
    LINK_LIBRARIES kritaflake kritatestsdk
    )

krita_add_broken_unit_test( TestSvgText.cpp
    TEST_NAME TestSvgText
    LINK_LIBRARIES kritaflake kritatestsdk
    )

krita_add_broken_unit_test( TestSvgText.cpp
    TEST_NAME TestSvgTextCloned
    LINK_LIBRARIES kritaflake kritatestsdk
    )
target_compile_definitions(TestSvgTextCloned PRIVATE USE_CLONED_SHAPES)

krita_add_broken_unit_test( TestSvgText.cpp
    TEST_NAME TestSvgTextRoundTrip
    LINK_LIBRARIES kritaflake kritatestsdk
    )
target_compile_definitions(TestSvgTextRoundTrip PRIVATE USE_ROUND_TRIP)

if (APPLE)
    set_property(TARGET TestKoMarkerCollection PROPERTY MACOSX_BUNDLE ON)
    set_property(TARGET TestSvgParser PROPERTY MACOSX_BUNDLE ON)
    set_property(TARGET TestSvgParserCloned PROPERTY MACOSX_BUNDLE ON)
    set_property(TARGET TestSvgParserRoundTrip PROPERTY MACOSX_BUNDLE ON)
endif()
