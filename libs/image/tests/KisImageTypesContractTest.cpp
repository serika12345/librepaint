/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paint_information.h"
#include "brushengine/kis_uniform_paintop_property.h"
#include "kis_default_bounds.h"
#include "kis_distance_information.h"
#include "kis_histogram.h"
#include "kis_image_animation_interface.h"
#include "kis_image_signal_router.h"
#include "kis_layer_utils.h"
#include "kis_paint_device_frames_interface.h"
#include "kis_projection_leaf.h"
#include "kis_selection_filters.h"
#include "kis_sequential_iterator.h"
#include "kis_stroke.h"
#include "kis_types.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_DEFAULT_BOUNDS_SIGNATURE(boundsType, method, signature)                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&boundsType::method)), signature>)
#define ASSERT_LAYER_UTILS_SIGNATURE(function, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerUtils::function)), signature>)

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

#undef ASSERT_DEFAULT_BOUNDS_SIGNATURE
#undef ASSERT_LAYER_UTILS_SIGNATURE

QTEST_GUILESS_MAIN(KisImageTypesContractTest)

#include "KisImageTypesContractTest.moc"
