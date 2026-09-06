/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "strokes/kis_filter_stroke_strategy.h"
#include "strokes/move_stroke_strategy.h"

#include <memory>
#include <type_traits>

#include <QTest>

namespace
{

#define ASSERT_FILTER_STROKE_MEMBER(type, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

} // namespace

class KisFilterStrokeStrategySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterStrokeJobDataTypeConstructionAndCloneSchemaRemainStable();
    void filterStrokeIdleBarrierDataSchemaRemainStable();
    void filterStrokeCancellationStorageSchemaRemainStable();
    void filterStrokeStrategyTypeLifetimeAndConstructionSchemaRemainStable();
    void filterStrokeStrategyCallbackSignaturesRemainStable();
    void moveStrokeDataValuesAndLodCloneRemainStable();
    void moveStrokePickLayerDataValuesAndLodCloneRemainStable();
    void moveStrokeBarrierUpdateDataValuesAndLodCloneRemainStable();
    void moveStrokeStrategyTypeLifetimeAndConstructionSchemaRemainStable();
    void moveStrokeStrategyCallbacksAndNotificationsRemainStable();
};

void KisFilterStrokeStrategySchemaContractTest::filterStrokeJobDataTypeConstructionAndCloneSchemaRemainStable()
{
    using Data = KisFilterStrokeStrategy::FilterJobData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_base_of_v<KisStrokeJobData, Data>);
    static_assert(std::is_default_constructible_v<Data>);
    static_assert(std::is_constructible_v<Data, int>);
    static_assert(std::is_same_v<decltype(&Data::frameTime), int Data::*>);
    ASSERT_FILTER_STROKE_MEMBER(Data, createLodClone, KisStrokeJobData * (Data::*)(int));

    QVERIFY(true);
}

void KisFilterStrokeStrategySchemaContractTest::filterStrokeIdleBarrierDataSchemaRemainStable()
{
    using Data = KisFilterStrokeStrategy::IdleBarrierData;
    using Cookie = Data::IdleBarrierCookie;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_base_of_v<KisStrokeJobData, Data>);
    static_assert(std::is_default_constructible_v<Data>);
    static_assert(std::is_same_v<Cookie, QWeakPointer<std::tuple<>>>);
    ASSERT_FILTER_STROKE_MEMBER(Data, createLodClone, KisStrokeJobData * (Data::*)(int));
    ASSERT_FILTER_STROKE_MEMBER(Data, idleBarrierCookie, Cookie (Data::*)() const);

    QVERIFY(true);
}

void KisFilterStrokeStrategySchemaContractTest::filterStrokeCancellationStorageSchemaRemainStable()
{
    using Storage = KisFilterStrokeStrategy::ExternalCancelUpdatesStorage;
    using StorageSP = KisFilterStrokeStrategy::ExternalCancelUpdatesStorageSP;

    static_assert(std::is_class_v<Storage>);
    static_assert(std::is_same_v<StorageSP, QSharedPointer<Storage>>);
    static_assert(std::is_same_v<decltype(&Storage::cancelledLod0UpdateRect), QRect Storage::*>);
    static_assert(std::is_same_v<decltype(&Storage::shouldIssueCancellationUpdates), QAtomicInt Storage::*>);
    static_assert(std::is_same_v<decltype(&Storage::updateRect), QRect Storage::*>);

    QVERIFY(true);
}

void KisFilterStrokeStrategySchemaContractTest::filterStrokeStrategyTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Strategy = KisFilterStrokeStrategy;
    using StorageSP = Strategy::ExternalCancelUpdatesStorageSP;

    static_assert(std::is_class_v<Strategy>);
    static_assert(std::is_base_of_v<KisStrokeStrategyUndoCommandBased, Strategy>);
    static_assert(std::is_constructible_v<Strategy, KisFilterSP, KisFilterConfigurationSP, KisResourcesSnapshotSP>);
    static_assert(
        std::is_constructible_v<Strategy, KisFilterSP, KisFilterConfigurationSP, KisResourcesSnapshotSP, StorageSP>);
    static_assert(std::is_constructible_v<Strategy, const Strategy &, int>);
    static_assert(std::has_virtual_destructor_v<Strategy>);

    QVERIFY(true);
}

void KisFilterStrokeStrategySchemaContractTest::filterStrokeStrategyCallbackSignaturesRemainStable()
{
    using Strategy = KisFilterStrokeStrategy;

    ASSERT_FILTER_STROKE_MEMBER(Strategy, cancelStrokeCallback, void (Strategy::*)());
    ASSERT_FILTER_STROKE_MEMBER(Strategy, createLodClone, KisStrokeStrategy * (Strategy::*)(int));
    ASSERT_FILTER_STROKE_MEMBER(Strategy, doStrokeCallback, void (Strategy::*)(KisStrokeJobData *));
    ASSERT_FILTER_STROKE_MEMBER(Strategy, finishStrokeCallback, void (Strategy::*)());
    ASSERT_FILTER_STROKE_MEMBER(Strategy, initStrokeCallback, void (Strategy::*)());

    QVERIFY(true);
}

void KisFilterStrokeStrategySchemaContractTest::moveStrokeDataValuesAndLodCloneRemainStable()
{
    using Data = MoveStrokeStrategy::Data;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_base_of_v<KisStrokeJobData, Data>);
    static_assert(std::is_constructible_v<Data, QPoint>);
    static_assert(std::is_same_v<decltype(&Data::offset), QPoint Data::*>);
    ASSERT_FILTER_STROKE_MEMBER(Data, createLodClone, KisStrokeJobData * (Data::*)(int));

    Data source(QPoint(16, -8));
    QVERIFY(source.isSequential());
    QVERIFY(!source.isBarrier());
    QVERIFY(!source.isExclusive());
    QCOMPARE(source.offset, QPoint(16, -8));

    std::unique_ptr<KisStrokeJobData> cloneBase(source.createLodClone(2));
    auto *clone = dynamic_cast<Data *>(cloneBase.get());
    QVERIFY(clone);
    QCOMPARE(clone->offset, QPoint(4, -2));
    QVERIFY(clone->isSequential());
    QVERIFY(!clone->isExclusive());

    clone->offset = QPoint(1, 2);
    QCOMPARE(source.offset, QPoint(16, -8));
}

void KisFilterStrokeStrategySchemaContractTest::moveStrokePickLayerDataValuesAndLodCloneRemainStable()
{
    using Data = MoveStrokeStrategy::PickLayerData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_base_of_v<KisStrokeJobData, Data>);
    static_assert(std::is_constructible_v<Data, QPoint>);
    static_assert(std::is_same_v<decltype(&Data::pos), QPoint Data::*>);
    ASSERT_FILTER_STROKE_MEMBER(Data, createLodClone, KisStrokeJobData * (Data::*)(int));

    Data source(QPoint(-12, 20));
    QVERIFY(source.isSequential());
    QVERIFY(!source.isBarrier());
    QVERIFY(!source.isExclusive());
    QCOMPARE(source.pos, QPoint(-12, 20));

    std::unique_ptr<KisStrokeJobData> cloneBase(source.createLodClone(1));
    auto *clone = dynamic_cast<Data *>(cloneBase.get());
    QVERIFY(clone);
    QCOMPARE(clone->pos, QPoint(-6, 10));
    QVERIFY(clone->isSequential());
    QVERIFY(!clone->isExclusive());

    clone->pos = QPoint(3, 5);
    QCOMPARE(source.pos, QPoint(-12, 20));
}

void KisFilterStrokeStrategySchemaContractTest::moveStrokeBarrierUpdateDataValuesAndLodCloneRemainStable()
{
    using Data = MoveStrokeStrategy::BarrierUpdateData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_base_of_v<KisAsynchronousStrokeUpdateHelper::UpdateData, Data>);
    static_assert(std::is_constructible_v<Data, bool>);
    ASSERT_FILTER_STROKE_MEMBER(Data, createLodClone, KisStrokeJobData * (Data::*)(int));

    Data source(true);
    QVERIFY(source.forceUpdate);
    QVERIFY(source.isBarrier());
    QVERIFY(source.isExclusive());

    std::unique_ptr<KisStrokeJobData> cloneBase(source.createLodClone(3));
    auto *clone = dynamic_cast<Data *>(cloneBase.get());
    QVERIFY(clone);
    QVERIFY(clone->forceUpdate);
    QVERIFY(clone->isBarrier());
    QVERIFY(clone->isExclusive());

    clone->forceUpdate = false;
    QVERIFY(source.forceUpdate);
}

void KisFilterStrokeStrategySchemaContractTest::moveStrokeStrategyTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Strategy = MoveStrokeStrategy;

    static_assert(std::is_class_v<Strategy>);
    static_assert(std::is_base_of_v<QObject, Strategy>);
    static_assert(std::is_base_of_v<KisStrokeStrategyUndoCommandBased, Strategy>);
    static_assert(std::is_constructible_v<Strategy, KisNodeSelectionRecipe, KisUpdatesFacade *, KisStrokeUndoFacade *>);
    static_assert(std::is_constructible_v<Strategy, KisNodeList, KisUpdatesFacade *, KisStrokeUndoFacade *>);
    static_assert(std::has_virtual_destructor_v<Strategy>);

    QVERIFY(true);
}

void KisFilterStrokeStrategySchemaContractTest::moveStrokeStrategyCallbacksAndNotificationsRemainStable()
{
    using Strategy = MoveStrokeStrategy;

    ASSERT_FILTER_STROKE_MEMBER(Strategy, cancelStrokeCallback, void (Strategy::*)());
    ASSERT_FILTER_STROKE_MEMBER(Strategy, createLodClone, KisStrokeStrategy * (Strategy::*)(int));
    ASSERT_FILTER_STROKE_MEMBER(Strategy, doStrokeCallback, void (Strategy::*)(KisStrokeJobData *));
    ASSERT_FILTER_STROKE_MEMBER(Strategy, finishStrokeCallback, void (Strategy::*)());
    ASSERT_FILTER_STROKE_MEMBER(Strategy, initStrokeCallback, void (Strategy::*)());
    ASSERT_FILTER_STROKE_MEMBER(Strategy, sigHandlesRectCalculated, void (Strategy::*)(const QRect &));
    ASSERT_FILTER_STROKE_MEMBER(Strategy, sigLayersPicked, void (Strategy::*)(const KisNodeList &));
    ASSERT_FILTER_STROKE_MEMBER(Strategy, sigStrokeStartedEmpty, void (Strategy::*)());

    QVERIFY(true);
}

#undef ASSERT_FILTER_STROKE_MEMBER

QTEST_GUILESS_MAIN(KisFilterStrokeStrategySchemaContractTest)

#include "KisFilterStrokeStrategySchemaContractTest.moc"
