project(kritaflake)

include_directories(
    ${CMAKE_SOURCE_DIR}/libs/flake/commands
    ${CMAKE_SOURCE_DIR}/libs/flake/tools
    ${CMAKE_SOURCE_DIR}/libs/flake/svg
    ${CMAKE_SOURCE_DIR}/libs/flake/text
    ${CMAKE_SOURCE_DIR}/libs/flake/xsimd
    ${CMAKE_SOURCE_DIR}/libs/flake/resources
    ${CMAKE_BINARY_DIR}/libs/flake
)

add_library(kritaflakeinsetsobjects OBJECT
    KoInsets.cpp
)
target_compile_definitions(kritaflakeinsetsobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeinsetsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeinsetsobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeinsetsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapeanchorobjects OBJECT
    KoShapeAnchor.cpp
)
target_compile_definitions(kritaflakeshapeanchorobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeshapeanchorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeshapeanchorobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshapeanchorobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapebackgroundobjects OBJECT
    KoShapeBackground.cpp
)
target_compile_definitions(kritaflakeshapebackgroundobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeshapebackgroundobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeshapebackgroundobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeshapebackgroundobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakecolorbackgroundobjects OBJECT
    KoColorBackground.cpp
)
target_compile_definitions(kritaflakecolorbackgroundobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakecolorbackgroundobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakecolorbackgroundobjects
    PUBLIC
        kritaflakeshapebackgroundobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaflakecolorbackgroundobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapebackgroundcommandobjects OBJECT
    commands/KoShapeBackgroundCommand.cpp
)
target_compile_definitions(kritaflakeshapebackgroundcommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakeshapebackgroundcommandobjects PRIVATE KRITAFLAKE_SHAPE_BACKGROUND_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakeshapebackgroundcommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakeshapebackgroundcommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshapebackgroundcommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeabstractcanvasresourceinterfaceobjects OBJECT
    KoAbstractCanvasResourceInterface.cpp
)
target_compile_definitions(kritaflakeabstractcanvasresourceinterfaceobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeabstractcanvasresourceinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeabstractcanvasresourceinterfaceobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeabstractcanvasresourceinterfaceobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakedeferredshapefactorybaseobjects OBJECT
    KoDeferredShapeFactoryBase.cpp
)
target_compile_definitions(kritaflakedeferredshapefactorybaseobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakedeferredshapefactorybaseobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakedeferredshapefactorybaseobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakedeferredshapefactorybaseobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapeconfigwidgetbaseobjects OBJECT
    KoShapeConfigWidgetBase.cpp
)
target_compile_definitions(kritaflakeshapeconfigwidgetbaseobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeshapeconfigwidgetbaseobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeshapeconfigwidgetbaseobjects PUBLIC Qt${QT_MAJOR_VERSION}::Widgets)
set_target_properties(kritaflakeshapeconfigwidgetbaseobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakedockfactorybaseobjects OBJECT
    KoDockFactoryBase.cpp
)
target_compile_definitions(kritaflakedockfactorybaseobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakedockfactorybaseobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakedockfactorybaseobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakedockfactorybaseobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapestrokemodelobjects OBJECT
    KoShapeStrokeModel.cpp
)
target_compile_definitions(kritaflakeshapestrokemodelobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeshapestrokemodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeshapestrokemodelobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeshapestrokemodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeinteractionstrategyfactoryobjects OBJECT
    tools/KoInteractionStrategyFactory.cpp
)
target_compile_definitions(kritaflakeinteractionstrategyfactoryobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeinteractionstrategyfactoryobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tools>
)
target_link_libraries(kritaflakeinteractionstrategyfactoryobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeinteractionstrategyfactoryobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeselectedshapesproxyobjects OBJECT
    KoSelectedShapesProxy.cpp
)
target_compile_definitions(kritaflakeselectedshapesproxyobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeselectedshapesproxyobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeselectedshapesproxyobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeselectedshapesproxyobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeactivecanvasresourcedependencyobjects OBJECT
    KoActiveCanvasResourceDependency.cpp
)
target_compile_definitions(kritaflakeactivecanvasresourcedependencyobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeactivecanvasresourcedependencyobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeactivecanvasresourcedependencyobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeactivecanvasresourcedependencyobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakederivedresourceconverterobjects OBJECT
    KoDerivedResourceConverter.cpp
)
target_compile_definitions(kritaflakederivedresourceconverterobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakederivedresourceconverterobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakederivedresourceconverterobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakederivedresourceconverterobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeresourceupdatemediatorobjects OBJECT
    KoResourceUpdateMediator.cpp
)
target_compile_definitions(kritaflakeresourceupdatemediatorobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeresourceupdatemediatorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeresourceupdatemediatorobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeresourceupdatemediatorobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakezoomstateobjects OBJECT
    KoZoomState.cpp
)
target_compile_definitions(kritaflakezoomstateobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakezoomstateobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakezoomstateobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritaflakezoomstateobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeviewtransformstillpointobjects OBJECT
    KoViewTransformStillPoint.cpp
)
target_compile_definitions(kritaflakeviewtransformstillpointobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeviewtransformstillpointobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeviewtransformstillpointobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakeviewtransformstillpointobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeviewconverterobjects OBJECT
    KoViewConverter.cpp
)
target_compile_definitions(kritaflakeviewconverterobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeviewconverterobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeviewconverterobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaflakeviewconverterobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeinputdeviceobjects OBJECT
    KoInputDevice.cpp
)
target_compile_definitions(kritaflakeinputdeviceobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeinputdeviceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeinputdeviceobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaflakeinputdeviceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakesvgtransformparserobjects OBJECT
    svg/parsers/SvgTransformParser.cpp
)
target_compile_definitions(kritaflakesvgtransformparserobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakesvgtransformparserobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakesvgtransformparserobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaflakesvgtransformparserobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakesvgmeshpatchobjects OBJECT
    svg/SvgMeshPatch.cpp
)
target_compile_definitions(kritaflakesvgmeshpatchobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakesvgmeshpatchobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritaflakesvgmeshpatchobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaflakesvgmeshpatchobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakesvgmesharrayobjects OBJECT
    svg/SvgMeshArray.cpp
)
target_compile_definitions(kritaflakesvgmesharrayobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakesvgmesharrayobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritaflakesvgmesharrayobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaflakesvgmesharrayobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakebehaviorinterfaceobjects OBJECT
    KoCanvasSupervisor.cpp
    KoLoadingShapeUpdater.cpp
    KoSharedLoadingData.cpp
    KoSharedSavingData.cpp
    KoShapeUserData.cpp
    KoToolSelection.cpp
)

add_library(kritaflakegradienthelperobjects OBJECT
    KoGradientHelper.cpp
)
target_compile_definitions(kritaflakegradienthelperobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakegradienthelperobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakegradienthelperobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaflakegradienthelperobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakecurvefitobjects OBJECT
    KoCurveFit.cpp
)
target_compile_definitions(kritaflakecurvefitobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakecurvefitobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakecurvefitobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaflakecurvefitobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakecurvefitpathshapeobjects OBJECT
    KoCurveFitPathShapeWriter.cpp
)
target_compile_definitions(kritaflakecurvefitpathshapeobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakecurvefitpathshapeobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakecurvefitpathshapeobjects PUBLIC
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakecurvefitpathshapeobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakedebugobjects OBJECT
    FlakeDebug.cpp
)
target_compile_definitions(kritaflakedebugobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakedebugobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakedebugobjects PUBLIC Qt${QT_MAJOR_VERSION}::Core)
set_target_properties(kritaflakedebugobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakewritingsystemutilsobjects OBJECT
    text/KoWritingSystemUtils.cpp
)
target_compile_definitions(kritaflakewritingsystemutilsobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakewritingsystemutilsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakewritingsystemutilsobjects PUBLIC Qt${QT_MAJOR_VERSION}::Gui)
set_target_properties(kritaflakewritingsystemutilsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeopentypefeatureinfoobjects OBJECT
    text/KoOpenTypeFeatureInfoFactory.cpp
)
target_compile_definitions(kritaflakeopentypefeatureinfoobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeopentypefeatureinfoobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeopentypefeatureinfoobjects
    PUBLIC
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritaflakeopentypefeatureinfoobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeunicodeblockdataobjects OBJECT
    text/data/KoUnicodeBlockData.cpp
)
target_compile_definitions(kritaflakeunicodeblockdataobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakeunicodeblockdataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakeunicodeblockdataobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritaflakeunicodeblockdataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakecsslengthpercentagemodelobjects OBJECT
    text/lager/CssLengthPercentageModel.cpp
)
target_compile_definitions(kritaflakecsslengthpercentagemodelobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakecsslengthpercentagemodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/text>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/text/lager>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritaflakecsslengthpercentagemodelobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
set_target_properties(kritaflakecsslengthpercentagemodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakepathfillrulecommandobjects OBJECT
    commands/KoPathFillRuleCommand.cpp
)
target_compile_definitions(kritaflakepathfillrulecommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakepathfillrulecommandobjects PRIVATE KRITAFLAKE_PATH_FILL_RULE_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakepathfillrulecommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakepathfillrulecommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakepathfillrulecommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshaperenamecommandobjects OBJECT
    commands/KoShapeRenameCommand.cpp
)
target_compile_definitions(kritaflakeshaperenamecommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakeshaperenamecommandobjects PRIVATE KRITAFLAKE_SHAPE_RENAME_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakeshaperenamecommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakeshaperenamecommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshaperenamecommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapekeepaspectratiocommandobjects OBJECT
    commands/KoShapeKeepAspectRatioCommand.cpp
)
target_compile_definitions(kritaflakeshapekeepaspectratiocommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakeshapekeepaspectratiocommandobjects PRIVATE KRITAFLAKE_SHAPE_KEEP_ASPECT_RATIO_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakeshapekeepaspectratiocommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakeshapekeepaspectratiocommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshapekeepaspectratiocommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakepathreversecommandobjects OBJECT
    commands/KoPathReverseCommand.cpp
)
target_compile_definitions(kritaflakepathreversecommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakepathreversecommandobjects PRIVATE KRITAFLAKE_PATH_REVERSE_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakepathreversecommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakepathreversecommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakepathreversecommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapesizecommandobjects OBJECT
    commands/KoShapeSizeCommand.cpp
)
target_compile_definitions(kritaflakeshapesizecommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakeshapesizecommandobjects PRIVATE KRITAFLAKE_SHAPE_SIZE_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakeshapesizecommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakeshapesizecommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshapesizecommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapeshearcommandobjects OBJECT
    commands/KoShapeShearCommand.cpp
)
target_compile_definitions(kritaflakeshapeshearcommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakeshapeshearcommandobjects PRIVATE KRITAFLAKE_SHAPE_SHEAR_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakeshapeshearcommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakeshapeshearcommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshapeshearcommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakeshapetransparencycommandobjects OBJECT
    commands/KoShapeTransparencyCommand.cpp
)
target_compile_definitions(kritaflakeshapetransparencycommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakeshapetransparencycommandobjects PRIVATE KRITAFLAKE_SHAPE_TRANSPARENCY_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakeshapetransparencycommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakeshapetransparencycommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakeshapetransparencycommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritaflakesubpathremovecommandobjects OBJECT
    commands/KoSubpathRemoveCommand.cpp
)
target_compile_definitions(kritaflakesubpathremovecommandobjects PRIVATE kritaflake_EXPORTS)
if(BUILD_TESTING)
    target_compile_definitions(kritaflakesubpathremovecommandobjects PRIVATE KRITAFLAKE_SUBPATH_REMOVE_COMMAND_CONTRACT_TESTING)
endif()
target_include_directories(kritaflakesubpathremovecommandobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/painting/undo>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/painting/undo>
)
target_link_libraries(kritaflakesubpathremovecommandobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
)
set_target_properties(kritaflakesubpathremovecommandobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(kritaflakebehaviorinterfaceobjects PRIVATE kritaflake_EXPORTS)
target_include_directories(kritaflakebehaviorinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritaflakebehaviorinterfaceobjects PUBLIC
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritaflakebehaviorinterfaceobjects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(styles)
add_subdirectory(tests)
add_subdirectory(resources/tests)

set(DISABLE_CLIP_MASK_PAINTER_ON_MACOS FALSE)
if (APPLE AND NOT IOS)
    set(DISABLE_CLIP_MASK_PAINTER_ON_MACOS TRUE)
endif()

# generate krita_xsimd_macos_workaround.h
configure_file(krita_xsimd_macos_workaround.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/krita_xsimd_macos_workaround.h)

if(NOT DISABLE_CLIP_MASK_PAINTER_ON_MACOS)
    ko_compile_for_all_implementations(__per_arch_alpha_applicator_factory_objs xsimd/KoClipMaskApplicatorFactoryImpl.cpp)
else()
    set(__per_arch_alpha_applicator_factory_objs xsimd/KoClipMaskApplicatorFactoryImpl.cpp)
endif()

set(kritaflake_SRCS
    KoFlake.cpp
    KoCanvasBase.cpp
    KoResourceManager_p.cpp
    KoCanvasResourceProvider.cpp
    KoDocumentResourceManager.cpp
    KoCanvasObserverBase.cpp
    KoDockRegistry.cpp
    KoPathShape.cpp
    KoPathPoint.cpp
    KoPathSegment.cpp
    KoSelection.cpp
    KoSelectedShapesProxySimple.cpp
    KoShape.cpp
    KoShapeBulkActionLock.cpp
    KoShapeControllerBase.cpp
    KoShapeContainer.cpp
    KoShapeContainerModel.cpp
    KoShapeGroup.cpp
    KoShapeManager.cpp
    KoMarker.cpp
    KoMarkerCollection.cpp
    KoToolBase.cpp
    KoCanvasController.cpp
    KoZoomMode.cpp
    KoShapeRegistry.cpp
    KoToolFactoryBase.cpp
    KoPathShapeFactory.cpp
    KoBasicShapeFactory.cpp
    KoShapeFactoryBase.cpp
    KoParameterShape.cpp
    KoPointerEvent.cpp
    KoShapeController.cpp
    KoShapeLayer.cpp
    KoToolManager_p.cpp
    KoToolManager.cpp
    KoToolRegistry.cpp
    KoToolProxy.cpp
    KoShapeSavingContext.cpp
    KoShapeLoadingContext.cpp
    KoPathShapeLoader.cpp
    KoShapeStroke.cpp
    KoGradientBackground.cpp
    KoMeshGradientBackground.cpp
    KoHatchBackground.cpp
    KoPatternBackground.cpp
    KoVectorPatternBackground.cpp
    KoShapeFillWrapper.cpp
    KoShapeFillResourceConnector.cpp
    KoDrag.cpp
    KoSvgPaste.cpp
    KoSnapGuide.cpp
    KoSnapProxy.cpp
    KoSnapStrategy.cpp
    KoSnapData.cpp
    KoClipMaskApplicatorBase.cpp
    ${__per_arch_alpha_applicator_factory_objs}
    KoClipMaskPainter.cpp
    KoClipPath.cpp
    KoClipMask.cpp
    commands/KoShapeGroupCommand.cpp
    commands/KoShapeAlignCommand.cpp
    commands/KoShapeCreateCommand.cpp
    commands/KoShapeDeleteCommand.cpp
    commands/KoShapeDistributeCommand.cpp
    commands/KoShapeLockCommand.cpp
    commands/KoShapeMoveCommand.cpp
    commands/KoShapeResizeCommand.cpp
    commands/KoShapeStrokeCommand.cpp
    commands/KoShapeUngroupCommand.cpp
    commands/KoShapeReorderCommand.cpp
    commands/KoPathBaseCommand.cpp
    commands/KoPathPointMoveCommand.cpp
    commands/KoPathControlPointMoveCommand.cpp
    commands/KoPathPointTypeCommand.cpp
    commands/KoPathPointRemoveCommand.cpp
    commands/KoPathPointInsertCommand.cpp
    commands/KoPathSegmentBreakCommand.cpp
    commands/KoPathBreakAtPointCommand.cpp
    commands/KoPathSegmentTypeCommand.cpp
    commands/KoPathCombineCommand.cpp
    commands/KoSubpathJoinCommand.cpp
    commands/KoParameterHandleMoveCommand.cpp
    commands/KoParameterToPathCommand.cpp
    commands/KoShapeTransformCommand.cpp
    commands/KoPathPointMergeCommand.cpp
    commands/KoShapeClipCommand.cpp
    commands/KoShapeUnclipCommand.cpp
    commands/KoPathShapeMarkerCommand.cpp
    commands/KoMultiPathPointMergeCommand.cpp
    commands/KoMultiPathPointJoinCommand.cpp
    commands/KoKeepShapesSelectedCommand.cpp
    commands/KoPathMergeUtils.cpp
    commands/KoAddRemoveShapeCommands.cpp
    commands/KoShapePaintOrderCommand.cpp
    commands/KoShapeMergeTextPropertiesCommand.cpp
    commands/KoSvgConvertTextTypeCommand.cpp
    commands/KoSvgTextAddRemoveShapeCommands.cpp
    commands/KoSvgTextReorderShapeInsideCommand.cpp
    commands/KoSvgTextFlipShapeContourTypeCommand.cpp
    commands/KoSvgTextPathInfoChangeCommand.cpp

    html/HtmlSavingContext.cpp
    html/HtmlWriter.cpp

    tools/KoPathToolFactory.cpp
    tools/KoPathTool.cpp
    tools/KoPathToolSelection.cpp
    tools/KoPathToolHandle.cpp
    tools/PathToolOptionWidget.cpp
    tools/KoPathPointRubberSelectStrategy.cpp
    tools/KoPathPointMoveStrategy.cpp
    tools/KoPathControlPointMoveStrategy.cpp
    tools/KoParameterChangeStrategy.cpp
    tools/KoZoomTool.cpp
    tools/KoZoomToolFactory.cpp
    tools/KoZoomToolWidget.cpp
    tools/KoZoomStrategy.cpp
    tools/KoInteractionTool.cpp
    tools/KoInteractionStrategy.cpp
    tools/KoShapeRubberSelectStrategy.cpp
    tools/KoPathSegmentChangeStrategy.cpp

    svg/KoShapePainter.cpp
    svg/SvgUtil.cpp
    svg/SvgGraphicContext.cpp
    svg/SvgSavingContext.cpp
    svg/SvgWriter.cpp
    svg/SvgStyleWriter.cpp
    svg/SvgShape.cpp
    svg/SvgParser.cpp
    svg/SvgStyleParser.cpp
    svg/SvgGradientHelper.cpp
    svg/SvgCssHelper.cpp
    svg/SvgClipPathHelper.cpp
    svg/SvgLoadingContext.cpp
    svg/SvgMeshGradient.cpp

    text/KisTofuGlyph.cpp
    text/KoSvgText.cpp
    text/KoSvgTextProperties.cpp
    text/KoSvgTextContentElement.cpp
    text/KoSvgTextShape.cpp
    text/KoSvgTextShapeWordNavigation.cpp
    text/KoSvgTextLoader.cpp
    text/KoSvgTextShape_p_glyphs.cpp
    text/KoSvgTextShape_p_layout.cpp
    text/KoSvgTextShape_p_output.cpp
    text/KoSvgTextShapeLayoutFunc_inShape.cpp
    text/KoSvgTextShapeLayoutFunc_lines.cpp
    text/KoSvgTextShapeMarkupConverter.cpp
    text/KoFontRegistry.cpp
    text/KoCssTextUtils.cpp
    text/KoPolygonUtils.cpp
    text/KoSvgTextPropertyData.cpp
    text/KoSvgTextPropertiesInterface.h
    text/lager/KoSvgTextPropertiesModel.cpp
    text/lager/LineHeightModel.cpp
    text/lager/TextIndentModel.cpp
    text/lager/TabSizeModel.cpp
    text/lager/TextTransformModel.cpp
    text/lager/CssFontStyleModel.cpp
    text/lager/FontVariantLigaturesModel.cpp
    text/lager/FontVariantNumericModel.cpp
    text/lager/FontVariantEastAsianModel.cpp
    text/KoFFWWSConverter.cpp
    text/KoFontChangeTracker.cpp
    text/KoFontGlyphModel.cpp
    text/KoSvgTextShapeOutlineHelper.cpp

    resources/KoSvgSymbolCollectionResource.cpp
    resources/KoGamutMask.cpp
    resources/KoFontFamily.cpp
    resources/KoFontStorage.cpp
    resources/KoCssStylePreset.cpp

    tests/MockShapes.cpp

    $<TARGET_OBJECTS:kritaflakeinsetsobjects>
    $<TARGET_OBJECTS:kritaflakeshapeanchorobjects>
    $<TARGET_OBJECTS:kritaflakeshapebackgroundobjects>
    $<TARGET_OBJECTS:kritaflakecolorbackgroundobjects>
    $<TARGET_OBJECTS:kritaflakeshapebackgroundcommandobjects>
    $<TARGET_OBJECTS:kritaflakeabstractcanvasresourceinterfaceobjects>
    $<TARGET_OBJECTS:kritaflakedeferredshapefactorybaseobjects>
    $<TARGET_OBJECTS:kritaflakeshapeconfigwidgetbaseobjects>
    $<TARGET_OBJECTS:kritaflakedockfactorybaseobjects>
    $<TARGET_OBJECTS:kritaflakeshapestrokemodelobjects>
    $<TARGET_OBJECTS:kritaflakeinteractionstrategyfactoryobjects>
    $<TARGET_OBJECTS:kritaflakeselectedshapesproxyobjects>
    $<TARGET_OBJECTS:kritaflakeactivecanvasresourcedependencyobjects>
    $<TARGET_OBJECTS:kritaflakederivedresourceconverterobjects>
    $<TARGET_OBJECTS:kritaflakeresourceupdatemediatorobjects>
    $<TARGET_OBJECTS:kritaflakezoomstateobjects>
    $<TARGET_OBJECTS:kritaflakeviewtransformstillpointobjects>
    $<TARGET_OBJECTS:kritaflakeviewconverterobjects>
    $<TARGET_OBJECTS:kritaflakeinputdeviceobjects>
    $<TARGET_OBJECTS:kritaflakesvgtransformparserobjects>
    $<TARGET_OBJECTS:kritaflakesvgmeshpatchobjects>
    $<TARGET_OBJECTS:kritaflakesvgmesharrayobjects>
    $<TARGET_OBJECTS:kritaflakebehaviorinterfaceobjects>
    $<TARGET_OBJECTS:kritaflakegradienthelperobjects>
    $<TARGET_OBJECTS:kritaflakecurvefitobjects>
    $<TARGET_OBJECTS:kritaflakecurvefitpathshapeobjects>
    $<TARGET_OBJECTS:kritaflakedebugobjects>
    $<TARGET_OBJECTS:kritaflakeopentypefeatureinfoobjects>
    $<TARGET_OBJECTS:kritaflakeunicodeblockdataobjects>
    $<TARGET_OBJECTS:kritaflakecsslengthpercentagemodelobjects>
    $<TARGET_OBJECTS:kritaflakewritingsystemutilsobjects>
    $<TARGET_OBJECTS:kritaflakepathfillrulecommandobjects>
    $<TARGET_OBJECTS:kritaflakeshaperenamecommandobjects>
    $<TARGET_OBJECTS:kritaflakeshapekeepaspectratiocommandobjects>
    $<TARGET_OBJECTS:kritaflakepathreversecommandobjects>
    $<TARGET_OBJECTS:kritaflakeshapesizecommandobjects>
    $<TARGET_OBJECTS:kritaflakeshapeshearcommandobjects>
    $<TARGET_OBJECTS:kritaflakeshapetransparencycommandobjects>
    $<TARGET_OBJECTS:kritaflakesubpathremovecommandobjects>

)

if (KSeExpr_FOUND)
    set(kritaflake_SRCS ${kritaflake_SRCS}
        resources/KisSeExprScript.cpp
    )
endif()

ki18n_wrap_ui(kritaflake_SRCS
    tools/PathToolOptionWidgetBase.ui
    tools/KoZoomToolWidget.ui
)

kis_add_library(kritaflake SHARED ${kritaflake_SRCS})
generate_export_header(kritaflake BASE_NAME kritaflake)

target_include_directories(kritaflake
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/commands>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tools>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/svg>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/text>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resources>
)

set_target_properties(libraqm PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(kritaflake
    PUBLIC
        kritapigment
        kritawidgetutils
        kritapaintingundo
        KF${KF_MAJOR}::WidgetsAddons
        Qt${QT_MAJOR_VERSION}::Svg
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
        Freetype::Freetype
        HarfBuzz::HarfBuzz
        Fontconfig::Fontconfig
    PRIVATE
        kritaresourcestorage
        kritaxmlserialization
        Raqm::Raqm
        libunibreak::libunibreak
)

if (IOS)
    # Static Fontconfig does not expose its private Expat dependency through
    # CMake, and static libintl uses the iconv shim provided by the iOS SDK.
    find_library(KRITA_IOS_EXPAT_LIBRARY NAMES expat REQUIRED)
    find_library(KRITA_IOS_ICONV_LIBRARY NAMES iconv REQUIRED)
    target_link_libraries(kritaflake PRIVATE
        ${KRITA_IOS_EXPAT_LIBRARY}
        ${KRITA_IOS_ICONV_LIBRARY}
    )
endif()

if (QT_MAJOR_VERSION STREQUAL "6")
target_link_libraries(kritaflake
    PRIVATE
        Qt${QT_MAJOR_VERSION}::OpenGLWidgets
)
endif()

set_target_properties(kritaflake PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritaflake  ${INSTALL_TARGETS_DEFAULT_ARGS})
