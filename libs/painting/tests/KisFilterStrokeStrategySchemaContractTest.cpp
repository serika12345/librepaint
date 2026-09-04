/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "strokes/kis_filter_stroke_strategy.h"

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

#undef ASSERT_FILTER_STROKE_MEMBER

QTEST_GUILESS_MAIN(KisFilterStrokeStrategySchemaContractTest)

#include "KisFilterStrokeStrategySchemaContractTest.moc"
