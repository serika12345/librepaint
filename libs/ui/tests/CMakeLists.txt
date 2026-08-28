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
    KisSpinBoxSplineUnitConverterTest.cpp
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
