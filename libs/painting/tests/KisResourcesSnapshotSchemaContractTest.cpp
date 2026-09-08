/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokeCompatibilityInfo.h"
#include "kis_resources_snapshot.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisResourcesSnapshot::method)), signature>)
} // namespace

class KisResourcesSnapshotSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourcesSnapshotTypeLifetimeAndConstructionSchemaRemainStable();
    void resourcesSnapshotPaintingContextSignaturesRemainStable();
    void resourcesSnapshotResourceValueSignaturesRemainStable();
    void resourcesSnapshotPaintingPolicyAndLodSignaturesRemainStable();
    void resourcesSnapshotOverrideAndBrushSignaturesRemainStable();
    void strokeCompatibilityTypeConstructionAndEqualitySchemaRemainStable();
    void strokeCompatibilityColorMembersSchemaRemainStable();
    void strokeCompatibilityResourceSignatureMembersSchemaRemainStable();
    void strokeCompatibilityGeneratorAndNodeMembersSchemaRemainStable();
    void strokeCompatibilityPaintingPolicyMembersSchemaRemainStable();
};

void KisResourcesSnapshotSchemaContractTest::resourcesSnapshotTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Snapshot = KisResourcesSnapshot;

    static_assert(std::is_same_v<KisResourcesSnapshotSP, KisSharedPtr<Snapshot>>);
    static_assert(std::is_class_v<Snapshot>);
    static_assert(std::is_base_of_v<KisShared, Snapshot>);
    static_assert(std::is_constructible_v<Snapshot, KisImageSP, KisNodeSP>);
    static_assert(std::is_constructible_v<Snapshot, KisImageSP, KisNodeSP, KisDefaultBoundsBaseSP>);
    static_assert(std::is_constructible_v<Snapshot, KisImageSP, KisNodeSP, KoCanvasResourcesInterfaceSP>);
    static_assert(std::is_constructible_v<Snapshot,
                                          KisImageSP,
                                          KisNodeSP,
                                          KoCanvasResourcesInterfaceSP,
                                          KisDefaultBoundsBaseSP,
                                          KisNodeList,
                                          KisPaintOpPresetSP>);
    static_assert(std::is_same_v<decltype(Snapshot(std::declval<KisImageSP>(), std::declval<KisNodeSP>())), Snapshot>);
    static_assert(std::is_same_v<decltype(Snapshot(std::declval<KisImageSP>(),
                                                   std::declval<KisNodeSP>(),
                                                   std::declval<KoCanvasResourcesInterfaceSP>())),
                                 Snapshot>);
    static_assert(std::is_destructible_v<Snapshot>);
}

void KisResourcesSnapshotSchemaContractTest::resourcesSnapshotPaintingContextSignaturesRemainStable()
{
    using Snapshot = KisResourcesSnapshot;

    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(activeSelection, KisSelectionSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentNode, KisNodeSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(fillTransform, QTransform (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(image, KisImageSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(postExecutionUndoAdapter, KisPostExecutionUndoAdapter * (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(selectedNodes, KisNodeList (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setCurrentNode, void (Snapshot::*)(KisNodeSP));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setFillStyle, void (Snapshot::*)(KisPainter::FillStyle));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setFillTransform, void (Snapshot::*)(QTransform));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setStrokeStyle, void (Snapshot::*)(KisPainter::StrokeStyle));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setupMaskingBrushPainter, void (Snapshot::*)(KisPainter *));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setupPainter, void (Snapshot::*)(KisPainter *));
}

void KisResourcesSnapshotSchemaContractTest::resourcesSnapshotResourceValueSignaturesRemainStable()
{
    using Snapshot = KisResourcesSnapshot;

    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(channelLockFlags, QBitArray (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(compositeOpId, QString (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentBgColor, KoColor (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentFgColor, KoColor (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentGenerator, KisFilterConfigurationSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentGradient, KoAbstractGradientSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentGradientSignature, KoResourceSignature (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentPaintOpPreset, KisPaintOpPresetSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentPaintOpPresetSignature, KoResourceSignature (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentPattern, KoPatternSP (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(currentPatternSignature, KoResourceSignature (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(isUsingOtherColor, bool (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(opacity, qreal (Snapshot::*)() const);
}

void KisResourcesSnapshotSchemaContractTest::resourcesSnapshotPaintingPolicyAndLodSignaturesRemainStable()
{
    using Snapshot = KisResourcesSnapshot;

    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(airbrushingInterval, qreal (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(effectiveZoom, qreal (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(indirectPaintingCompositeOp, QString (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(needsAirbrushing, bool (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(needsIndirectPainting, bool (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(needsMaskingBrushRendering, bool (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(needsSpacingUpdates, bool (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(presetAllowsLod, bool (Snapshot::*)() const);
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(presetNeedsAsynchronousUpdates, bool (Snapshot::*)() const);
}

void KisResourcesSnapshotSchemaContractTest::resourcesSnapshotOverrideAndBrushSignaturesRemainStable()
{
    using Snapshot = KisResourcesSnapshot;

    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setBGColorOverride, void (Snapshot::*)(const KoColor &));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setBrush, void (Snapshot::*)(const KisPaintOpPresetSP &));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setFGColorOverride, void (Snapshot::*)(const KoColor &));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setMirroring, void (Snapshot::*)(bool, bool));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setOpacity, void (Snapshot::*)(qreal));
    ASSERT_RESOURCES_SNAPSHOT_SIGNATURE(setSelectionOverride, void (Snapshot::*)(KisSelectionSP));
}

void KisResourcesSnapshotSchemaContractTest::strokeCompatibilityTypeConstructionAndEqualitySchemaRemainStable()
{
    using Info = KisStrokeCompatibilityInfo;

    static_assert(std::is_class_v<Info>);
    static_assert(std::is_default_constructible_v<Info>);
    static_assert(std::is_constructible_v<Info, KisResourcesSnapshot &>);
    static_assert(std::is_same_v<decltype(std::declval<const Info &>() == std::declval<const Info &>()), bool>);
}

void KisResourcesSnapshotSchemaContractTest::strokeCompatibilityColorMembersSchemaRemainStable()
{
    using Info = KisStrokeCompatibilityInfo;

    static_assert(std::is_same_v<decltype(&Info::currentBgColor), KoColor Info::*>);
    static_assert(std::is_same_v<decltype(&Info::currentFgColor), KoColor Info::*>);
}

void KisResourcesSnapshotSchemaContractTest::strokeCompatibilityResourceSignatureMembersSchemaRemainStable()
{
    using Info = KisStrokeCompatibilityInfo;

    static_assert(std::is_same_v<decltype(&Info::currentGradient), KoResourceSignature Info::*>);
    static_assert(std::is_same_v<decltype(&Info::currentPattern), KoResourceSignature Info::*>);
    static_assert(std::is_same_v<decltype(&Info::currentPreset), KoResourceSignature Info::*>);
}

void KisResourcesSnapshotSchemaContractTest::strokeCompatibilityGeneratorAndNodeMembersSchemaRemainStable()
{
    using Info = KisStrokeCompatibilityInfo;

    static_assert(std::is_same_v<decltype(&Info::currentGeneratorXml), QString Info::*>);
    static_assert(std::is_same_v<decltype(&Info::currentNode), QUuid Info::*>);
}

void KisResourcesSnapshotSchemaContractTest::strokeCompatibilityPaintingPolicyMembersSchemaRemainStable()
{
    using Info = KisStrokeCompatibilityInfo;

    static_assert(std::is_same_v<decltype(&Info::channelLockFlags), QBitArray Info::*>);
    static_assert(std::is_same_v<decltype(&Info::compositeOpId), QString Info::*>);
    static_assert(std::is_same_v<decltype(&Info::opacity), qreal Info::*>);
}

QTEST_GUILESS_MAIN(KisResourcesSnapshotSchemaContractTest)

#include "KisResourcesSnapshotSchemaContractTest.moc"
