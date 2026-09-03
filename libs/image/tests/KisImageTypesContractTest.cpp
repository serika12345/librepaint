/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paint_information.h"
#include "brushengine/kis_paintop_settings.h"
#include "brushengine/kis_slider_based_paintop_property.h"
#include "brushengine/kis_uniform_paintop_property.h"
#include "commands_new/kis_saved_commands.h"
#include "kis_base_mask_generator.h"
#include "kis_brush_mask_applicator_base.h"
#include "kis_circle_mask_generator.h"
#include "kis_clone_layer.h"
#include "kis_curve_circle_mask_generator.h"
#include "kis_curve_rect_mask_generator.h"
#include "kis_datamanager.h"
#include "kis_default_bounds.h"
#include "kis_distance_information.h"
#include "kis_edge_detection_kernel.h"
#include "kis_fixed_paint_device.h"
#include "kis_gauss_circle_mask_generator.h"
#include "kis_gauss_rect_mask_generator.h"
#include "kis_group_layer.h"
#include "kis_histogram.h"
#include "kis_image_animation_interface.h"
#include "kis_image_signal_router.h"
#include "kis_indirect_painting_support.h"
#include "kis_iterator_ng.h"
#include "kis_layer_utils.h"
#include "kis_liquify_transform_worker.h"
#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#endif
#include "kis_math_toolbox.h"
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif
#include "kis_memory_statistics_server.h"
#include "kis_paint_device_frames_interface.h"
#include "kis_pixel_selection.h"
#include "kis_processing_visitor.h"
#include "kis_projection_leaf.h"
#include "kis_rect_mask_generator.h"
#include "kis_selection_based_layer.h"
#include "kis_selection_filters.h"
#include "kis_selection_mask.h"
#include "kis_sequential_iterator.h"
#include "kis_simple_update_queue.h"
#include "kis_stroke.h"
#include "kis_types.h"
#include "kis_update_job_item.h"
#include "kis_updater_context.h"
#include "kis_warptransform_worker.h"
#include "tiles3/kis_hline_iterator.h"
#include "tiles3/kis_vline_iterator.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_DEFAULT_BOUNDS_SIGNATURE(boundsType, method, signature)                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&boundsType::method)), signature>)
#define ASSERT_LAYER_UTILS_SIGNATURE(function, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerUtils::function)), signature>)
#define ASSERT_PAINTOP_SETTINGS_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintOpSettings::method)), signature>)
#define ASSERT_GROUP_LAYER_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGroupLayer::method)), signature>)
#define ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisFixedPaintDevice::method)), signature>)
#define ASSERT_DATA_MANAGER_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDataManager::method)), signature>)
#define ASSERT_SAVED_COMMAND_SIGNATURE(type, method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
#define ASSERT_PIXEL_SELECTION_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPixelSelection::method)), signature>)
#define ASSERT_MASK_GENERATOR_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMaskGenerator::method)), signature>)
#define ASSERT_GAUSSIAN_MASK_SIGNATURE(type, method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
#define ASSERT_DEFAULT_MASK_SIGNATURE(type, method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
#define ASSERT_CURVE_MASK_SIGNATURE(type, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
#define ASSERT_MATH_TOOLBOX_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMathToolbox::method)), signature>)
#define ASSERT_UPDATER_CONTEXT_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisUpdaterContext::method)), signature>)
#define ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSimpleUpdateQueue::method)), signature>)
#define ASSERT_ITERATOR_SIGNATURE(type, method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
#define ASSERT_EDGE_DETECTION_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisEdgeDetectionKernel::method)), signature>)
#define ASSERT_SELECTION_MASK_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSelectionMask::method)), signature>)
#define ASSERT_INDIRECT_PAINTING_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisIndirectPaintingSupport::method)), signature>)
#define ASSERT_SELECTION_BASED_LAYER_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSelectionBasedLayer::method)), signature>)
#define ASSERT_CLONE_LAYER_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCloneLayer::method)), signature>)
#define ASSERT_PROCESSING_VISITOR_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisProcessingVisitor::method)), signature>)

class BaseConstIteratorConstructorProbe final : public KisBaseConstIteratorNG
{
public:
    const quint8 *oldRawData() const override;
    const quint8 *rawDataConst() const override;
    qint32 x() const override;
    qint32 y() const override;
    bool nextPixel() override;
    bool nextPixels(qint32 n) override;
    qint32 nConseqPixels() const override;
};

class HLineConstIteratorConstructorProbe final : public KisHLineConstIteratorNG
{
public:
    const quint8 *oldRawData() const override;
    const quint8 *rawDataConst() const override;
    qint32 x() const override;
    qint32 y() const override;
    bool nextPixel() override;
    bool nextPixels(qint32 n) override;
    qint32 nConseqPixels() const override;
    void nextRow() override;
    void resetPixelPos() override;
    void resetRowPos() override;
};

class HLineIteratorConstructorProbe final : public KisHLineIteratorNG
{
public:
    const quint8 *oldRawData() const override;
    const quint8 *rawDataConst() const override;
    quint8 *rawData() override;
    qint32 x() const override;
    qint32 y() const override;
    bool nextPixel() override;
    bool nextPixels(qint32 n) override;
    qint32 nConseqPixels() const override;
    void nextRow() override;
    void resetPixelPos() override;
    void resetRowPos() override;
};

class VLineConstIteratorConstructorProbe final : public KisVLineConstIteratorNG
{
public:
    const quint8 *oldRawData() const override;
    const quint8 *rawDataConst() const override;
    qint32 x() const override;
    qint32 y() const override;
    bool nextPixel() override;
    bool nextPixels(qint32 n) override;
    qint32 nConseqPixels() const override;
    void nextColumn() override;
    void resetColumnPos() override;
    void resetPixelPos() override;
};

class VLineIteratorConstructorProbe final : public KisVLineIteratorNG
{
public:
    const quint8 *oldRawData() const override;
    const quint8 *rawDataConst() const override;
    quint8 *rawData() override;
    qint32 x() const override;
    qint32 y() const override;
    bool nextPixel() override;
    bool nextPixels(qint32 n) override;
    qint32 nConseqPixels() const override;
    void nextColumn() override;
    void resetColumnPos() override;
    void resetPixelPos() override;
};

class SelectionDefaultBoundsConstructorProbe final : public KisSelectionDefaultBoundsBase
{
public:
    SelectionDefaultBoundsConstructorProbe() = default;

protected:
    KisPaintDeviceSP parentPaintDevice() const override;
};

class PaintDeviceConvertible
{
public:
    operator KisPaintDeviceSP() const;
};

class MaskGeneratorConstructorProbe final : public KisMaskGenerator
{
public:
    using KisMaskGenerator::KisMaskGenerator;
    explicit MaskGeneratorConstructorProbe(const KisMaskGenerator &rhs)
        : KisMaskGenerator(rhs)
    {
    }

    KisMaskGenerator *clone() const override
    {
        return nullptr;
    }
    quint8 valueAt(qreal, qreal) const override
    {
        return 0;
    }
    KisBrushMaskApplicatorBase *applicator() const override
    {
        return nullptr;
    }
};

class SavedCommandBaseConstructorProbe final : public KisSavedCommandBase
{
public:
    using KisSavedCommandBase::KisSavedCommandBase;

protected:
    void addCommands(KisStrokeId id, bool undo) override;
};

class SelectionBasedLayerConstructorProbe final : public KisSelectionBasedLayer
{
public:
    using KisSelectionBasedLayer::KisSelectionBasedLayer;
    SelectionBasedLayerConstructorProbe(const SelectionBasedLayerConstructorProbe &) = default;

    bool accept(KisNodeVisitor &) override;
    void accept(KisProcessingVisitor &, KisUndoAdapter *) override;
    KisNodeSP clone() const override;
    QIcon icon() const override;
    KisBaseNode::PropertyList sectionModelProperties() const override;
};

class SavedCommandUnwrapCallable
{
public:
    KUndo2Command *operator()(KUndo2Command *command) const;
};

class PaintOpSignatureProbe final : public KisPaintOpSettings
{
public:
    using KisPaintOpSettings::KisPaintOpSettings;

    void setPaintOpSize(qreal value) override;
    qreal paintOpSize() const override;
    void setPaintOpAngle(qreal value) override;
    qreal paintOpAngle() const override;
};

} // namespace

class KisImageTypesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void intrusivePointerAliasesPreserveOwnershipKinds();
    void qtPointerAliasesPreserveOwnershipKinds();
    void collectionAliasesPreserveElementAndIteratorTypes();
    void thumbnailBoundsModesRemainDistinct();
    void selectionFilterBaseInterfaceSchemaRemainsStable();
    void morphologicalSelectionFilterSchemaRemainsStable();
    void invertAndAntiAliasSelectionFilterSchemaRemainsStable();
    void borderAndFeatherSelectionFilterSchemaRemainsStable();
    void growthSelectionFilterPolicySchemaRemainsStable();
    void defaultBoundsOwnershipAndHierarchySchemaRemainsStable();
    void imageDefaultBoundsSignaturesRemainStable();
    void selectionDefaultBoundsSignaturesRemainStable();
    void emptySelectionBoundsSignaturesRemainStable();
    void wrapAroundBoundsSignaturesRemainStable();
    void paintInputPositionAndSensorSignaturesRemainStable();
    void paintInputStrokeProgressSignaturesRemainStable();
    void paintInputCanvasOrientationSignaturesRemainStable();
    void paintInputDerivedMotionSignaturesRemainStable();
    void paintInputTiltConversionSignaturesRemainStable();
    void paintInformationConstructionAndLifetimeSchemaRemainsStable();
    void paintInformationDistanceRegistrarSchemaRemainsStable();
    void paintInformationMixingSignaturesRemainStable();
    void paintInformationRandomDistanceAndLodSignaturesRemainStable();
    void paintInformationHoverPaintSerializationAndDebugSignaturesRemainStable();
    void distanceStateLifecycleSchemaRemainsStable();
    void distanceLastDabObservationSignaturesRemainStable();
    void distanceSpacingAndTimingSignaturesRemainStable();
    void distanceStrokeProgressSignaturesRemainStable();
    void distanceDrawingAngleLockSignaturesRemainStable();
    void histogramTypeAndLifecycleSchemaRemainStable();
    void histogramCalculationValueSchemaRemainsStable();
    void histogramComputationAndResultSignaturesRemainStable();
    void histogramTypeProducerAndChannelSignaturesRemainStable();
    void histogramSelectionSignaturesRemainStable();
    void strokeTypeAndLifecycleSchemaRemainStable();
    void strokeJobQueueOperationSignaturesRemainStable();
    void strokeIdentityAndLifecycleTransitionSignaturesRemainStable();
    void strokeStateAndStrategyPolicySignaturesRemainStable();
    void strokeLodAndBalancingSignaturesRemainStable();
    void sequentialIteratorTypeAndAliasSchemaRemainsStable();
    void sequentialIteratorDevicePolicySchemaRemainsStable();
    void sequentialIteratorAccessPolicySchemaRemainsStable();
    void sequentialIteratorNoProgressPolicySchemaRemainsStable();
    void sequentialIteratorTraversalSchemaRemainsStable();
    void layerMergePolicyAndOrderingSignaturesRemainStable();
    void layerMutationAndProjectionCommandSignaturesRemainStable();
    void layerHierarchySearchAndTraversalSignaturesRemainStable();
    void layerFrameQueryAndJobSchemaRemainStable();
    void layerCommandTypesAndLifecycleSchemaRemainStable();
    void uniformPaintOpPropertyOwnershipSchemaRemainsStable();
    void uniformPaintOpPropertyTypeSchemaRemainsStable();
    void uniformPaintOpPropertyLifetimeSchemaRemainsStable();
    void uniformPaintOpPropertyIdentityAndSettingsSignaturesRemainStable();
    void uniformPaintOpPropertyValueAndNotificationSignaturesRemainStable();
    void sliderPaintOpPropertyTypeAndAliasSchemaRemainStable();
    void sliderPaintOpPropertyConstructionSchemaRemainsStable();
    void sliderPaintOpPropertyRangeAndStepSignaturesRemainStable();
    void sliderPaintOpPropertyDisplaySignaturesRemainStable();
    void sliderPaintOpPropertyRangeNotificationSignatureRemainsStable();
    void memoryStatisticsServerTypeAndLifetimeSchemaRemainStable();
    void memoryStatisticsImageAndAggregateDefaultsRemainStable();
    void memoryStatisticsSwapAndLimitDefaultsRemainStable();
    void memoryStatisticsCollectionSignaturesRemainStable();
    void memoryStatisticsUpdateNotificationSignaturesRemainStable();
    void paintDeviceFrameOwnershipAndTestingDataSchemaRemainsStable();
    void paintDeviceFrameLifecycleAndIdentitySignaturesRemainStable();
    void paintDeviceFrameGeometryAndPixelSignaturesRemainStable();
    void paintDeviceFrameTransferAndPersistenceSignaturesRemainStable();
    void paintDeviceFrameUndoCacheAndTestingSignaturesRemainStable();
    void imageSignalRouterOwnershipAndLifetimeSchemaRemainsStable();
    void imageSignalRouterNotificationSignaturesRemainStable();
    void imageSignalRouterBatchAndLodSignaturesRemainStable();
    void imageSignalRouterNodeGraphSignaturesRemainStable();
    void imageSignalRouterImageMetadataSignaturesRemainStable();
    void projectionLeafOwnershipDropReasonAndLifetimeSchemaRemainsStable();
    void projectionLeafGraphNavigationSignaturesRemainStable();
    void projectionLeafVisitorAndProjectionAccessSignaturesRemainStable();
    void projectionLeafNodeClassificationAndRenderingSignaturesRemainStable();
    void projectionLeafDropOverlayAndTemporaryVisibilitySignaturesRemainStable();
    void animationInterfaceTypeAndOptionSchemaRemainsStable();
    void animationTimelineAndExportSignaturesRemainStable();
    void animationSwitchAndInvalidationSignaturesRemainStable();
    void animationGenerationLockSignaturesRemainStable();
    void animationNotificationSignaturesRemainStable();
    void paintOpUpdatePropertyKeySchemaRemainsStable();
    void paintOpCadenceAndLodPolicySignaturesRemainStable();
    void paintOpDepositParameterSignaturesRemainStable();
    void paintOpGeometryCompositeAndEraserSignaturesRemainStable();
    void paintOpSavedBrushAndEraserValueSignaturesRemainStable();
    void paintOpSettingsTypeUpdateListenerAndLifetimeSchemaRemainStable();
    void paintOpSettingsInteractionAndOutlineSignaturesRemainStable();
    void paintOpSettingsCompositionAndValiditySignaturesRemainStable();
    void paintOpSettingsResourceAndCanvasSignaturesRemainStable();
    void paintOpSettingsMaskingLodAndUniformPropertySignaturesRemainStable();
    void fixedPaintDeviceOwnershipAndCopySchemaRemainsStable();
    void fixedPaintDeviceGeometryCapacityAndColorSchemaRemainsStable();
    void fixedPaintDeviceBufferStorageSignaturesRemainStable();
    void fixedPaintDeviceTransferAndConversionSignaturesRemainStable();
    void fixedPaintDevicePixelMutationSignaturesRemainStable();
    void groupLayerOwnershipLifetimeAndHierarchySchemaRemainsStable();
    void groupLayerImageCoordinateAndDeviceSignaturesRemainStable();
    void groupLayerCompositionAndVisitorSignaturesRemainStable();
    void groupLayerPresentationAndPolicySignaturesRemainStable();
    void groupLayerChildrenBoundsSignaturesRemainStable();
    void dataManagerOwnershipAndDefaultPixelSchemaRemainsStable();
    void dataManagerExtentRegionAndContiguitySignaturesRemainStable();
    void dataManagerPixelTransferAndMutationSignaturesRemainStable();
    void dataManagerCopyAndHistorySignaturesRemainStable();
    void dataManagerPersistencePurgeAndPoolSignaturesRemainStable();
    void savedCommandHierarchyAndLifetimeSchemaRemainsStable();
    void savedCommandTimingAndIdentitySignaturesRemainStable();
    void savedCommandMergeAndUnwrapSignaturesRemainStable();
    void savedMacroCommandCompositionAndIdentitySignaturesRemainStable();
    void savedMacroCommandExecutionAndOverrideSignaturesRemainStable();
    void pixelSelectionOwnershipCopyAndLifetimeSchemaRemainsStable();
    void pixelSelectionMutationSignaturesRemainStable();
    void pixelSelectionGeometryAndOutlineCacheSignaturesRemainStable();
    void pixelSelectionParentCloneAndProjectionSignaturesRemainStable();
    void pixelSelectionThumbnailAndPersistenceSignaturesRemainStable();
    void maskGeneratorIdentityLifetimeAndConstantsSchemaRemainStable();
    void maskGeneratorGeometrySignaturesRemainStable();
    void maskGeneratorAppearanceSignaturesRemainStable();
    void maskGeneratorRenderingPolicySignaturesRemainStable();
    void maskGeneratorSerializationAndRegistrySignaturesRemainStable();
    void gaussianMaskTypeAndLifetimeSchemaRemainStable();
    void gaussianMaskCloneSignaturesRemainStable();
    void gaussianMaskSamplingSignaturesRemainStable();
    void gaussianMaskScaleAndVectorizationSignaturesRemainStable();
    void gaussianMaskApplicatorSignaturesRemainStable();
    void defaultMaskTypeAndLifetimeSchemaRemainStable();
    void defaultMaskCloneAndSamplingSignaturesRemainStable();
    void defaultMaskScaleAndSoftnessSignaturesRemainStable();
    void defaultMaskVectorizationSignaturesRemainStable();
    void defaultMaskApplicatorSignaturesRemainStable();
    void curveMaskTypeAndLifetimeSchemaRemainStable();
    void curveMaskCloneAndSamplingSignaturesRemainStable();
    void curveMaskScaleAndSoftnessSignaturesRemainStable();
    void curveMaskVectorizationAndApplicatorSignaturesRemainStable();
    void curveMaskSerializationAndTransformationSignaturesRemainStable();
    void mathToolboxTypeAndAliasSchemaRemainStable();
    void mathToolboxFloatRepresentationSchemaRemainStable();
    void mathToolboxWaveletPlanningSignaturesRemainStable();
    void mathToolboxWaveletTransformSignaturesRemainStable();
    void mathToolboxChannelConversionSignaturesRemainStable();
    void maskDataConstructionSchemaRemainStable();
    void maskDataDeviceAndColorSchemaRemainStable();
    void maskDataGeometrySchemaRemainStable();
    void brushMaskApplicatorSchemaRemainStable();
    void brushMaskOperatorSchemaRemainStable();
    void updaterContextOwnershipLifetimeAndConstructionSchemaRemainsStable();
    void updaterContextSnapshotLodAndCapacitySignaturesRemainStable();
    void updaterContextJobAdmissionAndSubmissionSignaturesRemainStable();
    void updaterContextSynchronizationAndLimitSignaturesRemainStable();
    void updaterContextContinuationAndCompletionSignaturesRemainStable();
    void simpleUpdateQueueTypeAndLifetimeSchemaRemainStable();
    void simpleUpdateQueueAliasSchemaRemainStable();
    void simpleUpdateQueueAdmissionSignaturesRemainStable();
    void simpleUpdateQueueStateAndProcessingSignaturesRemainStable();
    void simpleUpdateQueueTestingAccessSignaturesRemainStable();
    void updateJobItemTypeAndStateSchemaRemainStable();
    void updateJobItemLifetimeAndExecutionSignaturesRemainStable();
    void updateJobItemAssignmentSignaturesRemainStable();
    void updateJobItemStateTransitionSignaturesRemainStable();
    void updateJobItemGeometryAndStrokePolicySignaturesRemainStable();
    void warpTransformTypeAndLifetimeSchemaRemainStable();
    void warpTransformModeSchemaRemainStable();
    void warpTransformCalculationSchemaRemainStable();
    void warpTransformMathAndImageSignaturesRemainStable();
    void warpTransformDeviceAndBoundsSignaturesRemainStable();
    void liquifyTypeAndLifetimeSchemaRemainStable();
    void liquifyGridAndPointSignaturesRemainStable();
    void liquifyMutationSignaturesRemainStable();
    void liquifyOutputSignaturesRemainStable();
    void liquifyBoundsAndXmlSignaturesRemainStable();
    void baseConstIteratorOwnershipAndTraversalSchemaRemainsStable();
    void horizontalConstIteratorTraversalSchemaRemainsStable();
    void horizontalWritableIteratorSchemaRemainsStable();
    void verticalConstIteratorTraversalSchemaRemainsStable();
    void verticalWritableIteratorSchemaRemainsStable();
    void lineIteratorTypeAndConstructionSchemaRemainStable();
    void lineIteratorTileCacheSchemaRemainStable();
    void lineIteratorDataAccessSignaturesRemainStable();
    void horizontalLineIteratorTraversalSignaturesRemainStable();
    void verticalLineIteratorTraversalSignaturesRemainStable();
    void edgeDetectionTypeAndFilterSchemaRemainsStable();
    void edgeDetectionOutputPolicySchemaRemainsStable();
    void edgeDetectionMatrixAndKernelCreationSignaturesRemainStable();
    void edgeDetectionRadiusConversionSignaturesRemainStable();
    void edgeDetectionDeviceOperationSignaturesRemainStable();
    void selectionMaskOwnershipAndLifetimeSchemaRemainsStable();
    void selectionMaskVisitorAndCloneSignaturesRemainStable();
    void selectionMaskGeometrySignaturesRemainStable();
    void selectionMaskPresentationAndStateSignaturesRemainStable();
    void selectionMaskSelectionAndModelSignaturesRemainStable();
    void indirectPaintingTypeAndLifetimeSchemaRemainStable();
    void indirectPaintingTemporaryStateSignaturesRemainStable();
    void indirectPaintingPainterPolicySignaturesRemainStable();
    void indirectPaintingMergeAndSuspensionSignaturesRemainStable();
    void indirectPaintingGuardLifetimeSchemaRemainStable();
    void selectionBasedLayerOwnershipAndLifetimeSchemaRemainsStable();
    void selectionBasedLayerSelectionDeviceAndProjectionSignaturesRemainStable();
    void selectionBasedLayerHierarchyImageAndDirtySignaturesRemainStable();
    void selectionBasedLayerGeometryAndLodSignaturesRemainStable();
    void selectionBasedLayerThumbnailAndNotificationSignaturesRemainStable();
    void cloneLayerTypeOwnershipAndLifetimeSchemaRemainsStable();
    void cloneLayerSourceIdentityAndPolicySignaturesRemainStable();
    void cloneLayerDeviceProjectionAndPresentationSignaturesRemainStable();
    void cloneLayerImageAndGeometrySignaturesRemainStable();
    void cloneLayerVisitorCloneAndUpdateSignaturesRemainStable();
    void processingVisitorOwnershipAndLifetimeSchemaRemainsStable();
    void processingVisitorNodeAndLayerVisitSignaturesRemainStable();
    void processingVisitorCloneAndMaskVisitSignaturesRemainStable();
    void processingVisitorInitializationSignatureRemainsStable();
    void processingVisitorProgressHelperSchemaRemainsStable();
};

void KisImageTypesContractTest::intrusivePointerAliasesPreserveOwnershipKinds()
{
    static_assert(std::is_same_v<KisAdjustmentLayerSP, KisSharedPtr<KisAdjustmentLayer>>);
    static_assert(std::is_same_v<KisAnnotationSP, KisSharedPtr<KisAnnotation>>);
    static_assert(std::is_same_v<KisBaseNodeSP, KisSharedPtr<KisBaseNode>>);
    static_assert(std::is_same_v<KisBaseNodeWSP, KisWeakSharedPtr<KisBaseNode>>);
    static_assert(std::is_same_v<KisCloneLayerSP, KisSharedPtr<KisCloneLayer>>);
    static_assert(std::is_same_v<KisCloneLayerWSP, KisWeakSharedPtr<KisCloneLayer>>);
    static_assert(std::is_same_v<KisColorizeMaskSP, KisSharedPtr<KisColorizeMask>>);
    static_assert(std::is_same_v<KisColorizeMaskWSP, KisWeakSharedPtr<KisColorizeMask>>);
    static_assert(std::is_same_v<KisConvolutionKernelSP, KisSharedPtr<KisConvolutionKernel>>);
    static_assert(std::is_same_v<KisEffectMaskSP, KisSharedPtr<KisEffectMask>>);
    static_assert(std::is_same_v<KisEffectMaskWSP, KisWeakSharedPtr<KisEffectMask>>);
    static_assert(std::is_same_v<KisFileLayerSP, KisSharedPtr<KisFileLayer>>);
    static_assert(std::is_same_v<KisFileLayerWSP, KisWeakSharedPtr<KisFileLayer>>);
    static_assert(std::is_same_v<KisFilterChainSP, KisSharedPtr<KisFilterChain>>);
    static_assert(std::is_same_v<KisFilterConfigurationSP, KisPinnedSharedPtr<KisFilterConfiguration>>);
    static_assert(std::is_same_v<KisFilterMaskSP, KisSharedPtr<KisFilterMask>>);
    static_assert(std::is_same_v<KisFilterMaskWSP, KisWeakSharedPtr<KisFilterMask>>);
    static_assert(std::is_same_v<KisFilterSP, KisSharedPtr<KisFilter>>);
    static_assert(std::is_same_v<KisFixedPaintDeviceSP, KisSharedPtr<KisFixedPaintDevice>>);
    static_assert(std::is_same_v<KisGeneratorLayerSP, KisSharedPtr<KisGeneratorLayer>>);
    static_assert(std::is_same_v<KisGeneratorSP, KisSharedPtr<KisGenerator>>);
    static_assert(std::is_same_v<KisGroupLayerSP, KisSharedPtr<KisGroupLayer>>);
    static_assert(std::is_same_v<KisGroupLayerWSP, KisWeakSharedPtr<KisGroupLayer>>);
    static_assert(std::is_same_v<KisHLineConstIteratorSP, KisSharedPtr<KisHLineConstIteratorNG>>);
    static_assert(std::is_same_v<KisHLineIteratorSP, KisSharedPtr<KisHLineIteratorNG>>);
    static_assert(std::is_same_v<KisHistogramSP, KisSharedPtr<KisHistogram>>);
    static_assert(std::is_same_v<KisLayerStyleFilterSP, KisSharedPtr<KisLayerStyleFilter>>);
    static_assert(std::is_same_v<KisLayerWSP, KisWeakSharedPtr<KisLayer>>);
    static_assert(std::is_same_v<KisLockedPropertiesSP, KisSharedPtr<KisLockedProperties>>);
    static_assert(std::is_same_v<KisMaskWSP, KisWeakSharedPtr<KisMask>>);
    static_assert(std::is_same_v<KisMirrorAxisSP, KisSharedPtr<KisMirrorAxis>>);
    static_assert(std::is_same_v<KisMirrorAxisWSP, KisWeakSharedPtr<KisMirrorAxis>>);
    static_assert(std::is_same_v<KisNodeWSP, KisWeakSharedPtr<KisNode>>);
    static_assert(std::is_same_v<KisPaintDeviceWSP, KisWeakSharedPtr<KisPaintDevice>>);
    static_assert(std::is_same_v<KisPaintOpSP, KisSharedPtr<KisPaintOp>>);
    static_assert(std::is_same_v<KisPaintOpSettingsRestrictedSP, KisRestrictedSharedPtr<KisPaintOpSettings>>);
    static_assert(std::is_same_v<KisPaintOpSettingsSP, KisPinnedSharedPtr<KisPaintOpSettings>>);
    static_assert(std::is_same_v<KisPixelSelectionSP, KisSharedPtr<KisPixelSelection>>);
    static_assert(std::is_same_v<KisProcessingVisitorSP, KisSharedPtr<KisProcessingVisitor>>);
    static_assert(std::is_same_v<KisPropertiesConfigurationSP, KisPinnedSharedPtr<KisPropertiesConfiguration>>);
    static_assert(std::is_same_v<KisRandomAccessorSP, KisSharedPtr<KisRandomAccessorNG>>);
    static_assert(std::is_same_v<KisRandomConstAccessorSP, KisSharedPtr<KisRandomConstAccessorNG>>);
    static_assert(std::is_same_v<KisRandomSubAccessorSP, KisSharedPtr<KisRandomSubAccessor>>);
    static_assert(std::is_same_v<KisRepeatHLineConstIteratorSP, KisSharedPtr<KisRepeatHLineConstIteratorNG>>);
    static_assert(std::is_same_v<KisRepeatVLineConstIteratorSP, KisSharedPtr<KisRepeatVLineConstIteratorNG>>);
    static_assert(std::is_same_v<KisSelectionComponentSP, KisSharedPtr<KisSelectionComponent>>);
    static_assert(std::is_same_v<KisSelectionMaskSP, KisSharedPtr<KisSelectionMask>>);
    static_assert(std::is_same_v<KisSelectionWSP, KisWeakSharedPtr<KisSelection>>);
    static_assert(std::is_same_v<KisShapeLayerSP, KisSharedPtr<KisShapeLayer>>);
    static_assert(std::is_same_v<KisTransformMaskSP, KisSharedPtr<KisTransformMask>>);
    static_assert(std::is_same_v<KisTransformMaskWSP, KisWeakSharedPtr<KisTransformMask>>);
    static_assert(std::is_same_v<KisTransparencyMaskSP, KisSharedPtr<KisTransparencyMask>>);
    static_assert(std::is_same_v<KisTransparencyMaskWSP, KisWeakSharedPtr<KisTransparencyMask>>);
    static_assert(std::is_same_v<KisVLineConstIteratorSP, KisSharedPtr<KisVLineConstIteratorNG>>);
    static_assert(std::is_same_v<KisVLineIteratorSP, KisSharedPtr<KisVLineIteratorNG>>);

    QVERIFY(true);
}

void KisImageTypesContractTest::qtPointerAliasesPreserveOwnershipKinds()
{
    static_assert(std::is_same_v<KUndo2CommandSP, QSharedPointer<KUndo2Command>>);
    static_assert(std::is_same_v<KisAbstractProjectionPlaneSP, QSharedPointer<KisAbstractProjectionPlane>>);
    static_assert(std::is_same_v<KisAbstractProjectionPlaneWSP, QWeakPointer<KisAbstractProjectionPlane>>);
    static_assert(std::is_same_v<KisKeyframeSP, QSharedPointer<KisKeyframe>>);
    static_assert(std::is_same_v<KisKeyframeWSP, QWeakPointer<KisKeyframe>>);
    static_assert(std::is_same_v<KisLayerCompositionSP, QSharedPointer<KisLayerComposition>>);
    static_assert(std::is_same_v<KisLayerCompositionWSP, QWeakPointer<KisLayerComposition>>);
    static_assert(std::is_same_v<KisPaintOpPresetSP, QSharedPointer<KisPaintOpPreset>>);
    static_assert(std::is_same_v<KisPaintOpPresetWSP, QWeakPointer<KisPaintOpPreset>>);
    static_assert(std::is_same_v<KisPaintingAssistantSP, QSharedPointer<KisPaintingAssistant>>);
    static_assert(std::is_same_v<KisPaintingAssistantWSP, QWeakPointer<KisPaintingAssistant>>);
    static_assert(std::is_same_v<KisProjectionLeafSP, QSharedPointer<KisProjectionLeaf>>);
    static_assert(std::is_same_v<KisProjectionLeafWSP, QWeakPointer<KisProjectionLeaf>>);
    static_assert(std::is_same_v<KisProjectionUpdatesFilterSP, QSharedPointer<KisProjectionUpdatesFilter>>);
    static_assert(std::is_same_v<KisProofingConfigurationSP, QSharedPointer<KisProofingConfiguration>>);
    static_assert(std::is_same_v<KisProofingConfigurationWSP, QWeakPointer<KisProofingConfiguration>>);
    static_assert(std::is_same_v<KisRasterKeyframeSP, QSharedPointer<KisRasterKeyframe>>);
    static_assert(std::is_same_v<KisRasterKeyframeWSP, QWeakPointer<KisRasterKeyframe>>);
    static_assert(std::is_same_v<KisReferenceImageSP, QSharedPointer<KisReferenceImage>>);
    static_assert(std::is_same_v<KisReferenceImageWSP, QWeakPointer<KisReferenceImage>>);
    static_assert(std::is_same_v<KisScalarKeyframeSP, QSharedPointer<KisScalarKeyframe>>);
    static_assert(std::is_same_v<KisScalarKeyframeWSP, QWeakPointer<KisScalarKeyframe>>);
    static_assert(std::is_same_v<KisStrokeSP, QSharedPointer<KisStroke>>);
    static_assert(std::is_same_v<KisStrokeWSP, QWeakPointer<KisStroke>>);
    static_assert(std::is_same_v<KisStrokeId, KisStrokeWSP>);
    static_assert(std::is_same_v<StoryboardItemSP, QSharedPointer<StoryboardItem>>);

    QVERIFY(true);
}

void KisImageTypesContractTest::collectionAliasesPreserveElementAndIteratorTypes()
{
    static_assert(std::is_same_v<KisNodeList, QList<KisNodeSP>>);
    static_assert(std::is_same_v<KisNodeListSP, QSharedPointer<KisNodeList>>);
    static_assert(std::is_same_v<KisPaintDeviceList, QList<KisPaintDeviceSP>>);
    static_assert(std::is_same_v<KisProjectionUpdatesFilterCookie, void *>);
    static_assert(std::is_same_v<KoIDList, QList<KoID>>);
    static_assert(std::is_same_v<KoUpdaterPtr, QPointer<KoUpdater>>);
    static_assert(std::is_same_v<StoryboardCommentList, QVector<StoryboardComment>>);
    static_assert(std::is_same_v<StoryboardItemList, QVector<StoryboardItemSP>>);
    static_assert(std::is_same_v<vKisAnnotationSP, QVector<KisAnnotationSP>>);
    static_assert(std::is_same_v<vKisAnnotationSP_cit, vKisAnnotationSP::const_iterator>);
    static_assert(std::is_same_v<vKisAnnotationSP_it, vKisAnnotationSP::iterator>);
    static_assert(std::is_same_v<vKisNodeSP, QVector<KisNodeSP>>);
    static_assert(std::is_same_v<vKisNodeSP_cit, vKisNodeSP::const_iterator>);
    static_assert(std::is_same_v<vKisNodeSP_it, vKisNodeSP::iterator>);
    static_assert(std::is_same_v<vKisPaintDeviceSP, QVector<KisPaintDeviceSP>>);
    static_assert(std::is_same_v<vKisPaintDeviceSP_it, vKisPaintDeviceSP::iterator>);
    static_assert(std::is_same_v<vKisSegments, QVector<QPoint>>);
    static_assert(std::is_same_v<vQPointF, QVector<QPointF>>);
    static_assert(std::is_same_v<KisRepeatHLineConstIteratorNG, KisRepeatHLineIteratorPixelBase<KisHLineIterator2>>);
    static_assert(std::is_same_v<KisRepeatVLineConstIteratorNG, KisRepeatVLineIteratorPixelBase<KisVLineIterator2>>);

    QVERIFY(true);
}

void KisImageTypesContractTest::thumbnailBoundsModesRemainDistinct()
{
    static_assert(KisThumbnailBoundsMode::Coarse != KisThumbnailBoundsMode::Precise);
    QCOMPARE(static_cast<int>(KisThumbnailBoundsMode::Coarse), 0);
    QCOMPARE(static_cast<int>(KisThumbnailBoundsMode::Precise), 1);
}

void KisImageTypesContractTest::selectionFilterBaseInterfaceSchemaRemainsStable()
{
    using NameSignature = KUndo2MagicString (KisSelectionFilter::*)();
    using ChangeRectSignature = QRect (KisSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using ProcessSignature = void (KisSelectionFilter::*)(KisPixelSelectionSP, const QRect &);

    static_assert(std::is_class_v<KisSelectionFilter>);
    static_assert(std::is_abstract_v<KisSelectionFilter>);
    static_assert(std::has_virtual_destructor_v<KisSelectionFilter>);
    static_assert(std::is_same_v<decltype(static_cast<NameSignature>(&KisSelectionFilter::name)), NameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ChangeRectSignature>(&KisSelectionFilter::changeRect)),
                                 ChangeRectSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ProcessSignature>(&KisSelectionFilter::process)), ProcessSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::morphologicalSelectionFilterSchemaRemainsStable()
{
    using ErodeNameSignature = KUndo2MagicString (KisErodeSelectionFilter::*)();
    using ErodeChangeRectSignature = QRect (KisErodeSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using ErodeProcessSignature = void (KisErodeSelectionFilter::*)(KisPixelSelectionSP, const QRect &);
    using DilateNameSignature = KUndo2MagicString (KisDilateSelectionFilter::*)();
    using DilateChangeRectSignature = QRect (KisDilateSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using DilateProcessSignature = void (KisDilateSelectionFilter::*)(KisPixelSelectionSP, const QRect &);
    using SmoothNameSignature = KUndo2MagicString (KisSmoothSelectionFilter::*)();
    using SmoothChangeRectSignature = QRect (KisSmoothSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using SmoothProcessSignature = void (KisSmoothSelectionFilter::*)(KisPixelSelectionSP, const QRect &);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisErodeSelectionFilter>);
    static_assert(
        std::is_same_v<decltype(static_cast<ErodeNameSignature>(&KisErodeSelectionFilter::name)), ErodeNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ErodeChangeRectSignature>(&KisErodeSelectionFilter::changeRect)),
                                 ErodeChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ErodeProcessSignature>(&KisErodeSelectionFilter::process)),
                                 ErodeProcessSignature>);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisDilateSelectionFilter>);
    static_assert(std::is_same_v<decltype(static_cast<DilateNameSignature>(&KisDilateSelectionFilter::name)),
                                 DilateNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<DilateChangeRectSignature>(&KisDilateSelectionFilter::changeRect)),
                       DilateChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<DilateProcessSignature>(&KisDilateSelectionFilter::process)),
                                 DilateProcessSignature>);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisSmoothSelectionFilter>);
    static_assert(std::is_same_v<decltype(static_cast<SmoothNameSignature>(&KisSmoothSelectionFilter::name)),
                                 SmoothNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SmoothChangeRectSignature>(&KisSmoothSelectionFilter::changeRect)),
                       SmoothChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SmoothProcessSignature>(&KisSmoothSelectionFilter::process)),
                                 SmoothProcessSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::invertAndAntiAliasSelectionFilterSchemaRemainsStable()
{
    using InvertNameSignature = KUndo2MagicString (KisInvertSelectionFilter::*)();
    using InvertChangeRectSignature = QRect (KisInvertSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using InvertProcessSignature = void (KisInvertSelectionFilter::*)(KisPixelSelectionSP, const QRect &);
    using AntiAliasNameSignature = KUndo2MagicString (KisAntiAliasSelectionFilter::*)();
    using AntiAliasProcessSignature = void (KisAntiAliasSelectionFilter::*)(KisPixelSelectionSP, const QRect &);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisInvertSelectionFilter>);
    static_assert(std::is_same_v<decltype(static_cast<InvertNameSignature>(&KisInvertSelectionFilter::name)),
                                 InvertNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<InvertChangeRectSignature>(&KisInvertSelectionFilter::changeRect)),
                       InvertChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<InvertProcessSignature>(&KisInvertSelectionFilter::process)),
                                 InvertProcessSignature>);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisAntiAliasSelectionFilter>);
    static_assert(std::is_same_v<decltype(static_cast<AntiAliasNameSignature>(&KisAntiAliasSelectionFilter::name)),
                                 AntiAliasNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<AntiAliasProcessSignature>(&KisAntiAliasSelectionFilter::process)),
                       AntiAliasProcessSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::borderAndFeatherSelectionFilterSchemaRemainsStable()
{
    using BorderNameSignature = KUndo2MagicString (KisBorderSelectionFilter::*)();
    using BorderChangeRectSignature = QRect (KisBorderSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using BorderProcessSignature = void (KisBorderSelectionFilter::*)(KisPixelSelectionSP, const QRect &);
    using FeatherNameSignature = KUndo2MagicString (KisFeatherSelectionFilter::*)();
    using FeatherChangeRectSignature = QRect (KisFeatherSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using FeatherProcessSignature = void (KisFeatherSelectionFilter::*)(KisPixelSelectionSP, const QRect &);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisBorderSelectionFilter>);
    static_assert(std::is_constructible_v<KisBorderSelectionFilter, qint32, qint32, bool>);
    static_assert(std::is_same_v<decltype(static_cast<BorderNameSignature>(&KisBorderSelectionFilter::name)),
                                 BorderNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BorderChangeRectSignature>(&KisBorderSelectionFilter::changeRect)),
                       BorderChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<BorderProcessSignature>(&KisBorderSelectionFilter::process)),
                                 BorderProcessSignature>);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisFeatherSelectionFilter>);
    static_assert(std::is_constructible_v<KisFeatherSelectionFilter, qint32>);
    static_assert(std::is_same_v<decltype(static_cast<FeatherNameSignature>(&KisFeatherSelectionFilter::name)),
                                 FeatherNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<FeatherChangeRectSignature>(&KisFeatherSelectionFilter::changeRect)),
                       FeatherChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<FeatherProcessSignature>(&KisFeatherSelectionFilter::process)),
                                 FeatherProcessSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::growthSelectionFilterPolicySchemaRemainsStable()
{
    using GrowNameSignature = KUndo2MagicString (KisGrowSelectionFilter::*)();
    using GrowChangeRectSignature = QRect (KisGrowSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using GrowProcessSignature = void (KisGrowSelectionFilter::*)(KisPixelSelectionSP, const QRect &);
    using ShrinkNameSignature = KUndo2MagicString (KisShrinkSelectionFilter::*)();
    using ShrinkChangeRectSignature = QRect (KisShrinkSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using ShrinkProcessSignature = void (KisShrinkSelectionFilter::*)(KisPixelSelectionSP, const QRect &);
    using AdaptiveGrowNameSignature = KUndo2MagicString (KisGrowUntilDarkestPixelSelectionFilter::*)();
    using AdaptiveGrowChangeRectSignature =
        QRect (KisGrowUntilDarkestPixelSelectionFilter::*)(const QRect &, KisDefaultBoundsBaseSP);
    using AdaptiveGrowProcessSignature =
        void (KisGrowUntilDarkestPixelSelectionFilter::*)(KisPixelSelectionSP, const QRect &);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisGrowSelectionFilter>);
    static_assert(std::is_constructible_v<KisGrowSelectionFilter, qint32, qint32>);
    static_assert(
        std::is_same_v<decltype(static_cast<GrowNameSignature>(&KisGrowSelectionFilter::name)), GrowNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<GrowChangeRectSignature>(&KisGrowSelectionFilter::changeRect)),
                                 GrowChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<GrowProcessSignature>(&KisGrowSelectionFilter::process)),
                                 GrowProcessSignature>);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisShrinkSelectionFilter>);
    static_assert(std::is_constructible_v<KisShrinkSelectionFilter, qint32, qint32, bool>);
    static_assert(std::is_same_v<decltype(static_cast<ShrinkNameSignature>(&KisShrinkSelectionFilter::name)),
                                 ShrinkNameSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ShrinkChangeRectSignature>(&KisShrinkSelectionFilter::changeRect)),
                       ShrinkChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ShrinkProcessSignature>(&KisShrinkSelectionFilter::process)),
                                 ShrinkProcessSignature>);

    static_assert(std::is_base_of_v<KisSelectionFilter, KisGrowUntilDarkestPixelSelectionFilter>);
    static_assert(std::is_constructible_v<KisGrowUntilDarkestPixelSelectionFilter, qint32, KisPaintDeviceSP>);
    static_assert(
        std::is_same_v<decltype(static_cast<AdaptiveGrowNameSignature>(&KisGrowUntilDarkestPixelSelectionFilter::name)),
                       AdaptiveGrowNameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<AdaptiveGrowChangeRectSignature>(
                                     &KisGrowUntilDarkestPixelSelectionFilter::changeRect)),
                                 AdaptiveGrowChangeRectSignature>);
    static_assert(std::is_same_v<decltype(static_cast<AdaptiveGrowProcessSignature>(
                                     &KisGrowUntilDarkestPixelSelectionFilter::process)),
                                 AdaptiveGrowProcessSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::defaultBoundsOwnershipAndHierarchySchemaRemainsStable()
{
    static_assert(std::is_same_v<KisDefaultBoundsSP, KisSharedPtr<KisDefaultBounds>>);
    static_assert(std::is_same_v<KisSelectionDefaultBoundsSP, KisSharedPtr<KisSelectionDefaultBounds>>);
    static_assert(std::is_same_v<KisSelectionEmptyBoundsSP, KisSharedPtr<KisSelectionEmptyBounds>>);
    static_assert(std::is_same_v<KisWrapAroundBoundsWrapperSP, KisSharedPtr<KisWrapAroundBoundsWrapper>>);

    static_assert(std::is_class_v<KisDefaultBounds>);
    static_assert(std::is_class_v<KisSelectionDefaultBoundsBase>);
    static_assert(std::is_class_v<KisSelectionDefaultBounds>);
    static_assert(std::is_class_v<KisMaskDefaultBounds>);
    static_assert(std::is_class_v<KisSelectionEmptyBounds>);
    static_assert(std::is_class_v<KisWrapAroundBoundsWrapper>);

    static_assert(std::is_base_of_v<KisDefaultBoundsBase, KisDefaultBounds>);
    static_assert(std::is_base_of_v<KisDefaultBoundsBase, KisSelectionDefaultBoundsBase>);
    static_assert(std::is_base_of_v<KisSelectionDefaultBoundsBase, KisSelectionDefaultBounds>);
    static_assert(std::is_base_of_v<KisSelectionDefaultBoundsBase, KisMaskDefaultBounds>);
    static_assert(std::is_base_of_v<KisDefaultBounds, KisSelectionEmptyBounds>);
    static_assert(std::is_base_of_v<KisDefaultBoundsBase, KisWrapAroundBoundsWrapper>);

    QVERIFY(true);
}

void KisImageTypesContractTest::imageDefaultBoundsSignaturesRemainStable()
{
    static_assert(std::is_default_constructible_v<KisDefaultBounds>);
    static_assert(std::is_constructible_v<KisDefaultBounds, KisImageWSP>);
    static_assert(std::has_virtual_destructor_v<KisDefaultBounds>);

    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, bounds, QRect (KisDefaultBounds::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, wrapAroundMode, bool (KisDefaultBounds::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, wrapAroundModeAxis, WrapAroundAxis (KisDefaultBounds::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, currentLevelOfDetail, int (KisDefaultBounds::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, currentTime, int (KisDefaultBounds::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, externalFrameActive, bool (KisDefaultBounds::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisDefaultBounds, sourceCookie, void *(KisDefaultBounds::*)() const);

    QVERIFY(true);
}

void KisImageTypesContractTest::selectionDefaultBoundsSignaturesRemainStable()
{
    static_assert(std::is_abstract_v<KisSelectionDefaultBoundsBase>);
    static_assert(std::is_default_constructible_v<SelectionDefaultBoundsConstructorProbe>);
    static_assert(std::has_virtual_destructor_v<KisSelectionDefaultBoundsBase>);

    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    bounds,
                                    QRect (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    imageBorderRect,
                                    QRect (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    wrapAroundMode,
                                    bool (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    wrapAroundModeAxis,
                                    WrapAroundAxis (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    currentLevelOfDetail,
                                    int (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    currentTime,
                                    int (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    externalFrameActive,
                                    bool (KisSelectionDefaultBoundsBase::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionDefaultBoundsBase,
                                    sourceCookie,
                                    void *(KisSelectionDefaultBoundsBase::*)() const);

    static_assert(std::is_constructible_v<KisSelectionDefaultBounds, KisPaintDeviceSP>);
    static_assert(std::has_virtual_destructor_v<KisSelectionDefaultBounds>);
    static_assert(std::is_constructible_v<KisMaskDefaultBounds, KisNodeSP>);
    static_assert(std::has_virtual_destructor_v<KisMaskDefaultBounds>);

    QVERIFY(true);
}

void KisImageTypesContractTest::emptySelectionBoundsSignaturesRemainStable()
{
    static_assert(std::is_default_constructible_v<KisSelectionEmptyBounds>);
    static_assert(std::is_constructible_v<KisSelectionEmptyBounds, KisImageWSP>);
    static_assert(std::has_virtual_destructor_v<KisSelectionEmptyBounds>);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisSelectionEmptyBounds, bounds, QRect (KisSelectionEmptyBounds::*)() const);

    QVERIFY(true);
}

void KisImageTypesContractTest::wrapAroundBoundsSignaturesRemainStable()
{
    static_assert(std::is_constructible_v<KisWrapAroundBoundsWrapper, KisDefaultBoundsBaseSP, QRect>);
    static_assert(std::has_virtual_destructor_v<KisWrapAroundBoundsWrapper>);

    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper, bounds, QRect (KisWrapAroundBoundsWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper,
                                    wrapAroundMode,
                                    bool (KisWrapAroundBoundsWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper,
                                    wrapAroundModeAxis,
                                    WrapAroundAxis (KisWrapAroundBoundsWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper,
                                    currentLevelOfDetail,
                                    int (KisWrapAroundBoundsWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper,
                                    currentTime,
                                    int (KisWrapAroundBoundsWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper,
                                    externalFrameActive,
                                    bool (KisWrapAroundBoundsWrapper::*)() const);
    ASSERT_DEFAULT_BOUNDS_SIGNATURE(KisWrapAroundBoundsWrapper,
                                    sourceCookie,
                                    void *(KisWrapAroundBoundsWrapper::*)() const);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInputPositionAndSensorSignaturesRemainStable()
{
    using PositionSignature = const QPointF &(KisPaintInformation::*)() const;
    using SetPositionSignature = void (KisPaintInformation::*)(const QPointF &);
    using ScalarSignature = qreal (KisPaintInformation::*)() const;
    using SetScalarSignature = void (KisPaintInformation::*)(qreal);

    static_assert(std::is_class_v<KisPaintInformation>);
    static_assert(
        std::is_same_v<decltype(static_cast<PositionSignature>(&KisPaintInformation::pos)), PositionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetPositionSignature>(&KisPaintInformation::setPos)),
                                 SetPositionSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::pressure)), ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetScalarSignature>(&KisPaintInformation::setPressure)),
                                 SetScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::xTilt)), ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::yTilt)), ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::rotation)), ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::tangentialPressure)),
                                 ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::perspective)), ScalarSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInputStrokeProgressSignaturesRemainStable()
{
    using ScalarSignature = qreal (KisPaintInformation::*)() const;
    using SetCurrentTimeSignature = void (KisPaintInformation::*)(qreal) const;
    using DabSequenceSignature = int (KisPaintInformation::*)() const;
    using HoveringModeSignature = bool (KisPaintInformation::*)() const;

    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::currentTime)), ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetCurrentTimeSignature>(&KisPaintInformation::setCurrentTime)),
                                 SetCurrentTimeSignature>);
    static_assert(std::is_same_v<decltype(static_cast<DabSequenceSignature>(&KisPaintInformation::currentDabSeqNo)),
                                 DabSequenceSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::totalStrokeLength)),
                                 ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<HoveringModeSignature>(&KisPaintInformation::isHoveringMode)),
                                 HoveringModeSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInputCanvasOrientationSignaturesRemainStable()
{
    using ScalarSignature = qreal (KisPaintInformation::*)() const;
    using SetScalarSignature = void (KisPaintInformation::*)(qreal);
    using BooleanSignature = bool (KisPaintInformation::*)() const;
    using SetBooleanSignature = void (KisPaintInformation::*)(bool);

    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::canvasRotation)), ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetScalarSignature>(&KisPaintInformation::setCanvasRotation)),
                                 SetScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanSignature>(&KisPaintInformation::canvasMirroredH)),
                                 BooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetBooleanSignature>(&KisPaintInformation::setCanvasMirroredH)),
                                 SetBooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanSignature>(&KisPaintInformation::canvasMirroredV)),
                                 BooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetBooleanSignature>(&KisPaintInformation::setCanvasMirroredV)),
                                 SetBooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::tiltDirectionOffset)),
                                 ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetScalarSignature>(&KisPaintInformation::setTiltDirectionOffset)),
                       SetScalarSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInputDerivedMotionSignaturesRemainStable()
{
    using DrawingAngleSignature = qreal (KisPaintInformation::*)(bool) const;
    using DrawingAngleSafeSignature = qreal (KisPaintInformation::*)(const KisDistanceInformation &) const;
    using DirectionSignature = QPointF (KisPaintInformation::*)() const;
    using ScalarSignature = qreal (KisPaintInformation::*)() const;
    using OverrideDrawingAngleSignature = void (KisPaintInformation::*)(qreal);

    static_assert(std::is_same_v<decltype(static_cast<DrawingAngleSignature>(&KisPaintInformation::drawingAngle)),
                                 DrawingAngleSignature>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPaintInformation &>().drawingAngle()), qreal>);
    static_assert(
        std::is_same_v<decltype(static_cast<DrawingAngleSafeSignature>(&KisPaintInformation::drawingAngleSafe)),
                       DrawingAngleSafeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<DirectionSignature>(&KisPaintInformation::drawingDirectionVector)),
                       DirectionSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::drawingDistance)), ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::drawingSpeed)), ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&KisPaintInformation::maxPressure)), ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<OverrideDrawingAngleSignature>(&KisPaintInformation::overrideDrawingAngle)),
                       OverrideDrawingAngleSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInputTiltConversionSignaturesRemainStable()
{
    using TiltDirectionSignature = qreal (*)(const KisPaintInformation &, bool);
    using TiltElevationSignature = qreal (*)(const KisPaintInformation &, qreal, qreal, bool);

    static_assert(std::is_same_v<decltype(static_cast<TiltDirectionSignature>(&KisPaintInformation::tiltDirection)),
                                 TiltDirectionSignature>);
    static_assert(
        std::is_same_v<decltype(KisPaintInformation::tiltDirection(std::declval<const KisPaintInformation &>())),
                       qreal>);
    static_assert(std::is_same_v<decltype(static_cast<TiltElevationSignature>(&KisPaintInformation::tiltElevation)),
                                 TiltElevationSignature>);
    static_assert(
        std::is_same_v<decltype(KisPaintInformation::tiltElevation(std::declval<const KisPaintInformation &>())),
                       qreal>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInformationConstructionAndLifetimeSchemaRemainsStable()
{
    using Information = KisPaintInformation;
    using AssignmentSignature = void (Information::*)(const Information &);

    static_assert(std::is_default_constructible_v<Information>);
    static_assert(std::is_constructible_v<Information, const QPointF &, qreal>);
    static_assert(std::is_constructible_v<Information, const QPointF &, qreal, qreal, qreal, qreal>);
    static_assert(
        std::is_constructible_v<Information, const QPointF &, qreal, qreal, qreal, qreal, qreal, qreal, qreal, qreal>);
    static_assert(std::is_copy_constructible_v<Information>);
    static_assert(
        std::is_same_v<decltype(static_cast<AssignmentSignature>(&Information::operator=)), AssignmentSignature>);
    static_assert(std::is_destructible_v<Information>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInformationDistanceRegistrarSchemaRemainsStable()
{
    using Information = KisPaintInformation;
    using Registrar = Information::DistanceInformationRegistrar;

    static_assert(std::is_class_v<Registrar>);
    static_assert(std::is_constructible_v<Registrar, Information *, KisDistanceInformation *>);
    static_assert(!std::is_copy_constructible_v<Registrar>);
    static_assert(std::is_move_constructible_v<Registrar>);
    static_assert(std::is_destructible_v<Registrar>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInformationMixingSignaturesRemainStable()
{
    using Information = KisPaintInformation;
    using MixAtPositionSignature = Information (*)(const QPointF &, qreal, const Information &, const Information &);
    using MixSignature = Information (*)(qreal, const Information &, const Information &);
    using MixInPlaceSignature = void (Information::*)(qreal, const Information &);

    static_assert(
        std::is_same_v<decltype(static_cast<MixAtPositionSignature>(&Information::mix)), MixAtPositionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MixSignature>(&Information::mix)), MixSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MixSignature>(&Information::mixOnlyPosition)), MixSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MixAtPositionSignature>(&Information::mixWithoutTime)),
                                 MixAtPositionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MixSignature>(&Information::mixWithoutTime)), MixSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MixInPlaceSignature>(&Information::mixOtherOnlyPosition)),
                                 MixInPlaceSignature>);
    static_assert(std::is_same_v<decltype(static_cast<MixInPlaceSignature>(&Information::mixOtherWithoutTime)),
                                 MixInPlaceSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInformationRandomDistanceAndLodSignaturesRemainStable()
{
    using Information = KisPaintInformation;
    using RandomSourceSignature = KisRandomSourceSP (Information::*)() const;
    using SetRandomSourceSignature = void (Information::*)(KisRandomSourceSP);
    using PerStrokeRandomSourceSignature = KisPerStrokeRandomSourceSP (Information::*)() const;
    using SetPerStrokeRandomSourceSignature = void (Information::*)(KisPerStrokeRandomSourceSP);
    using RegisterDistanceSignature =
        Information::DistanceInformationRegistrar (Information::*)(KisDistanceInformation *);
    using SetLevelOfDetailSignature = void (Information::*)(int);

    static_assert(std::is_same_v<decltype(static_cast<RandomSourceSignature>(&Information::randomSource)),
                                 RandomSourceSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetRandomSourceSignature>(&Information::setRandomSource)),
                                 SetRandomSourceSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<PerStrokeRandomSourceSignature>(&Information::perStrokeRandomSource)),
                       PerStrokeRandomSourceSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetPerStrokeRandomSourceSignature>(&Information::setPerStrokeRandomSource)),
                       SetPerStrokeRandomSourceSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<RegisterDistanceSignature>(&Information::registerDistanceInformation)),
                       RegisterDistanceSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetLevelOfDetailSignature>(&Information::setLevelOfDetail)),
                                 SetLevelOfDetailSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::paintInformationHoverPaintSerializationAndDebugSignaturesRemainStable()
{
    using Information = KisPaintInformation;
    using HoveringSignature =
        Information (*)(const QPointF &, qreal, qreal, qreal, qreal, qreal, qreal, qreal, qreal, bool, bool, qreal);
    using FromXmlSignature = Information (*)(const QDomElement &);
    using ToXmlSignature = void (Information::*)(QDomDocument &, QDomElement &) const;
    using DebugSignature = QDebug (*)(QDebug, const Information &);

    static_assert(std::is_same_v<decltype(static_cast<HoveringSignature>(&Information::createHoveringModeInfo)),
                                 HoveringSignature>);
    static_assert(
        std::is_same_v<decltype(Information::createHoveringModeInfo(std::declval<const QPointF &>())), Information>);
    static_assert(std::is_same_v<decltype(static_cast<FromXmlSignature>(&Information::fromXML)), FromXmlSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ToXmlSignature>(&Information::toXML)), ToXmlSignature>);
    static_assert(
        std::is_same_v<decltype(std::declval<Information &>().paintAt(std::declval<PaintOpSignatureProbe &>(),
                                                                      static_cast<KisDistanceInformation *>(nullptr))),
                       void>);
    static_assert(std::is_same_v<decltype(static_cast<DebugSignature>(&operator<<)), DebugSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::distanceStateLifecycleSchemaRemainsStable()
{
    using Distance = KisDistanceInformation;

    static_assert(std::is_class_v<Distance>);
    static_assert(std::is_default_constructible_v<Distance>);
    static_assert(std::is_copy_constructible_v<Distance>);
    static_assert(std::is_constructible_v<Distance, const Distance &, int>);
    static_assert(std::is_constructible_v<Distance, const QPointF &, qreal>);
    static_assert(std::is_constructible_v<Distance, const QPointF &, qreal, qreal, qreal, int>);
    static_assert(std::is_constructible_v<Distance, qreal, qreal, int>);
    static_assert(std::is_constructible_v<Distance, qreal, qreal>);
    static_assert(std::is_copy_assignable_v<Distance>);
    static_assert(std::is_destructible_v<Distance>);

    QVERIFY(true);
}

void KisImageTypesContractTest::distanceLastDabObservationSignaturesRemainStable()
{
    using Distance = KisDistanceInformation;
    using BooleanSignature = bool (Distance::*)() const;
    using DabSequenceSignature = int (Distance::*)() const;
    using PaintInformationSignature = const KisPaintInformation &(Distance::*)() const;
    using PositionSignature = QPointF (Distance::*)() const;
    using ScalarSignature = qreal (Distance::*)() const;

    static_assert(
        std::is_same_v<decltype(static_cast<DabSequenceSignature>(&Distance::currentDabSeqNo)), DabSequenceSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BooleanSignature>(&Distance::hasLastDabInformation)), BooleanSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BooleanSignature>(&Distance::hasLastPaintInformation)), BooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanSignature>(&Distance::isStarted)), BooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&Distance::lastDrawingAngle)), ScalarSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PaintInformationSignature>(&Distance::lastPaintInformation)),
                                 PaintInformationSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PositionSignature>(&Distance::lastPosition)), PositionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ScalarSignature>(&Distance::maxPressure)), ScalarSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::distanceSpacingAndTimingSignaturesRemainStable()
{
    using Distance = KisDistanceInformation;
    using BooleanSignature = bool (Distance::*)() const;
    using ScalarSignature = qreal (Distance::*)() const;
    using SpacingSignature = const KisSpacingInformation &(Distance::*)() const;
    using TimingSignature = const KisTimingInformation &(Distance::*)() const;
    using UpdateSpacingSignature = void (Distance::*)(const KisSpacingInformation &);
    using UpdateTimingSignature = void (Distance::*)(const KisTimingInformation &);

    static_assert(std::is_same_v<decltype(static_cast<SpacingSignature>(&Distance::currentSpacing)), SpacingSignature>);
    static_assert(std::is_same_v<decltype(static_cast<TimingSignature>(&Distance::currentTiming)), TimingSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&Distance::getSpacingInterval)), ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&Distance::getTimingUpdateInterval)), ScalarSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BooleanSignature>(&Distance::needsSpacingUpdate)), BooleanSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<BooleanSignature>(&Distance::needsTimingUpdate)), BooleanSignature>);
    static_assert(std::is_same_v<decltype(static_cast<UpdateSpacingSignature>(&Distance::updateSpacing)),
                                 UpdateSpacingSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<UpdateTimingSignature>(&Distance::updateTiming)), UpdateTimingSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::distanceStrokeProgressSignaturesRemainStable()
{
    using Distance = KisDistanceInformation;
    using NextPointSignature = qreal (Distance::*)(const QPointF &, const QPointF &, qreal, qreal);
    using OverrideLastValuesSignature = void (Distance::*)(const QPointF &, qreal);
    using RegisterPaintedDabSignature =
        void (Distance::*)(const KisPaintInformation &, const KisSpacingInformation &, const KisTimingInformation &);
    using ScalarSignature = qreal (Distance::*)() const;

    static_assert(
        std::is_same_v<decltype(static_cast<NextPointSignature>(&Distance::getNextPointPosition)), NextPointSignature>);
    static_assert(std::is_same_v<decltype(static_cast<OverrideLastValuesSignature>(&Distance::overrideLastValues)),
                                 OverrideLastValuesSignature>);
    static_assert(std::is_same_v<decltype(static_cast<RegisterPaintedDabSignature>(&Distance::registerPaintedDab)),
                                 RegisterPaintedDabSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<ScalarSignature>(&Distance::scalarDistanceApprox)), ScalarSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::distanceDrawingAngleLockSignaturesRemainStable()
{
    using Distance = KisDistanceInformation;
    using LockDrawingAngleSignature = void (Distance::*)(const KisPaintInformation &) const;
    using LockedDrawingAngleSignature = boost::optional<qreal> (Distance::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<LockDrawingAngleSignature>(&Distance::lockCurrentDrawingAngle)),
                                 LockDrawingAngleSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<LockedDrawingAngleSignature>(&Distance::lockedDrawingAngleOptional)),
                       LockedDrawingAngleSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::histogramTypeAndLifecycleSchemaRemainStable()
{
    static_assert(std::is_class_v<KisHistogram>);
    static_assert(std::is_enum_v<enumHistogramType>);
    static_assert(std::is_constructible_v<KisHistogram, KisPaintLayerSP, KoHistogramProducer *, enumHistogramType>);
    static_assert(std::is_constructible_v<KisHistogram,
                                          KisPaintDeviceSP,
                                          const QRect &,
                                          KoHistogramProducer *,
                                          enumHistogramType>);
    static_assert(std::has_virtual_destructor_v<KisHistogram>);

    QCOMPARE(static_cast<int>(LINEAR), 0);
    QCOMPARE(static_cast<int>(LOGARITHMIC), 1);
}

void KisImageTypesContractTest::histogramCalculationValueSchemaRemainsStable()
{
    using Calculations = KisHistogram::Calculations;
    using DoubleGetter = double (Calculations::*)();
    using CountGetter = quint32 (Calculations::*)();

    static_assert(std::is_class_v<Calculations>);
    static_assert(std::is_default_constructible_v<Calculations>);
    static_assert(std::is_same_v<decltype(static_cast<DoubleGetter>(&Calculations::getMax)), DoubleGetter>);
    static_assert(std::is_same_v<decltype(static_cast<DoubleGetter>(&Calculations::getMin)), DoubleGetter>);
    static_assert(std::is_same_v<decltype(static_cast<DoubleGetter>(&Calculations::getMean)), DoubleGetter>);
    static_assert(std::is_same_v<decltype(static_cast<DoubleGetter>(&Calculations::getTotal)), DoubleGetter>);
    static_assert(std::is_same_v<decltype(static_cast<CountGetter>(&Calculations::getHighest)), CountGetter>);
    static_assert(std::is_same_v<decltype(static_cast<CountGetter>(&Calculations::getLowest)), CountGetter>);
    static_assert(std::is_same_v<decltype(static_cast<CountGetter>(&Calculations::getCount)), CountGetter>);

    Calculations calculations;
    QCOMPARE(calculations.getMax(), 0.0);
    QCOMPARE(calculations.getMin(), 0.0);
    QCOMPARE(calculations.getMean(), 0.0);
    QCOMPARE(calculations.getTotal(), 0.0);
    QCOMPARE(calculations.getHighest(), quint32(0));
    QCOMPARE(calculations.getLowest(), quint32(0));
    QCOMPARE(calculations.getCount(), quint32(0));
}

void KisImageTypesContractTest::histogramComputationAndResultSignaturesRemainStable()
{
    using Histogram = KisHistogram;
    using VoidOperation = void (Histogram::*)();
    using CalculationsQuery = Histogram::Calculations (Histogram::*)();
    using ValueQuery = quint32 (Histogram::*)(quint8);
    using TypeQuery = enumHistogramType (Histogram::*)();

    static_assert(std::is_same_v<decltype(static_cast<VoidOperation>(&Histogram::updateHistogram)), VoidOperation>);
    static_assert(std::is_same_v<decltype(static_cast<VoidOperation>(&Histogram::computeHistogram)), VoidOperation>);
    static_assert(
        std::is_same_v<decltype(static_cast<CalculationsQuery>(&Histogram::calculations)), CalculationsQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<CalculationsQuery>(&Histogram::selectionCalculations)), CalculationsQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ValueQuery>(&Histogram::getValue)), ValueQuery>);
    static_assert(std::is_same_v<decltype(static_cast<TypeQuery>(&Histogram::getHistogramType)), TypeQuery>);

    QVERIFY(true);
}

void KisImageTypesContractTest::histogramTypeProducerAndChannelSignaturesRemainStable()
{
    using Histogram = KisHistogram;
    using SetTypeSignature = void (Histogram::*)(enumHistogramType);
    using SetProducerSignature = void (Histogram::*)(KoHistogramProducer *);
    using ProducerQuery = KoHistogramProducer *(Histogram::*)();
    using SetChannelSignature = void (Histogram::*)(qint32);
    using ChannelQuery = qint32 (Histogram::*)();

    static_assert(
        std::is_same_v<decltype(static_cast<SetTypeSignature>(&Histogram::setHistogramType)), SetTypeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetProducerSignature>(&Histogram::setProducer)), SetProducerSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ProducerQuery>(&Histogram::producer)), ProducerQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetChannelSignature>(&Histogram::setChannel)), SetChannelSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ChannelQuery>(&Histogram::channel)), ChannelQuery>);

    QVERIFY(true);
}

void KisImageTypesContractTest::histogramSelectionSignaturesRemainStable()
{
    using Histogram = KisHistogram;
    using HasSelectionQuery = bool (Histogram::*)();
    using SelectionBoundQuery = double (Histogram::*)();
    using ClearSelectionSignature = void (Histogram::*)();
    using SetSelectionSignature = void (Histogram::*)(double, double);

    static_assert(
        std::is_same_v<decltype(static_cast<HasSelectionQuery>(&Histogram::hasSelection)), HasSelectionQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<SelectionBoundQuery>(&Histogram::selectionFrom)), SelectionBoundQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<SelectionBoundQuery>(&Histogram::selectionTo)), SelectionBoundQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ClearSelectionSignature>(&Histogram::setNoSelection)),
                                 ClearSelectionSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetSelectionSignature>(&Histogram::setSelection)), SetSelectionSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::strokeTypeAndLifecycleSchemaRemainStable()
{
    using Stroke = KisStroke;
    using Type = Stroke::Type;

    static_assert(std::is_class_v<Stroke>);
    static_assert(std::is_enum_v<Type>);
    static_assert(std::is_constructible_v<Stroke, KisStrokeStrategy *>);
    static_assert(std::is_constructible_v<Stroke, KisStrokeStrategy *, Type, int>);
    static_assert(std::is_destructible_v<Stroke>);

    QCOMPARE(static_cast<int>(Type::LEGACY), 0);
    QCOMPARE(static_cast<int>(Type::LOD0), 1);
    QCOMPARE(static_cast<int>(Type::LODN), 2);
    QCOMPARE(static_cast<int>(Type::SUSPEND), 3);
    QCOMPARE(static_cast<int>(Type::RESUME), 4);
}

void KisImageTypesContractTest::strokeJobQueueOperationSignaturesRemainStable()
{
    using Stroke = KisStroke;
    using AddJobSignature = void (Stroke::*)(KisStrokeJobData *);
    using AddMutatedJobsSignature = void (Stroke::*)(const QVector<KisStrokeJobData *>);
    using HasJobsSignature = bool (Stroke::*)() const;
    using NumJobsSignature = qint32 (Stroke::*)() const;
    using PopOneJobSignature = KisStrokeJob *(Stroke::*)();
    using NextJobSequentialitySignature = KisStrokeJobData::Sequentiality (Stroke::*)() const;
    using NextJobLevelOfDetailSignature = int (Stroke::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<AddJobSignature>(&Stroke::addJob)), AddJobSignature>);
    static_assert(std::is_same_v<decltype(static_cast<AddMutatedJobsSignature>(&Stroke::addMutatedJobs)),
                                 AddMutatedJobsSignature>);
    static_assert(std::is_same_v<decltype(static_cast<HasJobsSignature>(&Stroke::hasJobs)), HasJobsSignature>);
    static_assert(std::is_same_v<decltype(static_cast<NumJobsSignature>(&Stroke::numJobs)), NumJobsSignature>);
    static_assert(std::is_same_v<decltype(static_cast<PopOneJobSignature>(&Stroke::popOneJob)), PopOneJobSignature>);
    static_assert(std::is_same_v<decltype(static_cast<NextJobSequentialitySignature>(&Stroke::nextJobSequentiality)),
                                 NextJobSequentialitySignature>);
    static_assert(std::is_same_v<decltype(static_cast<NextJobLevelOfDetailSignature>(&Stroke::nextJobLevelOfDetail)),
                                 NextJobLevelOfDetailSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::strokeIdentityAndLifecycleTransitionSignaturesRemainStable()
{
    using Stroke = KisStroke;
    using NameSignature = KUndo2MagicString (Stroke::*)() const;
    using IdSignature = QString (Stroke::*)() const;
    using TransitionSignature = void (Stroke::*)();
    using CanCancelSignature = bool (Stroke::*)() const;
    using SupportsSuspensionSignature = bool (Stroke::*)();
    using SuspendStrokeSignature = void (Stroke::*)(KisStrokeSP);

    static_assert(std::is_same_v<decltype(static_cast<NameSignature>(&Stroke::name)), NameSignature>);
    static_assert(std::is_same_v<decltype(static_cast<IdSignature>(&Stroke::id)), IdSignature>);
    static_assert(std::is_same_v<decltype(static_cast<TransitionSignature>(&Stroke::endStroke)), TransitionSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<TransitionSignature>(&Stroke::cancelStroke)), TransitionSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CanCancelSignature>(&Stroke::canCancel)), CanCancelSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SupportsSuspensionSignature>(&Stroke::supportsSuspension)),
                                 SupportsSuspensionSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SuspendStrokeSignature>(&Stroke::suspendStroke)), SuspendStrokeSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::strokeStateAndStrategyPolicySignaturesRemainStable()
{
    using Stroke = KisStroke;
    using BooleanQuery = bool (Stroke::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::isInitialized)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::isEnded)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::isCancelled)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::isExclusive)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::supportsWrapAroundMode)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::canForgetAboutMe)), BooleanQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::isAsynchronouslyCancellable)), BooleanQuery>);
    static_assert(std::is_same_v<decltype(static_cast<BooleanQuery>(&Stroke::clearsRedoOnStart)), BooleanQuery>);

    QVERIFY(true);
}

void KisImageTypesContractTest::strokeLodAndBalancingSignaturesRemainStable()
{
    using Stroke = KisStroke;
    using IntegerQuery = int (Stroke::*)() const;
    using RatioQuery = qreal (Stroke::*)() const;
    using SetLodBuddySignature = void (Stroke::*)(KisStrokeSP);
    using LodBuddyQuery = KisStrokeSP (Stroke::*)() const;
    using TypeQuery = Stroke::Type (Stroke::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<IntegerQuery>(&Stroke::worksOnLevelOfDetail)), IntegerQuery>);
    static_assert(std::is_same_v<decltype(static_cast<RatioQuery>(&Stroke::balancingRatioOverride)), RatioQuery>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetLodBuddySignature>(&Stroke::setLodBuddy)), SetLodBuddySignature>);
    static_assert(std::is_same_v<decltype(static_cast<LodBuddyQuery>(&Stroke::lodBuddy)), LodBuddyQuery>);
    static_assert(std::is_same_v<decltype(static_cast<TypeQuery>(&Stroke::type)), TypeQuery>);

    QVERIFY(true);
}

void KisImageTypesContractTest::sequentialIteratorTypeAndAliasSchemaRemainsStable()
{
    using ConstPolicy = ReadOnlyIteratorPolicy<>;
    using WritablePolicy = WritableIteratorPolicy<>;
    using ConstIterator = KisSequentialIteratorBase<ConstPolicy>;
    using WritableIterator = KisSequentialIteratorBase<WritablePolicy>;

    static_assert(std::is_same_v<KisSequentialConstIterator, ConstIterator>);
    static_assert(std::is_same_v<KisSequentialIterator, WritableIterator>);
    static_assert(std::is_same_v<ConstPolicy::IteratorTypeSP, KisHLineConstIteratorSP>);
    static_assert(std::is_same_v<WritablePolicy::IteratorTypeSP, KisHLineIteratorSP>);
    static_assert(std::is_class_v<ConstIterator>);
    static_assert(std::is_class_v<DevicePolicy>);
    static_assert(std::is_class_v<NoProgressPolicy>);
    static_assert(std::is_class_v<ConstPolicy>);
    static_assert(std::is_class_v<WritablePolicy>);

    QVERIFY(true);
}

void KisImageTypesContractTest::sequentialIteratorDevicePolicySchemaRemainsStable()
{
    using PaintDeviceMember = KisPaintDeviceSP DevicePolicy::*;
    using CreateConstIteratorSignature = KisHLineConstIteratorSP (DevicePolicy::*)(const QRect &);
    using CreateIteratorSignature = KisHLineIteratorSP (DevicePolicy::*)(const QRect &);
    using PixelSizeSignature = int (DevicePolicy::*)() const;

    static_assert(std::is_same_v<decltype(&DevicePolicy::m_dev), PaintDeviceMember>);
    static_assert(std::is_constructible_v<DevicePolicy, KisPaintDeviceSP>);
    static_assert(std::is_constructible_v<DevicePolicy, PaintDeviceConvertible>);
    static_assert(
        std::is_same_v<decltype(static_cast<CreateConstIteratorSignature>(&DevicePolicy::createConstIterator)),
                       CreateConstIteratorSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CreateIteratorSignature>(&DevicePolicy::createIterator)),
                                 CreateIteratorSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<PixelSizeSignature>(&DevicePolicy::pixelSize)), PixelSizeSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::sequentialIteratorAccessPolicySchemaRemainsStable()
{
    using ConstPolicy = ReadOnlyIteratorPolicy<>;
    using WritablePolicy = WritableIteratorPolicy<>;
    using ConstIteratorMember = KisHLineConstIteratorSP ConstPolicy::*;
    using WritableIteratorMember = KisHLineIteratorSP WritablePolicy::*;
    using ConstRawDataSignature = const quint8 *(ConstPolicy::*)() const;
    using WritableConstRawDataSignature = const quint8 *(WritablePolicy::*)() const;
    using WritableRawDataSignature = quint8 *(WritablePolicy::*)();
    using ConstUpdateSignature = void (ConstPolicy::*)();
    using WritableUpdateSignature = void (WritablePolicy::*)();

    static_assert(std::is_same_v<decltype(&ConstPolicy::m_iter), ConstIteratorMember>);
    static_assert(std::is_constructible_v<ConstPolicy, DevicePolicy, const QRect &>);
    static_assert(
        std::is_same_v<decltype(static_cast<ConstRawDataSignature>(&ConstPolicy::oldRawData)), ConstRawDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ConstRawDataSignature>(&ConstPolicy::rawDataConst)),
                                 ConstRawDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<ConstUpdateSignature>(&ConstPolicy::updatePointersCache)),
                                 ConstUpdateSignature>);

    static_assert(std::is_same_v<decltype(&WritablePolicy::m_iter), WritableIteratorMember>);
    static_assert(std::is_constructible_v<WritablePolicy, DevicePolicy, const QRect &>);
    static_assert(std::is_same_v<decltype(static_cast<WritableConstRawDataSignature>(&WritablePolicy::oldRawData)),
                                 WritableConstRawDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WritableRawDataSignature>(&WritablePolicy::rawData)),
                                 WritableRawDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WritableConstRawDataSignature>(&WritablePolicy::rawDataConst)),
                                 WritableConstRawDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WritableUpdateSignature>(&WritablePolicy::updatePointersCache)),
                                 WritableUpdateSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::sequentialIteratorNoProgressPolicySchemaRemainsStable()
{
    using SetRangeSignature = void (NoProgressPolicy::*)(int, int);
    using SetValueSignature = void (NoProgressPolicy::*)(int);
    using SetFinishedSignature = void (NoProgressPolicy::*)();

    static_assert(
        std::is_same_v<decltype(static_cast<SetRangeSignature>(&NoProgressPolicy::setRange)), SetRangeSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetValueSignature>(&NoProgressPolicy::setValue)), SetValueSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetFinishedSignature>(&NoProgressPolicy::setFinished)),
                                 SetFinishedSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::sequentialIteratorTraversalSchemaRemainsStable()
{
    using Iterator = KisSequentialIterator;
    using ConsecutivePixelsSignature = int (Iterator::*)() const;
    using NextPixelSignature = bool (Iterator::*)();
    using NextPixelsSignature = bool (Iterator::*)(int);
    using OldRawDataSignature = const quint8 *(Iterator::*)() const;
    using RawDataSignature = quint8 *(Iterator::*)();
    using RawDataConstSignature = const quint8 *(Iterator::*)() const;
    using CoordinateSignature = int (Iterator::*)() const;

    static_assert(std::is_constructible_v<Iterator, DevicePolicy, const QRect &, NoProgressPolicy>);
    static_assert(std::is_constructible_v<Iterator, DevicePolicy, const QRect &>);
    static_assert(std::is_destructible_v<Iterator>);
    static_assert(std::is_same_v<decltype(static_cast<ConsecutivePixelsSignature>(&Iterator::nConseqPixels)),
                                 ConsecutivePixelsSignature>);
    static_assert(std::is_same_v<decltype(static_cast<NextPixelSignature>(&Iterator::nextPixel)), NextPixelSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<NextPixelsSignature>(&Iterator::nextPixels)), NextPixelsSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<OldRawDataSignature>(&Iterator::oldRawData)), OldRawDataSignature>);
    static_assert(std::is_same_v<decltype(static_cast<RawDataSignature>(&Iterator::rawData)), RawDataSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<RawDataConstSignature>(&Iterator::rawDataConst)), RawDataConstSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CoordinateSignature>(&Iterator::x)), CoordinateSignature>);
    static_assert(std::is_same_v<decltype(static_cast<CoordinateSignature>(&Iterator::y)), CoordinateSignature>);

    QVERIFY(true);
}

void KisImageTypesContractTest::layerMergePolicyAndOrderingSignaturesRemainStable()
{
    static_assert(std::is_same_v<KisLayerUtils::MergeFlags, QFlags<KisLayerUtils::MergeFlag>>);
    static_assert(KisLayerUtils::None == 0);
    static_assert(KisLayerUtils::SkipMergingFrames == 1);

    ASSERT_LAYER_UTILS_SIGNATURE(sortMergeableNodes, KisNodeList (*)(KisNodeSP, KisNodeList));
    ASSERT_LAYER_UTILS_SIGNATURE(sortMergeableNodes, void (*)(KisNodeSP, QList<KisNodeSP> &, QList<KisNodeSP> &));
    ASSERT_LAYER_UTILS_SIGNATURE(filterMergeableNodes, void (*)(KisNodeList &, bool));
    ASSERT_LAYER_UTILS_SIGNATURE(sortAndFilterAnyMergeableNodesSafe, KisNodeList (*)(const KisNodeList &, KisImageSP));
    ASSERT_LAYER_UTILS_SIGNATURE(checkIsChildOf, bool (*)(KisNodeSP, const KisNodeList &));
    ASSERT_LAYER_UTILS_SIGNATURE(checkIsCloneOf, bool (*)(KisNodeSP, const KisNodeList &));
    ASSERT_LAYER_UTILS_SIGNATURE(filterUnlockedNodes, void (*)(KisNodeList &));
    ASSERT_LAYER_UTILS_SIGNATURE(sortAndFilterMergeableInternalNodes, KisNodeList (*)(KisNodeList, bool));
    ASSERT_LAYER_UTILS_SIGNATURE(sortMergeableInternalNodes, KisNodeList (*)(KisNodeList));

    static_assert(std::is_same_v<decltype(KisLayerUtils::filterMergeableNodes(std::declval<KisNodeList &>())), void>);
    static_assert(
        std::is_same_v<decltype(KisLayerUtils::sortAndFilterMergeableInternalNodes(std::declval<KisNodeList>())),
                       KisNodeList>);

    QVERIFY(true);
}

void KisImageTypesContractTest::layerMutationAndProjectionCommandSignaturesRemainStable()
{
    ASSERT_LAYER_UTILS_SIGNATURE(refreshHiddenAreaAsync, void (*)(KisImageSP, KisNodeSP, const QRect &));
    ASSERT_LAYER_UTILS_SIGNATURE(recursiveTightNodeVisibleBounds, QRect (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(forceAllDelayedNodesUpdate, void (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(hasDelayedNodeWithUpdates, bool (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(forceAllHiddenOriginalsUpdate, void (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(
        mergeDown,
        void (*)(KisImageSP, KisLayerSP, const KisMetaData::MergeStrategy *, KisLayerUtils::MergeFlags));
    ASSERT_LAYER_UTILS_SIGNATURE(mergeMultipleLayers,
                                 void (*)(KisImageSP, KisNodeList, KisNodeSP, KisLayerUtils::MergeFlags));
    ASSERT_LAYER_UTILS_SIGNATURE(newLayerFromVisible, void (*)(KisImageSP, KisNodeSP, KisLayerUtils::MergeFlags));
    ASSERT_LAYER_UTILS_SIGNATURE(mergeMultipleNodes,
                                 void (*)(KisImageSP, KisNodeList, KisNodeSP, KisLayerUtils::MergeFlags));
    ASSERT_LAYER_UTILS_SIGNATURE(tryMergeSelectionMasks, bool (*)(KisImageSP, KisNodeList, KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(flattenLayer, void (*)(KisImageSP, KisLayerSP, KisLayerUtils::MergeFlags));
    ASSERT_LAYER_UTILS_SIGNATURE(flattenImage, void (*)(KisImageSP, KisNodeSP, KisLayerUtils::MergeFlags));
    ASSERT_LAYER_UTILS_SIGNATURE(addCopyOfNameTag, void (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(changeImageDefaultProjectionColor, void (*)(KisImageSP, const KoColor &));
    ASSERT_LAYER_UTILS_SIGNATURE(canChangeImageProfileInvisibly, bool (*)(KisImageSP));
    ASSERT_LAYER_UTILS_SIGNATURE(splitAlphaToMask, void (*)(KisImageSP, KisNodeSP, const QString &));
    ASSERT_LAYER_UTILS_SIGNATURE(convertToPaintLayer, std::future<KisNodeSP> (*)(KisImageSP, KisNodeSP));

    static_assert(std::is_same_v<decltype(KisLayerUtils::mergeDown(std::declval<KisImageSP>(),
                                                                   std::declval<KisLayerSP>(),
                                                                   std::declval<const KisMetaData::MergeStrategy *>())),
                                 void>);
    static_assert(std::is_same_v<decltype(KisLayerUtils::mergeMultipleLayers(std::declval<KisImageSP>(),
                                                                             std::declval<KisNodeList>(),
                                                                             std::declval<KisNodeSP>())),
                                 void>);
    static_assert(std::is_same_v<decltype(KisLayerUtils::newLayerFromVisible(std::declval<KisImageSP>(),
                                                                             std::declval<KisNodeSP>())),
                                 void>);
    static_assert(std::is_same_v<decltype(KisLayerUtils::mergeMultipleNodes(std::declval<KisImageSP>(),
                                                                            std::declval<KisNodeList>(),
                                                                            std::declval<KisNodeSP>())),
                                 void>);
    static_assert(
        std::is_same_v<decltype(KisLayerUtils::flattenLayer(std::declval<KisImageSP>(), std::declval<KisLayerSP>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(KisLayerUtils::flattenImage(std::declval<KisImageSP>(), std::declval<KisNodeSP>())),
                       void>);

    QVERIFY(true);
}

void KisImageTypesContractTest::layerHierarchySearchAndTraversalSignaturesRemainStable()
{
    using IntegerChecker = std::function<int(KisNodeSP)>;
    using NodeVisitor = void (*)(KisNodeSP);

    static_assert(std::is_same_v<decltype(KisLayerUtils::checkNodesDiffer<int>(std::declval<KisNodeList>(),
                                                                               std::declval<IntegerChecker>())),
                                 bool>);
    ASSERT_LAYER_UTILS_SIGNATURE(findNodesWithProps, KisNodeList (*)(KisNodeSP, const KoProperties &, bool));
    ASSERT_LAYER_UTILS_SIGNATURE(findRoot, KisNodeSP (*)(KisNodeSP));
    static_assert(std::is_same_v<decltype(KisLayerUtils::recursiveApplyNodes(std::declval<KisNodeSP>(),
                                                                             std::declval<NodeVisitor>())),
                                 void>);
    ASSERT_LAYER_UTILS_SIGNATURE(recursiveFindNode, KisNodeSP (*)(KisNodeSP, std::function<bool(KisNodeSP)>));
    ASSERT_LAYER_UTILS_SIGNATURE(findNodeByUuid, KisNodeSP (*)(KisNodeSP, const QUuid &));
    ASSERT_LAYER_UTILS_SIGNATURE(findNodesByName, QList<KisNodeSP> (*)(KisNodeSP, const QString &, bool, bool));
    ASSERT_LAYER_UTILS_SIGNATURE(findNodeByName, KisNodeSP (*)(KisNodeSP, const QString &));
    ASSERT_LAYER_UTILS_SIGNATURE(findIsolationRoot, KisNodeSP (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(findImageByHierarchy, KisImageSP (*)(KisNodeSP));
    static_assert(
        std::is_same_v<decltype(KisLayerUtils::findNodeByType<KisNode>(std::declval<KisNodeSP>())), KisNode *>);

    QVERIFY(true);
}

void KisImageTypesContractTest::layerFrameQueryAndJobSchemaRemainStable()
{
    static_assert(std::is_same_v<KisLayerUtils::FrameJobs, QMap<int, QSet<KisNodeSP>>>);
    ASSERT_LAYER_UTILS_SIGNATURE(updateFrameJobs, void (*)(KisLayerUtils::FrameJobs *, KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(updateFrameJobsRecursive, void (*)(KisLayerUtils::FrameJobs *, KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(fetchLayerFrames, QSet<int> (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(fetchLayerFramesRecursive, QSet<int> (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(fetchLayerActiveRasterFrameTime, int (*)(KisNodeSP));
    ASSERT_LAYER_UTILS_SIGNATURE(fetchLayerActiveRasterFrameSpan, KisTimeSpan (*)(KisNodeSP, int));
    ASSERT_LAYER_UTILS_SIGNATURE(fetchLayerIdenticalRasterFrameTimes, QSet<int> (*)(KisNodeSP, const int &));
    ASSERT_LAYER_UTILS_SIGNATURE(filterTimesForOnlyRasterKeyedTimes, QSet<int> (*)(KisNodeSP, const QSet<int> &));
    ASSERT_LAYER_UTILS_SIGNATURE(fetchUniqueFrameTimes, QSet<int> (*)(KisNodeSP, QSet<int>, bool));

    QVERIFY(true);
}

void KisImageTypesContractTest::layerCommandTypesAndLifecycleSchemaRemainStable()
{
    using SwitchCommand = KisLayerUtils::SwitchFrameCommand;
    using SharedStorage = SwitchCommand::SharedStorage;
    using KeepSelected = KisLayerUtils::KeepNodesSelectedCommand;
    using RemoveNodeHelper = KisLayerUtils::RemoveNodeHelper;
    using SelectGlobalMask = KisLayerUtils::SelectGlobalSelectionMask;
    using SimpleRemove = KisLayerUtils::SimpleRemoveLayers;
    using SimpleUpdate = KisLayerUtils::KisSimpleUpdateCommand;
    using PartBSignature = void (KeepSelected::*)();
    using SimplePartBSignature = void (SimpleUpdate::*)();
    using UpdateNodesSignature = void (*)(const KisNodeList &);
    using RedoSignature = void (SelectGlobalMask::*)();
    using PopulateSignature = void (SimpleRemove::*)();

    static_assert(std::is_same_v<SwitchCommand::SharedStorageSP, QSharedPointer<SharedStorage>>);
    static_assert(std::is_class_v<SwitchCommand>);
    static_assert(std::is_class_v<SharedStorage>);
    static_assert(std::is_class_v<KeepSelected>);
    static_assert(std::is_class_v<RemoveNodeHelper>);
    static_assert(std::is_class_v<SelectGlobalMask>);
    static_assert(std::is_class_v<SimpleRemove>);
    static_assert(std::is_class_v<SimpleUpdate>);
    static_assert(std::is_base_of_v<KisCommandUtils::FlipFlopCommand, SwitchCommand>);
    static_assert(std::is_base_of_v<KisCommandUtils::FlipFlopCommand, KeepSelected>);
    static_assert(std::is_base_of_v<KisCommandUtils::FlipFlopCommand, SimpleUpdate>);
    static_assert(std::is_base_of_v<KUndo2Command, SelectGlobalMask>);
    static_assert(std::is_base_of_v<RemoveNodeHelper, SimpleRemove>);
    static_assert(std::is_base_of_v<KisCommandUtils::AggregateCommand, SimpleRemove>);
    static_assert(std::is_abstract_v<RemoveNodeHelper>);
    static_assert(std::has_virtual_destructor_v<RemoveNodeHelper>);
    static_assert(std::is_same_v<decltype(SharedStorage::value), int>);

    static_assert(std::is_constructible_v<KeepSelected,
                                          const KisNodeList &,
                                          const KisNodeList &,
                                          KisNodeSP,
                                          KisNodeSP,
                                          KisImageSP,
                                          bool,
                                          KUndo2Command *>);
    static_assert(std::is_constructible_v<KeepSelected,
                                          const KisNodeList &,
                                          const KisNodeList &,
                                          KisNodeSP,
                                          KisNodeSP,
                                          KisImageSP,
                                          bool>);
    static_assert(std::is_same_v<decltype(static_cast<PartBSignature>(&KeepSelected::partB)), PartBSignature>);

    static_assert(std::is_constructible_v<SimpleUpdate, KisNodeList, bool, KUndo2Command *>);
    static_assert(std::is_constructible_v<SimpleUpdate, KisNodeList, bool>);
    static_assert(
        std::is_same_v<decltype(static_cast<SimplePartBSignature>(&SimpleUpdate::partB)), SimplePartBSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<UpdateNodesSignature>(&SimpleUpdate::updateNodes)), UpdateNodesSignature>);

    static_assert(std::is_constructible_v<SelectGlobalMask, KisImageSP>);
    static_assert(std::is_destructible_v<SelectGlobalMask>);
    static_assert(std::is_same_v<decltype(static_cast<RedoSignature>(&SelectGlobalMask::redo)), RedoSignature>);

    static_assert(std::is_constructible_v<SimpleRemove, const KisNodeList &, KisImageSP>);
    static_assert(std::is_same_v<decltype(static_cast<PopulateSignature>(&SimpleRemove::populateChildCommands)),
                                 PopulateSignature>);

    static_assert(std::is_destructible_v<SharedStorage>);
    static_assert(std::is_constructible_v<SwitchCommand, KisImageSP, int, bool, SwitchCommand::SharedStorageSP>);
    static_assert(std::is_destructible_v<SwitchCommand>);

    QVERIFY(true);
}

void KisImageTypesContractTest::uniformPaintOpPropertyOwnershipSchemaRemainsStable()
{
    static_assert(std::is_same_v<KisUniformPaintOpPropertySP, QSharedPointer<KisUniformPaintOpProperty>>);
    static_assert(std::is_same_v<KisUniformPaintOpPropertyWSP, QWeakPointer<KisUniformPaintOpProperty>>);
    static_assert(
        std::is_same_v<KisUniformPaintOpPropertyCallback, KisCallbackBasedPaintopProperty<KisUniformPaintOpProperty>>);
    static_assert(std::is_base_of_v<QObject, KisUniformPaintOpProperty>);
}

void KisImageTypesContractTest::uniformPaintOpPropertyTypeSchemaRemainsStable()
{
    static_assert(std::is_enum_v<KisUniformPaintOpProperty::Type>);
    static_assert(std::is_enum_v<KisUniformPaintOpProperty::SubType>);
    static_assert(KisUniformPaintOpProperty::Int == 0);
    static_assert(KisUniformPaintOpProperty::Double == 1);
    static_assert(KisUniformPaintOpProperty::Bool == 2);
    static_assert(KisUniformPaintOpProperty::Combo == 3);
    static_assert(KisUniformPaintOpProperty::SubType_None == 0);
    static_assert(KisUniformPaintOpProperty::SubType_Angle == 1);
}

void KisImageTypesContractTest::uniformPaintOpPropertyLifetimeSchemaRemainsStable()
{
    static_assert(std::is_constructible_v<KisUniformPaintOpProperty,
                                          KisUniformPaintOpProperty::Type,
                                          KisUniformPaintOpProperty::SubType,
                                          const KoID &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);
    static_assert(std::is_constructible_v<KisUniformPaintOpProperty,
                                          KisUniformPaintOpProperty::Type,
                                          const KoID &,
                                          KisPaintOpSettingsRestrictedSP,
                                          QObject *>);
    static_assert(
        std::is_constructible_v<KisUniformPaintOpProperty, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisUniformPaintOpProperty>);
}

void KisImageTypesContractTest::uniformPaintOpPropertyIdentityAndSettingsSignaturesRemainStable()
{
    using ConfigurationSignature = KisPropertiesConfiguration *(KisUniformPaintOpProperty::*)() const;
    using StringSignature = QString (KisUniformPaintOpProperty::*)() const;
    using VisibilitySignature = bool (KisUniformPaintOpProperty::*)() const;
    using SettingsSignature = KisPaintOpSettingsSP (KisUniformPaintOpProperty::*)() const;
    using SubTypeSignature = KisUniformPaintOpProperty::SubType (KisUniformPaintOpProperty::*)() const;
    using TypeSignature = KisUniformPaintOpProperty::Type (KisUniformPaintOpProperty::*)() const;

    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::configuration), ConfigurationSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::id), StringSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::isVisible), VisibilitySignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::name), StringSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::settings), SettingsSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::subType), SubTypeSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::type), TypeSignature>);
}

void KisImageTypesContractTest::uniformPaintOpPropertyValueAndNotificationSignaturesRemainStable()
{
    using WidgetSignature = QWidget *(KisUniformPaintOpProperty::*)();
    using RequestSignature = void (KisUniformPaintOpProperty::*)();
    using ValueSetterSignature = void (KisUniformPaintOpProperty::*)(const QVariant &);
    using ValueSignature = QVariant (KisUniformPaintOpProperty::*)() const;

    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::createPropertyWidget), WidgetSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::requestReadValue), RequestSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::setValue), ValueSetterSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::value), ValueSignature>);
    static_assert(std::is_same_v<decltype(&KisUniformPaintOpProperty::valueChanged), ValueSetterSignature>);
}

void KisImageTypesContractTest::sliderPaintOpPropertyTypeAndAliasSchemaRemainStable()
{
    using IntSlider = KisSliderBasedPaintOpProperty<int>;
    using DoubleSlider = KisSliderBasedPaintOpProperty<qreal>;

    static_assert(std::is_class_v<KisSliderBasedPaintOpPropertyBase>);
    static_assert(std::is_base_of_v<KisUniformPaintOpProperty, KisSliderBasedPaintOpPropertyBase>);
    static_assert(std::is_class_v<IntSlider>);
    static_assert(std::is_base_of_v<KisSliderBasedPaintOpPropertyBase, IntSlider>);
    static_assert(std::is_base_of_v<KisSliderBasedPaintOpPropertyBase, DoubleSlider>);
    static_assert(std::is_same_v<KisIntSliderBasedPaintOpProperty, IntSlider>);
    static_assert(std::is_same_v<KisDoubleSliderBasedPaintOpProperty, DoubleSlider>);
    static_assert(std::is_same_v<KisIntSliderBasedPaintOpPropertyCallback, KisCallbackBasedPaintopProperty<IntSlider>>);
    static_assert(
        std::is_same_v<KisDoubleSliderBasedPaintOpPropertyCallback, KisCallbackBasedPaintopProperty<DoubleSlider>>);
}

void KisImageTypesContractTest::sliderPaintOpPropertyConstructionSchemaRemainsStable()
{
    using IntSlider = KisIntSliderBasedPaintOpProperty;
    using DoubleSlider = KisDoubleSliderBasedPaintOpProperty;
    using Type = KisUniformPaintOpProperty::Type;
    using SubType = KisUniformPaintOpProperty::SubType;

    static_assert(
        std::is_constructible_v<IntSlider, Type, SubType, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(
        std::is_constructible_v<DoubleSlider, Type, SubType, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<IntSlider, Type, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<DoubleSlider, Type, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<IntSlider, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
    static_assert(std::is_constructible_v<DoubleSlider, const KoID &, KisPaintOpSettingsRestrictedSP, QObject *>);
}

void KisImageTypesContractTest::sliderPaintOpPropertyRangeAndStepSignaturesRemainStable()
{
    using IntSlider = KisIntSliderBasedPaintOpProperty;
    using DoubleSlider = KisDoubleSliderBasedPaintOpProperty;
    using IntGetter = int (IntSlider::*)() const;
    using IntSetter = void (IntSlider::*)(int);
    using DoubleGetter = qreal (DoubleSlider::*)() const;
    using DoubleSetter = void (DoubleSlider::*)(qreal);

    static_assert(std::is_same_v<decltype(&IntSlider::min), IntGetter>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::min), DoubleGetter>);
    static_assert(std::is_same_v<decltype(&IntSlider::max), IntGetter>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::max), DoubleGetter>);
    static_assert(std::is_same_v<decltype(&IntSlider::setRange), void (IntSlider::*)(int, int)>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::setRange), void (DoubleSlider::*)(qreal, qreal)>);
    static_assert(std::is_same_v<decltype(&IntSlider::singleStep), IntGetter>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::singleStep), DoubleGetter>);
    static_assert(std::is_same_v<decltype(&IntSlider::setSingleStep), IntSetter>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::setSingleStep), DoubleSetter>);
    static_assert(std::is_same_v<decltype(&IntSlider::pageStep), IntGetter>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::pageStep), DoubleGetter>);
    static_assert(std::is_same_v<decltype(&IntSlider::setPageStep), IntSetter>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::setPageStep), DoubleSetter>);
}

void KisImageTypesContractTest::sliderPaintOpPropertyDisplaySignaturesRemainStable()
{
    using IntSlider = KisIntSliderBasedPaintOpProperty;
    using DoubleSlider = KisDoubleSliderBasedPaintOpProperty;

    static_assert(std::is_same_v<decltype(&IntSlider::exponentRatio), qreal (IntSlider::*)() const>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::exponentRatio), qreal (DoubleSlider::*)() const>);
    static_assert(std::is_same_v<decltype(&IntSlider::setExponentRatio), void (IntSlider::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::setExponentRatio), void (DoubleSlider::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&IntSlider::decimals), int (IntSlider::*)() const>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::decimals), int (DoubleSlider::*)() const>);
    static_assert(std::is_same_v<decltype(&IntSlider::setDecimals), void (IntSlider::*)(int)>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::setDecimals), void (DoubleSlider::*)(int)>);
    static_assert(std::is_same_v<decltype(&IntSlider::suffix), QString (IntSlider::*)() const>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::suffix), QString (DoubleSlider::*)() const>);
    static_assert(std::is_same_v<decltype(&IntSlider::setSuffix), void (IntSlider::*)(QString)>);
    static_assert(std::is_same_v<decltype(&DoubleSlider::setSuffix), void (DoubleSlider::*)(QString)>);
}

void KisImageTypesContractTest::sliderPaintOpPropertyRangeNotificationSignatureRemainsStable()
{
    static_assert(std::is_same_v<decltype(&KisSliderBasedPaintOpPropertyBase::sigRangeChanged),
                                 void (KisSliderBasedPaintOpPropertyBase::*)()>);
}

void KisImageTypesContractTest::memoryStatisticsServerTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMemoryStatisticsServer>);
    static_assert(std::is_base_of_v<QObject, KisMemoryStatisticsServer>);
    static_assert(std::is_default_constructible_v<KisMemoryStatisticsServer>);
    static_assert(std::has_virtual_destructor_v<KisMemoryStatisticsServer>);
}

void KisImageTypesContractTest::memoryStatisticsImageAndAggregateDefaultsRemainStable()
{
    using Statistics = KisMemoryStatisticsServer::Statistics;
    using Member = qint64 Statistics::*;

    static_assert(std::is_class_v<Statistics>);
    static_assert(std::is_default_constructible_v<Statistics>);
    static_assert(std::is_same_v<decltype(&Statistics::imageSize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::layersSize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::projectionsSize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::lodSize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::totalMemorySize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::realMemorySize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::historicalMemorySize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::poolSize), Member>);

    const Statistics statistics;
    QCOMPARE(statistics.imageSize, qint64(0));
    QCOMPARE(statistics.layersSize, qint64(0));
    QCOMPARE(statistics.projectionsSize, qint64(0));
    QCOMPARE(statistics.lodSize, qint64(0));
    QCOMPARE(statistics.totalMemorySize, qint64(0));
    QCOMPARE(statistics.realMemorySize, qint64(0));
    QCOMPARE(statistics.historicalMemorySize, qint64(0));
    QCOMPARE(statistics.poolSize, qint64(0));
}

void KisImageTypesContractTest::memoryStatisticsSwapAndLimitDefaultsRemainStable()
{
    using Statistics = KisMemoryStatisticsServer::Statistics;
    using Member = qint64 Statistics::*;

    static_assert(std::is_same_v<decltype(&Statistics::swapSize), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::totalMemoryLimit), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::tilesHardLimit), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::tilesSoftLimit), Member>);
    static_assert(std::is_same_v<decltype(&Statistics::tilesPoolLimit), Member>);

    const Statistics statistics;
    QCOMPARE(statistics.swapSize, qint64(0));
    QCOMPARE(statistics.totalMemoryLimit, qint64(0));
    QCOMPARE(statistics.tilesHardLimit, qint64(0));
    QCOMPARE(statistics.tilesSoftLimit, qint64(0));
    QCOMPARE(statistics.tilesPoolLimit, qint64(0));
}

void KisImageTypesContractTest::memoryStatisticsCollectionSignaturesRemainStable()
{
    using InstanceSignature = KisMemoryStatisticsServer *(*)();
    using FetchSignature = KisMemoryStatisticsServer::Statistics (KisMemoryStatisticsServer::*)(KisImageSP) const;

    static_assert(std::is_same_v<decltype(&KisMemoryStatisticsServer::instance), InstanceSignature>);
    static_assert(std::is_same_v<decltype(&KisMemoryStatisticsServer::fetchMemoryStatistics), FetchSignature>);
}

void KisImageTypesContractTest::memoryStatisticsUpdateNotificationSignaturesRemainStable()
{
    using VoidSignature = void (KisMemoryStatisticsServer::*)();

    static_assert(std::is_same_v<decltype(&KisMemoryStatisticsServer::notifyImageChanged), VoidSignature>);
    static_assert(
        std::is_same_v<decltype(&KisMemoryStatisticsServer::tryForceUpdateMemoryStatisticsWhileIdle), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisMemoryStatisticsServer::sigUpdateMemoryStatistics), VoidSignature>);
}

void KisImageTypesContractTest::paintDeviceFrameOwnershipAndTestingDataSchemaRemainsStable()
{
    using TestingData = KisPaintDeviceFramesInterface::TestingDataObjects;

    static_assert(std::is_class_v<KisPaintDeviceFramesInterface>);
    static_assert(std::is_class_v<TestingData>);
    static_assert(std::is_same_v<TestingData::Data, KisPaintDeviceData>);
    static_assert(std::is_same_v<TestingData::FramesHash, QHash<int, KisPaintDeviceData *>>);
    static_assert(std::is_same_v<decltype(TestingData::m_currentData), KisPaintDeviceData *>);
    static_assert(std::is_same_v<decltype(TestingData::m_data), KisPaintDeviceData *>);
    static_assert(std::is_same_v<decltype(TestingData::m_externalFrameData), KisPaintDeviceData *>);
    static_assert(std::is_same_v<decltype(TestingData::m_frames), TestingData::FramesHash>);
    static_assert(std::is_same_v<decltype(TestingData::m_lodData), KisPaintDeviceData *>);
}

void KisImageTypesContractTest::paintDeviceFrameLifecycleAndIdentitySignaturesRemainStable()
{
    using CreateSignature = int (KisPaintDeviceFramesInterface::*)(bool, int, const QPoint &, KUndo2Command *);
    using CurrentSignature = int (KisPaintDeviceFramesInterface::*)() const;
    using FramesSignature = QList<int> (KisPaintDeviceFramesInterface::*)();

    static_assert(std::is_constructible_v<KisPaintDeviceFramesInterface, KisPaintDevice *>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::createFrame), CreateSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::currentFrameId), CurrentSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::frames), FramesSignature>);
}

void KisImageTypesContractTest::paintDeviceFrameGeometryAndPixelSignaturesRemainStable()
{
    using BoundsSignature = QRect (KisPaintDeviceFramesInterface::*)(int);
    using DefaultPixelSignature = KoColor (KisPaintDeviceFramesInterface::*)(int) const;
    using OffsetSignature = QPoint (KisPaintDeviceFramesInterface::*)(int) const;
    using SetDefaultPixelSignature = void (KisPaintDeviceFramesInterface::*)(const KoColor &, int);
    using SetOffsetSignature = void (KisPaintDeviceFramesInterface::*)(int, const QPoint &);

    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::frameBounds), BoundsSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::frameDefaultPixel), DefaultPixelSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::frameOffset), OffsetSignature>);
    static_assert(
        std::is_same_v<decltype(&KisPaintDeviceFramesInterface::setFrameDefaultPixel), SetDefaultPixelSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::setFrameOffset), SetOffsetSignature>);
}

void KisImageTypesContractTest::paintDeviceFrameTransferAndPersistenceSignaturesRemainStable()
{
    using DataManagerSignature = KisDataManagerSP (KisPaintDeviceFramesInterface::*)(int) const;
    using ReadSignature = bool (KisPaintDeviceFramesInterface::*)(QIODevice *, int);
    using UploadSignature = void (KisPaintDeviceFramesInterface::*)(int, KisPaintDeviceSP);
    using UploadFromFrameSignature = void (KisPaintDeviceFramesInterface::*)(int, int, KisPaintDeviceSP);
    using WriteSignature = bool (KisPaintDeviceFramesInterface::*)(KisPaintDeviceWriter &, int);
    using WriteToDeviceSignature = void (KisPaintDeviceFramesInterface::*)(int, KisPaintDeviceSP);

    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::frameDataManager), DataManagerSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::readFrame), ReadSignature>);
    static_assert(std::is_same_v<decltype(static_cast<UploadSignature>(&KisPaintDeviceFramesInterface::uploadFrame)),
                                 UploadSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<UploadFromFrameSignature>(&KisPaintDeviceFramesInterface::uploadFrame)),
                       UploadFromFrameSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::writeFrame), WriteSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::writeFrameToDevice), WriteToDeviceSignature>);
}

void KisImageTypesContractTest::paintDeviceFrameUndoCacheAndTestingSignaturesRemainStable()
{
    using DeleteSignature = void (KisPaintDeviceFramesInterface::*)(int, KUndo2Command *);
    using InvalidateSignature = void (KisPaintDeviceFramesInterface::*)(int);
    using TestingDataSignature =
        KisPaintDeviceFramesInterface::TestingDataObjects (KisPaintDeviceFramesInterface::*)() const;
    using TestingListSignature = QList<KisPaintDeviceData *> (KisPaintDeviceFramesInterface::*)() const;

    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::deleteFrame), DeleteSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintDeviceFramesInterface::invalidateFrameCache), InvalidateSignature>);
    static_assert(
        std::is_same_v<decltype(&KisPaintDeviceFramesInterface::testingGetDataObjects), TestingDataSignature>);
    static_assert(
        std::is_same_v<decltype(&KisPaintDeviceFramesInterface::testingGetDataObjectsList), TestingListSignature>);
}

void KisImageTypesContractTest::imageSignalRouterOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_base_of_v<QObject, KisImageSignalRouter>);
    static_assert(std::is_constructible_v<KisImageSignalRouter, KisImageWSP>);
    static_assert(std::has_virtual_destructor_v<KisImageSignalRouter>);
}

void KisImageTypesContractTest::imageSignalRouterNotificationSignaturesRemainStable()
{
    using VoidSignature = void (KisImageSignalRouter::*)();
    using NotificationSignature = void (KisImageSignalRouter::*)(KisImageSignalType);
    using NotificationsSignature = void (KisImageSignalRouter::*)(KisImageSignalVector);

    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitImageModifiedNotification), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitNotification), NotificationSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitNotifications), NotificationsSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigImageModified), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigImageModifiedWithoutUndo), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigNotification), NotificationSignature>);
}

void KisImageTypesContractTest::imageSignalRouterBatchAndLodSignaturesRemainStable()
{
    using VoidSignature = void (KisImageSignalRouter::*)();
    using BooleanSignature = void (KisImageSignalRouter::*)(bool);

    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitNotifyBatchUpdateEnded), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitNotifyBatchUpdateStarted), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitRequestLodPlanesSyncBlocked), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigNotifyBatchUpdateEnded), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigNotifyBatchUpdateStarted), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigRequestLodPlanesSyncBlocked), BooleanSignature>);
}

void KisImageTypesContractTest::imageSignalRouterNodeGraphSignaturesRemainStable()
{
    using RemoveNotificationSignature = void (KisImageSignalRouter::*)(KisNode *, int);
    using NodeSignature = void (KisImageSignalRouter::*)(KisNodeSP);
    using NodeAdditionSignature = void (KisImageSignalRouter::*)(KisNode *, int, KisNodeAdditionFlags);
    using NodeAddedSignalSignature = void (KisImageSignalRouter::*)(KisNodeSP, KisNodeAdditionFlags);
    using VoidSignature = void (KisImageSignalRouter::*)();

    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitAboutToRemoveANode), RemoveNotificationSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitNodeChanged), NodeSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::emitNodeHasBeenAdded), NodeAdditionSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigLayersChangedAsync), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigNodeAddedAsync), NodeAddedSignalSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigNodeChanged), NodeSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigRemoveNodeAsync), NodeSignature>);
}

void KisImageTypesContractTest::imageSignalRouterImageMetadataSignaturesRemainStable()
{
    using ColorSpaceSignature = void (KisImageSignalRouter::*)(const KoColorSpace *);
    using ProfileSignature = void (KisImageSignalRouter::*)(const KoColorProfile *);
    using ReselectionSignature = void (KisImageSignalRouter::*)(KisNodeSP, const KisNodeList &);
    using ResolutionSignature = void (KisImageSignalRouter::*)(double, double);
    using SizeSignature = void (KisImageSignalRouter::*)(const QPointF &, const QPointF &);

    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigColorSpaceChanged), ColorSpaceSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigProfileChanged), ProfileSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigRequestNodeReselection), ReselectionSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigResolutionChanged), ResolutionSignature>);
    static_assert(std::is_same_v<decltype(&KisImageSignalRouter::sigSizeChanged), SizeSignature>);
}

void KisImageTypesContractTest::projectionLeafOwnershipDropReasonAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisProjectionLeaf>);
    static_assert(std::is_enum_v<KisProjectionLeaf::NodeDropReason>);
    static_assert(KisProjectionLeaf::NodeAvailable == 0);
    static_assert(KisProjectionLeaf::DropPassThroughMask == 1);
    static_assert(KisProjectionLeaf::DropPassThroughClone == 2);
    static_assert(std::is_constructible_v<KisProjectionLeaf, KisNode *>);
    static_assert(std::has_virtual_destructor_v<KisProjectionLeaf>);
}

void KisImageTypesContractTest::projectionLeafGraphNavigationSignaturesRemainStable()
{
    using LeafSignature = KisProjectionLeafSP (KisProjectionLeaf::*)() const;
    using NodeSignature = KisNodeSP (KisProjectionLeaf::*)() const;
    using PlaneSignature = KisAbstractProjectionPlaneSP (KisProjectionLeaf::*)() const;

    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::parent), LeafSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::firstChild), LeafSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::lastChild), LeafSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::prevSibling), LeafSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::nextSibling), LeafSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::node), NodeSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::projectionPlane), PlaneSignature>);
}

void KisImageTypesContractTest::projectionLeafVisitorAndProjectionAccessSignaturesRemainStable()
{
    using AcceptSignature = bool (KisProjectionLeaf::*)(KisNodeVisitor &);
    using DeviceSignature = KisPaintDeviceSP (KisProjectionLeaf::*)();
    using RegenerateSignature = void (KisProjectionLeaf::*)();

    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::accept), AcceptSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::original), DeviceSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::projection), DeviceSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::lazyDestinationForSubtreeComposition), DeviceSignature>);
    static_assert(
        std::is_same_v<decltype(&KisProjectionLeaf::explicitlyRegeneratePassThroughProjection), RegenerateSignature>);
}

void KisImageTypesContractTest::projectionLeafNodeClassificationAndRenderingSignaturesRemainStable()
{
    using BooleanSignature = bool (KisProjectionLeaf::*)() const;
    using OpacitySignature = quint8 (KisProjectionLeaf::*)() const;
    using ChannelFlagsSignature = QBitArray (KisProjectionLeaf::*)() const;

    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isRoot), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isLayer), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isMask), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::canHaveChildLayers), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::dependsOnLowerNodes), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::visible), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::opacity), OpacitySignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::channelFlags), ChannelFlagsSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isStillInGraph), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::hasClones), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::shouldBeRendered), BooleanSignature>);
}

void KisImageTypesContractTest::projectionLeafDropOverlayAndTemporaryVisibilitySignaturesRemainStable()
{
    using BooleanSignature = bool (KisProjectionLeaf::*)() const;
    using DropReasonSignature = KisProjectionLeaf::NodeDropReason (KisProjectionLeaf::*)() const;
    using SetVisibilitySignature = void (KisProjectionLeaf::*)(bool);

    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isDroppedNode), BooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::dropReason), DropReasonSignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isOverlayProjectionLeaf), BooleanSignature>);
    static_assert(
        std::is_same_v<decltype(&KisProjectionLeaf::setTemporaryHiddenFromRendering), SetVisibilitySignature>);
    static_assert(std::is_same_v<decltype(&KisProjectionLeaf::isTemporaryHiddenFromRendering), BooleanSignature>);
}

void KisImageTypesContractTest::animationInterfaceTypeAndOptionSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisImageAnimationInterface>);
    static_assert(std::is_same_v<KisImageAnimationInterface::SwitchTimeAsyncFlags,
                                 QFlags<KisImageAnimationInterface::SwitchTimeAsyncOption>>);
    static_assert(KisImageAnimationInterface::STAO_NONE == 0);
    static_assert(KisImageAnimationInterface::STAO_USE_UNDO == (1 << 1));
    static_assert(KisImageAnimationInterface::STAO_FORCE_REGENERATION == (1 << 2));
    static_assert(std::is_constructible_v<KisImageAnimationInterface, KisImage *>);
    static_assert(std::is_constructible_v<KisImageAnimationInterface, const KisImageAnimationInterface &, KisImage *>);
    static_assert(std::is_destructible_v<KisImageAnimationInterface>);
}

void KisImageTypesContractTest::animationTimelineAndExportSignaturesRemainStable()
{
    using ConstBooleanSignature = bool (KisImageAnimationInterface::*)() const;
    using ConstIntegerSignature = int (KisImageAnimationInterface::*)() const;
    using StringSignature = QString (KisImageAnimationInterface::*)();

    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::hasAnimation), ConstBooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::currentTime), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::currentUITime), ConstIntegerSignature>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::externalFrameActive), ConstBooleanSignature>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::documentPlaybackRange),
                                 const KisTimeSpan &(KisImageAnimationInterface::*)() const>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setDocumentRange),
                                 void (KisImageAnimationInterface::*)(const KisTimeSpan)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::activePlaybackRange),
                                 const KisTimeSpan &(KisImageAnimationInterface::*)() const>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setActivePlaybackRange),
                                 void (KisImageAnimationInterface::*)(const KisTimeSpan)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::framerate), ConstIntegerSignature>);
    static_assert(
        std::is_same_v<decltype(&KisImageAnimationInterface::setFramerate), void (KisImageAnimationInterface::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::exportSequenceFilePath), StringSignature>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setExportSequenceFilePath),
                                 void (KisImageAnimationInterface::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::exportSequenceBaseName), StringSignature>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setExportSequenceBaseName),
                                 void (KisImageAnimationInterface::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::exportInitialFrameNumber),
                                 int (KisImageAnimationInterface::*)()>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setExportInitialFrameNumber),
                                 void (KisImageAnimationInterface::*)(const int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::activeLayerSelectedTimes),
                                 QSet<int> (KisImageAnimationInterface::*)()>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setActiveLayerSelectedTimes),
                                 void (KisImageAnimationInterface::*)(const QSet<int> &)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::image),
                                 KisImageWSP (KisImageAnimationInterface::*)() const>);
    static_assert(
        std::is_same_v<decltype(&KisImageAnimationInterface::totalLength), int (KisImageAnimationInterface::*)()>);
}

void KisImageTypesContractTest::animationSwitchAndInvalidationSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::requestTimeSwitchWithUndo),
                                 void (KisImageAnimationInterface::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::requestTimeSwitchNonGUI),
                                 void (KisImageAnimationInterface::*)(int, bool)>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisImageAnimationInterface &>().requestTimeSwitchNonGUI(0)), void>);
    static_assert(
        std::is_same_v<decltype(&KisImageAnimationInterface::switchCurrentTimeAsync),
                       void (KisImageAnimationInterface::*)(int, KisImageAnimationInterface::SwitchTimeAsyncFlags)>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisImageAnimationInterface &>().switchCurrentTimeAsync(0)), void>);
    static_assert(std::is_same_v<
                  decltype(&KisImageAnimationInterface::requestFrameRegeneration),
                  void (KisImageAnimationInterface::*)(int, const KisRegion &, bool, KisLockFrameGenerationLock &&)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (KisImageAnimationInterface::*)(const KisNode *, const QRect &, bool)>(
                           &KisImageAnimationInterface::notifyNodeChanged)),
                       void (KisImageAnimationInterface::*)(const KisNode *, const QRect &, bool)>);
    static_assert(
        std::is_same_v<
            decltype(static_cast<void (KisImageAnimationInterface::*)(const KisNode *, const QVector<QRect> &, bool)>(
                &KisImageAnimationInterface::notifyNodeChanged)),
            void (KisImageAnimationInterface::*)(const KisNode *, const QVector<QRect> &, bool)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::invalidateFrames),
                                 void (KisImageAnimationInterface::*)(const KisTimeSpan &, const QRect &)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::invalidateFrame),
                                 void (KisImageAnimationInterface::*)(const int, KisNodeSP)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setDefaultProjectionColor),
                                 void (KisImageAnimationInterface::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setDocumentRangeStartFrame),
                                 void (KisImageAnimationInterface::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::setDocumentRangeEndFrame),
                                 void (KisImageAnimationInterface::*)(int)>);
}

void KisImageTypesContractTest::animationGenerationLockSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::backgroundFrameGenerationBlocked),
                                 bool (KisImageAnimationInterface::*)() const>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::blockBackgroundFrameGeneration),
                                 void (KisImageAnimationInterface::*)()>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::unblockBackgroundFrameGeneration),
                                 void (KisImageAnimationInterface::*)()>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::lockFrameGeneration),
                                 void (KisImageAnimationInterface::*)()>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::unlockFrameGeneration),
                                 void (KisImageAnimationInterface::*)()>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::tryLockFrameGeneration),
                                 bool (KisImageAnimationInterface::*)()>);
}

void KisImageTypesContractTest::animationNotificationSignaturesRemainStable()
{
    using EmptySignal = void (KisImageAnimationInterface::*)();

    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigDocumentRangeChanged), EmptySignal>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigFrameCancelled), EmptySignal>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigFrameReady),
                                 void (KisImageAnimationInterface::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigFrameRegenerated),
                                 void (KisImageAnimationInterface::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigFrameRegenerationSkipped),
                                 void (KisImageAnimationInterface::*)(int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigFramerateChanged), EmptySignal>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigFramesChanged),
                                 void (KisImageAnimationInterface::*)(const KisTimeSpan &, const QRect &)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigInternalRequestTimeSwitch),
                                 void (KisImageAnimationInterface::*)(int, bool)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigKeyframeAdded),
                                 void (KisImageAnimationInterface::*)(const KisKeyframeChannel *, int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigKeyframeRemoved),
                                 void (KisImageAnimationInterface::*)(const KisKeyframeChannel *, int)>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigPlaybackRangeChanged), EmptySignal>);
    static_assert(std::is_same_v<decltype(&KisImageAnimationInterface::sigUiTimeChanged),
                                 void (KisImageAnimationInterface::*)(int)>);
}

void KisImageTypesContractTest::paintOpUpdatePropertyKeySchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(AIRBRUSH_ENABLED), const QString>);
    static_assert(std::is_same_v<decltype(AIRBRUSH_IGNORE_SPACING), const QString>);
    static_assert(std::is_same_v<decltype(AIRBRUSH_RATE), const QString>);
    static_assert(std::is_same_v<decltype(SPACING_USE_UPDATES), const QString>);
}

void KisImageTypesContractTest::paintOpCadenceAndLodPolicySignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(airbrushInterval, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(isAirbrushing, bool (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(useSpacingUpdates, bool (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(needsAsynchronousUpdates, bool (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(lodSizeThreshold, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(lodSizeThresholdSupported, bool (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setLodSizeThreshold, void (KisPaintOpSettings::*)(qreal));
}

void KisImageTypesContractTest::paintOpDepositParameterSignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpOpacity, qreal (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpOpacity, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpFlow, qreal (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpFlow, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpFade, qreal (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpFade, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpScatter, qreal (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpScatter, void (KisPaintOpSettings::*)(qreal));
}

void KisImageTypesContractTest::paintOpGeometryCompositeAndEraserSignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpCompositeOp, QString (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpCompositeOp, void (KisPaintOpSettings::*)(const QString &));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpSize, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpSize, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpAngle, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setPaintOpAngle, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintOpPatternSize, qreal (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(eraserMode, bool (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setEraserMode, void (KisPaintOpSettings::*)(bool));
}

void KisImageTypesContractTest::paintOpSavedBrushAndEraserValueSignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(savedBrushOpacity, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setSavedBrushOpacity, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(savedBrushSize, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setSavedBrushSize, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(savedEraserOpacity, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setSavedEraserOpacity, void (KisPaintOpSettings::*)(qreal));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(savedEraserSize, qreal (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setSavedEraserSize, void (KisPaintOpSettings::*)(qreal));
}

void KisImageTypesContractTest::paintOpSettingsTypeUpdateListenerAndLifetimeSchemaRemainStable()
{
    using UpdateListener = KisPaintOpSettings::UpdateListener;

    static_assert(std::is_class_v<KisPaintOpSettings>);
    static_assert(std::is_abstract_v<KisPaintOpSettings>);
    static_assert(std::is_base_of_v<KisPropertiesConfiguration, KisPaintOpSettings>);
    static_assert(std::is_class_v<UpdateListener>);
    static_assert(std::is_abstract_v<UpdateListener>);
    static_assert(std::is_same_v<KisPaintOpSettings::UpdateListenerSP, QSharedPointer<UpdateListener>>);
    static_assert(std::is_same_v<KisPaintOpSettings::UpdateListenerWSP, QWeakPointer<UpdateListener>>);
    static_assert(std::has_virtual_destructor_v<UpdateListener>);
    static_assert(std::is_same_v<decltype(&UpdateListener::setDirty), void (UpdateListener::*)(bool)>);
    static_assert(std::is_same_v<decltype(&UpdateListener::isDirty), bool (UpdateListener::*)() const>);
    static_assert(std::is_same_v<decltype(&UpdateListener::notifySettingsChanged), void (UpdateListener::*)()>);
    static_assert(std::is_constructible_v<PaintOpSignatureProbe, KisResourcesInterfaceSP>);
    static_assert(std::is_copy_constructible_v<PaintOpSignatureProbe>);
    static_assert(std::has_virtual_destructor_v<KisPaintOpSettings>);
}

void KisImageTypesContractTest::paintOpSettingsInteractionAndOutlineSignaturesRemainStable()
{
    using OutlineMode = KisPaintOpSettings::OutlineMode;

    static_assert(std::is_class_v<OutlineMode>);
    static_assert(std::is_aggregate_v<OutlineMode>);
    static_assert(std::is_same_v<decltype(&OutlineMode::forceCircle), bool OutlineMode::*>);
    static_assert(std::is_same_v<decltype(&OutlineMode::forceFullSize), bool OutlineMode::*>);
    static_assert(std::is_same_v<decltype(&OutlineMode::isVisible), bool OutlineMode::*>);
    static_assert(std::is_same_v<decltype(&OutlineMode::showTiltDecoration), bool OutlineMode::*>);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(activate, void (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(
        brushOutline,
        KisOptimizedBrushOutline (KisPaintOpSettings::*)(const KisPaintInformation &, const OutlineMode &, qreal));
    static_assert(std::is_same_v<decltype(&KisPaintOpSettings::ellipseOutline),
                                 KisOptimizedBrushOutline (*)(qreal, qreal, qreal, qreal)>);
    static_assert(std::is_same_v<decltype(&KisPaintOpSettings::makeTiltIndicator),
                                 QPainterPath (*)(const KisPaintInformation &, const QPointF &, qreal, qreal)>);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(
        mousePressEvent,
        bool (KisPaintOpSettings::*)(const KisPaintInformation &, Qt::KeyboardModifiers, KisNodeWSP));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(mouseReleaseEvent, bool (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(node, KisNodeSP (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(paintIncremental, bool (KisPaintOpSettings::*)());
}

void KisImageTypesContractTest::paintOpSettingsCompositionAndValiditySignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(clone, KisPaintOpSettingsSP (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(resetSettings, void (KisPaintOpSettings::*)(const QStringList &));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(indirectPaintingCompositeOp, QString (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(effectivePaintOpCompositeOp, QString (KisPaintOpSettings::*)());
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(modelName, QString (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setModelName, void (KisPaintOpSettings::*)(const QString &));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(isValid, bool (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setProperty, void (KisPaintOpSettings::*)(const QString &, const QVariant &));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(sanityVersionCookie, quint64 (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(hasPatternSettings, bool (KisPaintOpSettings::*)() const);
}

void KisImageTypesContractTest::paintOpSettingsResourceAndCanvasSignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(resourcesInterface, KisResourcesInterfaceSP (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setResourcesInterface, void (KisPaintOpSettings::*)(KisResourcesInterfaceSP));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(canvasResourcesInterface,
                                      KoCanvasResourcesInterfaceSP (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setCanvasResourcesInterface,
                                      void (KisPaintOpSettings::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(resourceCacheInterface,
                                      KoResourceCacheInterfaceSP (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setResourceCacheInterface,
                                      void (KisPaintOpSettings::*)(KoResourceCacheInterfaceSP));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(regenerateResourceCache,
                                      void (KisPaintOpSettings::*)(KoResourceCacheInterfaceSP));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(requiredCanvasResources, QList<int> (KisPaintOpSettings::*)() const);
}

void KisImageTypesContractTest::paintOpSettingsMaskingLodAndUniformPropertySignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(hasMaskingSettings, bool (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(createMaskingSettings, KisPaintOpSettingsSP (KisPaintOpSettings::*)() const);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(maskingBrushCompositeOp, QString (KisPaintOpSettings::*)() const);
    static_assert(
        std::is_same_v<decltype(&KisPaintOpSettings::isLodUserAllowed), bool (*)(KisPropertiesConfigurationSP)>);
    static_assert(
        std::is_same_v<decltype(&KisPaintOpSettings::setLodUserAllowed), void (*)(KisPropertiesConfigurationSP, bool)>);
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(
        uniformProperties,
        QList<KisUniformPaintOpPropertySP> (KisPaintOpSettings::*)(KisPaintOpSettingsSP,
                                                                   QPointer<KisPaintOpPresetUpdateProxy>));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(setUpdateListener,
                                      void (KisPaintOpSettings::*)(KisPaintOpSettings::UpdateListenerWSP));
    ASSERT_PAINTOP_SETTINGS_SIGNATURE(updateListener,
                                      KisPaintOpSettings::UpdateListenerWSP (KisPaintOpSettings::*)() const);
}

void KisImageTypesContractTest::fixedPaintDeviceOwnershipAndCopySchemaRemainsStable()
{
    using AllocatorSP = KisOptimizedByteArray::MemoryAllocatorSP;

    static_assert(std::is_class_v<KisFixedPaintDevice>);
    static_assert(std::is_base_of_v<KisShared, KisFixedPaintDevice>);
    static_assert(std::is_constructible_v<KisFixedPaintDevice, const KoColorSpace *>);
    static_assert(std::is_constructible_v<KisFixedPaintDevice, const KoColorSpace *, AllocatorSP>);
    static_assert(std::is_copy_constructible_v<KisFixedPaintDevice>);
    static_assert(std::is_copy_assignable_v<KisFixedPaintDevice>);
    static_assert(std::has_virtual_destructor_v<KisFixedPaintDevice>);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(operator=,
                                        KisFixedPaintDevice & (KisFixedPaintDevice::*)(const KisFixedPaintDevice &));
}

void KisImageTypesContractTest::fixedPaintDeviceGeometryCapacityAndColorSchemaRemainsStable()
{
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(setRect, void (KisFixedPaintDevice::*)(const QRect &));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(bounds, QRect (KisFixedPaintDevice::*)() const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(allocatedPixels, int (KisFixedPaintDevice::*)() const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(pixelSize, quint32 (KisFixedPaintDevice::*)() const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(colorSpace, const KoColorSpace *(KisFixedPaintDevice::*)() const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(setColorSpace, void (KisFixedPaintDevice::*)(const KoColorSpace *));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(setProfile, void (KisFixedPaintDevice::*)(const KoColorProfile *));
}

void KisImageTypesContractTest::fixedPaintDeviceBufferStorageSignaturesRemainStable()
{
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(initialize, bool (KisFixedPaintDevice::*)(quint8));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(reallocateBufferWithoutInitialization, void (KisFixedPaintDevice::*)());
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(lazyGrowBufferWithoutInitialization, void (KisFixedPaintDevice::*)());
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(data, quint8 * (KisFixedPaintDevice::*)());
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(data, quint8 * (KisFixedPaintDevice::*)() const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(constData, const quint8 *(KisFixedPaintDevice::*)() const);

    using InitializeWithDefaultValue = decltype(std::declval<KisFixedPaintDevice &>().initialize());
    static_assert(std::is_same_v<InitializeWithDefaultValue, bool>);
}

void KisImageTypesContractTest::fixedPaintDeviceTransferAndConversionSignaturesRemainStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using Flags = KoColorConversionTransformation::ConversionFlags;

    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(readBytes,
                                        void (KisFixedPaintDevice::*)(quint8 *, qint32, qint32, qint32, qint32) const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(convertTo, void (KisFixedPaintDevice::*)(const KoColorSpace *, Intent, Flags));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(convertFromQImage,
                                        void (KisFixedPaintDevice::*)(const QImage &, const QString &));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(convertToQImage,
                                        QImage (KisFixedPaintDevice::*)(const KoColorProfile *, Intent, Flags) const);
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(
        convertToQImage,
        QImage (KisFixedPaintDevice::*)(const KoColorProfile *, qint32, qint32, qint32, qint32, Intent, Flags) const);

    using ConvertWithDefaults = decltype(std::declval<KisFixedPaintDevice &>().convertTo());
    using ImageWithDefaults = decltype(std::declval<const KisFixedPaintDevice &>().convertToQImage(nullptr));
    using RectImageWithDefaults =
        decltype(std::declval<const KisFixedPaintDevice &>().convertToQImage(nullptr, 0, 0, 0, 0));
    static_assert(std::is_same_v<ConvertWithDefaults, void>);
    static_assert(std::is_same_v<ImageWithDefaults, QImage>);
    static_assert(std::is_same_v<RectImageWithDefaults, QImage>);
}

void KisImageTypesContractTest::fixedPaintDevicePixelMutationSignaturesRemainStable()
{
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(clear, void (KisFixedPaintDevice::*)(const QRect &));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(fill,
                                        void (KisFixedPaintDevice::*)(qint32, qint32, qint32, qint32, const quint8 *));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(fill, void (KisFixedPaintDevice::*)(const QRect &, const KoColor &));
    ASSERT_FIXED_PAINT_DEVICE_SIGNATURE(mirror, void (KisFixedPaintDevice::*)(bool, bool));
}

void KisImageTypesContractTest::groupLayerOwnershipLifetimeAndHierarchySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisGroupLayer>);
    static_assert(std::is_base_of_v<KisLayer, KisGroupLayer>);
    static_assert(std::is_constructible_v<KisGroupLayer, KisImageWSP, const QString &, quint8>);
    static_assert(std::is_constructible_v<KisGroupLayer, KisImageWSP, const QString &, quint8, const KoColorSpace *>);
    static_assert(std::is_copy_constructible_v<KisGroupLayer>);
    static_assert(std::has_virtual_destructor_v<KisGroupLayer>);
    ASSERT_GROUP_LAYER_SIGNATURE(allowAsChild, bool (KisGroupLayer::*)(KisNodeSP) const);
    using CloneResult = decltype(std::declval<const KisGroupLayer &>().clone());
    static_assert(std::is_same_v<CloneResult, KisNodeSP>);
}

void KisImageTypesContractTest::groupLayerImageCoordinateAndDeviceSignaturesRemainStable()
{
    ASSERT_GROUP_LAYER_SIGNATURE(setImage, void (KisGroupLayer::*)(KisImageWSP));
    ASSERT_GROUP_LAYER_SIGNATURE(colorSpace, const KoColorSpace *(KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(original, KisPaintDeviceSP (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(lazyDestinationForSubtreeComposition, KisPaintDeviceSP (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(paintDevice, KisPaintDeviceSP (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(x, qint32 (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(y, qint32 (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(setX, void (KisGroupLayer::*)(qint32));
    ASSERT_GROUP_LAYER_SIGNATURE(setY, void (KisGroupLayer::*)(qint32));
}

void KisImageTypesContractTest::groupLayerCompositionAndVisitorSignaturesRemainStable()
{
    ASSERT_GROUP_LAYER_SIGNATURE(accept, bool (KisGroupLayer::*)(KisNodeVisitor &));
    ASSERT_GROUP_LAYER_SIGNATURE(accept, void (KisGroupLayer::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_GROUP_LAYER_SIGNATURE(createMergedLayerTemplate, KisLayerSP (KisGroupLayer::*)(KisLayerSP));
    ASSERT_GROUP_LAYER_SIGNATURE(fillMergedLayerTemplate, void (KisGroupLayer::*)(KisLayerSP, KisLayerSP, bool));
    ASSERT_GROUP_LAYER_SIGNATURE(resetCache, void (KisGroupLayer::*)(const KoColorSpace *));
    ASSERT_GROUP_LAYER_SIGNATURE(projectionIsValid, bool (KisGroupLayer::*)() const);
}

void KisImageTypesContractTest::groupLayerPresentationAndPolicySignaturesRemainStable()
{
    ASSERT_GROUP_LAYER_SIGNATURE(icon, QIcon (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(setSectionModelProperties, void (KisGroupLayer::*)(const KisBaseNode::PropertyList &));
    ASSERT_GROUP_LAYER_SIGNATURE(defaultProjectionColor, KoColor (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(setDefaultProjectionColor, void (KisGroupLayer::*)(KoColor));
    ASSERT_GROUP_LAYER_SIGNATURE(passThroughMode, bool (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(setPassThroughMode, void (KisGroupLayer::*)(bool));
}

void KisImageTypesContractTest::groupLayerChildrenBoundsSignaturesRemainStable()
{
    ASSERT_GROUP_LAYER_SIGNATURE(extent, QRect (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(exactBounds, QRect (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(calculateChildrenTightUserVisibleBounds, QRect (KisGroupLayer::*)() const);
    ASSERT_GROUP_LAYER_SIGNATURE(calculateChildrenLooseUserVisibleBounds, QRect (KisGroupLayer::*)() const);
}

void KisImageTypesContractTest::dataManagerOwnershipAndDefaultPixelSchemaRemainsStable()
{
    static_assert(std::is_base_of_v<KisTiledDataManager, KisDataManager>);
    static_assert(std::is_constructible_v<KisDataManager, const KisDataManager &>);
    static_assert(std::is_constructible_v<KisDataManager, quint32, const quint8 *>);
    static_assert(std::has_virtual_destructor_v<KisDataManager>);
    ASSERT_DATA_MANAGER_SIGNATURE(defaultPixel, const quint8 *(KisDataManager::*)() const);
    ASSERT_DATA_MANAGER_SIGNATURE(setDefaultPixel, void (KisDataManager::*)(const quint8 *));
    ASSERT_DATA_MANAGER_SIGNATURE(pixelSize, quint32 (KisDataManager::*)() const);
}

void KisImageTypesContractTest::dataManagerExtentRegionAndContiguitySignaturesRemainStable()
{
    ASSERT_DATA_MANAGER_SIGNATURE(extent, QRect (KisDataManager::*)() const);
    ASSERT_DATA_MANAGER_SIGNATURE(extent, void (KisDataManager::*)(qint32 &, qint32 &, qint32 &, qint32 &) const);
    ASSERT_DATA_MANAGER_SIGNATURE(region, KisRegion (KisDataManager::*)() const);
    ASSERT_DATA_MANAGER_SIGNATURE(setExtent, void (KisDataManager::*)(const QRect &));
    ASSERT_DATA_MANAGER_SIGNATURE(setExtent, void (KisDataManager::*)(qint32, qint32, qint32, qint32));
    ASSERT_DATA_MANAGER_SIGNATURE(rowStride, qint32 (KisDataManager::*)(qint32, qint32) const);
    ASSERT_DATA_MANAGER_SIGNATURE(numContiguousColumns, qint32 (KisDataManager::*)(qint32, qint32, qint32) const);
    ASSERT_DATA_MANAGER_SIGNATURE(numContiguousRows, qint32 (KisDataManager::*)(qint32, qint32, qint32) const);
}

void KisImageTypesContractTest::dataManagerPixelTransferAndMutationSignaturesRemainStable()
{
    ASSERT_DATA_MANAGER_SIGNATURE(clear, void (KisDataManager::*)());
    ASSERT_DATA_MANAGER_SIGNATURE(clear, void (KisDataManager::*)(qint32, qint32, qint32, qint32, const quint8 *));
    ASSERT_DATA_MANAGER_SIGNATURE(clear, void (KisDataManager::*)(qint32, qint32, qint32, qint32, quint8));
    ASSERT_DATA_MANAGER_SIGNATURE(setPixel, void (KisDataManager::*)(qint32, qint32, const quint8 *));
    ASSERT_DATA_MANAGER_SIGNATURE(readBytes,
                                  void (KisDataManager::*)(quint8 *, qint32, qint32, qint32, qint32, qint32) const);
    ASSERT_DATA_MANAGER_SIGNATURE(writeBytes,
                                  void (KisDataManager::*)(const quint8 *, qint32, qint32, qint32, qint32, qint32));
    ASSERT_DATA_MANAGER_SIGNATURE(readPlanarBytes,
                                  QVector<quint8 *> (KisDataManager::*)(QVector<qint32>, qint32, qint32, qint32, qint32)
                                      const);
    ASSERT_DATA_MANAGER_SIGNATURE(
        writePlanarBytes,
        void (KisDataManager::*)(QVector<quint8 *>, QVector<qint32>, qint32, qint32, qint32, qint32));
    static_assert(
        std::is_same_v<decltype(std::declval<const KisDataManager &>()
                                    .readBytes(std::declval<quint8 *>(), qint32{}, qint32{}, qint32{}, qint32{})),
                       void>);
    static_assert(std::is_same_v<
                  decltype(std::declval<KisDataManager &>()
                               .writeBytes(std::declval<const quint8 *>(), qint32{}, qint32{}, qint32{}, qint32{})),
                  void>);
}

void KisImageTypesContractTest::dataManagerCopyAndHistorySignaturesRemainStable()
{
    ASSERT_DATA_MANAGER_SIGNATURE(bitBlt, void (KisDataManager::*)(KisTiledDataManagerSP, const QRect &));
    ASSERT_DATA_MANAGER_SIGNATURE(bitBltOldData, void (KisDataManager::*)(KisTiledDataManagerSP, const QRect &));
    ASSERT_DATA_MANAGER_SIGNATURE(bitBltRough, void (KisDataManager::*)(KisTiledDataManagerSP, const QRect &));
    ASSERT_DATA_MANAGER_SIGNATURE(bitBltRoughOldData, void (KisDataManager::*)(KisTiledDataManagerSP, const QRect &));
    ASSERT_DATA_MANAGER_SIGNATURE(getMemento, KisMementoSP (KisDataManager::*)());
    ASSERT_DATA_MANAGER_SIGNATURE(hasCurrentMemento, bool (KisDataManager::*)() const);
    ASSERT_DATA_MANAGER_SIGNATURE(rollback, void (KisDataManager::*)(KisMementoSP));
    ASSERT_DATA_MANAGER_SIGNATURE(rollforward, void (KisDataManager::*)(KisMementoSP));
}

void KisImageTypesContractTest::dataManagerPersistencePurgeAndPoolSignaturesRemainStable()
{
    ASSERT_DATA_MANAGER_SIGNATURE(purge, void (KisDataManager::*)(const QRect &));
    ASSERT_DATA_MANAGER_SIGNATURE(read, bool (KisDataManager::*)(QIODevice *));
    ASSERT_DATA_MANAGER_SIGNATURE(write, bool (KisDataManager::*)(KisPaintDeviceWriter &));
    ASSERT_DATA_MANAGER_SIGNATURE(releaseInternalPools, void (*)());
}

void KisImageTypesContractTest::savedCommandHierarchyAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_base_of_v<KUndo2Command, KisSavedCommandBase>);
    static_assert(std::is_base_of_v<KisSavedCommandBase, KisSavedCommand>);
    static_assert(std::is_base_of_v<KisSavedCommandBase, KisSavedMacroCommand>);
    static_assert(
        std::is_constructible_v<SavedCommandBaseConstructorProbe, const KUndo2MagicString &, KisStrokesFacade *>);
    static_assert(std::has_virtual_destructor_v<KisSavedCommandBase>);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommandBase, redo, void (KisSavedCommandBase::*)());
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommandBase, undo, void (KisSavedCommandBase::*)());
    static_assert(std::is_constructible_v<KisSavedCommand, KUndo2CommandSP, KisStrokesFacade *>);
    static_assert(std::is_constructible_v<KisSavedMacroCommand, const KUndo2MagicString &, KisStrokesFacade *>);
    static_assert(std::has_virtual_destructor_v<KisSavedMacroCommand>);
}

void KisImageTypesContractTest::savedCommandTimingAndIdentitySignaturesRemainStable()
{
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, endTime, QTime (KisSavedCommand::*)() const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, id, int (KisSavedCommand::*)() const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, isMerged, bool (KisSavedCommand::*)() const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, setEndTime, void (KisSavedCommand::*)(const QTime &));
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, setTime, void (KisSavedCommand::*)(const QTime &));
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, setTimedID, void (KisSavedCommand::*)(int));
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, time, QTime (KisSavedCommand::*)() const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, timedId, int (KisSavedCommand::*)() const);
}

void KisImageTypesContractTest::savedCommandMergeAndUnwrapSignaturesRemainStable()
{
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand,
                                   canAnnihilateWith,
                                   bool (KisSavedCommand::*)(const KUndo2Command *) const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand,
                                   mergeCommandsVector,
                                   QVector<KUndo2Command *> (KisSavedCommand::*)() const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, mergeWith, bool (KisSavedCommand::*)(const KUndo2Command *));
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedCommand, timedMergeWith, bool (KisSavedCommand::*)(KUndo2Command *));
    static_assert(std::is_same_v<decltype(KisSavedCommand::unwrap(std::declval<KUndo2Command *>(),
                                                                  std::declval<SavedCommandUnwrapCallable>())),
                                 KUndo2Command *>);
}

void KisImageTypesContractTest::savedMacroCommandCompositionAndIdentitySignaturesRemainStable()
{
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedMacroCommand,
                                   addCommand,
                                   void (KisSavedMacroCommand::*)(KUndo2CommandSP,
                                                                  KisStrokeJobData::Sequentiality,
                                                                  KisStrokeJobData::Exclusivity));
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedMacroCommand,
                                   canAnnihilateWith,
                                   bool (KisSavedMacroCommand::*)(const KUndo2Command *) const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedMacroCommand, id, int (KisSavedMacroCommand::*)() const);
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedMacroCommand,
                                   mergeWith,
                                   bool (KisSavedMacroCommand::*)(const KUndo2Command *));
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedMacroCommand, setMacroId, void (KisSavedMacroCommand::*)(int));
    static_assert(
        std::is_same_v<decltype(std::declval<KisSavedMacroCommand &>().addCommand(std::declval<KUndo2CommandSP>())),
                       void>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisSavedMacroCommand &>().addCommand(std::declval<KUndo2CommandSP>(),
                                                                                  KisStrokeJobData::SEQUENTIAL)),
                       void>);
}

void KisImageTypesContractTest::savedMacroCommandExecutionAndOverrideSignaturesRemainStable()
{
    ASSERT_SAVED_COMMAND_SIGNATURE(KisSavedMacroCommand,
                                   getCommandExecutionJobs,
                                   void (KisSavedMacroCommand::*)(QVector<KisStrokeJobData *> *, bool, bool) const);
    ASSERT_SAVED_COMMAND_SIGNATURE(
        KisSavedMacroCommand,
        setOverrideInfo,
        void (KisSavedMacroCommand::*)(const KisSavedMacroCommand *, const QVector<const KUndo2Command *> &));
    static_assert(
        std::is_same_v<decltype(std::declval<const KisSavedMacroCommand &>()
                                    .getCommandExecutionJobs(std::declval<QVector<KisStrokeJobData *> *>(), true)),
                       void>);
}

void KisImageTypesContractTest::pixelSelectionOwnershipCopyAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisPixelSelection>);
    static_assert(std::is_base_of_v<KisPaintDevice, KisPixelSelection>);
    static_assert(std::is_base_of_v<KisSelectionComponent, KisPixelSelection>);
    static_assert(std::has_virtual_destructor_v<KisPixelSelection>);
    static_assert(std::is_constructible_v<KisPixelSelection, KisDefaultBoundsBaseSP, KisSelectionWSP>);
    static_assert(
        std::
            is_constructible_v<KisPixelSelection, const KisPaintDeviceSP, KritaUtils::DeviceCopyMode, KisSelectionWSP>);
    static_assert(std::is_constructible_v<KisPixelSelection, const KisPixelSelection &, KritaUtils::DeviceCopyMode>);
    static_assert(std::is_same_v<decltype(KisPixelSelection()), KisPixelSelection>);
    static_assert(
        std::is_same_v<decltype(KisPixelSelection(std::declval<KisDefaultBoundsBaseSP>())), KisPixelSelection>);
    static_assert(std::is_same_v<decltype(KisPixelSelection(std::declval<KisPaintDeviceSP>())), KisPixelSelection>);
    static_assert(
        std::is_same_v<decltype(KisPixelSelection(std::declval<const KisPixelSelection &>())), KisPixelSelection>);
}

void KisImageTypesContractTest::pixelSelectionMutationSignaturesRemainStable()
{
    ASSERT_PIXEL_SELECTION_SIGNATURE(applySelection, void (KisPixelSelection::*)(KisPixelSelectionSP, SelectionAction));
    ASSERT_PIXEL_SELECTION_SIGNATURE(clear, void (KisPixelSelection::*)());
    ASSERT_PIXEL_SELECTION_SIGNATURE(clear, void (KisPixelSelection::*)(const QRect &));
    ASSERT_PIXEL_SELECTION_SIGNATURE(copyAlphaFrom, void (KisPixelSelection::*)(KisPaintDeviceSP, const QRect &));
    ASSERT_PIXEL_SELECTION_SIGNATURE(invert, void (KisPixelSelection::*)());
    ASSERT_PIXEL_SELECTION_SIGNATURE(isEmpty, bool (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(moveTo, void (KisPixelSelection::*)(const QPoint &));
    ASSERT_PIXEL_SELECTION_SIGNATURE(select, void (KisPixelSelection::*)(const QRect &, quint8));
    static_assert(
        std::is_same_v<decltype(std::declval<KisPixelSelection &>().select(std::declval<const QRect &>())), void>);
}

void KisImageTypesContractTest::pixelSelectionGeometryAndOutlineCacheSignaturesRemainStable()
{
    ASSERT_PIXEL_SELECTION_SIGNATURE(isTotallyUnselected, bool (KisPixelSelection::*)(const QRect &) const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(outline, QVector<QPolygon> (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(outlineCache, QPainterPath (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(outlineCacheValid, bool (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(recalculateOutlineCache, void (KisPixelSelection::*)());
    ASSERT_PIXEL_SELECTION_SIGNATURE(setOutlineCache, void (KisPixelSelection::*)(const QPainterPath &));
    ASSERT_PIXEL_SELECTION_SIGNATURE(invalidateOutlineCache, void (KisPixelSelection::*)());
    ASSERT_PIXEL_SELECTION_SIGNATURE(selectedExactRect, QRect (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(selectedRect, QRect (KisPixelSelection::*)() const);
}

void KisImageTypesContractTest::pixelSelectionParentCloneAndProjectionSignaturesRemainStable()
{
    ASSERT_PIXEL_SELECTION_SIGNATURE(clone, KisSelectionComponent * (KisPixelSelection::*)(KisSelection *));
    ASSERT_PIXEL_SELECTION_SIGNATURE(compositionSourceColorSpace, const KoColorSpace *(KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(parentSelection, KisSelectionWSP (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(setParentSelection, void (KisPixelSelection::*)(KisSelectionWSP));
    ASSERT_PIXEL_SELECTION_SIGNATURE(renderToProjection, void (KisPixelSelection::*)(KisPaintDeviceSP));
    ASSERT_PIXEL_SELECTION_SIGNATURE(renderToProjection, void (KisPixelSelection::*)(KisPaintDeviceSP, const QRect &));
}

void KisImageTypesContractTest::pixelSelectionThumbnailAndPersistenceSignaturesRemainStable()
{
    ASSERT_PIXEL_SELECTION_SIGNATURE(read, bool (KisPixelSelection::*)(QIODevice *));
    ASSERT_PIXEL_SELECTION_SIGNATURE(thumbnailImage, QImage (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(thumbnailImageTransform, QTransform (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(thumbnailImageValid, bool (KisPixelSelection::*)() const);
    ASSERT_PIXEL_SELECTION_SIGNATURE(recalculateThumbnailImage, void (KisPixelSelection::*)(const QColor &));
}

void KisImageTypesContractTest::maskGeneratorIdentityLifetimeAndConstantsSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMaskGenerator>);
    static_assert(std::is_abstract_v<KisMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisMaskGenerator>);
    static_assert(std::is_enum_v<KisMaskGenerator::Type>);
    static_assert(KisMaskGenerator::CIRCLE == 0);
    static_assert(KisMaskGenerator::RECTANGLE == 1);
    static_assert(std::is_same_v<decltype(DefaultId), const KoID>);
    static_assert(std::is_same_v<decltype(SoftId), const KoID>);
    static_assert(std::is_same_v<decltype(GaussId), const KoID>);
    static_assert(std::is_same_v<decltype(OVERSAMPLING), const int>);
    static_assert(OVERSAMPLING == 4);
    static_assert(std::is_constructible_v<MaskGeneratorConstructorProbe,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          int,
                                          bool,
                                          KisMaskGenerator::Type,
                                          const KoID &>);
    static_assert(std::is_constructible_v<MaskGeneratorConstructorProbe,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          int,
                                          bool,
                                          KisMaskGenerator::Type>);
    static_assert(std::is_constructible_v<MaskGeneratorConstructorProbe, const KisMaskGenerator &>);
}

void KisImageTypesContractTest::maskGeneratorGeometrySignaturesRemainStable()
{
    ASSERT_MASK_GENERATOR_SIGNATURE(width, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(height, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(diameter, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(setDiameter, void (KisMaskGenerator::*)(qreal));
    ASSERT_MASK_GENERATOR_SIGNATURE(ratio, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(horizontalFade, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(verticalFade, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(spikes, int (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(type, KisMaskGenerator::Type (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(isEmpty, bool (KisMaskGenerator::*)() const);
}

void KisImageTypesContractTest::maskGeneratorAppearanceSignaturesRemainStable()
{
    ASSERT_MASK_GENERATOR_SIGNATURE(antialiasEdges, bool (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(softness, qreal (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(setSoftness, void (KisMaskGenerator::*)(qreal));
    ASSERT_MASK_GENERATOR_SIGNATURE(curveString, QString (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(setCurveString, void (KisMaskGenerator::*)(const QString &));
    ASSERT_MASK_GENERATOR_SIGNATURE(setScale, void (KisMaskGenerator::*)(qreal, qreal));
    ASSERT_MASK_GENERATOR_SIGNATURE(fixRotation, void (KisMaskGenerator::*)(qreal &, qreal &) const);
    ASSERT_MASK_GENERATOR_SIGNATURE(shouldSupersample, bool (KisMaskGenerator::*)() const);
}

void KisImageTypesContractTest::maskGeneratorRenderingPolicySignaturesRemainStable()
{
    ASSERT_MASK_GENERATOR_SIGNATURE(shouldSupersample6x6, bool (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(shouldVectorize, bool (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(valueAt, quint8 (KisMaskGenerator::*)(qreal, qreal) const);
    ASSERT_MASK_GENERATOR_SIGNATURE(applicator, KisBrushMaskApplicatorBase * (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(clone, KisMaskGenerator * (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(id, QString (KisMaskGenerator::*)() const);
    ASSERT_MASK_GENERATOR_SIGNATURE(name, QString (KisMaskGenerator::*)() const);
}

void KisImageTypesContractTest::maskGeneratorSerializationAndRegistrySignaturesRemainStable()
{
    ASSERT_MASK_GENERATOR_SIGNATURE(toXML, void (KisMaskGenerator::*)(QDomDocument &, QDomElement &) const);
    static_assert(std::is_same_v<decltype(&KisMaskGenerator::fromXML), KisMaskGenerator *(*)(const QDomElement &)>);
    static_assert(std::is_same_v<decltype(&KisMaskGenerator::maskGeneratorIds), QList<KoID> (*)()>);
}

void KisImageTypesContractTest::gaussianMaskTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisGaussCircleMaskGenerator>);
    static_assert(std::is_base_of_v<KisMaskGenerator, KisGaussCircleMaskGenerator>);
    static_assert(std::is_constructible_v<KisGaussCircleMaskGenerator, qreal, qreal, qreal, qreal, int, bool>);
    static_assert(std::is_copy_constructible_v<KisGaussCircleMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisGaussCircleMaskGenerator>);

    static_assert(std::is_class_v<KisGaussRectangleMaskGenerator>);
    static_assert(std::is_base_of_v<KisMaskGenerator, KisGaussRectangleMaskGenerator>);
    static_assert(std::is_constructible_v<KisGaussRectangleMaskGenerator, qreal, qreal, qreal, qreal, int, bool>);
    static_assert(std::is_copy_constructible_v<KisGaussRectangleMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisGaussRectangleMaskGenerator>);
}

void KisImageTypesContractTest::gaussianMaskCloneSignaturesRemainStable()
{
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussCircleMaskGenerator,
                                   clone,
                                   KisMaskGenerator * (KisGaussCircleMaskGenerator::*)() const);
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussRectangleMaskGenerator,
                                   clone,
                                   KisMaskGenerator * (KisGaussRectangleMaskGenerator::*)() const);
}

void KisImageTypesContractTest::gaussianMaskSamplingSignaturesRemainStable()
{
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussCircleMaskGenerator,
                                   valueAt,
                                   quint8 (KisGaussCircleMaskGenerator::*)(qreal, qreal) const);
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussRectangleMaskGenerator,
                                   valueAt,
                                   quint8 (KisGaussRectangleMaskGenerator::*)(qreal, qreal) const);
}

void KisImageTypesContractTest::gaussianMaskScaleAndVectorizationSignaturesRemainStable()
{
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussCircleMaskGenerator,
                                   setScale,
                                   void (KisGaussCircleMaskGenerator::*)(qreal, qreal));
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussCircleMaskGenerator,
                                   shouldVectorize,
                                   bool (KisGaussCircleMaskGenerator::*)() const);
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussRectangleMaskGenerator,
                                   setScale,
                                   void (KisGaussRectangleMaskGenerator::*)(qreal, qreal));
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussRectangleMaskGenerator,
                                   shouldVectorize,
                                   bool (KisGaussRectangleMaskGenerator::*)() const);
}

void KisImageTypesContractTest::gaussianMaskApplicatorSignaturesRemainStable()
{
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussCircleMaskGenerator,
                                   applicator,
                                   KisBrushMaskApplicatorBase * (KisGaussCircleMaskGenerator::*)() const);
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussCircleMaskGenerator,
                                   setMaskScalarApplicator,
                                   void (KisGaussCircleMaskGenerator::*)());
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussRectangleMaskGenerator,
                                   applicator,
                                   KisBrushMaskApplicatorBase * (KisGaussRectangleMaskGenerator::*)() const);
    ASSERT_GAUSSIAN_MASK_SIGNATURE(KisGaussRectangleMaskGenerator,
                                   setMaskScalarApplicator,
                                   void (KisGaussRectangleMaskGenerator::*)());
}

void KisImageTypesContractTest::defaultMaskTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisCircleMaskGenerator>);
    static_assert(std::is_base_of_v<KisMaskGenerator, KisCircleMaskGenerator>);
    static_assert(std::is_constructible_v<KisCircleMaskGenerator, qreal, qreal, qreal, qreal, int, bool>);
    static_assert(std::is_copy_constructible_v<KisCircleMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisCircleMaskGenerator>);

    static_assert(std::is_class_v<KisRectangleMaskGenerator>);
    static_assert(std::is_base_of_v<KisMaskGenerator, KisRectangleMaskGenerator>);
    static_assert(std::is_constructible_v<KisRectangleMaskGenerator, qreal, qreal, qreal, qreal, int, bool>);
    static_assert(std::is_copy_constructible_v<KisRectangleMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisRectangleMaskGenerator>);
}

void KisImageTypesContractTest::defaultMaskCloneAndSamplingSignaturesRemainStable()
{
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator,
                                  clone,
                                  KisMaskGenerator * (KisCircleMaskGenerator::*)() const);
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator,
                                  valueAt,
                                  quint8 (KisCircleMaskGenerator::*)(qreal, qreal) const);
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator,
                                  clone,
                                  KisMaskGenerator * (KisRectangleMaskGenerator::*)() const);
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator,
                                  valueAt,
                                  quint8 (KisRectangleMaskGenerator::*)(qreal, qreal) const);
}

void KisImageTypesContractTest::defaultMaskScaleAndSoftnessSignaturesRemainStable()
{
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator, setScale, void (KisCircleMaskGenerator::*)(qreal, qreal));
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator, setSoftness, void (KisCircleMaskGenerator::*)(qreal));
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator,
                                  setScale,
                                  void (KisRectangleMaskGenerator::*)(qreal, qreal));
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator, setSoftness, void (KisRectangleMaskGenerator::*)(qreal));
}

void KisImageTypesContractTest::defaultMaskVectorizationSignaturesRemainStable()
{
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator, shouldVectorize, bool (KisCircleMaskGenerator::*)() const);
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator,
                                  shouldVectorize,
                                  bool (KisRectangleMaskGenerator::*)() const);
}

void KisImageTypesContractTest::defaultMaskApplicatorSignaturesRemainStable()
{
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator,
                                  applicator,
                                  KisBrushMaskApplicatorBase * (KisCircleMaskGenerator::*)() const);
    ASSERT_DEFAULT_MASK_SIGNATURE(KisCircleMaskGenerator, setMaskScalarApplicator, void (KisCircleMaskGenerator::*)());
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator,
                                  applicator,
                                  KisBrushMaskApplicatorBase * (KisRectangleMaskGenerator::*)() const);
    ASSERT_DEFAULT_MASK_SIGNATURE(KisRectangleMaskGenerator,
                                  setMaskScalarApplicator,
                                  void (KisRectangleMaskGenerator::*)());
}

void KisImageTypesContractTest::curveMaskTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisCurveCircleMaskGenerator>);
    static_assert(std::is_base_of_v<KisMaskGenerator, KisCurveCircleMaskGenerator>);
    static_assert(std::is_constructible_v<KisCurveCircleMaskGenerator,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          int,
                                          const KisCubicCurve &,
                                          bool>);
    static_assert(std::is_copy_constructible_v<KisCurveCircleMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisCurveCircleMaskGenerator>);

    static_assert(std::is_class_v<KisCurveRectangleMaskGenerator>);
    static_assert(std::is_base_of_v<KisMaskGenerator, KisCurveRectangleMaskGenerator>);
    static_assert(std::is_constructible_v<KisCurveRectangleMaskGenerator,
                                          qreal,
                                          qreal,
                                          qreal,
                                          qreal,
                                          int,
                                          const KisCubicCurve &,
                                          bool>);
    static_assert(std::is_copy_constructible_v<KisCurveRectangleMaskGenerator>);
    static_assert(std::has_virtual_destructor_v<KisCurveRectangleMaskGenerator>);
}

void KisImageTypesContractTest::curveMaskCloneAndSamplingSignaturesRemainStable()
{
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                clone,
                                KisMaskGenerator * (KisCurveCircleMaskGenerator::*)() const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                valueAt,
                                quint8 (KisCurveCircleMaskGenerator::*)(qreal, qreal) const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                clone,
                                KisMaskGenerator * (KisCurveRectangleMaskGenerator::*)() const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                valueAt,
                                quint8 (KisCurveRectangleMaskGenerator::*)(qreal, qreal) const);
}

void KisImageTypesContractTest::curveMaskScaleAndSoftnessSignaturesRemainStable()
{
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                setScale,
                                void (KisCurveCircleMaskGenerator::*)(qreal, qreal));
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator, setSoftness, void (KisCurveCircleMaskGenerator::*)(qreal));
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                setScale,
                                void (KisCurveRectangleMaskGenerator::*)(qreal, qreal));
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                setSoftness,
                                void (KisCurveRectangleMaskGenerator::*)(qreal));
}

void KisImageTypesContractTest::curveMaskVectorizationAndApplicatorSignaturesRemainStable()
{
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                shouldVectorize,
                                bool (KisCurveCircleMaskGenerator::*)() const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                applicator,
                                KisBrushMaskApplicatorBase * (KisCurveCircleMaskGenerator::*)() const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                setMaskScalarApplicator,
                                void (KisCurveCircleMaskGenerator::*)());
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                shouldVectorize,
                                bool (KisCurveRectangleMaskGenerator::*)() const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                applicator,
                                KisBrushMaskApplicatorBase * (KisCurveRectangleMaskGenerator::*)() const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                setMaskScalarApplicator,
                                void (KisCurveRectangleMaskGenerator::*)());
}

void KisImageTypesContractTest::curveMaskSerializationAndTransformationSignaturesRemainStable()
{
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                toXML,
                                void (KisCurveCircleMaskGenerator::*)(QDomDocument &, QDomElement &) const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveRectangleMaskGenerator,
                                toXML,
                                void (KisCurveRectangleMaskGenerator::*)(QDomDocument &, QDomElement &) const);
    ASSERT_CURVE_MASK_SIGNATURE(KisCurveCircleMaskGenerator,
                                transformCurveForSoftness,
                                void (*)(qreal, const QList<KisCubicCurvePoint> &, int, QVector<qreal> &));
}

void KisImageTypesContractTest::mathToolboxTypeAndAliasSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMathToolbox>);
    static_assert(std::is_class_v<KisMathToolbox::KisFloatRepresentation>);
    static_assert(std::is_same_v<KisMathToolbox::KisWavelet, KisMathToolbox::KisFloatRepresentation>);
    static_assert(std::is_same_v<PtrToDouble, double (*)(const quint8 *, int)>);
    static_assert(std::is_same_v<PtrFromDouble, void (*)(quint8 *, int, double)>);
    static_assert(std::is_same_v<PtrFromDoubleCheckNull, void (*)(quint8 *, int, double, bool *)>);
}

void KisImageTypesContractTest::mathToolboxFloatRepresentationSchemaRemainStable()
{
    using Representation = KisMathToolbox::KisFloatRepresentation;

    static_assert(std::is_constructible_v<Representation, uint, uint>);
    static_assert(std::is_destructible_v<Representation>);
    static_assert(std::is_same_v<decltype(Representation::coeffs), float *>);
    static_assert(std::is_same_v<decltype(Representation::size), uint>);
    static_assert(std::is_same_v<decltype(Representation::depth), uint>);
}

void KisImageTypesContractTest::mathToolboxWaveletPlanningSignaturesRemainStable()
{
    ASSERT_MATH_TOOLBOX_SIGNATURE(initWavelet,
                                  KisMathToolbox::KisWavelet * (KisMathToolbox::*)(KisPaintDeviceSP, const QRect &));
    ASSERT_MATH_TOOLBOX_SIGNATURE(fastWaveletTotalSteps, uint (KisMathToolbox::*)(const QRect &));
}

void KisImageTypesContractTest::mathToolboxWaveletTransformSignaturesRemainStable()
{
    ASSERT_MATH_TOOLBOX_SIGNATURE(
        fastWaveletTransformation,
        KisMathToolbox::KisWavelet
            * (KisMathToolbox::*)(KisPaintDeviceSP, const QRect &, KisMathToolbox::KisWavelet *));
    ASSERT_MATH_TOOLBOX_SIGNATURE(fastWaveletUntransformation,
                                  void (KisMathToolbox::*)(KisPaintDeviceSP,
                                                           const QRect &,
                                                           KisMathToolbox::KisWavelet *,
                                                           KisMathToolbox::KisWavelet *));
}

void KisImageTypesContractTest::mathToolboxChannelConversionSignaturesRemainStable()
{
    ASSERT_MATH_TOOLBOX_SIGNATURE(getToDoubleChannelPtr,
                                  bool (KisMathToolbox::*)(QList<KoChannelInfo *>, QVector<PtrToDouble> &));
    ASSERT_MATH_TOOLBOX_SIGNATURE(getFromDoubleChannelPtr,
                                  bool (KisMathToolbox::*)(QList<KoChannelInfo *>, QVector<PtrFromDouble> &));
    ASSERT_MATH_TOOLBOX_SIGNATURE(getFromDoubleCheckNullChannelPtr,
                                  bool (KisMathToolbox::*)(QList<KoChannelInfo *>, QVector<PtrFromDoubleCheckNull> &));
    ASSERT_MATH_TOOLBOX_SIGNATURE(minChannelValue, double (KisMathToolbox::*)(KoChannelInfo *));
    ASSERT_MATH_TOOLBOX_SIGNATURE(maxChannelValue, double (KisMathToolbox::*)(KoChannelInfo *));
}

void KisImageTypesContractTest::maskDataConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<MaskProcessingData>);
    static_assert(std::is_constructible_v<MaskProcessingData,
                                          KisFixedPaintDeviceSP,
                                          const KoColorSpace *,
                                          const quint8 *,
                                          qreal,
                                          qreal,
                                          double,
                                          double,
                                          double>);
}

void KisImageTypesContractTest::maskDataDeviceAndColorSchemaRemainStable()
{
    static_assert(std::is_same_v<decltype(MaskProcessingData::device), KisFixedPaintDeviceSP>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::colorSpace), const KoColorSpace *>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::color), const quint8 *>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::pixelSize), quint32>);
}

void KisImageTypesContractTest::maskDataGeometrySchemaRemainStable()
{
    static_assert(std::is_same_v<decltype(MaskProcessingData::randomness), qreal>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::density), qreal>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::centerX), double>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::centerY), double>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::cosa), double>);
    static_assert(std::is_same_v<decltype(MaskProcessingData::sina), double>);
}

void KisImageTypesContractTest::brushMaskApplicatorSchemaRemainStable()
{
    using ProcessSignature = void (KisBrushMaskApplicatorBase::*)(const QRect &);
    using InitializeDataSignature = void (KisBrushMaskApplicatorBase::*)(const MaskProcessingData *);

    static_assert(std::is_class_v<KisBrushMaskApplicatorBase>);
    static_assert(std::is_abstract_v<KisBrushMaskApplicatorBase>);
    static_assert(std::has_virtual_destructor_v<KisBrushMaskApplicatorBase>);
    static_assert(std::is_same_v<decltype(static_cast<ProcessSignature>(&KisBrushMaskApplicatorBase::process)),
                                 ProcessSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<InitializeDataSignature>(&KisBrushMaskApplicatorBase::initializeData)),
                       InitializeDataSignature>);
}

void KisImageTypesContractTest::brushMaskOperatorSchemaRemainStable()
{
    using CallSignature = void (OperatorWrapper::*)(const QRect &) const;

    static_assert(std::is_class_v<OperatorWrapper>);
    static_assert(std::is_constructible_v<OperatorWrapper, KisBrushMaskApplicatorBase *>);
    static_assert(std::is_same_v<decltype(static_cast<CallSignature>(&OperatorWrapper::operator())), CallSignature>);
    static_assert(std::is_same_v<decltype(OperatorWrapper::m_applicator), KisBrushMaskApplicatorBase *>);
}

void KisImageTypesContractTest::updaterContextOwnershipLifetimeAndConstructionSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisUpdaterContext>);
    static_assert(std::is_class_v<KisTestableUpdaterContext>);
    static_assert(std::is_base_of_v<KisUpdaterContext, KisTestableUpdaterContext>);
    static_assert(std::is_same_v<decltype(KisUpdaterContext::useIdealThreadCountTag), const int>);
    static_assert(std::is_constructible_v<KisUpdaterContext>);
    static_assert(std::is_constructible_v<KisUpdaterContext, qint32>);
    static_assert(std::is_constructible_v<KisUpdaterContext, qint32, KisUpdateScheduler *>);
    static_assert(std::is_destructible_v<KisUpdaterContext>);
    static_assert(std::is_constructible_v<KisTestableUpdaterContext, qint32>);
}

void KisImageTypesContractTest::updaterContextSnapshotLodAndCapacitySignaturesRemainStable()
{
    ASSERT_UPDATER_CONTEXT_SIGNATURE(getJobsSnapshot, void (KisUpdaterContext::*)(qint32 &, qint32 &));
    ASSERT_UPDATER_CONTEXT_SIGNATURE(getContextSnapshotEx, KisUpdaterContextSnapshotEx (KisUpdaterContext::*)() const);
    ASSERT_UPDATER_CONTEXT_SIGNATURE(currentLevelOfDetail, int (KisUpdaterContext::*)() const);
    ASSERT_UPDATER_CONTEXT_SIGNATURE(hasSpareThread, bool (KisUpdaterContext::*)());
    ASSERT_UPDATER_CONTEXT_SIGNATURE(isIdle, bool (KisUpdaterContext::*)() const);
    ASSERT_UPDATER_CONTEXT_SIGNATURE(threadsLimit, int (KisUpdaterContext::*)() const);
}

void KisImageTypesContractTest::updaterContextJobAdmissionAndSubmissionSignaturesRemainStable()
{
    ASSERT_UPDATER_CONTEXT_SIGNATURE(isJobAllowed, bool (KisUpdaterContext::*)(KisBaseRectsWalkerSP));
    ASSERT_UPDATER_CONTEXT_SIGNATURE(addMergeJob, void (KisUpdaterContext::*)(KisBaseRectsWalkerSP));
    ASSERT_UPDATER_CONTEXT_SIGNATURE(addStrokeJob, void (KisUpdaterContext::*)(KisStrokeJob *));
    ASSERT_UPDATER_CONTEXT_SIGNATURE(addSpontaneousJob, void (KisUpdaterContext::*)(KisSpontaneousJob *));
}

void KisImageTypesContractTest::updaterContextSynchronizationAndLimitSignaturesRemainStable()
{
    ASSERT_UPDATER_CONTEXT_SIGNATURE(waitForDone, void (KisUpdaterContext::*)());
    ASSERT_UPDATER_CONTEXT_SIGNATURE(lock, void (KisUpdaterContext::*)());
    ASSERT_UPDATER_CONTEXT_SIGNATURE(unlock, void (KisUpdaterContext::*)());
    ASSERT_UPDATER_CONTEXT_SIGNATURE(setThreadsLimit, void (KisUpdaterContext::*)(int));
    ASSERT_UPDATER_CONTEXT_SIGNATURE(setTestingMode, void (KisUpdaterContext::*)(bool));
}

void KisImageTypesContractTest::updaterContextContinuationAndCompletionSignaturesRemainStable()
{
    ASSERT_UPDATER_CONTEXT_SIGNATURE(continueUpdate, void (KisUpdaterContext::*)(const QRect &));
    ASSERT_UPDATER_CONTEXT_SIGNATURE(doSomeUsefulWork, void (KisUpdaterContext::*)());
    ASSERT_UPDATER_CONTEXT_SIGNATURE(jobFinished, void (KisUpdaterContext::*)());
    ASSERT_UPDATER_CONTEXT_SIGNATURE(jobThreadExited, void (KisUpdaterContext::*)());
}

void KisImageTypesContractTest::simpleUpdateQueueTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisSimpleUpdateQueue>);
    static_assert(std::is_class_v<KisTestableSimpleUpdateQueue>);
    static_assert(std::is_base_of_v<KisSimpleUpdateQueue, KisTestableSimpleUpdateQueue>);
    static_assert(std::is_default_constructible_v<KisSimpleUpdateQueue>);
    static_assert(std::has_virtual_destructor_v<KisSimpleUpdateQueue>);
}

void KisImageTypesContractTest::simpleUpdateQueueAliasSchemaRemainStable()
{
    static_assert(std::is_same_v<KisMutableSpontaneousJobsListIterator, QMutableListIterator<KisSpontaneousJob *>>);
    static_assert(std::is_same_v<KisMutableWalkersListIterator, QMutableListIterator<KisBaseRectsWalkerSP>>);
    static_assert(std::is_same_v<KisSpontaneousJobsList, QList<KisSpontaneousJob *>>);
    static_assert(std::is_same_v<KisSpontaneousJobsListIterator, QListIterator<KisSpontaneousJob *>>);
    static_assert(std::is_same_v<KisWalkersList, QList<KisBaseRectsWalkerSP>>);
    static_assert(std::is_same_v<KisWalkersListIterator, QListIterator<KisBaseRectsWalkerSP>>);
}

void KisImageTypesContractTest::simpleUpdateQueueAdmissionSignaturesRemainStable()
{
    using RectJobSignature = void (KisSimpleUpdateQueue::*)(KisNodeSP, const QRect &, const QRect &, int);
    using RectsJobSignature =
        void (KisSimpleUpdateQueue::*)(KisNodeSP, const QVector<QRect> &, const QRect &, int, KisProjectionUpdateFlags);

    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(addFullRefreshJob, RectJobSignature);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(addFullRefreshJob, RectsJobSignature);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(addSpontaneousJob, void (KisSimpleUpdateQueue::*)(KisSpontaneousJob *));
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(addUpdateJob, RectJobSignature);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(addUpdateJob, RectsJobSignature);
}

void KisImageTypesContractTest::simpleUpdateQueueStateAndProcessingSignaturesRemainStable()
{
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(isEmpty, bool (KisSimpleUpdateQueue::*)() const);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(isIdle, bool (KisSimpleUpdateQueue::*)() const);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(optimize, void (KisSimpleUpdateQueue::*)());
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(overrideLevelOfDetail, int (KisSimpleUpdateQueue::*)() const);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(processQueue, void (KisSimpleUpdateQueue::*)(KisUpdaterContext &));
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(sizeMetric, qint32 (KisSimpleUpdateQueue::*)() const);
    ASSERT_SIMPLE_UPDATE_QUEUE_SIGNATURE(updateSettings, void (KisSimpleUpdateQueue::*)());
}

void KisImageTypesContractTest::simpleUpdateQueueTestingAccessSignaturesRemainStable()
{
    using WalkersSignature = KisWalkersList &(KisTestableSimpleUpdateQueue::*)();
    using SpontaneousJobsSignature = KisSpontaneousJobsList &(KisTestableSimpleUpdateQueue::*)();

    static_assert(
        std::is_same_v<decltype(&KisTestableSimpleUpdateQueue::getSpontaneousJobsList), SpontaneousJobsSignature>);
    static_assert(std::is_same_v<decltype(&KisTestableSimpleUpdateQueue::getWalkersList), WalkersSignature>);
}

void KisImageTypesContractTest::updateJobItemTypeAndStateSchemaRemainStable()
{
    using Type = KisUpdateJobItem::Type;

    static_assert(std::is_class_v<KisUpdateJobItem>);
    static_assert(std::is_base_of_v<QObject, KisUpdateJobItem>);
    static_assert(std::is_base_of_v<QRunnable, KisUpdateJobItem>);
    static_assert(std::is_enum_v<Type>);
    static_assert(std::is_same_v<std::underlying_type_t<Type>, int>);
    static_assert(static_cast<int>(Type::EMPTY) == 0);
    static_assert(static_cast<int>(Type::WAITING) == 1);
    static_assert(static_cast<int>(Type::MERGE) == 2);
    static_assert(static_cast<int>(Type::STROKE) == 3);
    static_assert(static_cast<int>(Type::SPONTANEOUS) == 4);
}

void KisImageTypesContractTest::updateJobItemLifetimeAndExecutionSignaturesRemainStable()
{
    using VoidSignature = void (KisUpdateJobItem::*)();

    static_assert(std::is_constructible_v<KisUpdateJobItem, KisUpdaterContext *>);
    static_assert(std::has_virtual_destructor_v<KisUpdateJobItem>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::run), VoidSignature>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::runMergeJob), VoidSignature>);
}

void KisImageTypesContractTest::updateJobItemAssignmentSignaturesRemainStable()
{
    using WalkerSignature = bool (KisUpdateJobItem::*)(KisBaseRectsWalkerSP);
    using StrokeSignature = bool (KisUpdateJobItem::*)(KisStrokeJob *);
    using SpontaneousSignature = bool (KisUpdateJobItem::*)(KisSpontaneousJob *);

    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::setWalker), WalkerSignature>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::setStrokeJob), StrokeSignature>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::setSpontaneousJob), SpontaneousSignature>);
}

void KisImageTypesContractTest::updateJobItemStateTransitionSignaturesRemainStable()
{
    using Type = KisUpdateJobItem::Type;

    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::setDone), void (KisUpdateJobItem::*)()>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::isRunning), bool (KisUpdateJobItem::*)() const>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::type), Type (KisUpdateJobItem::*)() const>);
}

void KisImageTypesContractTest::updateJobItemGeometryAndStrokePolicySignaturesRemainStable()
{
    using RectSignature = const QRect &(KisUpdateJobItem::*)() const;
    using SequentialitySignature = KisStrokeJobData::Sequentiality (KisUpdateJobItem::*)() const;

    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::accessRect), RectSignature>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::changeRect), RectSignature>);
    static_assert(std::is_same_v<decltype(&KisUpdateJobItem::strokeJobSequentiality), SequentialitySignature>);
}

void KisImageTypesContractTest::warpTransformTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisWarpTransformWorker>);
    static_assert(std::is_base_of_v<QObject, KisWarpTransformWorker>);
    static_assert(std::is_constructible_v<KisWarpTransformWorker,
                                          KisWarpTransformWorker::WarpType,
                                          QVector<QPointF>,
                                          QVector<QPointF>,
                                          qreal,
                                          KoUpdater *>);
    static_assert(std::has_virtual_destructor_v<KisWarpTransformWorker>);
}

void KisImageTypesContractTest::warpTransformModeSchemaRemainStable()
{
    using WarpType = KisWarpTransformWorker::WarpType;
    using WarpTypeEnum = KisWarpTransformWorker::WarpType_;

    static_assert(std::is_same_v<WarpType, WarpTypeEnum>);
    static_assert(std::is_enum_v<WarpTypeEnum>);
    static_assert(std::is_integral_v<std::underlying_type_t<WarpTypeEnum>>);
    static_assert(static_cast<int>(KisWarpTransformWorker::AFFINE_TRANSFORM) == 0);
    static_assert(static_cast<int>(KisWarpTransformWorker::SIMILITUDE_TRANSFORM) == 1);
    static_assert(static_cast<int>(KisWarpTransformWorker::RIGID_TRANSFORM) == 2);
    static_assert(static_cast<int>(KisWarpTransformWorker::N_MODES) == 3);
}

void KisImageTypesContractTest::warpTransformCalculationSchemaRemainStable()
{
    using WarpCalculation = KisWarpTransformWorker::WarpCalculation;
    using WarpCalculationEnum = KisWarpTransformWorker::WarpCalculation_;

    static_assert(std::is_same_v<WarpCalculation, WarpCalculationEnum>);
    static_assert(std::is_enum_v<WarpCalculationEnum>);
    static_assert(std::is_integral_v<std::underlying_type_t<WarpCalculationEnum>>);
    static_assert(static_cast<int>(KisWarpTransformWorker::GRID) == 0);
    static_assert(static_cast<int>(KisWarpTransformWorker::DRAW) == 1);
}

void KisImageTypesContractTest::warpTransformMathAndImageSignaturesRemainStable()
{
    using MathSignature = QPointF (*)(QPointF, QVector<QPointF>, QVector<QPointF>, qreal);
    using ImageSignature = QImage (*)(KisWarpTransformWorker::WarpType,
                                      const QVector<QPointF> &,
                                      const QVector<QPointF> &,
                                      qreal,
                                      const QImage &,
                                      const QPointF &,
                                      QPointF *);

    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::affineTransformMath), MathSignature>);
    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::similitudeTransformMath), MathSignature>);
    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::rigidTransformMath), MathSignature>);
    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::transformQImage), ImageSignature>);
}

void KisImageTypesContractTest::warpTransformDeviceAndBoundsSignaturesRemainStable()
{
    using RunSignature = void (KisWarpTransformWorker::*)(KisPaintDeviceSP, KisPaintDeviceSP);
    using ChangeRectSignature = QRect (KisWarpTransformWorker::*)(const QRect &);
    using NeedRectSignature = QRect (KisWarpTransformWorker::*)(const QRect &, const QRect &);

    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::run), RunSignature>);
    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::approxChangeRect), ChangeRectSignature>);
    static_assert(std::is_same_v<decltype(&KisWarpTransformWorker::approxNeedRect), NeedRectSignature>);
}

void KisImageTypesContractTest::liquifyTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisLiquifyTransformWorker>);
    static_assert(std::is_constructible_v<KisLiquifyTransformWorker, const QRect &, KoUpdater *, int>);
    static_assert(std::is_copy_constructible_v<KisLiquifyTransformWorker>);
    static_assert(std::is_destructible_v<KisLiquifyTransformWorker>);
}

void KisImageTypesContractTest::liquifyGridAndPointSignaturesRemainStable()
{
    using Worker = KisLiquifyTransformWorker;

    static_assert(std::is_same_v<decltype(&Worker::operator==), bool (Worker::*)(const Worker &) const>);
    static_assert(std::is_same_v<decltype(&Worker::isIdentity), bool (Worker::*)() const>);
    static_assert(std::is_same_v<decltype(&Worker::pointToIndex), int (Worker::*)(const QPoint &)>);
    static_assert(std::is_same_v<decltype(&Worker::gridSize), QSize (Worker::*)() const>);
    static_assert(std::is_same_v<decltype(&Worker::originalPoints), const QVector<QPointF> &(Worker::*)() const>);
    static_assert(std::is_same_v<decltype(&Worker::transformedPoints), QVector<QPointF> &(Worker::*)()>);
}

void KisImageTypesContractTest::liquifyMutationSignaturesRemainStable()
{
    using Worker = KisLiquifyTransformWorker;
    using TranslatePointsSignature = void (Worker::*)(const QPointF &, const QPointF &, qreal, bool, qreal);
    using DeformPointsSignature = void (Worker::*)(const QPointF &, qreal, qreal, bool, qreal);
    using UndoPointsSignature = void (Worker::*)(const QPointF &, qreal, qreal);
    using TranslateSignature = void (Worker::*)(const QPointF &);

    static_assert(std::is_same_v<decltype(&Worker::translatePoints), TranslatePointsSignature>);
    static_assert(std::is_same_v<decltype(&Worker::scalePoints), DeformPointsSignature>);
    static_assert(std::is_same_v<decltype(&Worker::rotatePoints), DeformPointsSignature>);
    static_assert(std::is_same_v<decltype(&Worker::undoPoints), UndoPointsSignature>);
    static_assert(std::is_same_v<decltype(&Worker::translate), TranslateSignature>);
    static_assert(std::is_same_v<decltype(&Worker::translateDstSpace), TranslateSignature>);
    static_assert(std::is_same_v<decltype(&Worker::transformSrcAndDst), void (Worker::*)(const QTransform &)>);
}

void KisImageTypesContractTest::liquifyOutputSignaturesRemainStable()
{
    using Worker = KisLiquifyTransformWorker;
    using ImageSignature = QImage (Worker::*)(const QImage &, const QPointF &, const QTransform &, QPointF *);

    static_assert(std::is_same_v<decltype(&Worker::run), void (Worker::*)(KisPaintDeviceSP, KisPaintDeviceSP)>);
    static_assert(std::is_same_v<decltype(&Worker::runOnQImage), ImageSignature>);
}

void KisImageTypesContractTest::liquifyBoundsAndXmlSignaturesRemainStable()
{
    using Worker = KisLiquifyTransformWorker;

    static_assert(std::is_same_v<decltype(&Worker::approxChangeRect), QRect (Worker::*)(const QRect &)>);
    static_assert(std::is_same_v<decltype(&Worker::approxNeedRect), QRect (Worker::*)(const QRect &, const QRect &)>);
    static_assert(std::is_same_v<decltype(&Worker::accumulatedStrokesBounds), QRectF (Worker::*)() const>);
    static_assert(std::is_same_v<decltype(&Worker::toXML), void (Worker::*)(QDomElement *) const>);
    static_assert(std::is_same_v<decltype(&Worker::fromXML), Worker *(*)(const QDomElement &)>);
}

void KisImageTypesContractTest::baseConstIteratorOwnershipAndTraversalSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisBaseConstIteratorNG>);
    static_assert(std::is_base_of_v<KisBaseConstAccessor, KisBaseConstIteratorNG>);
    static_assert(std::is_abstract_v<KisBaseConstIteratorNG>);
    static_assert(std::has_virtual_destructor_v<KisBaseConstIteratorNG>);
    static_assert(!std::is_copy_constructible_v<KisBaseConstIteratorNG>);
    static_assert(std::is_default_constructible_v<BaseConstIteratorConstructorProbe>);
    ASSERT_ITERATOR_SIGNATURE(KisBaseConstIteratorNG, nextPixel, bool (KisBaseConstIteratorNG::*)());
    ASSERT_ITERATOR_SIGNATURE(KisBaseConstIteratorNG, nextPixels, bool (KisBaseConstIteratorNG::*)(qint32));
    ASSERT_ITERATOR_SIGNATURE(KisBaseConstIteratorNG, nConseqPixels, qint32 (KisBaseConstIteratorNG::*)() const);
}

void KisImageTypesContractTest::horizontalConstIteratorTraversalSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisHLineConstIteratorNG>);
    static_assert(std::is_base_of_v<KisBaseConstIteratorNG, KisHLineConstIteratorNG>);
    static_assert(std::is_abstract_v<KisHLineConstIteratorNG>);
    static_assert(std::has_virtual_destructor_v<KisHLineConstIteratorNG>);
    static_assert(std::is_default_constructible_v<HLineConstIteratorConstructorProbe>);
    ASSERT_ITERATOR_SIGNATURE(KisHLineConstIteratorNG, nextRow, void (KisHLineConstIteratorNG::*)());
    ASSERT_ITERATOR_SIGNATURE(KisHLineConstIteratorNG, resetPixelPos, void (KisHLineConstIteratorNG::*)());
    ASSERT_ITERATOR_SIGNATURE(KisHLineConstIteratorNG, resetRowPos, void (KisHLineConstIteratorNG::*)());
}

void KisImageTypesContractTest::horizontalWritableIteratorSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisHLineIteratorNG>);
    static_assert(std::is_base_of_v<KisHLineConstIteratorNG, KisHLineIteratorNG>);
    static_assert(std::is_base_of_v<KisBaseAccessor, KisHLineIteratorNG>);
    static_assert(std::is_abstract_v<KisHLineIteratorNG>);
    static_assert(std::has_virtual_destructor_v<KisHLineIteratorNG>);
    static_assert(std::is_default_constructible_v<HLineIteratorConstructorProbe>);
}

void KisImageTypesContractTest::verticalConstIteratorTraversalSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisVLineConstIteratorNG>);
    static_assert(std::is_base_of_v<KisBaseConstIteratorNG, KisVLineConstIteratorNG>);
    static_assert(std::is_abstract_v<KisVLineConstIteratorNG>);
    static_assert(std::has_virtual_destructor_v<KisVLineConstIteratorNG>);
    static_assert(std::is_default_constructible_v<VLineConstIteratorConstructorProbe>);
    ASSERT_ITERATOR_SIGNATURE(KisVLineConstIteratorNG, nextColumn, void (KisVLineConstIteratorNG::*)());
    ASSERT_ITERATOR_SIGNATURE(KisVLineConstIteratorNG, resetColumnPos, void (KisVLineConstIteratorNG::*)());
    ASSERT_ITERATOR_SIGNATURE(KisVLineConstIteratorNG, resetPixelPos, void (KisVLineConstIteratorNG::*)());
}

void KisImageTypesContractTest::verticalWritableIteratorSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisVLineIteratorNG>);
    static_assert(std::is_base_of_v<KisVLineConstIteratorNG, KisVLineIteratorNG>);
    static_assert(std::is_base_of_v<KisBaseAccessor, KisVLineIteratorNG>);
    static_assert(std::is_abstract_v<KisVLineIteratorNG>);
    static_assert(std::has_virtual_destructor_v<KisVLineIteratorNG>);
    static_assert(std::is_default_constructible_v<VLineIteratorConstructorProbe>);
}

void KisImageTypesContractTest::lineIteratorTypeAndConstructionSchemaRemainStable()
{
    using HLineConstructor = std::is_constructible<KisHLineIterator2,
                                                   KisDataManager *,
                                                   qint32,
                                                   qint32,
                                                   qint32,
                                                   qint32,
                                                   qint32,
                                                   bool,
                                                   KisIteratorCompleteListener *>;
    using VLineConstructor = std::is_constructible<KisVLineIterator2,
                                                   KisDataManager *,
                                                   qint32,
                                                   qint32,
                                                   qint32,
                                                   qint32,
                                                   qint32,
                                                   bool,
                                                   KisIteratorCompleteListener *>;

    static_assert(std::is_class_v<KisHLineIterator2>);
    static_assert(std::is_base_of_v<KisHLineIteratorNG, KisHLineIterator2>);
    static_assert(std::is_base_of_v<KisBaseIterator, KisHLineIterator2>);
    static_assert(HLineConstructor::value);
    static_assert(std::has_virtual_destructor_v<KisHLineIterator2>);
    static_assert(std::is_class_v<KisVLineIterator2>);
    static_assert(std::is_base_of_v<KisVLineIteratorNG, KisVLineIterator2>);
    static_assert(std::is_base_of_v<KisBaseIterator, KisVLineIterator2>);
    static_assert(VLineConstructor::value);
    static_assert(std::has_virtual_destructor_v<KisVLineIterator2>);
}

void KisImageTypesContractTest::lineIteratorTileCacheSchemaRemainStable()
{
    using HLineTileInfo = KisHLineIterator2::KisTileInfo;
    using VLineTileInfo = KisVLineIterator2::KisTileInfo;

    static_assert(std::is_class_v<HLineTileInfo>);
    static_assert(std::is_same_v<decltype(HLineTileInfo::tile), KisTileSP>);
    static_assert(std::is_same_v<decltype(HLineTileInfo::oldtile), KisTileSP>);
    static_assert(std::is_same_v<decltype(HLineTileInfo::data), quint8 *>);
    static_assert(std::is_same_v<decltype(HLineTileInfo::oldData), quint8 *>);
    static_assert(std::is_class_v<VLineTileInfo>);
    static_assert(std::is_same_v<decltype(VLineTileInfo::tile), KisTileSP>);
    static_assert(std::is_same_v<decltype(VLineTileInfo::oldtile), KisTileSP>);
    static_assert(std::is_same_v<decltype(VLineTileInfo::data), quint8 *>);
    static_assert(std::is_same_v<decltype(VLineTileInfo::oldData), quint8 *>);
}

void KisImageTypesContractTest::lineIteratorDataAccessSignaturesRemainStable()
{
    using HLineConstDataSignature = const quint8 *(KisHLineIterator2::*)() const;
    using HLineDataSignature = quint8 *(KisHLineIterator2::*)();
    using VLineConstDataSignature = const quint8 *(KisVLineIterator2::*)() const;
    using VLineDataSignature = quint8 *(KisVLineIterator2::*)();

    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, oldRawData, HLineConstDataSignature);
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, rawDataConst, HLineConstDataSignature);
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, rawData, HLineDataSignature);
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, nConseqPixels, qint32 (KisHLineIterator2::*)() const);
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, oldRawData, VLineConstDataSignature);
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, rawDataConst, VLineConstDataSignature);
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, rawData, VLineDataSignature);
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, nConseqPixels, qint32 (KisVLineIterator2::*)() const);
}

void KisImageTypesContractTest::horizontalLineIteratorTraversalSignaturesRemainStable()
{
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, nextPixel, bool (KisHLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, nextPixels, bool (KisHLineIterator2::*)(qint32));
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, nextRow, void (KisHLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, resetPixelPos, void (KisHLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, resetRowPos, void (KisHLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, x, qint32 (KisHLineIterator2::*)() const);
    ASSERT_ITERATOR_SIGNATURE(KisHLineIterator2, y, qint32 (KisHLineIterator2::*)() const);
}

void KisImageTypesContractTest::verticalLineIteratorTraversalSignaturesRemainStable()
{
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, nextPixel, bool (KisVLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, nextPixels, bool (KisVLineIterator2::*)(qint32));
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, nextColumn, void (KisVLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, resetPixelPos, void (KisVLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, resetColumnPos, void (KisVLineIterator2::*)());
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, x, qint32 (KisVLineIterator2::*)() const);
    ASSERT_ITERATOR_SIGNATURE(KisVLineIterator2, y, qint32 (KisVLineIterator2::*)() const);
}

void KisImageTypesContractTest::edgeDetectionTypeAndFilterSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisEdgeDetectionKernel>);
    static_assert(std::is_default_constructible_v<KisEdgeDetectionKernel>);
    static_assert(std::is_enum_v<KisEdgeDetectionKernel::FilterType>);
    static_assert(KisEdgeDetectionKernel::Simple == 0);
    static_assert(KisEdgeDetectionKernel::Prewitt == 1);
    static_assert(KisEdgeDetectionKernel::SobelVector == 2);
    static_assert(std::is_enum_v<KisEdgeDetectionKernel::FilterOutput>);
}

void KisImageTypesContractTest::edgeDetectionOutputPolicySchemaRemainsStable()
{
    static_assert(KisEdgeDetectionKernel::pythagorean == 0);
    static_assert(KisEdgeDetectionKernel::xGrowth == 1);
    static_assert(KisEdgeDetectionKernel::xFall == 2);
    static_assert(KisEdgeDetectionKernel::yGrowth == 3);
    static_assert(KisEdgeDetectionKernel::yFall == 4);
    static_assert(KisEdgeDetectionKernel::radian == 5);
}

void KisImageTypesContractTest::edgeDetectionMatrixAndKernelCreationSignaturesRemainStable()
{
    using Matrix = Eigen::Matrix<qreal, Eigen::Dynamic, Eigen::Dynamic>;
    ASSERT_EDGE_DETECTION_SIGNATURE(createHorizontalMatrix,
                                    Matrix (*)(qreal, KisEdgeDetectionKernel::FilterType, bool));
    ASSERT_EDGE_DETECTION_SIGNATURE(createVerticalMatrix, Matrix (*)(qreal, KisEdgeDetectionKernel::FilterType, bool));
    ASSERT_EDGE_DETECTION_SIGNATURE(createHorizontalKernel,
                                    KisConvolutionKernelSP (*)(qreal, KisEdgeDetectionKernel::FilterType, bool, bool));
    ASSERT_EDGE_DETECTION_SIGNATURE(createVerticalKernel,
                                    KisConvolutionKernelSP (*)(qreal, KisEdgeDetectionKernel::FilterType, bool, bool));
    static_assert(
        std::is_same_v<decltype(KisEdgeDetectionKernel::createHorizontalMatrix(1.0, KisEdgeDetectionKernel::Simple)),
                       Matrix>);
    static_assert(
        std::is_same_v<decltype(KisEdgeDetectionKernel::createVerticalMatrix(1.0, KisEdgeDetectionKernel::Simple)),
                       Matrix>);
    static_assert(
        std::is_same_v<decltype(KisEdgeDetectionKernel::createHorizontalKernel(1.0, KisEdgeDetectionKernel::Simple)),
                       KisConvolutionKernelSP>);
    static_assert(std::is_same_v<
                  decltype(KisEdgeDetectionKernel::createHorizontalKernel(1.0, KisEdgeDetectionKernel::Simple, true)),
                  KisConvolutionKernelSP>);
    static_assert(
        std::is_same_v<decltype(KisEdgeDetectionKernel::createVerticalKernel(1.0, KisEdgeDetectionKernel::Simple)),
                       KisConvolutionKernelSP>);
    static_assert(std::is_same_v<
                  decltype(KisEdgeDetectionKernel::createVerticalKernel(1.0, KisEdgeDetectionKernel::Simple, true)),
                  KisConvolutionKernelSP>);
}

void KisImageTypesContractTest::edgeDetectionRadiusConversionSignaturesRemainStable()
{
    ASSERT_EDGE_DETECTION_SIGNATURE(kernelSizeFromRadius, int (*)(qreal));
    ASSERT_EDGE_DETECTION_SIGNATURE(sigmaFromRadius, qreal (*)(qreal));
}

void KisImageTypesContractTest::edgeDetectionDeviceOperationSignaturesRemainStable()
{
    using ApplySignature = void (*)(KisPaintDeviceSP,
                                    const QRect &,
                                    qreal,
                                    qreal,
                                    KisEdgeDetectionKernel::FilterType,
                                    const QBitArray &,
                                    KoUpdater *,
                                    KisEdgeDetectionKernel::FilterOutput,
                                    bool);
    using NormalMapSignature = void (*)(KisPaintDeviceSP,
                                        const QRect &,
                                        qreal,
                                        qreal,
                                        KisEdgeDetectionKernel::FilterType,
                                        int,
                                        QVector<int>,
                                        QVector<bool>,
                                        const QBitArray &,
                                        KoUpdater *,
                                        boost::optional<bool>);
    ASSERT_EDGE_DETECTION_SIGNATURE(applyEdgeDetection, ApplySignature);
    ASSERT_EDGE_DETECTION_SIGNATURE(convertToNormalMap, NormalMapSignature);
    static_assert(std::is_same_v<decltype(KisEdgeDetectionKernel::applyEdgeDetection(std::declval<KisPaintDeviceSP>(),
                                                                                     std::declval<const QRect &>(),
                                                                                     1.0,
                                                                                     1.0,
                                                                                     KisEdgeDetectionKernel::Simple,
                                                                                     std::declval<const QBitArray &>(),
                                                                                     nullptr)),
                                 void>);
    static_assert(
        std::is_same_v<decltype(KisEdgeDetectionKernel::applyEdgeDetection(std::declval<KisPaintDeviceSP>(),
                                                                           std::declval<const QRect &>(),
                                                                           1.0,
                                                                           1.0,
                                                                           KisEdgeDetectionKernel::Simple,
                                                                           std::declval<const QBitArray &>(),
                                                                           nullptr,
                                                                           KisEdgeDetectionKernel::pythagorean)),
                       void>);
    static_assert(std::is_same_v<decltype(KisEdgeDetectionKernel::convertToNormalMap(std::declval<KisPaintDeviceSP>(),
                                                                                     std::declval<const QRect &>(),
                                                                                     1.0,
                                                                                     1.0,
                                                                                     KisEdgeDetectionKernel::Simple,
                                                                                     0,
                                                                                     std::declval<QVector<int>>(),
                                                                                     std::declval<QVector<bool>>(),
                                                                                     std::declval<const QBitArray &>(),
                                                                                     nullptr)),
                                 void>);
}

void KisImageTypesContractTest::selectionMaskOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisSelectionMask>);
    static_assert(std::is_base_of_v<KisEffectMask, KisSelectionMask>);
    static_assert(std::is_base_of_v<KisDecoratedNodeInterface, KisSelectionMask>);
    static_assert(std::is_constructible_v<KisSelectionMask, KisImageWSP>);
    static_assert(std::is_constructible_v<KisSelectionMask, KisImageWSP, const QString &>);
    static_assert(std::is_copy_constructible_v<KisSelectionMask>);
    static_assert(std::has_virtual_destructor_v<KisSelectionMask>);
}

void KisImageTypesContractTest::selectionMaskVisitorAndCloneSignaturesRemainStable()
{
    using NodeVisitorSignature = bool (KisSelectionMask::*)(KisNodeVisitor &);
    using ProcessingVisitorSignature = void (KisSelectionMask::*)(KisProcessingVisitor &, KisUndoAdapter *);
    using CloneSignature = KisNodeSP (KisSelectionMask::*)() const;

    ASSERT_SELECTION_MASK_SIGNATURE(accept, NodeVisitorSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(accept, ProcessingVisitorSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(clone, CloneSignature);
}

void KisImageTypesContractTest::selectionMaskGeometrySignaturesRemainStable()
{
    using BoundsSignature = QRect (KisSelectionMask::*)(const QRect &, KisNode::PositionToFilthy) const;
    using RectSignature = QRect (KisSelectionMask::*)() const;
    using SetDirtySignature = void (KisSelectionMask::*)(const QVector<QRect> &);

    ASSERT_SELECTION_MASK_SIGNATURE(needRect, BoundsSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(changeRect, BoundsSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(extent, RectSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(exactBounds, RectSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(setDirty, SetDirtySignature);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisSelectionMask &>().needRect(std::declval<const QRect &>())),
                       QRect>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisSelectionMask &>().changeRect(std::declval<const QRect &>())),
                       QRect>);
}

void KisImageTypesContractTest::selectionMaskPresentationAndStateSignaturesRemainStable()
{
    using IconSignature = QIcon (KisSelectionMask::*)() const;
    using StateGetterSignature = bool (KisSelectionMask::*)() const;
    using SetStateSignature = void (KisSelectionMask::*)(bool);
    using SetDecorationsSignature = void (KisSelectionMask::*)(bool, bool);
    using SetVisibleSignature = void (KisSelectionMask::*)(bool, bool);

    ASSERT_SELECTION_MASK_SIGNATURE(icon, IconSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(active, StateGetterSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(setActive, SetStateSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(decorationsVisible, StateGetterSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(setDecorationsVisible, SetDecorationsSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(setVisible, SetVisibleSignature);
    static_assert(std::is_same_v<decltype(std::declval<KisSelectionMask &>().setVisible(true)), void>);
}

void KisImageTypesContractTest::selectionMaskSelectionAndModelSignaturesRemainStable()
{
    using SetSelectionSignature = void (KisSelectionMask::*)(KisSelectionSP);
    using NotifySignature = void (KisSelectionMask::*)();
    using PropertiesSignature = KisBaseNode::PropertyList (KisSelectionMask::*)() const;
    using SetPropertiesSignature = void (KisSelectionMask::*)(const KisBaseNode::PropertyList &);

    ASSERT_SELECTION_MASK_SIGNATURE(setSelection, SetSelectionSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(notifySelectionChangedCompressed, NotifySignature);
    ASSERT_SELECTION_MASK_SIGNATURE(sectionModelProperties, PropertiesSignature);
    ASSERT_SELECTION_MASK_SIGNATURE(setSectionModelProperties, SetPropertiesSignature);
}

void KisImageTypesContractTest::indirectPaintingTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisIndirectPaintingSupport>);
    static_assert(std::is_default_constructible_v<KisIndirectPaintingSupport>);
    static_assert(std::has_virtual_destructor_v<KisIndirectPaintingSupport>);
}

void KisImageTypesContractTest::indirectPaintingTemporaryStateSignaturesRemainStable()
{
    ASSERT_INDIRECT_PAINTING_SIGNATURE(hasTemporaryTarget, bool (KisIndirectPaintingSupport::*)() const);
    ASSERT_INDIRECT_PAINTING_SIGNATURE(temporaryTarget, KisPaintDeviceSP (KisIndirectPaintingSupport::*)() const);
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setTemporaryTarget, void (KisIndirectPaintingSupport::*)(KisPaintDeviceSP));
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setTemporarySelection, void (KisIndirectPaintingSupport::*)(KisSelectionSP));
}

void KisImageTypesContractTest::indirectPaintingPainterPolicySignaturesRemainStable()
{
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setTemporaryOpacity, void (KisIndirectPaintingSupport::*)(qreal));
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setTemporaryCompositeOp, void (KisIndirectPaintingSupport::*)(const QString &));
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setTemporaryChannelFlags,
                                       void (KisIndirectPaintingSupport::*)(const QBitArray &));
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setCurrentColor, void (KisIndirectPaintingSupport::*)(const KoColor &));
    ASSERT_INDIRECT_PAINTING_SIGNATURE(setupTemporaryPainter, void (KisIndirectPaintingSupport::*)(KisPainter *) const);
    ASSERT_INDIRECT_PAINTING_SIGNATURE(supportsNonIndirectPainting, bool (KisIndirectPaintingSupport::*)() const);
}

void KisImageTypesContractTest::indirectPaintingMergeAndSuspensionSignaturesRemainStable()
{
    using MergeSignature =
        void (KisIndirectPaintingSupport::*)(KisNodeSP, KUndo2Command *, const KUndo2MagicString &, int);
    using ThreadedMergeSignature = void (KisIndirectPaintingSupport::*)(KisNodeSP,
                                                                        KUndo2Command *,
                                                                        const KUndo2MagicString &,
                                                                        int,
                                                                        QVector<KisRunnableStrokeJobData *> *);
    using SuspendSignature = KisIndirectPaintingSupport::FinalMergeSuspenderSP (KisIndirectPaintingSupport::*)();

    ASSERT_INDIRECT_PAINTING_SIGNATURE(mergeToLayer, MergeSignature);
    ASSERT_INDIRECT_PAINTING_SIGNATURE(mergeToLayerThreaded, ThreadedMergeSignature);
    ASSERT_INDIRECT_PAINTING_SIGNATURE(trySuspendFinalMerge, SuspendSignature);
    static_assert(std::is_same_v<KisIndirectPaintingSupport::FinalMergeSuspenderSP,
                                 QSharedPointer<KisIndirectPaintingSupport::FinalMergeSuspender>>);
}

void KisImageTypesContractTest::indirectPaintingGuardLifetimeSchemaRemainStable()
{
    using FinalMergeSuspender = KisIndirectPaintingSupport::FinalMergeSuspender;
    using ReadLocker = KisIndirectPaintingSupport::ReadLocker;

    static_assert(std::is_class_v<FinalMergeSuspender>);
    static_assert(std::is_constructible_v<FinalMergeSuspender, KisIndirectPaintingSupport *>);
    static_assert(std::is_destructible_v<FinalMergeSuspender>);
    static_assert(std::is_class_v<ReadLocker>);
    static_assert(std::is_constructible_v<ReadLocker, const KisIndirectPaintingSupport *>);
    static_assert(std::is_destructible_v<ReadLocker>);
}

void KisImageTypesContractTest::selectionBasedLayerOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisSelectionBasedLayer>);
    static_assert(std::is_base_of_v<KisLayer, KisSelectionBasedLayer>);
    static_assert(std::is_base_of_v<KisIndirectPaintingSupport, KisSelectionBasedLayer>);
    static_assert(std::is_base_of_v<KisNodeFilterInterface, KisSelectionBasedLayer>);
    static_assert(std::is_abstract_v<KisSelectionBasedLayer>);
    static_assert(std::is_constructible_v<SelectionBasedLayerConstructorProbe,
                                          KisImageWSP,
                                          const QString &,
                                          KisSelectionSP,
                                          KisFilterConfigurationSP>);
    static_assert(std::is_copy_constructible_v<SelectionBasedLayerConstructorProbe>);
    static_assert(std::has_virtual_destructor_v<KisSelectionBasedLayer>);
}

void KisImageTypesContractTest::selectionBasedLayerSelectionDeviceAndProjectionSignaturesRemainStable()
{
    using SelectionGetterSignature = KisSelectionSP (KisSelectionBasedLayer::*)() const;
    using SetSelectionSignature = void (KisSelectionBasedLayer::*)(KisSelectionSP);
    using FetchSelectionSignature = KisSelectionSP (KisSelectionBasedLayer::*)(const QRect &) const;
    using DeviceGetterSignature = KisPaintDeviceSP (KisSelectionBasedLayer::*)() const;
    using ProjectionSignature = bool (KisSelectionBasedLayer::*)() const;
    using ResetCacheSignature = void (KisSelectionBasedLayer::*)(const KoColorSpace *);

    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(internalSelection, SelectionGetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(setInternalSelection, SetSelectionSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(fetchComposedInternalSelection, FetchSelectionSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(original, DeviceGetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(paintDevice, DeviceGetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(needProjection, ProjectionSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(resetCache, ResetCacheSignature);
}

void KisImageTypesContractTest::selectionBasedLayerHierarchyImageAndDirtySignaturesRemainStable()
{
    using AllowChildSignature = bool (KisSelectionBasedLayer::*)(KisNodeSP) const;
    using SetImageSignature = void (KisSelectionBasedLayer::*)(KisImageWSP);
    using DirtySignature = void (KisSelectionBasedLayer::*)();
    using LayerSignature = KisLayer *(KisSelectionBasedLayer::*)();

    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(allowAsChild, AllowChildSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(setImage, SetImageSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(setDirty, DirtySignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(layer, LayerSignature);
}

void KisImageTypesContractTest::selectionBasedLayerGeometryAndLodSignaturesRemainStable()
{
    using CoordinateGetterSignature = qint32 (KisSelectionBasedLayer::*)() const;
    using CoordinateSetterSignature = void (KisSelectionBasedLayer::*)(qint32);
    using LodSignature = bool (KisSelectionBasedLayer::*)() const;
    using BoundsSignature = QRect (KisSelectionBasedLayer::*)() const;

    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(x, CoordinateGetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(y, CoordinateGetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(setX, CoordinateSetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(setY, CoordinateSetterSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(supportsLodPainting, LodSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(extent, BoundsSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(exactBounds, BoundsSignature);
}

void KisImageTypesContractTest::selectionBasedLayerThumbnailAndNotificationSignaturesRemainStable()
{
    using ThumbnailSignature =
        QImage (KisSelectionBasedLayer::*)(qint32, qint32, Qt::AspectRatioMode, KisThumbnailBoundsMode);
    using SequenceSignature = int (KisSelectionBasedLayer::*)() const;
    using NotificationSignature = void (KisSelectionBasedLayer::*)();

    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(createThumbnail, ThumbnailSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(thumbnailSeqNo, SequenceSignature);
    ASSERT_SELECTION_BASED_LAYER_SIGNATURE(slotImageSizeChanged, NotificationSignature);
}

void KisImageTypesContractTest::cloneLayerTypeOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisCloneLayer>);
    static_assert(std::is_enum_v<CopyLayerType>);
    QCOMPARE(static_cast<int>(COPY_PROJECTION), 0);
    QCOMPARE(static_cast<int>(COPY_ORIGINAL), 1);
    static_assert(std::is_constructible_v<KisCloneLayer, KisLayerSP, KisImageWSP, const QString &, quint8>);
    static_assert(std::is_copy_constructible_v<KisCloneLayer>);
    static_assert(std::has_virtual_destructor_v<KisCloneLayer>);
}

void KisImageTypesContractTest::cloneLayerSourceIdentityAndPolicySignaturesRemainStable()
{
    ASSERT_CLONE_LAYER_SIGNATURE(copyFrom, KisLayerSP (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(setCopyFrom, void (KisCloneLayer::*)(KisLayerSP));
    ASSERT_CLONE_LAYER_SIGNATURE(copyFromInfo, KisNodeUuidInfo (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(setCopyFromInfo, void (KisCloneLayer::*)(KisNodeUuidInfo));
    ASSERT_CLONE_LAYER_SIGNATURE(copyType, CopyLayerType (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(setCopyType, void (KisCloneLayer::*)(CopyLayerType));
    ASSERT_CLONE_LAYER_SIGNATURE(reincarnateAsPaintLayer, KisLayerSP (KisCloneLayer::*)() const);
}

void KisImageTypesContractTest::cloneLayerDeviceProjectionAndPresentationSignaturesRemainStable()
{
    ASSERT_CLONE_LAYER_SIGNATURE(original, KisPaintDeviceSP (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(paintDevice, KisPaintDeviceSP (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(needProjection, bool (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(colorSpace, const KoColorSpace *(KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(icon, QIcon (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(sectionModelProperties, KisBaseNode::PropertyList (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(allowAsChild, bool (KisCloneLayer::*)(KisNodeSP) const);
}

void KisImageTypesContractTest::cloneLayerImageAndGeometrySignaturesRemainStable()
{
    ASSERT_CLONE_LAYER_SIGNATURE(setImage, void (KisCloneLayer::*)(KisImageWSP));
    ASSERT_CLONE_LAYER_SIGNATURE(x, qint32 (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(y, qint32 (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(setX, void (KisCloneLayer::*)(qint32));
    ASSERT_CLONE_LAYER_SIGNATURE(setY, void (KisCloneLayer::*)(qint32));
    ASSERT_CLONE_LAYER_SIGNATURE(extent, QRect (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(exactBounds, QRect (KisCloneLayer::*)() const);
}

void KisImageTypesContractTest::cloneLayerVisitorCloneAndUpdateSignaturesRemainStable()
{
    ASSERT_CLONE_LAYER_SIGNATURE(accept, bool (KisCloneLayer::*)(KisNodeVisitor &));
    ASSERT_CLONE_LAYER_SIGNATURE(accept, void (KisCloneLayer::*)(KisProcessingVisitor &, KisUndoAdapter *));
    ASSERT_CLONE_LAYER_SIGNATURE(clone, KisNodeSP (KisCloneLayer::*)() const);
    ASSERT_CLONE_LAYER_SIGNATURE(setDirtyOriginal, void (KisCloneLayer::*)(const QRect &, bool));
    ASSERT_CLONE_LAYER_SIGNATURE(needRectOnSourceForMasks, QRect (KisCloneLayer::*)(const QRect &) const);
    ASSERT_CLONE_LAYER_SIGNATURE(syncLodCache, void (KisCloneLayer::*)());
}

void KisImageTypesContractTest::processingVisitorOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisProcessingVisitor>);
    static_assert(std::is_abstract_v<KisProcessingVisitor>);
    static_assert(std::is_base_of_v<KisShared, KisProcessingVisitor>);
    static_assert(std::has_virtual_destructor_v<KisProcessingVisitor>);
}

void KisImageTypesContractTest::processingVisitorNodeAndLayerVisitSignaturesRemainStable()
{
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisNode *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisPaintLayer *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisGroupLayer *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisAdjustmentLayer *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisExternalLayer *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisGeneratorLayer *, KisUndoAdapter *));
}

void KisImageTypesContractTest::processingVisitorCloneAndMaskVisitSignaturesRemainStable()
{
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisCloneLayer *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisFilterMask *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisTransformMask *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisTransparencyMask *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisColorizeMask *, KisUndoAdapter *));
    ASSERT_PROCESSING_VISITOR_SIGNATURE(visit, void (KisProcessingVisitor::*)(KisSelectionMask *, KisUndoAdapter *));
}

void KisImageTypesContractTest::processingVisitorInitializationSignatureRemainsStable()
{
    ASSERT_PROCESSING_VISITOR_SIGNATURE(createInitCommand, KUndo2Command * (KisProcessingVisitor::*)());
}

void KisImageTypesContractTest::processingVisitorProgressHelperSchemaRemainsStable()
{
    using ProgressHelper = KisProcessingVisitor::ProgressHelper;

    static_assert(std::is_class_v<ProgressHelper>);
    static_assert(std::is_constructible_v<ProgressHelper, const KisNode *>);
    static_assert(std::is_destructible_v<ProgressHelper>);
    static_assert(
        std::is_same_v<decltype(static_cast<KoUpdater *(ProgressHelper::*)() const>(&ProgressHelper::updater)),
                       KoUpdater *(ProgressHelper::*)() const>);
    static_assert(std::is_same_v<decltype(static_cast<void (ProgressHelper::*)()>(&ProgressHelper::cancel)),
                                 void (ProgressHelper::*)()>);
}

#undef ASSERT_DEFAULT_BOUNDS_SIGNATURE
#undef ASSERT_LAYER_UTILS_SIGNATURE
#undef ASSERT_PAINTOP_SETTINGS_SIGNATURE
#undef ASSERT_GROUP_LAYER_SIGNATURE
#undef ASSERT_FIXED_PAINT_DEVICE_SIGNATURE
#undef ASSERT_DATA_MANAGER_SIGNATURE
#undef ASSERT_SAVED_COMMAND_SIGNATURE
#undef ASSERT_PIXEL_SELECTION_SIGNATURE
#undef ASSERT_MASK_GENERATOR_SIGNATURE
#undef ASSERT_GAUSSIAN_MASK_SIGNATURE
#undef ASSERT_DEFAULT_MASK_SIGNATURE
#undef ASSERT_CURVE_MASK_SIGNATURE
#undef ASSERT_MATH_TOOLBOX_SIGNATURE
#undef ASSERT_UPDATER_CONTEXT_SIGNATURE
#undef ASSERT_ITERATOR_SIGNATURE
#undef ASSERT_EDGE_DETECTION_SIGNATURE
#undef ASSERT_SELECTION_MASK_SIGNATURE
#undef ASSERT_SELECTION_BASED_LAYER_SIGNATURE
#undef ASSERT_CLONE_LAYER_SIGNATURE
#undef ASSERT_PROCESSING_VISITOR_SIGNATURE

QTEST_GUILESS_MAIN(KisImageTypesContractTest)

#include "KisImageTypesContractTest.moc"
