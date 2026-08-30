set(kritalibpaintop_LIB_SRCS
    kis_auto_brush_widget.cpp
    kis_brush_based_paintop.cpp
    kis_predefined_brush_chooser.cpp
    KisAutoBrushModel.cpp
    KisPredefinedBrushModel.cpp
    kis_brush_option_widget.cpp
    kis_brush_option.cpp
    kis_brush_selection_widget.cpp
    KisColorOptionData.cpp
    KisColorOptionWidget.cpp
    kis_color_source.cpp
    kis_color_source_option.cpp
    KisCurveOptionDataUniformProperty.cpp
    KisSensorData.cpp
    KisKritaSensorPack.cpp
    KisCurveOptionDataCommon.cpp
    KisCurveOptionData.cpp
    KisCurveOptionInputControlsStrategy.cpp
    KisCurveRangeModel.cpp
    KisCurveOptionModel.cpp
    KisCurveOptionWidget.cpp
    KisCurveOption.cpp
    KisMultiSensorsSelector.cpp
    KisMultiSensorsModel.cpp
    KisDynamicSensorFactory.cpp
    KisDynamicSensorFactoryRegistry.cpp
    KisSimpleDynamicSensorFactory.cpp
    KisSpacingOptionWidget.cpp
    KisSpacingOptionData.cpp
    KisSpacingOptionModel.cpp
    KisSpacingOption.cpp
    KisMirrorOptionData.cpp
    KisMirrorOptionWidget.cpp
    KisMirrorOption.cpp
    KisSharpnessOptionData.cpp
    KisSharpnessOptionModel.cpp
    KisSharpnessOptionWidget.cpp
    KisSharpnessOption.cpp
    KisScatterOptionData.cpp
    KisScatterOptionModel.cpp
    KisScatterOptionWidget.cpp
    KisScatterOption.cpp
    KisAirbrushOptionData.cpp
    KisAirbrushOptionModel.cpp
    KisAirbrushOptionWidget.cpp
    KisCompositeOpOptionData.cpp
    KisCompositeOpOptionModel.cpp
    KisCompositeOpOptionWidget.cpp
    KisPaintingModeOptionData.cpp
    KisPaintingModeOptionModel.cpp
    KisPaintingModeOptionWidget.cpp
    KisColorSourceOptionData.cpp
    KisColorSourceOptionModel.cpp
    KisColorSourceOptionWidget.cpp
    KisLightnessStrengthOptionWidget.cpp
    KisTextureOptionData.cpp
    KisEmbeddedTextureData.cpp
    KisTextureOptionModel.cpp
    KisTextureOptionWidget.cpp
    KisSizeOptionData.cpp
    KisSizeOptionWidget.cpp
    KisStandardOptionData.cpp
    KisStandardOptions.cpp
    KisRotationOption.cpp
    KisOpacityOption.cpp
    KisFlowOpacityOption.cpp
    KisDarkenOption.cpp
    KisHSVOption.cpp
    KisFilterOptionData.cpp
    KisFilterOptionModel.cpp
    KisFilterOptionWidget.cpp
    kis_custom_brush_widget.cpp
    kis_clipboard_brush_widget.cpp
    KisDabCacheUtils.cpp
    kis_dab_cache_base.cpp
    kis_dab_cache.cpp
    kis_precision_option.cpp
    kis_current_outline_fetcher.cpp
    kis_text_brush_chooser.cpp
    kis_brush_based_paintop_options_widget.cpp
    kis_brush_based_paintop_settings.cpp
    kis_texture_option.cpp
    kis_texture_chooser.cpp
    KisTextureMaskInfo.cpp
    KisMaskingBrushOption.cpp
    KisMaskingBrushOptionProperties.cpp
    sensors/KisDynamicSensor.cpp
    sensors/KisDynamicSensors.cpp
    sensors/KisDynamicSensorDrawingAngle.cpp
    sensors/KisDynamicSensorFuzzy.cpp
    sensors/KisDynamicSensorDistance.cpp
    sensors/KisDynamicSensorFade.cpp
    sensors/KisDynamicSensorTime.cpp
    KisDynamicSensorFactoryTime.cpp
    KisDynamicSensorFactoryFade.cpp
    KisDynamicSensorFactoryDistance.cpp
    KisDynamicSensorFactoryDrawingAngle.cpp
    KisSensorWithLengthModel.cpp
    KisDrawingAngleSensorModel.cpp
    KisTextBrushModel.cpp
)

# Keep the paint-op execution path independently buildable. These objects are
# incorporated into kritalibpaintop so its public ABI remains unchanged, and
# can also be reused by focused paint-operation contracts without building the
# settings UI.
set(kritapaintopruntime_LIB_SRCS
    KisColorSourceOptionData.cpp
    KisCurveOption.cpp
    KisCurveOptionData.cpp
    KisCurveOptionDataCommon.cpp
    KisCurveOptionDataUniformProperty.cpp
    KisDabCacheUtils.cpp
    KisDarkenOption.cpp
    KisEmbeddedTextureData.cpp
    KisFlowOpacityOption.cpp
    KisHSVOption.cpp
    KisKritaSensorPack.cpp
    KisMirrorOption.cpp
    KisMirrorOptionData.cpp
    KisOpacityOption.cpp
    KisPaintingModeOptionData.cpp
    KisRotationOption.cpp
    KisScatterOption.cpp
    KisScatterOptionData.cpp
    KisSharpnessOption.cpp
    KisSharpnessOptionData.cpp
    KisSizeOptionData.cpp
    KisSpacingOption.cpp
    KisSpacingOptionData.cpp
    KisTextureMaskInfo.cpp
    kis_brush_based_paintop.cpp
    kis_brush_based_paintop_settings.cpp
    kis_brush_option.cpp
    kis_color_source.cpp
    kis_color_source_option.cpp
    kis_current_outline_fetcher.cpp
    kis_dab_cache.cpp
    kis_dab_cache_base.cpp
    kis_precision_option.cpp
    kis_texture_option.cpp
    sensors/KisDynamicSensor.cpp
    sensors/KisDynamicSensorDistance.cpp
    sensors/KisDynamicSensorDrawingAngle.cpp
    sensors/KisDynamicSensorFade.cpp
    sensors/KisDynamicSensorFuzzy.cpp
    sensors/KisDynamicSensorTime.cpp
    sensors/KisDynamicSensors.cpp
)

list(REMOVE_ITEM kritalibpaintop_LIB_SRCS
    ${kritapaintopruntime_LIB_SRCS}
    KisAirbrushOptionData.cpp
    KisSensorData.cpp
    KisTextureOptionData.cpp
)

ki18n_wrap_ui(kritalibpaintop_LIB_SRCS
    forms/wdgautobrush.ui
    forms/wdgBrushSizeOptions.ui
    forms/wdgcurveoption.ui
    forms/wdgcurveoption2.ui
    forms/wdgcustombrush.ui
    forms/wdgclipboardbrush.ui
    forms/wdgtextbrush.ui
    forms/wdgincremental.ui
    forms/wdgmultisensorsselector.ui
    forms/wdgairbrush.ui
    forms/wdgfilteroption.ui
    forms/wdgcoloroptions.ui
    forms/wdgbrushchooser.ui
    forms/wdgpredefinedbrushchooser.ui
    forms/wdgtexturechooser.ui
    forms/wdgCompositeOpOption.ui
    forms/wdgflowopacityoption.ui
    sensors/SensorDistanceConfiguration.ui
    sensors/SensorTimeConfiguration.ui
    sensors/SensorFadeConfiguration.ui

)

add_library(kritapaintopairbrushoptiondataobjects OBJECT
    KisAirbrushOptionData.cpp
)
target_compile_definitions(kritapaintopairbrushoptiondataobjects PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopairbrushoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritapaintopairbrushoptiondataobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintopairbrushoptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintoptexturelodobjects OBJECT
    KisTextureOptionLodLimitations.cpp
)
target_compile_definitions(kritapaintoptexturelodobjects PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintoptexturelodobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritapaintoptexturelodobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
        kritaglobalidobjects
)
set_target_properties(kritapaintoptexturelodobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcurvecontrolinterfaceobjects OBJECT
    KisCurveOptionInputControlsStrategyInterface.cpp
    KisCurveOptionRangeControlsStrategyInterface.cpp
)
target_compile_definitions(kritapaintopcurvecontrolinterfaceobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurvecontrolinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
set_target_properties(kritapaintopcurvecontrolinterfaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcurverangemodelinterfaceobjects OBJECT
    KisCurveRangeModelInterface.cpp
)
target_compile_definitions(kritapaintopcurverangemodelinterfaceobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurverangemodelinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopcurverangemodelinterfaceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        lager
)
set_target_properties(kritapaintopcurverangemodelinterfaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcoloroptionmodelobjects OBJECT
    KisColorOptionModel.cpp
)
target_compile_definitions(kritapaintopcoloroptionmodelobjects PRIVATE
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritawidgets_EXPORTS
)
target_include_directories(kritapaintopcoloroptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_SOURCE_DIR}/libs/widgets
        ${CMAKE_BINARY_DIR}/libs/widgets
)
target_link_libraries(kritapaintopcoloroptionmodelobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        lager
)
set_target_properties(kritapaintopcoloroptionmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopmirroroptionmodelobjects OBJECT
    KisMirrorOptionModel.cpp
)
target_compile_definitions(kritapaintopmirroroptionmodelobjects PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritalibpaintop_EXPORTS
    kritapigment_EXPORTS
)
target_include_directories(kritapaintopmirroroptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
        ${CMAKE_SOURCE_DIR}/libs/image
        ${CMAKE_BINARY_DIR}/libs/image
        ${CMAKE_SOURCE_DIR}/libs/pigment
        ${CMAKE_BINARY_DIR}/libs/pigment
)
target_link_libraries(kritapaintopmirroroptionmodelobjects
    PUBLIC
        Boost::boost
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        lager
)
set_target_properties(kritapaintopmirroroptionmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintoptextureoptionioobjects OBJECT
    KisEmbeddedTextureDataIO.cpp
    KisTextureOptionData.cpp
)
target_compile_definitions(kritapaintoptextureoptionioobjects PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintoptextureoptionioobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment/resources>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/resources>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/resources>
)
target_link_libraries(kritapaintoptextureoptionioobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        KF${KF_MAJOR}::I18n
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintoptextureoptionioobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopsensordataobjects OBJECT
    KisSensorData.cpp
)
target_compile_definitions(kritapaintopsensordataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopsensordataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
)
target_link_libraries(kritapaintopsensordataobjects
    PUBLIC
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Eigen3::Eigen
)
set_target_properties(kritapaintopsensordataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopsensorpackinterfaceobjects OBJECT
    KisSensorPackInterface.cpp
)
target_compile_definitions(kritapaintopsensorpackinterfaceobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopsensorpackinterfaceobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopsensorpackinterfaceobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
)
set_target_properties(kritapaintopsensorpackinterfaceobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritapaintopruntime OBJECT
    ${kritapaintopruntime_LIB_SRCS}
    $<TARGET_OBJECTS:kritapaintopairbrushoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopsensorpackinterfaceobjects>
    $<TARGET_OBJECTS:kritapaintoptexturelodobjects>
)
target_compile_definitions(kritapaintopruntime PRIVATE kritalibpaintop_EXPORTS)
target_include_directories(kritapaintopruntime INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_sources(kritapaintopruntime INTERFACE
    $<TARGET_OBJECTS:kritapaintoptextureoptionioobjects>
    $<TARGET_OBJECTS:kritapaintopsensordataobjects>
)
target_link_libraries(kritapaintopruntime
    kritalibbrush
    kritapainting
    kritapaintopsensordataobjects
    kritapaintoptextureoptionioobjects
)

kis_add_library(kritalibpaintop SHARED
    ${kritalibpaintop_LIB_SRCS}
    $<TARGET_OBJECTS:kritapaintopruntime>
    $<TARGET_OBJECTS:kritapaintopcoloroptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopcurvecontrolinterfaceobjects>
    $<TARGET_OBJECTS:kritapaintopcurverangemodelinterfaceobjects>
    $<TARGET_OBJECTS:kritapaintopmirroroptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopsensordataobjects>
    $<TARGET_OBJECTS:kritapaintoptextureoptionioobjects>
)
generate_export_header(kritalibpaintop BASE_NAME kritapaintop EXPORT_MACRO_NAME PAINTOP_EXPORT)

target_link_libraries(kritalibpaintop kritaapplicationui kritatoolsui kritalibbrush kritapainting kritawidgets kritawidgetutils)
target_link_libraries(kritalibpaintop LINK_INTERFACE_LIBRARIES kritaapplicationui kritatoolsui kritalibbrush kritapainting)

set_target_properties(kritalibpaintop PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritalibpaintop  ${INSTALL_TARGETS_DEFAULT_ARGS})


add_subdirectory(tests)
