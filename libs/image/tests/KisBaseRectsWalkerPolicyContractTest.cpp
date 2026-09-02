/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_base_rects_walker.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_RECTS_WALKER_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisBaseRectsWalker::method)), signature>)

class WalkerPolicyProbe final : public KisBaseRectsWalker
{
public:
    UpdateType type() const override;

protected:
    void startTrip(KisProjectionLeafSP startWith) override;
};

} // namespace

class KisBaseRectsWalkerPolicyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateTypesKeepStableOrdinals();
    void nodePositionsKeepStableMasksAndConversion();
    void subtreeVisitFlagsComposeWithoutGraph();
    void jobItemsPreserveAssignedGeometryAndFlags();
    void notificationCollectionsAndWalkerTypesRemainUsable();
    void rectsWalkerOwnershipAndLifetimeSchemaRemainsStable();
    void rectsWalkerGeometryObservationSignaturesRemainStable();
    void rectsWalkerStatePolicySignaturesRemainStable();
    void rectsWalkerCollectionAndConfigurationSignaturesRemainStable();
    void rectsWalkerCloneNotificationSchemaRemainsStable();
};

void KisBaseRectsWalkerPolicyContractTest::updateTypesKeepStableOrdinals()
{
    static_assert(std::is_enum_v<KisBaseRectsWalker::UpdateType>);

    QCOMPARE(static_cast<int>(KisBaseRectsWalker::UPDATE), 0);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::UPDATE_NO_FILTHY), 1);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::FULL_REFRESH), 2);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::FULL_REFRESH_NO_FILTHY), 3);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::UNSUPPORTED), 4);
}

void KisBaseRectsWalkerPolicyContractTest::nodePositionsKeepStableMasksAndConversion()
{
    static_assert(std::is_same_v<KisBaseRectsWalker::NodePosition, qint32>);
    static_assert(std::is_enum_v<KisBaseRectsWalker::NodePositionValues>);

    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_NORMAL), 0x00);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_TOPMOST), 0x01);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_BOTTOMMOST), 0x02);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_EXTRA), 0x04);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_ABOVE_FILTHY), 0x08);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_FILTHY_ORIGINAL), 0x10);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_FILTHY_PROJECTION), 0x20);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_FILTHY), 0x40);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::N_BELOW_FILTHY), 0x80);

    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_ABOVE_FILTHY), KisNode::N_ABOVE_FILTHY);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_FILTHY_PROJECTION),
             KisNode::N_FILTHY_PROJECTION);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_FILTHY), KisNode::N_FILTHY);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_BELOW_FILTHY), KisNode::N_BELOW_FILTHY);
    QCOMPARE(KisBaseRectsWalker::convertPositionToFilthy(KisBaseRectsWalker::N_EXTRA), KisNode::N_FILTHY);
}

void KisBaseRectsWalkerPolicyContractTest::subtreeVisitFlagsComposeWithoutGraph()
{
    static_assert(std::is_enum_v<KisBaseRectsWalker::SubtreeVisitFlag>);
    static_assert(std::is_same_v<KisBaseRectsWalker::SubtreeVisitFlags, QFlags<KisBaseRectsWalker::SubtreeVisitFlag>>);

    QCOMPARE(static_cast<int>(KisBaseRectsWalker::None), 0x0);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::SkipNonRenderableNodes), 0x1);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::NoFilthyMode), 0x2);
    QCOMPARE(static_cast<int>(KisBaseRectsWalker::DontNotifyClones), 0x4);

    const KisBaseRectsWalker::SubtreeVisitFlags flags =
        KisBaseRectsWalker::SkipNonRenderableNodes | KisBaseRectsWalker::DontNotifyClones;
    QVERIFY(flags.testFlag(KisBaseRectsWalker::SkipNonRenderableNodes));
    QVERIFY(!flags.testFlag(KisBaseRectsWalker::NoFilthyMode));
    QVERIFY(flags.testFlag(KisBaseRectsWalker::DontNotifyClones));
}

void KisBaseRectsWalkerPolicyContractTest::jobItemsPreserveAssignedGeometryAndFlags()
{
    const QRect applyRect(-13, 17, 19, 23);
    const KisBaseRectsWalker::NodePosition position = KisBaseRectsWalker::N_EXTRA | KisBaseRectsWalker::N_TOPMOST;
    const KisRenderPassFlags renderFlags = KisRenderPassFlag::NoTransformMaskUpdates;
    const KisBaseRectsWalker::JobItem item{KisProjectionLeafSP(), position, applyRect, renderFlags};

    QVERIFY(!item.m_leaf);
    QCOMPARE(item.m_position, position);
    QCOMPARE(item.m_applyRect, applyRect);
    QCOMPARE(item.m_renderFlags, renderFlags);

    KisBaseRectsWalker::LeafStack jobs;
    jobs.push(item);
    KisBaseRectsWalker::LeafStack copy = jobs;
    copy.top().m_applyRect.translate(3, -5);
    copy.top().m_position = KisBaseRectsWalker::N_BELOW_FILTHY;

    QCOMPARE(jobs.top().m_applyRect, applyRect);
    QCOMPARE(jobs.top().m_position, position);
    QCOMPARE(copy.top().m_applyRect, applyRect.translated(3, -5));
    QCOMPARE(copy.top().m_position, static_cast<KisBaseRectsWalker::NodePosition>(KisBaseRectsWalker::N_BELOW_FILTHY));
}

void KisBaseRectsWalkerPolicyContractTest::notificationCollectionsAndWalkerTypesRemainUsable()
{
    static_assert(std::is_class_v<KisBaseRectsWalker>);
    static_assert(std::is_same_v<KisBaseRectsWalkerSP, KisSharedPtr<KisBaseRectsWalker>>);
    static_assert(std::is_base_of_v<KisBaseRectsWalker, WalkerPolicyProbe>);
    static_assert(!std::is_abstract_v<WalkerPolicyProbe>);
    static_assert(std::is_default_constructible_v<KisBaseRectsWalker::CloneNotification>);

    KisBaseRectsWalker::CloneNotificationsVector notifications;
    notifications.append(KisBaseRectsWalker::CloneNotification());
    KisBaseRectsWalker::CloneNotificationsVector copy = notifications;
    copy.append(KisBaseRectsWalker::CloneNotification());

    QCOMPARE(notifications.size(), 1);
    QCOMPARE(copy.size(), 2);
}

void KisBaseRectsWalkerPolicyContractTest::rectsWalkerOwnershipAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_default_constructible_v<WalkerPolicyProbe>);
    static_assert(std::has_virtual_destructor_v<KisBaseRectsWalker>);
}

void KisBaseRectsWalkerPolicyContractTest::rectsWalkerGeometryObservationSignaturesRemainStable()
{
    ASSERT_RECTS_WALKER_SIGNATURE(accessRect, QRect (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(changeRect, QRect (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(cropRect, QRect (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(levelOfDetail, int (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(requestedRect, QRect (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(startNode, KisNodeSP (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(uncroppedChangeRect, QRect (KisBaseRectsWalker::*)() const);
}

void KisBaseRectsWalkerPolicyContractTest::rectsWalkerStatePolicySignaturesRemainStable()
{
    ASSERT_RECTS_WALKER_SIGNATURE(changeRectVaries, bool (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(checksumValid, bool (KisBaseRectsWalker::*)());
    ASSERT_RECTS_WALKER_SIGNATURE(clonesDontInvalidateFrames, bool (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(isEmpty, bool (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(needRectVaries, bool (KisBaseRectsWalker::*)() const);
    ASSERT_RECTS_WALKER_SIGNATURE(type, KisBaseRectsWalker::UpdateType (KisBaseRectsWalker::*)() const);
}

void KisBaseRectsWalkerPolicyContractTest::rectsWalkerCollectionAndConfigurationSignaturesRemainStable()
{
    ASSERT_RECTS_WALKER_SIGNATURE(cloneNotifications,
                                  KisBaseRectsWalker::CloneNotificationsVector & (KisBaseRectsWalker::*)());
    ASSERT_RECTS_WALKER_SIGNATURE(collectRects, void (KisBaseRectsWalker::*)(KisNodeSP, const QRect &));
    ASSERT_RECTS_WALKER_SIGNATURE(leafStack, KisBaseRectsWalker::LeafStack & (KisBaseRectsWalker::*)());
    ASSERT_RECTS_WALKER_SIGNATURE(recalculate, void (KisBaseRectsWalker::*)(const QRect &));
    ASSERT_RECTS_WALKER_SIGNATURE(setClonesDontInvalidateFrames, void (KisBaseRectsWalker::*)(bool));
    ASSERT_RECTS_WALKER_SIGNATURE(setCropRect, void (KisBaseRectsWalker::*)(QRect));
}

void KisBaseRectsWalkerPolicyContractTest::rectsWalkerCloneNotificationSchemaRemainsStable()
{
    using CloneNotification = KisBaseRectsWalker::CloneNotification;

    static_assert(std::is_constructible_v<CloneNotification, KisNodeSP, const QRect &, bool>);
    static_assert(std::is_same_v<decltype(static_cast<void (CloneNotification::*)()>(&CloneNotification::notify)),
                                 void (CloneNotification::*)()>);
}

#undef ASSERT_RECTS_WALKER_SIGNATURE

QTEST_GUILESS_MAIN(KisBaseRectsWalkerPolicyContractTest)

#include "KisBaseRectsWalkerPolicyContractTest.moc"
