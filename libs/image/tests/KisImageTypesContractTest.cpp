/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_default_bounds.h"
#include "kis_selection_filters.h"
#include "kis_types.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_DEFAULT_BOUNDS_SIGNATURE(boundsType, method, signature)                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&boundsType::method)), signature>)

class SelectionDefaultBoundsConstructorProbe final : public KisSelectionDefaultBoundsBase
{
public:
    SelectionDefaultBoundsConstructorProbe() = default;

protected:
    KisPaintDeviceSP parentPaintDevice() const override;
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

#undef ASSERT_DEFAULT_BOUNDS_SIGNATURE

QTEST_GUILESS_MAIN(KisImageTypesContractTest)

#include "KisImageTypesContractTest.moc"
