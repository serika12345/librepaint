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
    kis_image_commands_test.cpp
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
    kis_time_span_test.cpp
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
    kis_lod_capable_layer_offset_test.cpp
    LINK_LIBRARIES
        kritaimagelodcapablelayeroffsetobjects
        kritaglobalsharedobjects
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(kis_lod_capable_layer_offset_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(kis_lod_capable_layer_offset_test PRIVATE
    kritaglobal_EXPORTS
    kritaimage_EXPORTS
)

kis_add_test(
    kis_node_commands_test.cpp
    LINK_LIBRARIES
        kritaimagenodecommandobjects
        kritapaintingundo
        Qt${QT_MAJOR_VERSION}::Test
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
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisImageTypesContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/libs/global
    ${CMAKE_BINARY_DIR}/libs/global
)
target_compile_definitions(KisImageTypesContractTest PRIVATE kritaimage_EXPORTS)

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
    KisStrokeJobContractTest.cpp
    LINK_LIBRARIES
        kritaimagestrokejobobjects
        Qt${QT_MAJOR_VERSION}::Test
)
target_include_directories(KisStrokeJobContractTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
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
