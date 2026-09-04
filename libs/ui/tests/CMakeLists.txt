kis_add_tests(
    kis_image_view_converter_test.cpp
    kis_shape_selection_test.cpp
    kis_grid_config_test.cpp
    kis_shape_commands_test.cpp
    kis_stop_gradient_editor_test.cpp
    KisFrameCacheSwapperTest.cpp
    kis_animation_exporter_test.cpp
    kis_prescaled_projection_test.cpp
    kis_display_color_converter_contract_test.cpp
    kis_animation_importer_test.cpp
    KisDocumentReplaceTest.cpp
    kis_derived_resources_test.cpp
    kis_animation_frame_cache_test.cpp
    TestCanvasUiPublicHeaders.cpp
    TestDocumentStateUiPublicHeaders.cpp
    TestApplicationWorkspaceToolUiPublicHeaders.cpp
    KisWorkspacePersistenceTest.cpp
    TestApplicationPaintingResourceRegistration.cpp
    kis_shape_layer_test.cpp
    KisSafeDocumentLoaderTest.cpp
    LINK_LIBRARIES kritaapplication kritaapplicationui kritalibkis kritatestsdk
    )

kis_add_test(
    KisSpinBoxSplineUnitConverterTest.cpp
    LINK_LIBRARIES
        kritauispinboxsplineunitconverterobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSpinBoxSplineUnitConverterTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisApplicationInputActionsTest.cpp
    TEST_NAME KisApplicationInputActionsTest
    LINK_LIBRARIES kritainputui kritaapplicationui kritalibkis kritatestsdk
)

kis_add_test(
    TestRemainingUiRootPublicHeaders.cpp
    TEST_NAME TestRemainingUiRootPublicHeaders
    LINK_LIBRARIES kritainputui kritaapplicationui kritalibkis kritatestsdk
)

kis_add_test(
    KisDlgPreferencesEnumContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::Completion
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        KF${KF_MAJOR}::WidgetsAddons
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisDlgPreferencesEnumContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_BINARY_DIR}/libs
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../widgets
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
    ${CMAKE_SOURCE_DIR}/libs/widgetutils/xmlgui
    ${CMAKE_SOURCE_DIR}/libs/widgets
    ${CMAKE_BINARY_DIR}/libs/widgets
)
target_compile_definitions(KisDlgPreferencesEnumContractTest PRIVATE
    kritaapplication_EXPORTS
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
    kritaui_EXPORTS
    kritawidgets_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisOpenGLRendererConfigContractTest.cpp
    LINK_LIBRARIES
        kritauiopenglrendererconfigobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisOpenGLRendererConfigContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_tests(
    kis_doc2_test.cpp
    kis_file_layer_test.cpp
    LINK_LIBRARIES kritaapplication kritaapplicationui kritatransformmaskstubs
    )

kis_add_test( kis_selection_decoration_test.cpp ../../../sdk/tests/stroke_testing_utils.cpp
    TEST_NAME KisSelectionDecorationTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test( FreehandStrokeContractTest.cpp
    TEST_NAME FreehandStrokeContractTest
    LINK_LIBRARIES kritapainting kritalibbrush kritatestsdk
    )

kis_add_test(
    KisCanvasUpdatesCompressorContractTest.cpp
    ../canvas/kis_canvas_updates_compressor.cpp
    TEST_NAME KisCanvasUpdatesCompressorContractTest
    LINK_LIBRARIES kritacanvas kritatestsdk
    )

kis_add_test(
    KisQPainterCanvasDrawImageContractTest.cpp
    TEST_NAME KisQPainterCanvasDrawImageContractTest
    LINK_LIBRARIES kritacanvas kritatestsdk
    )

kis_add_test(
    KisAnimationFrameCacheFwdContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisAnimationFrameCacheFwdContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
)

kis_add_test(
    KisPlaybackEngineSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisPlaybackEngineSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/flake
    ${CMAKE_BINARY_DIR}/libs/flake
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Xml,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisPlaybackEngineSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaflake_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisGridConfigValueContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
)
target_include_directories(KisGridConfigValueContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
)
target_compile_definitions(KisGridConfigValueContractTest PRIVATE
    kritaui_EXPORTS
)

kis_add_test(
    KisGuidesConfigSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Gui
        Boost::boost
)
target_include_directories(KisGuidesConfigSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
)
target_compile_definitions(KisGuidesConfigSchemaContractTest PRIVATE
    kritaui_EXPORTS
)

kis_add_test(
    KisDisplayConfigSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisDisplayConfigSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(KisDisplayConfigSchemaContractTest PRIVATE
    kritaui_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisSegmentGradientSliderSchemaContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Gui Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSegmentGradientSliderSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/application ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/pigment ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/global ${CMAKE_BINARY_DIR}/libs/global
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisSegmentGradientSliderSchemaContractTest PRIVATE
    kritaui_EXPORTS kritapigment_EXPORTS kritaresources_EXPORTS kritaglobal_EXPORTS
)

kis_add_test(
    KisNodeViewColorSchemeSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeViewColorSchemeSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/application
    ${CMAKE_BINARY_DIR}/libs/application
)
target_compile_definitions(KisNodeViewColorSchemeSchemaContractTest PRIVATE
    kritaui_EXPORTS
)

kis_add_test(
    KisHistogramPainterSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisHistogramPainterSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/application
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisHistogramPainterSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisAsyncColorSamplerHelperSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisAsyncColorSamplerHelperSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisAsyncColorSamplerHelperSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    KisAnimationFrameCacheSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisAnimationFrameCacheSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/libs/application
    ${CMAKE_SOURCE_DIR}/libs/image
    ${CMAKE_BINARY_DIR}/libs/image
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisAnimationFrameCacheSchemaContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    KisExposureGammaCorrectionInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritauiexposuregammacorrectionobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisExposureGammaCorrectionInterfaceContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisSizeGroupContractTest.cpp
    LINK_LIBRARIES
        kritauisizegroupobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisSizeGroupContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisElidedLabelContractTest.cpp
    LINK_LIBRARIES
        kritauielidedlabelobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_compile_definitions(KisElidedLabelContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisWarningWidgetContractTest.cpp
    LINK_LIBRARIES
        kritauiwarningwidgetobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
)
target_include_directories(KisWarningWidgetContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(KisWarningWidgetContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisSelectionActionsPanelContractTest.cpp
    LINK_LIBRARIES
        kritauiselectionactionspanelobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::ConfigCore
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisSelectionActionsPanelContractTest PRIVATE
    ${kritauiselectionactionspanel_INCLUDE_DIRS}
)
target_compile_definitions(KisSelectionActionsPanelContractTest PRIVATE
    kritaapplication_EXPORTS
    kritaapplicationui_EXPORTS
    kritaglobal_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    kis_multinode_property_test.cpp
    TEST_NAME KisMultinodePropertyContractTest
    LINK_LIBRARIES
        kritauimultinodepropertyobjects
        kritapaintingundo
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
)
target_include_directories(KisMultinodePropertyContractTest PRIVATE
    ${kritauimultinodeproperty_INCLUDE_DIRS}
)
target_compile_definitions(KisMultinodePropertyContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerImageStateContractTest.cpp
    TEST_NAME KisNodeManagerImageStateContractTest
    LINK_LIBRARIES
        kritauinodemanagerimagestateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerImageStateContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerImageStateContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerNodeStateContractTest.cpp
    TEST_NAME KisNodeManagerNodeStateContractTest
    LINK_LIBRARIES
        kritauinodemanagernodestateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerNodeStateContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerNodeStateContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerNodeChangeContractTest.cpp
    TEST_NAME KisNodeManagerNodeChangeContractTest
    LINK_LIBRARIES
        kritauinodemanagernodechangeobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerNodeChangeContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerNodeChangeContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerModificationContractTest.cpp
    TEST_NAME KisNodeManagerModificationContractTest
    LINK_LIBRARIES
        kritauinodemanagermodificationobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerModificationContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerModificationContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerActiveContractTest.cpp
    TEST_NAME KisNodeManagerActiveContractTest
    LINK_LIBRARIES
        kritauinodemanageractiveobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerActiveContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerActiveContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerAccessorsContractTest.cpp
    TEST_NAME KisNodeManagerAccessorsContractTest
    LINK_LIBRARIES
        kritauinodemanageraccessorobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerAccessorsContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerAccessorsContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerSelectionStateContractTest.cpp
    TEST_NAME KisNodeManagerSelectionStateContractTest
    LINK_LIBRARIES
        kritauinodemanagerselectionstateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerSelectionStateContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerSelectionStateContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerPropertiesContractTest.cpp
    TEST_NAME KisNodeManagerPropertiesContractTest
    LINK_LIBRARIES
        kritauinodemanagerpropertiesobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerPropertiesContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerPropertiesContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerPropertyDialogContractTest.cpp
    TEST_NAME KisNodeManagerPropertyDialogContractTest
    LINK_LIBRARIES
        kritauinodemanagerpropertydialogobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerPropertyDialogContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerPropertyDialogContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerNodeUpdateContractTest.cpp
    TEST_NAME KisNodeManagerNodeUpdateContractTest
    LINK_LIBRARIES
        kritauinodemanagernodeupdateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerNodeUpdateContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerNodeUpdateContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerNavigationContractTest.cpp
    TEST_NAME KisNodeManagerNavigationContractTest
    LINK_LIBRARIES
        kritauinodemanageravigationobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerNavigationContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerNavigationContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerOrderingContractTest.cpp
    TEST_NAME KisNodeManagerOrderingContractTest
    LINK_LIBRARIES
        kritauinodemanagerorderingobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerOrderingContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerOrderingContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerRemovalContractTest.cpp
    TEST_NAME KisNodeManagerRemovalContractTest
    LINK_LIBRARIES
        kritauinodemanagerremovalobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerRemovalContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerRemovalContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerTreeOperationContractTest.cpp
    TEST_NAME KisNodeManagerTreeOperationContractTest
    LINK_LIBRARIES
        kritauinodemanagertreeoperationobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerTreeOperationContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerTreeOperationContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerIsolationContractTest.cpp
    TEST_NAME KisNodeManagerIsolationContractTest
    LINK_LIBRARIES
        kritauinodemanagerisolationobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerIsolationContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerIsolationContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerSelectionContractTest.cpp
    TEST_NAME KisNodeManagerSelectionContractTest
    LINK_LIBRARIES
        kritauinodemanagerselectionobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerSelectionContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerSelectionContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerToggleContractTest.cpp
    TEST_NAME KisNodeManagerToggleContractTest
    LINK_LIBRARIES
        kritauinodemanagertoggleobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerToggleContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerToggleContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerMirrorContractTest.cpp
    TEST_NAME KisNodeManagerMirrorContractTest
    LINK_LIBRARIES
        kritauinodemanagermirrorobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerMirrorContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisNodeManagerMirrorContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerClipboardContractTest.cpp
    TEST_NAME KisNodeManagerClipboardContractTest
    LINK_LIBRARIES
        kritauinodemanagerclipboardobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerClipboardContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisNodeManagerClipboardContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerSplitAlphaContractTest.cpp
    TEST_NAME KisNodeManagerSplitAlphaContractTest
    LINK_LIBRARIES
        kritauinodemanagersplitalphaobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerSplitAlphaContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerSplitAlphaContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerQuickGroupContractTest.cpp
    TEST_NAME KisNodeManagerQuickGroupContractTest
    LINK_LIBRARIES
        kritauinodemanagerquickgroupobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerQuickGroupContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisNodeManagerQuickGroupContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerReferenceImageContractTest.cpp
    TEST_NAME KisNodeManagerReferenceImageContractTest
    LINK_LIBRARIES
        kritauinodemanagerreferenceimageobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerReferenceImageContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerReferenceImageContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerLayerCreationContractTest.cpp
    TEST_NAME KisNodeManagerLayerCreationContractTest
    LINK_LIBRARIES
        kritauinodemanagerlayercreationobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerLayerCreationContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerLayerCreationContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerNodeTypeContractTest.cpp
    TEST_NAME KisNodeManagerNodeTypeContractTest
    LINK_LIBRARIES
        kritauinodemanagernodetypeobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerNodeTypeContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisNodeManagerNodeTypeContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerExportContractTest.cpp
    TEST_NAME KisNodeManagerExportContractTest
    LINK_LIBRARIES
        kritauinodemanagerexportobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerExportContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerExportContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerActivationContractTest.cpp
    TEST_NAME KisNodeManagerActivationContractTest
    LINK_LIBRARIES
        kritauinodemanageractivationobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerActivationContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerActivationContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerLifecycleContractTest.cpp
    TEST_NAME KisNodeManagerLifecycleContractTest
    LINK_LIBRARIES
        kritauinodemanagerlifecyclecontractobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerLifecycleContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerLifecycleContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeManagerSetupContractTest.cpp
    TEST_NAME KisNodeManagerSetupContractTest
    LINK_LIBRARIES
        kritauinodemanagersetupobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeManagerSetupContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeManagerSetupContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelDisplayStateContractTest.cpp
    TEST_NAME KisNodeModelDisplayStateContractTest
    LINK_LIBRARIES
        kritauinodemodeldisplaystateobjects
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        KF${KF_MAJOR}::I18n
        Boost::boost
        Eigen3::Eigen
        ${LINK_OPENEXR_LIB}
)
target_include_directories(KisNodeModelDisplayStateContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelDisplayStateContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelItemDataRoleContractTest.cpp
    TEST_NAME KisNodeModelItemDataRoleContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelItemDataRoleContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelItemDataRoleContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelTransferProtocolContractTest.cpp
    TEST_NAME KisNodeModelTransferProtocolContractTest
    LINK_LIBRARIES
        kritauinodemodeltransferprotocolobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelTransferProtocolContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelTransferProtocolContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelIndexMappingContractTest.cpp
    TEST_NAME KisNodeModelIndexMappingContractTest
    LINK_LIBRARIES
        kritauinodemodelindexmappingobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelIndexMappingContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelIndexMappingContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisModelIndexConverterBaseContractTest.cpp
    TEST_NAME KisModelIndexConverterBaseContractTest
    LINK_LIBRARIES
        kritauimodelindexconverterbaseobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisModelIndexConverterBaseContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelIsolationMembershipContractTest.cpp
    TEST_NAME KisNodeModelIsolationMembershipContractTest
    LINK_LIBRARIES
        kritauinodemodelisolationmembershipobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelIsolationMembershipContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelIsolationMembershipContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelStructureContractTest.cpp
    TEST_NAME KisNodeModelStructureContractTest
    LINK_LIBRARIES
        kritauinodemodelstructureobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelStructureContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelStructureContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelLifecycleContractTest.cpp
    TEST_NAME KisNodeModelLifecycleContractTest
    LINK_LIBRARIES
        kritauinodemodellifecycleobjects
        kritauinodemodelstructureobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelLifecycleContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_sources(KisNodeModelLifecycleContractTest PRIVATE
    ../nodes/kis_node_model.h
)
target_compile_definitions(KisNodeModelLifecycleContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelFacadeSetupContractTest.cpp
    TEST_NAME KisNodeModelFacadeSetupContractTest
    LINK_LIBRARIES
        kritauinodemodelfacadesetupobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelFacadeSetupContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelFacadeSetupContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelItemFlagsContractTest.cpp
    TEST_NAME KisNodeModelItemFlagsContractTest
    LINK_LIBRARIES
        kritauinodemodelitemflagsobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelItemFlagsContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelItemFlagsContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelDropAcceptanceContractTest.cpp
    TEST_NAME KisNodeModelDropAcceptanceContractTest
    LINK_LIBRARIES
        kritauinodemodeldropacceptanceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelDropAcceptanceContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelDropAcceptanceContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelMimeDataContractTest.cpp
    TEST_NAME KisNodeModelMimeDataContractTest
    LINK_LIBRARIES
        kritauinodemodelmimedataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelMimeDataContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelMimeDataContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelDropInsertionContractTest.cpp
    TEST_NAME KisNodeModelDropInsertionContractTest
    LINK_LIBRARIES
        kritauinodemodeldropinsertionobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelDropInsertionContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_compile_definitions(KisNodeModelDropInsertionContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelRemovalContractTest.cpp
    TEST_NAME KisNodeModelRemovalContractTest
    LINK_LIBRARIES
        kritauinodemodellifecycleobjects
        kritauinodemodelstructureobjects
        kritauinodemodelremovalobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelRemovalContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_sources(KisNodeModelRemovalContractTest PRIVATE
    ../nodes/kis_node_model.h
)
target_compile_definitions(KisNodeModelRemovalContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelSetDataContractTest.cpp
    TEST_NAME KisNodeModelSetDataContractTest
    LINK_LIBRARIES
        kritauinodemodellifecycleobjects
        kritauinodemodelstructureobjects
        kritauinodemodelsetdataobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelSetDataContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_sources(KisNodeModelSetDataContractTest PRIVATE
    ../nodes/kis_node_model.h
)
target_compile_definitions(KisNodeModelSetDataContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test(
    KisNodeModelDataContractTest.cpp
    TEST_NAME KisNodeModelDataContractTest
    LINK_LIBRARIES
        kritauinodemodellifecycleobjects
        kritauinodemodeldataobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeModelDataContractTest PRIVATE
    ${kritauinodemanager_INCLUDE_DIRS}
)
target_sources(KisNodeModelDataContractTest PRIVATE
    ../nodes/kis_node_model.h
)
target_compile_definitions(KisNodeModelDataContractTest PRIVATE
    kritaapplicationui_EXPORTS
)

kis_add_test( kis_node_dummies_graph_test.cpp ../../../sdk/tests/testutil.cpp
    TEST_NAME KisNodeDummiesGraphTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test( kis_node_shapes_graph_test.cpp ../../../sdk/tests/testutil.cpp
    TEST_NAME KisNodeShapesGraphTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test( kis_model_index_converter_test.cpp ../../../sdk/tests/testutil.cpp
    TEST_NAME KisModelIndexConverterTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )


kis_add_test(
    kis_node_model_test.cpp modeltest.cpp
    TEST_NAME kis_node_model_test
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test( kis_shape_controller_test.cpp kis_dummies_facade_base_test.cpp
    TEST_NAME kis_shape_controller_test
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test( kis_dummies_facade_test.cpp kis_dummies_facade_base_test.cpp ../../../sdk/tests/testutil.cpp
    TEST_NAME kis_dummies_facade_test
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

kis_add_test(
    kis_view_signals_test.cpp ../../../krita/krita.qrc
    TEST_NAME kis_view_signals_test
    LINK_LIBRARIES kritaapplicationui kritalibkis kritatestsdk
    )


##### Tests that currently fail and should be fixed #####

# Works under Linux but does not work under Windows
krita_add_broken_unit_test(
    kis_painting_assistants_decoration_test.cpp ../../../sdk/tests/testutil.cpp
    TEST_NAME KisPaintingAssistantsDecorationTest
    LINK_LIBRARIES kritaapplication kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( kis_clipboard_test.cpp
    TEST_NAME KisClipboardTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( FreehandStrokeBenchmark.cpp  $<TARGET_PROPERTY:kritatestsdk,SOURCE_DIR>/stroke_testing_utils.cpp
    TEST_NAME FreehandStrokeBenchmark
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( KisPaintOnTransparencyMaskTest.cpp  $<TARGET_PROPERTY:kritatestsdk,SOURCE_DIR>/stroke_testing_utils.cpp
    TEST_NAME KisPaintOnTransparencyMaskTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( fill_processing_visitor_test.cpp  $<TARGET_PROPERTY:kritatestsdk,SOURCE_DIR>/stroke_testing_utils.cpp
    TEST_NAME FillProcessingVisitorTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( filter_stroke_test.cpp ../../../sdk/tests/stroke_testing_utils.cpp
    TEST_NAME FilterStrokeTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( kis_selection_manager_test.cpp
    TEST_NAME KisSelectionManagerTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )
#set_tests_properties(libs-ui-KisSelectionManagerTest PROPERTIES TIMEOUT 300)

krita_add_broken_unit_test( kis_node_manager_test.cpp
    TEST_NAME KisNodeManagerTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

krita_add_broken_unit_test( kis_action_manager_test.cpp
    TEST_NAME KisActionManagerTest
    LINK_LIBRARIES kritaapplicationui kritatestsdk
    )

if (${INSTALL_BENCHMARKS})
    install(TARGETS FreehandStrokeBenchmark  ${INSTALL_TARGETS_DEFAULT_ARGS})

    install(FILES data/testing_200px_colorsmudge_default_dulling_old_sa.kpp
        data/testing_200px_colorsmudge_default_dulling_new_nsa.kpp
        data/testing_200px_colorsmudge_default_dulling_new_sa.kpp
        data/testing_800px_colorsmudge_default_dulling_new_sa.kpp
        data/testing_200px_colorsmudge_default_dulling_old_nsa.kpp
        data/testing_200px_colorsmudge_default_smearing_new_nsa.kpp
        data/testing_200px_colorsmudge_default_smearing_new_sa.kpp
        data/testing_200px_colorsmudge_default_smearing_old_nsa.kpp
        data/testing_200px_colorsmudge_default_smearing_old_sa.kpp
        data/testing_200px_colorsmudge_lightness_smearing_new_nsa_maskmode.kpp
        data/testing_200px_colorsmudge_lightness_smearing_new_nsa_nopt.kpp
        data/testing_200px_colorsmudge_lightness_smearing_new_nsa_ptoverlay.kpp
        data/testing_200px_colorsmudge_lightness_smearing_new_nsa_ptoverwrite.kpp
        data/testing_1000px_auto_default.kpp
        data/testing_1000px_auto_gaussian.kpp
        data/testing_1000px_auto_gaussian_rect.kpp
        data/testing_1000px_auto_rectangular.kpp
        data/testing_1000px_auto_soft.kpp
        data/testing_1000px_auto_soft_rect.kpp
        data/testing_1000px_stamp_450_rotated.kpp
        data/3_texture.png
        data/DA_RGBA\ bluegreen_small1.png
        DESTINATION ${CMAKE_INSTALL_BINDIR}/data)
endif()
kis_add_test(
    KisMirrorAxisConfigContractTest.cpp
    LINK_LIBRARIES
        kritauimirroraxisconfigobjects
        kritaglobaldomutilsobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
)
target_sources(KisMirrorAxisConfigContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../canvas/KisMirrorAxisConfig.h
)
target_include_directories(KisMirrorAxisConfigContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisMirrorAxisConfigContractTest PRIVATE
    kritaapplicationui_EXPORTS
    kritaglobal_EXPORTS
)
target_compile_options(KisMirrorAxisConfigContractTest PRIVATE
    -include QObject
    SHELL:-include\ QPointF
)

kis_add_test(
    KisStoryboardItemContractTest.cpp
    LINK_LIBRARIES
        kritauistoryboarditemobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisStoryboardItemContractTest PRIVATE kritaui_EXPORTS)
