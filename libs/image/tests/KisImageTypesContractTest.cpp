/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_types.h"

#include <QTest>

#include <type_traits>

class KisImageTypesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void intrusivePointerAliasesPreserveOwnershipKinds();
    void qtPointerAliasesPreserveOwnershipKinds();
    void collectionAliasesPreserveElementAndIteratorTypes();
    void thumbnailBoundsModesRemainDistinct();
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

QTEST_GUILESS_MAIN(KisImageTypesContractTest)

#include "KisImageTypesContractTest.moc"
