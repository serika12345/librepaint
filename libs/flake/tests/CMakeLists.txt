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
    TestSnapStrategy.cpp
    TestPathShape.cpp
    TestControlPointMoveCommand.cpp
    TestPointTypeCommand.cpp
    TestPointRemoveCommand.cpp
    TestRemoveSubpathCommand.cpp
    TestSegmentTypeCommand.cpp
    TestKoDrag.cpp
    TestKoMarkerCollection.cpp
    TestSvgSavingContext.cpp
    TestSvgTextShape.cpp
    TestDependentShapes.cpp

    LINK_LIBRARIES kritaflake kritatestsdk kritaxmlserialization
    )

kis_add_test(
    TestShapeStrokeCommand.cpp
    LINK_LIBRARIES
        kritaflakeshapestrokecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(TestShapeStrokeCommand PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(TestShapeStrokeCommand PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeMoveCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapemovecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeMoveCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeMoveCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeTransformCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapetransformcommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeTransformCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeTransformCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeAlignCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapealigncommandobjects
        kritaflakeshapemovecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeAlignCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeAlignCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeDistributeCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapedistributecommandobjects
        kritaflakeshapemovecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeDistributeCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeDistributeCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    TestPathSegment.cpp
    LINK_LIBRARIES
        kritaflakepathsegmentobjects
        kritaflakepathpointobjects
        kritaglobalbeziercurvelengthobjects
        kritaglobalbezierutilsobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(TestPathSegment PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(TestPathSegment PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    TestXsimdPainting.cpp
    LINK_LIBRARIES
        kritaflakeclipmaskpainterobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestXsimdPainting PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(TestXsimdPainting PRIVATE kritaflake_EXPORTS)

kis_add_test(
    TestFontLibraryResourceUtils.cpp
    LINK_LIBRARIES
        Freetype::Freetype
        HarfBuzz::HarfBuzz
        Fontconfig::Fontconfig
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(TestFontLibraryResourceUtils PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
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
    KoSnapDataContractTest.cpp
    LINK_LIBRARIES
        kritaflakesnapdataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoSnapDataContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoSnapDataContractTest PRIVATE kritaflake_EXPORTS)

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
    KoCssTextUtilsContractTest.cpp
    LINK_LIBRARIES
        kritaflakecsstextutilsobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCssTextUtilsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoCssTextUtilsContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoSvgTextPropertyDataContractTest.cpp
    LINK_LIBRARIES
        kritaflakesvgtextpropertydataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoSvgTextPropertyDataContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoSvgTextPropertyDataContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoSvgTextPropertiesInterfaceContractTest.cpp
    LINK_LIBRARIES
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_sources(KoSvgTextPropertiesInterfaceContractTest PRIVATE
    ../text/KoSvgTextPropertiesInterface.h
)
target_include_directories(KoSvgTextPropertiesInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoSvgTextPropertiesInterfaceContractTest PRIVATE kritaflake_EXPORTS)

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
    KoShapeAnchorContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapeanchorobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoShapeAnchorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeAnchorContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoShapeBackgroundContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapebackgroundobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeBackgroundContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeBackgroundContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoShapeContainerModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapecontainermodelobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeContainerModelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeContainerModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoColorBackgroundContractTest.cpp
    LINK_LIBRARIES
        kritaflakecolorbackgroundobjects
        kritaflakeshapebackgroundobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoColorBackgroundContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoColorBackgroundContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoAbstractCanvasResourceInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritaflakeabstractcanvasresourceinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoAbstractCanvasResourceInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoAbstractCanvasResourceInterfaceContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoDeferredShapeFactoryBaseContractTest.cpp
    LINK_LIBRARIES
        kritaflakedeferredshapefactorybaseobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoDeferredShapeFactoryBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoDeferredShapeFactoryBaseContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoShapeConfigWidgetBaseContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapeconfigwidgetbaseobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KoShapeConfigWidgetBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeConfigWidgetBaseContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoDockFactoryBaseContractTest.cpp
    LINK_LIBRARIES
        kritaflakedockfactorybaseobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoDockFactoryBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoDockFactoryBaseContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoDockRegistryContractTest.cpp
    LINK_LIBRARIES
        kritaflakedockregistryobjects
        kritaflakedockfactorybaseobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KoDockRegistryContractTest PRIVATE
    ../../koplugin/KoPluginLoader.h
)
target_include_directories(KoDockRegistryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/koplugin
    ${CMAKE_BINARY_DIR}/libs/koplugin
)
target_compile_definitions(KoDockRegistryContractTest PRIVATE
    kritaflake_EXPORTS
    kritaplugin_EXPORTS
)

kis_add_test(
    KoShapeStrokeModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapestrokemodelobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeStrokeModelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoShapeStrokeModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoInteractionStrategyFactoryContractTest.cpp
    LINK_LIBRARIES
        kritaflakeinteractionstrategyfactoryobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoInteractionStrategyFactoryContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoInteractionStrategyFactoryContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoSelectedShapesProxyContractTest.cpp
    LINK_LIBRARIES
        kritaflakeselectedshapesproxyobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoSelectedShapesProxyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoSelectedShapesProxyContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoActiveCanvasResourceDependencyContractTest.cpp
    LINK_LIBRARIES
        kritaflakeactivecanvasresourcedependencyobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoActiveCanvasResourceDependencyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoActiveCanvasResourceDependencyContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoActiveCanvasResourceDependencyKoResourceContractTest.cpp
    LINK_LIBRARIES
        kritaflakeactivecanvasresourcedependencyobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoActiveCanvasResourceDependencyKoResourceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoActiveCanvasResourceDependencyKoResourceContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoDerivedResourceConverterContractTest.cpp
    LINK_LIBRARIES
        kritaflakederivedresourceconverterobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoDerivedResourceConverterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoDerivedResourceConverterContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoResourceUpdateMediatorContractTest.cpp
    LINK_LIBRARIES
        kritaflakeresourceupdatemediatorobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoResourceUpdateMediatorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoResourceUpdateMediatorContractTest PRIVATE kritaflake_EXPORTS)

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
    CssLengthPercentageModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakecsslengthpercentagemodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(CssLengthPercentageModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(CssLengthPercentageModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    FontVariantNumericModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakefontvariantnumericmodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(FontVariantNumericModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(FontVariantNumericModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    FontVariantLigaturesModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakefontvariantligaturesmodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(FontVariantLigaturesModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(FontVariantLigaturesModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    FontVariantEastAsianModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakefontvarianteastasianmodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(FontVariantEastAsianModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(FontVariantEastAsianModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    TextTransformModelContractTest.cpp
    LINK_LIBRARIES
        kritaflaketexttransformmodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(TextTransformModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(TextTransformModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    TabSizeModelContractTest.cpp
    LINK_LIBRARIES
        kritaflaketabsizemodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(TabSizeModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(TabSizeModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    LineHeightModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakelineheightmodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(LineHeightModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(LineHeightModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    TextIndentModelContractTest.cpp
    LINK_LIBRARIES
        kritaflaketextindentmodelobjects
        kritaflakecsslengthpercentagemodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(TextIndentModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(TextIndentModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    CssFontStyleModelContractTest.cpp
    LINK_LIBRARIES
        kritaflakecssfontstylemodelobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
target_include_directories(CssFontStyleModelContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(CssFontStyleModelContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    KoCSSFontInfoContractTest.cpp
    LINK_LIBRARIES
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoCSSFontInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoCSSFontInfoContractTest PRIVATE kritaflake_EXPORTS)

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
    KoUnicodeBlockDataContractTest.cpp
    LINK_LIBRARIES
        kritaflakeunicodeblockdataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoUnicodeBlockDataContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoUnicodeBlockDataContractTest PRIVATE kritaflake_EXPORTS)

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
    KoPathPointContractTest.cpp
    LINK_LIBRARIES
        kritaflakepathpointobjects
        kritaglobalhandlepainterhelperobjects
        kritaglobaldecomposedmatrixobjects
        kritaglobalhandlestyleobjects
        kritaglobalpaintingtweaksobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
)
target_include_directories(KoPathPointContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KoPathPointContractTest PRIVATE
    kritaflake_EXPORTS
    kritaglobal_EXPORTS
)

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
    SvgMeshArrayContractTest.cpp
    LINK_LIBRARIES
        kritaflakesvgmesharrayobjects
        kritaflakesvgmeshpatchobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(SvgMeshArrayContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(SvgMeshArrayContractTest PRIVATE kritaflake_EXPORTS)

kis_add_test(
    SvgMeshGradientContractTest.cpp
    LINK_LIBRARIES
        kritaflakesvgmeshgradientobjects
        kritaflakesvgmesharrayobjects
        kritaflakesvgmeshpatchobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(SvgMeshGradientContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(SvgMeshGradientContractTest PRIVATE kritaflake_EXPORTS)

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

kis_add_test(
    KoShapeRenameCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshaperenamecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeRenameCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeRenameCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeKeepAspectRatioCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapekeepaspectratiocommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeKeepAspectRatioCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeKeepAspectRatioCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoPathReverseCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakepathreversecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoPathReverseCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoPathReverseCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoSubpathRemoveCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakesubpathremovecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoSubpathRemoveCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoSubpathRemoveCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeSizeCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapesizecommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeSizeCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeSizeCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeShearCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapeshearcommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeShearCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeShearCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeTransparencyCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapetransparencycommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeTransparencyCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeTransparencyCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapePaintOrderCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapepaintordercommandobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapePaintOrderCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapePaintOrderCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoShapeBackgroundCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakeshapebackgroundcommandobjects
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoShapeBackgroundCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoShapeBackgroundCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoKeepShapesSelectedCommandContractTest.cpp
    LINK_LIBRARIES
        kritaflakekeepshapesselectedcommandobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KoKeepShapesSelectedCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../KoSelectedShapesProxy.h
)
target_include_directories(KoKeepShapesSelectedCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoKeepShapesSelectedCommandContractTest PRIVATE
    kritaflake_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KoAddRemoveShapeCommandsContractTest.cpp
    LINK_LIBRARIES
        kritaflakeaddremoveshapecommandobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KoAddRemoveShapeCommandsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KoAddRemoveShapeCommandsContractTest PRIVATE
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
