# cmake in some versions for some not yet known reasons fails to run automoc
# on random targets (changing target names already has an effect)
# As temporary workaround skipping build of tests on these versions for now
# See https://mail.kde.org/pipermail/kde-buildsystem/2015-June/010819.html
# extend range of affected cmake versions as needed
if(NOT ${CMAKE_VERSION} VERSION_LESS 3.1.3 AND
   NOT ${CMAKE_VERSION} VERSION_GREATER 3.2.3)
    message(WARNING "Skipping krita/image/tests, CMake in at least versions 3.1.3 - 3.2.3 seems to have a problem with automoc.  \n(FRIENDLY REMINDER: PLEASE DON'T BREAK THE TESTS!)")
    set (HAVE_FAILING_CMAKE TRUE)
else()
    set (HAVE_FAILING_CMAKE FALSE)
endif()

set(KisRandomGenerator2DDemoSources KisRandomGenerator2DDemo.cpp kimageframe.cpp)
ki18n_wrap_ui(KisRandomGenerator2DDemoSources KisRandomGenerator2DDemo.ui)
add_executable(KisRandomGenerator2DDemo ${KisRandomGenerator2DDemoSources})
target_link_libraries(KisRandomGenerator2DDemo kritaimage)
ecm_mark_as_test(KisRandomGenerator2DDemo)


kis_add_tests(
    kis_base_node_test.cpp
    kis_node_test.cpp
    kis_node_facade_test.cpp
    kis_fixed_paint_device_test.cpp
    kis_layer_test.cpp
    kis_effect_mask_test.cpp
    kis_iterator_test.cpp
    kis_painter_test.cpp
    kis_count_visitor_test.cpp
    kis_projection_test.cpp
    kis_properties_configuration_test.cpp
    kis_transaction_test.cpp
    kis_pixel_selection_test.cpp
    kis_group_layer_test.cpp
    kis_paint_layer_test.cpp
    kis_adjustment_layer_test.cpp
    kis_annotation_test.cpp
    kis_convolution_painter_test.cpp
    kis_crop_processing_visitor_test.cpp
    kis_processing_applicator_test.cpp
    kis_datamanager_test.cpp
    kis_fill_painter_test.cpp
    kis_filter_configuration_test.cpp
    kis_filter_test.cpp
    kis_filter_processing_information_test.cpp
    kis_filter_registry_test.cpp
    kis_filter_strategy_test.cpp
    kis_gradient_painter_test.cpp
    kis_image_test.cpp
    kis_image_signal_router_test.cpp
    kis_iterators_ng_test.cpp
    kis_iterator_benchmark.cpp
    kis_updater_context_test.cpp
    kis_simple_update_queue_test.cpp
    kis_stroke_test.cpp
    kis_simple_stroke_strategy_test.cpp
    kis_stroke_strategy_undo_command_based_test.cpp
    kis_strokes_queue_test.cpp
    kis_mask_test.cpp
    kis_math_toolbox_test.cpp
    kis_node_graph_listener_test.cpp
    kis_node_visitor_test.cpp
    kis_paint_information_test.cpp
    kis_distance_information_test.cpp
    kis_paintop_test.cpp
    kis_pattern_test.cpp
    kis_selection_mask_test.cpp
    kis_shared_ptr_test.cpp
    kis_bsplines_test.cpp
    kis_warp_transform_worker_test.cpp
    kis_transparency_mask_test.cpp
    kis_types_test.cpp
    kis_filter_config_widget_test.cpp
    kis_mask_generator_test.cpp
    kis_cubic_curve_test.cpp
    kis_cubic_spline_test.cpp
    kis_fixed_point_maths_test.cpp
    kis_node_query_path_test.cpp
    kis_filter_weights_buffer_test.cpp
    kis_filter_weights_applicator_test.cpp
    kis_fill_interval_test.cpp
    kis_fill_interval_map_test.cpp
    kis_scanline_fill_test.cpp
    kis_psd_layer_style_test.cpp
    kis_layer_style_projection_plane_test.cpp
    kis_algebra_2d_test.cpp
    kis_marker_painter_test.cpp
    kis_lazy_brush_test.cpp
    kis_mask_similarity_test.cpp
    KisMaskGeneratorBenchmark.cpp
    kis_layer_style_filter_environment_test.cpp
    kis_asl_parser_test.cpp
    KisPerStrokeRandomSourceTest.cpp
    KisWatershedWorkerTest.cpp
    kis_dom_utils_test.cpp
    kis_transform_worker_test.cpp
    kis_cs_conversion_test.cpp
    kis_projection_leaf_test.cpp
    kis_histogram_test.cpp
    kis_onion_skin_compositor_test.cpp
    kis_queues_progress_updater_test.cpp
    kis_image_animation_interface_test.cpp
    kis_walkers_test.cpp
    kis_cage_transform_worker_test.cpp
    KisRandomGenerator2DTest.cpp
    kis_keyframing_test.cpp
    kis_filter_mask_test.cpp
    kis_asl_layer_style_serializer_test.cpp
    TestAslStorage.cpp
    kis_async_merger_test.cpp
    kis_selection_test.cpp
    kis_update_scheduler_test.cpp
    kis_colorize_mask_test.cpp
    kis_processings_test.cpp
    kis_paint_device_test.cpp
    kis_layer_styles_test.cpp
    kis_mesh_transform_worker_test.cpp
    KisKeyframeAnimationInterfaceSignalTest.cpp
    KisOverlayPaintDeviceWrapperTest.cpp
    KisPaintOpPresetTest.cpp
    KisFourPointInterpolatorTest.cpp
    KisSpatialContainerTest.cpp
    KisGridInterpolationToolsTest.cpp
    LINK_LIBRARIES kritaimage kritatestsdk kritaresourcestorage
    )

kis_add_test(
    KisTimeSpanContractTest.cpp
    LINK_LIBRARIES
        kritaimagetimespanvalueobjects
        kritaglobaldomutilsobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(KisTimeSpanContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisTimeSpanContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    kis_image_commands_test.cpp
    LINK_LIBRARIES
        kritaimagedisabledirtyrequestscommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(kis_image_commands_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands_new
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(kis_image_commands_test PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisUpdateCommandExContractTest.cpp
    LINK_LIBRARIES
        kritaimageupdatecommandexobjects
        kritaimageupdatecommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisUpdateCommandExContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands_new
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisUpdateCommandExContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    kis_lod_capable_layer_offset_test.cpp
    LINK_LIBRARIES
        kritaimagelodcapablelayeroffsetobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(kis_lod_capable_layer_offset_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(kis_lod_capable_layer_offset_test PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    kis_node_commands_test.cpp
    LINK_LIBRARIES
        kritaimagenodecommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(kis_node_commands_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(kis_node_commands_test PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisNodeRenameCommandContractTest.cpp
    LINK_LIBRARIES
        kritaimagenoderenamecommandobjects
        kritaimagenodecommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisNodeRenameCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands_new
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisNodeRenameCommandContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisNodeUuidInfoContractTest.cpp
    LINK_LIBRARIES
        kritaimagenodeuuidinfoobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisNodeUuidInfoContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisNodeUuidInfoContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisBatchNodeUpdateContractTest.cpp
    LINK_LIBRARIES
        kritaimagebatchnodeupdateobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisBatchNodeUpdateContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisBatchNodeUpdateContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisUpdateOutlineJobContractTest.cpp
    LINK_LIBRARIES
        kritaimageupdateoutlinejobobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisUpdateOutlineJobContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisUpdateOutlineJobContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisUpdateSelectionJobContractTest.cpp
    LINK_LIBRARIES
        kritaimageupdateselectionjobobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisUpdateSelectionJobContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisUpdateSelectionJobContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisDoSomethingCommandContractTest.cpp
    LINK_LIBRARIES
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisDoSomethingCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisDoSomethingCommandContractTest PRIVATE
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisMoveCommandCommonContractTest.cpp
    LINK_LIBRARIES
        kritaimageselectionmovecommand2objects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisMoveCommandCommonContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisMoveCommandCommonContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisLodTransformContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisLodTransformContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisLodTransformContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisTransactionBasedCommandContractTest.cpp
    LINK_LIBRARIES
        kritaimagetransactionbasedcommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisTransactionBasedCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands_new
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisTransactionBasedCommandContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisLayerCollapseCommandContractTest.cpp
    LINK_LIBRARIES
        kritaimagelayercollapsecommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)

kis_add_test(
    KisImageAnimSettingCommandContractTest.cpp
    LINK_LIBRARIES
        kritaimageanimsettingcommandobjects
        kritapaintingundokundo2coreobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisImageAnimSettingCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands_new
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisImageAnimSettingCommandContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)
target_include_directories(KisLayerCollapseCommandContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../commands_new
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisLayerCollapseCommandContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    kis_fast_math_test.cpp
    LINK_LIBRARIES kritaimagefastmathobjects Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(kis_fast_math_test PRIVATE kritaimage_EXPORTS)

kis_add_test(
    kis_name_server_test.cpp
    LINK_LIBRARIES
        kritaimagenameserverobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(kis_name_server_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(kis_name_server_test PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisPaintopLodLimitationsContractTest.cpp
    LINK_LIBRARIES
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPaintopLodLimitationsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisPaintopLodLimitationsContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisFilterStrategyContractTest.cpp
    LINK_LIBRARIES
        kritaimagefilterstrategyobjects
        kritaglobalidobjects
        kritaglobaldebugobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisFilterStrategyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisFilterStrategyContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisFilterCategoryIdsContractTest.cpp
    LINK_LIBRARIES
        kritaimagefiltercategoryidsobjects
        kritaglobalidobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(KisFilterCategoryIdsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../filter
    ${CMAKE_CURRENT_SOURCE_DIR}/../../global
    ${CMAKE_CURRENT_BINARY_DIR}/../../global
)
target_compile_definitions(KisFilterCategoryIdsContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    kis_vec_test.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Eigen3::Eigen
)
target_include_directories(kis_vec_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KisWraparoundAxisContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisWraparoundAxisContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KisTimingInformationContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisTimingInformationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisPaintopSettingsIdsContractTest.cpp
    LINK_LIBRARIES
        kritaimagepaintopsettingsidsobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisGreenCoordinatesContractTest.cpp
    LINK_LIBRARIES
        kritaimagegreencoordinatesobjects
        Qt${QT_MAJOR_VERSION}::Test
)

kis_add_test(
    KisConvexHullPointsContractTest.cpp
    LINK_LIBRARIES
        kritaimageconvexhullpointsobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisConvexHullPointsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisConvexHullPointsContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisConvexHullDeviceContractTest.cpp
    LINK_LIBRARIES
        kritaimageconvexhullpointsobjects
        kritaimageconvexhulldeviceobjects
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisConvexHullDeviceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisConvexHullDeviceContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisRandomGenerator2DContractTest.cpp
    LINK_LIBRARIES
        kritaimagerandomgenerator2dobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisRandomGenerator2DContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisNodeGraphListenerContractTest.cpp
    LINK_LIBRARIES
        kritaimagenodegraphlistenerobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Xml
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(KisNodeGraphListenerContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisNodeGraphListenerContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisSelectionTagsContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSelectionTagsContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KisImageSignalsContractTest.cpp
    LINK_LIBRARIES
        kritaimagesignalsobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisImageSignalsContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisImageInterfacesContractTest.cpp
    LINK_LIBRARIES
        kritaimageinterfacesobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisImageInterfacesContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisProjectionUpdatesFilterContractTest.cpp
    LINK_LIBRARIES
        kritaimageprojectionupdatesfilterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisProjectionUpdatesFilterContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisGradientShapeStrategyContractTest.cpp
    LINK_LIBRARIES
        kritaimagegradientshapestrategyobjects
        kritaimagecachedgradientshapestrategyobjects
        kritaimagepolygonalgradientshapestrategyobjects
        kritaimagebspline2dobjects
        kritaimageuniformbsplineengineobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisGradientShapeStrategyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsplines
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisGradientShapeStrategyContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisKeyframeContractTest.cpp
    LINK_LIBRARIES
        kritaimagekeyframeobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisKeyframeContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisKeyframeContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisKeyframeChannelSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisKeyframeChannelSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisKeyframeChannelSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisRasterKeyframeChannelSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisRasterKeyframeChannelSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisRasterKeyframeChannelSchemaContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
    kritapigment_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisSafeBlockingQueueConnectionProxyContractTest.cpp
    LINK_LIBRARIES
        kritaimagesafeblockingqueueconnectionproxyobjects
        kritaimagebusywaitbrokerobjects
        kritaglobalsignalcompressorobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSafeBlockingQueueConnectionProxyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(KisSafeBlockingQueueConnectionProxyContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisLevelsCurveContractTest.cpp
    LINK_LIBRARIES
        kritaimagelevelscurveobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisLevelsCurveContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisLevelsCurveContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisCubicCurveContractTest.cpp
    LINK_LIBRARIES
        kritaimagecubiccurveobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
        Eigen3::Eigen
)
target_include_directories(KisCubicCurveContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisCubicCurveContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisSpatialContainerContractTest.cpp
    LINK_LIBRARIES
        kritaimagespatialcontainerobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSpatialContainerContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisSpatialContainerContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisOptimizedByteArrayContractTest.cpp
    LINK_LIBRARIES
        kritaimageoptimizedbytearrayobjects
        kritaglobalrollingmeanobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisOptimizedByteArrayContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisOptimizedByteArrayContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisRenderedDabContractTest.cpp
    LINK_LIBRARIES
        kritaimageoptimizedbytearrayobjects
        kritaglobalrollingmeanobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisRenderedDabContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisRenderedDabContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisSpacingInformationContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSpacingInformationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    kis_clone_layer_test.cpp
    LINK_LIBRARIES kritaimage kritatestsdk kritaglobal kritatransformmaskstubs
    )

kis_add_test(
    kis_transform_mask_test.cpp
    LINK_LIBRARIES kritaimage kritatestsdk kritaglobal kritatransformmaskstubs
    )

kis_add_test(
    kis_node_commands_adapter_test.cpp
    TEST_NAME KisNodeCommandsAdapterTest
    LINK_LIBRARIES kritaimage kritatestsdk
    )

kis_add_test(
    kis_node_operation_batch_test.cpp ../../../sdk/tests/testutil.cpp
    TEST_NAME KisNodeOperationBatchTest
    LINK_LIBRARIES kritaimage kritatestsdk
    )

kis_add_test(
    TestBuiltInResourceLoaderRegistration.cpp
    TEST_NAME TestBuiltInResourceLoaderRegistration
    LINK_LIBRARIES kritaimage kritalibbrush kritatestsdk
    )

kis_add_test(
    KisRunnableContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisRunnableContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisImageBarrierLockContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisImageBarrierLockContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisImageBarrierLockContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisBaseRectsWalkerPolicyContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisBaseRectsWalkerPolicyContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisBaseRectsWalkerPolicyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)

kis_add_test(
    KisSpontaneousJobContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSpontaneousJobContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisSpontaneousJobContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisNodeVisitorContractTest.cpp
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisNodeVisitorContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisNodeVisitorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisDefaultBoundsBaseContractTest.cpp
    LINK_LIBRARIES
        kritaimagedefaultboundsbaseobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisDefaultBoundsBaseContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisDefaultBoundsBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisPerspectiveMathContractTest.cpp
    LINK_LIBRARIES
        kritaimageperspectivemathobjects
        Eigen3::Eigen
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisPerspectiveMathContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisPerspectiveMathContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisConvolutionKernelContractTest.cpp
    LINK_LIBRARIES
        kritaimageconvolutionkernelobjects
        kritaglobalsharedobjects
        Eigen3::Eigen
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_compile_definitions(KisConvolutionKernelContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisConvolutionKernelContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisSafeTransformContractTest.cpp
    LINK_LIBRARIES
        kritaimagesafetransformobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_compile_definitions(KisSafeTransformContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisSafeTransformContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisOptimizedBrushOutlineContractTest.cpp
    LINK_LIBRARIES
        kritaimageoptimizedbrushoutlineobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_compile_definitions(KisOptimizedBrushOutlineContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisOptimizedBrushOutlineContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisBSpline1DContractTest.cpp
    LINK_LIBRARIES
        kritaimagebspline1dobjects
        kritaimageuniformbsplineengineobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_compile_definitions(KisBSpline1DContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisBSpline1DContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisBSpline2DContractTest.cpp
    LINK_LIBRARIES
        kritaimagebspline2dobjects
        kritaimageuniformbsplineengineobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_compile_definitions(KisBSpline2DContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisBSpline2DContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsplines
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisNUBSpline2DContractTest.cpp
    LINK_LIBRARIES
        kritaimagenubspline2dobjects
        kritaimagenonuniformbsplineengineobjects
        kritaimageuniformbsplineengineobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisNUBSpline2DContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)
target_include_directories(KisNUBSpline2DContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisPaintOpPresetUpdateProxyContractTest.cpp
    LINK_LIBRARIES
        kritaimagepaintoppresetupdateproxyobjects
        kritaglobalsignalcompressorobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPaintOpPresetUpdateProxyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../brushengine
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisPaintOpPresetUpdateProxyContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisImageConfigNotifierContractTest.cpp
    LINK_LIBRARIES
        kritaimageconfignotifierobjects
        kritaglobalsignalcompressorobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisImageConfigNotifierContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisImageConfigNotifierContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisSelectionComponentContractTest.cpp
    LINK_LIBRARIES
        kritaimageselectioncomponentobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisSelectionComponentContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisSelectionComponentContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisProcessingInformationContractTest.cpp
    LINK_LIBRARIES
        kritaimageprocessinginformationobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisProcessingInformationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisProcessingInformationContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisNodeFilterInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritaimagenodefilterinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisNodeFilterInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisNodeFilterInterfaceContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisAbstractProjectionPlaneContractTest.cpp
    LINK_LIBRARIES
        kritaimageprojectionplaneobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisAbstractProjectionPlaneContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisAbstractProjectionPlaneContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisImageTypesContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisImageTypesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Core5Compat,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisImageTypesContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisProcessingApplicatorSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisProcessingApplicatorSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisProcessingApplicatorSchemaContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisOverlayPaintDeviceWrapperSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisOverlayPaintDeviceWrapperSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisOverlayPaintDeviceWrapperSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritaglobal_EXPORTS
)

kis_add_test(
    KisStrokesQueueSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisStrokesQueueSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisStrokesQueueSchemaContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisUpdateSchedulerSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisUpdateSchedulerSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisUpdateSchedulerSchemaContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisImageSharedPointerHooksSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisImageSharedPointerHooksSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisImageSharedPointerHooksSchemaContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisStrokeStrategyUndoCommandBasedSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisStrokeStrategyUndoCommandBasedSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Gui,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisStrokeStrategyUndoCommandBasedSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisLayerPropertiesIconsSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisLayerPropertiesIconsSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:Qt${QT_MAJOR_VERSION}::Widgets,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisLayerPropertiesIconsSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisPaintLayerSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisPaintLayerSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisPaintLayerSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisGradientPainterSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisGradientPainterSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisGradientPainterSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisEncloseAndFillPainterSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisEncloseAndFillPainterSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/image/filter
    ${CMAKE_BINARY_DIR}/libs/image/filter
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisEncloseAndFillPainterSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisPaintOpPresetSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisPaintOpPresetSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/image/brushengine
    ${CMAKE_BINARY_DIR}/libs/image/brushengine
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisPaintOpPresetSchemaContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisPSDLayerStyleSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisPSDLayerStyleSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisPSDLayerStyleSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisMaskSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisMaskSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisMaskSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisGeneratorLayerSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisGeneratorLayerSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/psdutils
    ${CMAKE_BINARY_DIR}/libs/psdutils
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisGeneratorLayerSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisTransformMaskSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisTransformMaskSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisTransformMaskSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisColorizeMaskSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Qt${QT_MAJOR_VERSION}::Xml
        Boost::boost
)
target_include_directories(KisColorizeMaskSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    ${CMAKE_SOURCE_DIR}/libs/pigment/resources
    ${CMAKE_BINARY_DIR}/libs/pigment/resources
    ${CMAKE_SOURCE_DIR}/libs/resources
    ${CMAKE_BINARY_DIR}/libs/resources
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisColorizeMaskSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisImageConfigAnimationSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisImageConfigAnimationSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:KF${KF_MAJOR}::ConfigCore,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisImageConfigAnimationSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisImageConfigResourceBudgetSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisImageConfigResourceBudgetSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:KF${KF_MAJOR}::ConfigCore,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisImageConfigResourceBudgetSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisImageConfigGeneralSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisImageConfigGeneralSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:KF${KF_MAJOR}::ConfigCore,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisImageConfigGeneralSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisPropertiesConfigurationSchemaContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisPropertiesConfigurationSchemaContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisPropertiesConfigurationSchemaContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisProofingConfigurationContractTest.cpp
    LINK_LIBRARIES
        kritaimageproofingconfigurationobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisProofingConfigurationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
    $<TARGET_PROPERTY:KF${KF_MAJOR}::I18n,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:Imath::Imath,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(KisProofingConfigurationContractTest PRIVATE
    kritaimage_EXPORTS
    kritapigment_EXPORTS
)

kis_add_test(
    KisCropSavedExtraDataContractTest.cpp
    LINK_LIBRARIES
        kritaimagecropsavedextradataobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisCropSavedExtraDataContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisCropSavedExtraDataContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisAnnotationContractTest.cpp
    LINK_LIBRARIES
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisAnnotationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisAnnotationContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisBaseAccessorContractTest.cpp
    LINK_LIBRARIES
        kritaimagebaseaccessorobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisBaseAccessorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisBaseAccessorContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisRandomAccessorNGContractTest.cpp
    LINK_LIBRARIES
        kritaimagerandomaccessorobjects
        kritaimagebaseaccessorobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisRandomAccessorNGContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisRandomAccessorNGContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisRandomSubAccessorContractTest.cpp
    LINK_LIBRARIES
        kritaimagerandomsubaccessorobjects
        kritaimagerandomaccessorobjects
        kritaimagebaseaccessorobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisRandomSubAccessorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisRandomSubAccessorContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisPaintopPropertiesBaseContractTest.cpp
    LINK_LIBRARIES
        kritaimagepaintoppropertiesbaseobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPaintopPropertiesBaseContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisPaintopPropertiesBaseContractTest PRIVATE
    kritaimage_EXPORTS
)

kis_add_test(
    KisLayerStyleFilterContractTest.cpp
    LINK_LIBRARIES
        kritaimagelayerstylefilterobjects
        kritaglobalidobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        KF${KF_MAJOR}::I18n
        Boost::boost
)
target_include_directories(KisLayerStyleFilterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../layerstyles
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisLayerStyleFilterContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisSerializableConfigurationContractTest.cpp
    LINK_LIBRARIES
        kritaimageserializableconfigurationobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSerializableConfigurationContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisSerializableConfigurationContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisTransformMaskParamsInterfaceContractTest.cpp
    LINK_LIBRARIES
        kritaimagetransformmaskparamsinterfaceobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Xml
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisTransformMaskParamsInterfaceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisTransformMaskParamsInterfaceContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisWrappedRectContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisWrappedRectContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

kis_add_test(
    KisPaintDeviceWriterContractTest.cpp
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPaintDeviceWriterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisPaintDeviceWriterContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisStrokeStrategyContractTest.cpp
    ../../painting/undo/kundo2magicstring.cpp
    TEST_NAME KisStrokeStrategyContractTest
    LINK_LIBRARIES
        kritaimagestrokestrategyobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisStrokeStrategyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisStrokeStrategyContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisStrokeJobContractTest.cpp
    LINK_LIBRARIES
        kritaimagejobstrategyobjects
        kritaimagestrokejobobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisStrokeJobContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

kis_add_test(
    KisSimpleStrokeStrategyContractTest.cpp
    ../../painting/undo/kundo2magicstring.cpp
    TEST_NAME KisSimpleStrokeStrategyContractTest
    LINK_LIBRARIES
        kritaimagestrokestrategyobjects
        kritaimagejobstrategyobjects
        kritaimagesimplestrokestrategyobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisSimpleStrokeStrategyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisSimpleStrokeStrategyContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisRunnableBasedStrokeStrategyContractTest.cpp
    ../../painting/undo/kundo2magicstring.cpp
    TEST_NAME KisRunnableBasedStrokeStrategyContractTest
    LINK_LIBRARIES
        kritaimagestrokestrategyobjects
        kritaimagejobstrategyobjects
        kritaimagesimplestrokestrategyobjects
        kritaimagerunnablebasedstrokestrategyobjects
        KF${KF_MAJOR}::I18n
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisRunnableBasedStrokeStrategyContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisRunnableBasedStrokeStrategyContractTest PRIVATE
    kritaimage_EXPORTS
    kritapaintingundo_EXPORTS
)

kis_add_test(
    KisBehaviorInterfacesContractTest.cpp
    LINK_LIBRARIES
        kritaimagebehaviorinterfaceobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisBehaviorInterfacesContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisBehaviorInterfacesContractTest PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)

kis_add_test(
    KisBlockBackgroundFrameGenerationLockContractTest.cpp
    LINK_LIBRARIES
        kritaimagebackgroundframegenerationlockobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KisBlockBackgroundFrameGenerationLockContractTest PRIVATE
    ../kis_image_animation_interface.h
)
target_compile_definitions(KisBlockBackgroundFrameGenerationLockContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisBlockBackgroundFrameGenerationLockContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisLockFrameGenerationLockContractTest.cpp
    LINK_LIBRARIES
        kritaimageframegenerationlockobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KisLockFrameGenerationLockContractTest PRIVATE
    ../kis_image_animation_interface.h
)
target_compile_definitions(KisLockFrameGenerationLockContractTest PRIVATE kritaimage_EXPORTS)
target_include_directories(KisLockFrameGenerationLockContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)

kis_add_test(
    KisProgressUpdateHelperContractTest.cpp
    ../../widgetutils/KoProgressProxy.cpp
    ../../widgetutils/KoUpdater.cpp
    ../../widgetutils/KoUpdaterPrivate_p.cpp
    TEST_NAME KisProgressUpdateHelperContractTest
    LINK_LIBRARIES
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisProgressUpdateHelperContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(KisProgressUpdateHelperContractTest PRIVATE
    kritaimage_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisSequentialIteratorProgressContractTest.cpp
    LINK_LIBRARIES
        kritawidgetutilsfakeprogressproxyobjects
        kritawidgetutilsprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisSequentialIteratorProgressContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
    ${CMAKE_SOURCE_DIR}/libs/pigment
    ${CMAKE_BINARY_DIR}/libs/pigment
)
target_compile_definitions(KisSequentialIteratorProgressContractTest PRIVATE
    kritaimage_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisCompositeProgressProxyContractTest.cpp
    LINK_LIBRARIES
        kritaimagecompositeprogressproxyobjects
        kritawidgetutilsprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_compile_definitions(KisCompositeProgressProxyContractTest PRIVATE
    kritaimage_EXPORTS
    kritawidgetutils_EXPORTS
)

kis_add_test(
    KisPerStrokeRandomSourceContractTest.cpp
    LINK_LIBRARIES
        kritaimageperstrokerandomsourceobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisPerStrokeRandomSourceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../brushengine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../global
    ${CMAKE_CURRENT_BINARY_DIR}/../../global
)
target_compile_definitions(KisPerStrokeRandomSourceContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisStrokeRandomSourceContractTest.cpp
    LINK_LIBRARIES
        kritaimagestrokerandomsourceobjects
        kritaimageperstrokerandomsourceobjects
        kritaglobalrandomsourceobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisStrokeRandomSourceContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../brushengine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../global
    ${CMAKE_CURRENT_BINARY_DIR}/../../global
)
target_compile_definitions(KisStrokeRandomSourceContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisStrokeSpeedMeasurerContractTest.cpp
    LINK_LIBRARIES
        kritaimagestrokespeedmeasurerobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisStrokeSpeedMeasurerContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../brushengine
    ${CMAKE_CURRENT_SOURCE_DIR}/../../global
    ${CMAKE_CURRENT_BINARY_DIR}/../../global
)
target_compile_definitions(KisStrokeSpeedMeasurerContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisFillIntervalMapContractTest.cpp
    LINK_LIBRARIES
        kritaimagefillintervalmapobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
        Boost::boost
)
target_include_directories(KisFillIntervalMapContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../floodfill
    ${CMAKE_CURRENT_SOURCE_DIR}/../../global
    ${CMAKE_CURRENT_BINARY_DIR}/../../global
)
target_compile_definitions(KisFillIntervalMapContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisPostExecutionUndoAdapterContractTest.cpp
    LINK_LIBRARIES
        kritaimagepostexecutionundoadapterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisPostExecutionUndoAdapterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)
target_compile_definitions(KisPostExecutionUndoAdapterContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisDoNothingProcessingVisitorContractTest.cpp
    LINK_LIBRARIES
        kritaimagedonothingprocessingvisitorobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisDoNothingProcessingVisitorContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisDoNothingProcessingVisitorContractTest PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    KisQueuesProgressUpdaterContractTest.cpp
    LINK_LIBRARIES
        kritaimagequeuesprogressupdaterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KisQueuesProgressUpdaterContractTest PRIVATE ../kis_queues_progress_updater.h)
target_include_directories(KisQueuesProgressUpdaterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/widgetutils
    ${CMAKE_BINARY_DIR}/libs/widgetutils
)
target_compile_definitions(KisQueuesProgressUpdaterContractTest PRIVATE kritaimage_EXPORTS)

kis_add_test(
    KisUndoAdapterContractTest.cpp
    LINK_LIBRARIES
        kritaimageundoadapterobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Test
)
target_sources(KisUndoAdapterContractTest PRIVATE ../kis_undo_adapter.h)
target_include_directories(KisUndoAdapterContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/painting/undo
    ${CMAKE_BINARY_DIR}/libs/painting/undo
)
target_compile_definitions(KisUndoAdapterContractTest PRIVATE kritaimage_EXPORTS)


include_directories(${QUAZIP_INCLUDE_DIRS})

kis_add_test(
    kis_liquify_transform_worker_test.cpp
    LINK_LIBRARIES kritaimage kritatestsdk ${QUAZIP_LIBRARIES}
    )

krita_add_broken_unit_tests(
    kis_perspective_transform_worker_test.cpp
    LINK_LIBRARIES kritaimage kritatestsdk
    )

if(APPLE)
    set_property(TARGET kis_image_animation_interface_test PROPERTY MACOSX_BUNDLE ON)
    set_property(TARGET kis_cage_transform_worker_test PROPERTY MACOSX_BUNDLE ON)

    macos_test_fixrpath(KisRandomGenerator2DDemo)
endif()
