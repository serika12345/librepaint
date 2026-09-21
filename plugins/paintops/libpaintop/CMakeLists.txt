set(kritalibpaintop_LIB_SRCS
    kis_auto_brush_widget.cpp
    kis_brush_based_paintop.cpp
    kis_predefined_brush_chooser.cpp
    KisAutoBrushModel.cpp
    KisPredefinedBrushModel.cpp
    kis_brush_option_widget.cpp
    kis_brush_option.cpp
    kis_brush_selection_widget.cpp
    KisColorOptionWidget.cpp
    kis_color_source.cpp
    kis_color_source_option.cpp
    KisCurveOptionDataUniformProperty.cpp
    KisSensorData.cpp
    KisCurveOption.cpp
    KisSpacingOption.cpp
    KisMirrorOptionWidget.cpp
    KisMirrorOption.cpp
    KisSharpnessOption.cpp
    KisScatterOptionModel.cpp
    KisScatterOptionWidget.cpp
    KisScatterOption.cpp
    KisAirbrushOptionData.cpp
    KisAirbrushOptionModel.cpp
    KisAirbrushOptionWidget.cpp
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
    KisDrawingAngleSensorModel.cpp
    KisTextBrushModel.cpp
)

# Keep the paint-op execution path independently buildable. These objects are
# incorporated into kritalibpaintop so its public ABI remains unchanged, and
# can also be reused by focused paint-operation contracts without building the
# settings UI.
set(kritapaintopruntime_LIB_SRCS
    KisCurveOption.cpp
    KisCurveOptionDataUniformProperty.cpp
    KisDabCacheUtils.cpp
    KisDarkenOption.cpp
    KisFlowOpacityOption.cpp
    KisHSVOption.cpp
    KisMirrorOption.cpp
    KisOpacityOption.cpp
    KisRotationOption.cpp
    KisScatterOption.cpp
    KisSharpnessOption.cpp
    KisSpacingOption.cpp
    KisTextureMaskInfo.cpp
    kis_brush_based_paintop.cpp
    kis_brush_based_paintop_settings.cpp
    kis_brush_option.cpp
    kis_color_source.cpp
    kis_color_source_option.cpp
    kis_current_outline_fetcher.cpp
    kis_dab_cache.cpp
    kis_dab_cache_base.cpp
    kis_texture_option.cpp
    sensors/KisDynamicSensor.cpp
    sensors/KisDynamicSensorDistance.cpp
    sensors/KisDynamicSensorDrawingAngle.cpp
    sensors/KisDynamicSensorFade.cpp
    sensors/KisDynamicSensorFuzzy.cpp
    sensors/KisDynamicSensorTime.cpp
    sensors/KisDynamicSensors.cpp
)

set(kritapaintopruntime_OBJECTS
    $<TARGET_OBJECTS:kritapaintopairbrushoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopcolorsourceoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopcurveoptiondatacommonobjects>
    $<TARGET_OBJECTS:kritapaintopcurveoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopembeddedtexturedataobjects>
    $<TARGET_OBJECTS:kritapaintopmirroroptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopsharpnessoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopscatteroptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopspacingoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopsizeoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopkritasensorpackobjects>
    $<TARGET_OBJECTS:kritapaintoppaintingmodeoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopprecisionoptionobjects>
    $<TARGET_OBJECTS:kritapaintopsensorpackinterfaceobjects>
    $<TARGET_OBJECTS:kritapaintoptexturelodobjects>
)

list(REMOVE_ITEM kritalibpaintop_LIB_SRCS
    ${kritapaintopruntime_LIB_SRCS}
    KisAirbrushOptionData.cpp
    KisAirbrushOptionModel.cpp
    KisColorSourceOptionData.cpp
    KisColorSourceOptionModel.cpp
    KisCompositeOpOptionModel.cpp
    KisDrawingAngleSensorModel.cpp
    KisEmbeddedTextureData.cpp
    KisFilterOptionData.cpp
    KisFilterOptionModel.cpp
    KisPaintingModeOptionData.cpp
    KisPaintingModeOptionModel.cpp
    KisScatterOptionModel.cpp
    KisSensorData.cpp
    KisSharpnessOptionWidget.cpp
    KisSpacingOptionWidget.cpp
    KisLightnessStrengthOptionWidget.cpp
    KisSizeOptionWidget.cpp
    KisStandardOptionData.cpp
    KisStandardOptions.cpp
    KisTextureOptionData.cpp
    KisTextureOptionModel.cpp
)

ki18n_wrap_ui(kritalibpaintop_LIB_SRCS
    forms/wdgautobrush.ui
    forms/wdgBrushSizeOptions.ui
    forms/wdgcurveoption.ui
    forms/wdgcustombrush.ui
    forms/wdgclipboardbrush.ui
    forms/wdgtextbrush.ui
    forms/wdgincremental.ui
    forms/wdgairbrush.ui
    forms/wdgfilteroption.ui
    forms/wdgcoloroptions.ui
    forms/wdgbrushchooser.ui
    forms/wdgpredefinedbrushchooser.ui
    forms/wdgtexturechooser.ui
    forms/wdgCompositeOpOption.ui
    forms/wdgflowopacityoption.ui
)

add_library(kritapaintopairbrushoptiondataobjects OBJECT
    KisAirbrushOptionData.cpp
)
target_compile_definitions(kritapaintopairbrushoptiondataobjects PRIVATE
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
)
target_link_libraries(kritapaintopairbrushoptiondataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritapaintopairbrushoptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcoloroptiondataobjects OBJECT
    KisColorOptionData.cpp
)
target_compile_definitions(kritapaintopcoloroptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcoloroptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopcoloroptiondataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritapaintopcoloroptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcompositeopoptiondataobjects OBJECT
    KisCompositeOpOptionData.cpp
)
target_compile_definitions(kritapaintopcompositeopoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcompositeopoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopcompositeopoptiondataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritapaintopcompositeopoptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcolorsourceoptiondataobjects OBJECT
    KisColorSourceOptionData.cpp
)
target_compile_definitions(kritapaintopcolorsourceoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcolorsourceoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopcolorsourceoptiondataobjects
    PUBLIC
        kritaglobalidobjects
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Eigen3::Eigen
        Imath::ImathConfig
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritapaintopcolorsourceoptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcurveoptiondatacommonobjects OBJECT
    KisCurveOptionDataCommon.cpp
)

add_library(kritapaintopcurveoptiondataobjects OBJECT
    KisCurveOptionData.cpp
)

add_library(kritapaintopsizeoptiondataobjects OBJECT KisSizeOptionData.cpp)
target_compile_definitions(kritapaintopsizeoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopsizeoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>
)
target_link_libraries(kritapaintopsizeoptiondataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        kritaglobalidobjects
        kritapaintopcurveoptiondataobjects
    PRIVATE
        KF${KF_MAJOR}::I18n
)
set_target_properties(kritapaintopsizeoptiondataobjects PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)

add_library(kritapaintopstandardoptionobjects OBJECT
    KisStandardOptionData.cpp
    KisStandardOptions.cpp
)
target_compile_definitions(kritapaintopstandardoptionobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopstandardoptionobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopstandardoptionobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        kritaglobalidobjects
        kritapaintopcurveoptiondataobjects
        kritapaintopkritasensorpackobjects
        kritapaintopsizeoptiondataobjects
    PRIVATE
        kritaglobal
        kritaimage
        kritatoolsui
        lager
)
set_target_properties(kritapaintopstandardoptionobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopstandardoptionwidgetobjects OBJECT
    KisLightnessStrengthOptionWidget.cpp
    KisSizeOptionWidget.cpp
)
target_compile_definitions(kritapaintopstandardoptionwidgetobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopstandardoptionwidgetobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopstandardoptionwidgetobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopcurveoptiondataobjects
        kritapaintopsizeoptiondataobjects
        kritapaintopstandardoptionobjects
        kritatoolsui
        lager
    PRIVATE
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Widgets
        kritaglobal
        kritaimage
)
set_target_properties(kritapaintopstandardoptionwidgetobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcurveoptionwidgetobjects OBJECT
    KisCurveOptionWidget.cpp
)
ki18n_wrap_ui(kritapaintopcurveoptionwidgetobjects
    forms/wdgcurveoption2.ui
)
target_compile_definitions(kritapaintopcurveoptionwidgetobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurveoptionwidgetobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopcurveoptionwidgetobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        kritapaintopcurveoptiondatacommonobjects
        kritapaintopcurvecontrolinterfaceobjects
        kritapaintopcurverangemodelinterfaceobjects
        kritatoolsui
        lager
    PRIVATE
        KF${KF_MAJOR}::I18n
        kritaapplicationui
        kritaglobal
        kritaimage
        kritapaintopcurveoptionmodelobjects
        kritawidgetutils
)
set_target_properties(kritapaintopcurveoptionwidgetobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopmirroroptiondataobjects OBJECT KisMirrorOptionData.cpp)
target_compile_definitions(kritapaintopmirroroptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopmirroroptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopmirroroptiondataobjects
    PUBLIC
        Boost::boost
        Eigen3::Eigen
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaglobalidobjects
        kritapaintopcurveoptiondataobjects
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintopmirroroptiondataobjects PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)

add_library(kritapaintopsharpnessoptiondataobjects OBJECT KisSharpnessOptionData.cpp)
target_compile_definitions(kritapaintopsharpnessoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopsharpnessoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopsharpnessoptiondataobjects
    PUBLIC
        Boost::boost
        Eigen3::Eigen
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaglobalidobjects
        kritapaintopcurveoptiondataobjects
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintopsharpnessoptiondataobjects PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
add_library(kritapaintopscatteroptiondataobjects OBJECT KisScatterOptionData.cpp)
target_compile_definitions(kritapaintopscatteroptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopscatteroptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopscatteroptiondataobjects
    PUBLIC
        Boost::boost
        Eigen3::Eigen
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaglobalidobjects
        kritapaintopcurveoptiondataobjects
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintopscatteroptiondataobjects PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
add_library(kritapaintopspacingoptiondataobjects OBJECT KisSpacingOptionData.cpp)
target_compile_definitions(kritapaintopspacingoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopspacingoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image/brushengine>)
target_link_libraries(kritapaintopspacingoptiondataobjects
    PUBLIC
        Boost::boost
        Eigen3::Eigen
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        kritaglobalidobjects
        kritapaintopcurveoptiondataobjects
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintopspacingoptiondataobjects PROPERTIES AUTOMOC OFF POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(kritapaintopcurveoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurveoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopcurveoptiondataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        kritaglobalidobjects
        kritapaintopcurveoptiondatacommonobjects
        kritapaintopkritasensorpackobjects
)
set_target_properties(kritapaintopcurveoptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(kritapaintopcurveoptiondatacommonobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurveoptiondatacommonobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopcurveoptiondatacommonobjects
    PUBLIC
        Boost::boost
        Eigen3::Eigen
        Imath::ImathConfig
        Qt${QT_MAJOR_VERSION}::Core
        kritaglobalidobjects
        kritapaintopsensorpackinterfaceobjects
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritapaintopcurveoptiondatacommonobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintoppaintingmodeoptiondataobjects OBJECT
    KisPaintingModeOptionData.cpp
)
target_compile_definitions(kritapaintoppaintingmodeoptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintoppaintingmodeoptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintoppaintingmodeoptiondataobjects
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintoppaintingmodeoptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopprecisionoptionobjects OBJECT
    kis_precision_option.cpp
)
target_compile_definitions(kritapaintopprecisionoptionobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopprecisionoptionobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopprecisionoptionobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Eigen3::Eigen
        Imath::ImathConfig
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
)
set_target_properties(kritapaintopprecisionoptionobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopfilteroptiondataobjects OBJECT
    KisFilterOptionData.cpp
)
target_compile_definitions(kritapaintopfilteroptiondataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopfilteroptiondataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopfilteroptiondataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintopfilteroptiondataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopfilteroptionmodelobjects OBJECT
    KisFilterOptionModel.cpp
)
target_compile_definitions(kritapaintopfilteroptionmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopfilteroptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopfilteroptionmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopfilteroptiondataobjects
        lager
    PRIVATE
        kritaimage
        kritaresources
)
set_target_properties(kritapaintopfilteroptionmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintoptexturelodobjects OBJECT
    KisTextureOptionLodLimitations.cpp
)
target_compile_definitions(kritapaintoptexturelodobjects PRIVATE
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
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
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

add_library(kritapaintopcurveinputcontrolsobjects OBJECT
    KisCurveOptionInputControlsStrategy.cpp
)
target_compile_definitions(kritapaintopcurveinputcontrolsobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurveinputcontrolsobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopcurveinputcontrolsobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopcurvecontrolinterfaceobjects
        kritapaintopcurverangemodelinterfaceobjects
        lager
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Widgets
        kritaapplicationui
        kritaglobal
)
set_target_properties(kritapaintopcurveinputcontrolsobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopmultisensorsmodelobjects OBJECT
    KisMultiSensorsModel.cpp
)
target_compile_definitions(kritapaintopmultisensorsmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopmultisensorsmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopmultisensorsmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritaglobalidobjects
        lager
)
set_target_properties(kritapaintopmultisensorsmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopmultisensorsselectorobjects OBJECT
    KisMultiSensorsSelector.cpp
)
ki18n_wrap_ui(kritapaintopmultisensorsselectorobjects
    forms/wdgmultisensorsselector.ui
)
target_compile_definitions(kritapaintopmultisensorsselectorobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopmultisensorsselectorobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopmultisensorsselectorobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        kritapaintopcurveoptiondatacommonobjects
        lager
    PRIVATE
        kritaglobal
        kritapaintopdynamicsensorregistryobjects
        kritapaintopmultisensorsmodelobjects
)
set_target_properties(kritapaintopmultisensorsselectorobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcurverangemodelobjects OBJECT
    KisCurveRangeModel.cpp
)
target_compile_definitions(kritapaintopcurverangemodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurverangemodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopcurverangemodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopcurverangemodelinterfaceobjects
        lager
    PRIVATE
        kritaglobal
        kritapaintopdynamicsensorfactoryobjects
)
set_target_properties(kritapaintopcurverangemodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcoloroptionmodelobjects OBJECT
    KisColorOptionModel.cpp
)
target_compile_definitions(kritapaintopcoloroptionmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcoloroptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopcoloroptionmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopcoloroptiondataobjects
        lager
)
set_target_properties(kritapaintopcoloroptionmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopcurveoptionmodelobjects OBJECT
    KisCurveOptionModel.cpp
)
target_compile_definitions(kritapaintopcurveoptionmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopcurveoptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritapaintopcurveoptionmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopcurveoptiondataobjects
        kritapaintopcurverangemodelinterfaceobjects
        lager
    PRIVATE
        zug
)
set_target_properties(kritapaintopcurveoptionmodelobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopsensormodelobjects OBJECT
    KisDrawingAngleSensorModel.cpp
    KisSensorWithLengthModel.cpp
)
target_compile_definitions(kritapaintopsensormodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopsensormodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/global
        ${CMAKE_BINARY_DIR}/libs/global
)
target_link_libraries(kritapaintopsensormodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopsensordataobjects
        lager
    PRIVATE
        zug
)
set_target_properties(kritapaintopsensormodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopmirroroptionmodelobjects OBJECT
    KisMirrorOptionModel.cpp
)
target_compile_definitions(kritapaintopmirroroptionmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopmirroroptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopmirroroptionmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopmirroroptiondataobjects
        lager
)
set_target_properties(kritapaintopmirroroptionmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

# These models bind preset option data to the settings UI. Keeping their
# compilation separate lets focused option-data changes avoid the remaining
# settings widgets while preserving the objects in kritalibpaintop.
add_library(kritapaintopoptionmodelobjects OBJECT
    KisAirbrushOptionModel.cpp
    KisColorSourceOptionModel.cpp
    KisCompositeOpOptionModel.cpp
    KisPaintingModeOptionModel.cpp
    KisScatterOptionModel.cpp
)
target_compile_definitions(kritapaintopoptionmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopoptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopoptionmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopairbrushoptiondataobjects
        kritapaintopcolorsourceoptiondataobjects
        kritapaintopcompositeopoptiondataobjects
        kritapaintoppaintingmodeoptiondataobjects
        kritapaintopscatteroptiondataobjects
        kritapaintopsharpnessoptiondataobjects
        kritapaintopspacingoptiondataobjects
        kritawidgets
        lager
)
set_target_properties(kritapaintopoptionmodelobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopspacingoptionwidgetobjects OBJECT
    KisSpacingOptionModel.cpp
    KisSpacingOptionWidget.cpp
)
target_compile_definitions(kritapaintopspacingoptionwidgetobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopspacingoptionwidgetobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopspacingoptionwidgetobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        kritapaintopcurveoptionwidgetobjects
        kritapaintopspacingoptiondataobjects
        lager
    PRIVATE
        KF${KF_MAJOR}::I18n
        kritawidgetutils
)
set_target_properties(kritapaintopspacingoptionwidgetobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopsharpnessoptionwidgetobjects OBJECT
    KisSharpnessOptionModel.cpp
    KisSharpnessOptionWidget.cpp
)
target_compile_definitions(kritapaintopsharpnessoptionwidgetobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopsharpnessoptionwidgetobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintopsharpnessoptionwidgetobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        kritapaintopcurveoptionwidgetobjects
        kritapaintopsharpnessoptiondataobjects
        lager
    PRIVATE
        KF${KF_MAJOR}::I18n
        kritawidgets
        kritawidgetutils
)
set_target_properties(kritapaintopsharpnessoptionwidgetobjects PROPERTIES
    AUTOMOC ON
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintoptextureoptionioobjects OBJECT
    KisEmbeddedTextureDataIO.cpp
    KisTextureOptionData.cpp
)
target_compile_definitions(kritapaintoptextureoptionioobjects PRIVATE
    kritaimage_EXPORTS
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
)
target_link_libraries(kritapaintoptextureoptionioobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
        Eigen3::Eigen
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Gui
        ${LINK_OPENEXR_LIB}
)
set_target_properties(kritapaintoptextureoptionioobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopembeddedtexturedataobjects OBJECT
    KisEmbeddedTextureData.cpp
)
target_compile_definitions(kritapaintopembeddedtexturedataobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopembeddedtexturedataobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopembeddedtexturedataobjects
    PUBLIC
        Boost::boost
        Qt${QT_MAJOR_VERSION}::Core
    PRIVATE
        kritapigment
        kritaresources
)
set_target_properties(kritapaintopembeddedtexturedataobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintoptextureoptionmodelobjects OBJECT
    KisTextureOptionModel.cpp
)
target_compile_definitions(kritapaintoptextureoptionmodelobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintoptextureoptionmodelobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/global>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/global>
)
target_link_libraries(kritapaintoptextureoptionmodelobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintoptextureoptionioobjects
        kritaresources
        lager
    PRIVATE
        kritapaintopembeddedtexturedataobjects
        kritapigment
)
set_target_properties(kritapaintoptextureoptionmodelobjects PROPERTIES
    AUTOMOC ON
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
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
)
target_link_libraries(kritapaintopsensordataobjects
    PUBLIC
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
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

add_library(kritapaintopkritasensorpackobjects OBJECT
    KisKritaSensorPack.cpp
)
target_compile_definitions(kritapaintopkritasensorpackobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopkritasensorpackobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/image>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/pigment>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/libs/pigment>
)
target_link_libraries(kritapaintopkritasensorpackobjects
    PUBLIC
        Boost::boost
        kritapaintopsensordataobjects
        kritapaintopsensorpackinterfaceobjects
    PRIVATE
        Eigen3::Eigen
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        kritaglobalidobjects
        kritapaintopcurveoptiondatacommonobjects
)
set_target_properties(kritapaintopkritasensorpackobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopdynamicsensorfactoryobjects OBJECT
    KisDynamicSensorFactory.cpp
    KisSimpleDynamicSensorFactory.cpp
)
target_compile_definitions(kritapaintopdynamicsensorfactoryobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopdynamicsensorfactoryobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopdynamicsensorfactoryobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritapaintopcurveoptiondatacommonobjects
        lager
)
set_target_properties(kritapaintopdynamicsensorfactoryobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

add_library(kritapaintopdynamicsensorregistryobjects OBJECT
    KisDynamicSensorFactoryRegistry.cpp
    KisDynamicSensorFactoryTime.cpp
    KisDynamicSensorFactoryFade.cpp
    KisDynamicSensorFactoryDistance.cpp
    KisDynamicSensorFactoryDrawingAngle.cpp
)
ki18n_wrap_ui(kritapaintopdynamicsensorregistryobjects
    sensors/SensorDistanceConfiguration.ui
    sensors/SensorTimeConfiguration.ui
    sensors/SensorFadeConfiguration.ui
)
target_compile_definitions(kritapaintopdynamicsensorregistryobjects PRIVATE
    kritalibpaintop_EXPORTS
)
target_include_directories(kritapaintopdynamicsensorregistryobjects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
target_link_libraries(kritapaintopdynamicsensorregistryobjects
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        kritaglobal
        kritapaintopcurveoptiondatacommonobjects
        kritapaintopdynamicsensorfactoryobjects
        lager
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        kritaglobalidobjects
        kritapaintopkritasensorpackobjects
        kritapaintopsensormodelobjects
        kritatoolsui
        kritawidgetutils
)
set_target_properties(kritapaintopdynamicsensorregistryobjects PROPERTIES
    AUTOMOC OFF
    POSITION_INDEPENDENT_CODE ON
)

kis_add_library(kritapaintopruntime OBJECT
    ${kritapaintopruntime_LIB_SRCS}
    ${kritapaintopruntime_OBJECTS}
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
    PUBLIC
        Boost::boost
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        kritaglobal
        kritaimage
        kritalibbrush
        kritapainting
        kritapaintingundo
        kritapaintopsensordataobjects
        kritapaintoptextureoptionioobjects
        kritapigment
        kritaresources
)

kis_add_library(kritalibpaintop SHARED
    ${kritalibpaintop_LIB_SRCS}
    KisCurveOptionModel.h
    $<TARGET_OBJECTS:kritapaintopcoloroptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopcompositeopoptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopdynamicsensorfactoryobjects>
    $<TARGET_OBJECTS:kritapaintopdynamicsensorregistryobjects>
    $<TARGET_OBJECTS:kritapaintopfilteroptiondataobjects>
    $<TARGET_OBJECTS:kritapaintopfilteroptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopruntime>
    ${kritapaintopruntime_OBJECTS}
    $<TARGET_OBJECTS:kritapaintopcoloroptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopcurveoptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopcurvecontrolinterfaceobjects>
    $<TARGET_OBJECTS:kritapaintopcurveinputcontrolsobjects>
    $<TARGET_OBJECTS:kritapaintopcurverangemodelinterfaceobjects>
    $<TARGET_OBJECTS:kritapaintopmultisensorsmodelobjects>
    $<TARGET_OBJECTS:kritapaintopmultisensorsselectorobjects>
    $<TARGET_OBJECTS:kritapaintopcurverangemodelobjects>
    $<TARGET_OBJECTS:kritapaintopmirroroptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopoptionmodelobjects>
    $<TARGET_OBJECTS:kritapaintopsharpnessoptionwidgetobjects>
    $<TARGET_OBJECTS:kritapaintopspacingoptionwidgetobjects>
    $<TARGET_OBJECTS:kritapaintopsensormodelobjects>
    $<TARGET_OBJECTS:kritapaintopsensordataobjects>
    $<TARGET_OBJECTS:kritapaintopcurveoptionwidgetobjects>
    $<TARGET_OBJECTS:kritapaintopstandardoptionobjects>
    $<TARGET_OBJECTS:kritapaintopstandardoptionwidgetobjects>
    $<TARGET_OBJECTS:kritapaintoptextureoptionioobjects>
    $<TARGET_OBJECTS:kritapaintoptextureoptionmodelobjects>
)
generate_export_header(kritalibpaintop BASE_NAME kritapaintop EXPORT_MACRO_NAME PAINTOP_EXPORT)

target_link_libraries(kritalibpaintop kritaapplicationui kritatoolsui kritalibbrush kritapainting kritaimage kritapigment kritaresources kritawidgets kritawidgetutils)
target_link_libraries(kritalibpaintop LINK_INTERFACE_LIBRARIES kritaapplicationui kritatoolsui kritalibbrush kritapainting)

set_target_properties(kritalibpaintop PROPERTIES
    VERSION ${GENERIC_KRITA_LIB_VERSION} SOVERSION ${GENERIC_KRITA_LIB_SOVERSION}
)

install(TARGETS kritalibpaintop  ${INSTALL_TARGETS_DEFAULT_ARGS})


add_subdirectory(tests)
