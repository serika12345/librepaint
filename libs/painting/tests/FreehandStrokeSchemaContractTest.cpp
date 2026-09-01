/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QTest>

#include <strokes/freehand_stroke.h>

namespace
{

#define ASSERT_FREEHAND_MEMBER(type, method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

} // namespace

class FreehandStrokeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void freehandStrategyFlagSchemaRemainsStable();
    void freehandDabTypeSchemaRemainsStable();
    void freehandDabPayloadMemberSchemaRemainsStable();
    void freehandDabConstructionAndCloneSignaturesRemainStable();
    void freehandStrategyLifecycleAndCallbackSignaturesRemainStable();
};

void FreehandStrokeSchemaContractTest::freehandStrategyFlagSchemaRemainsStable()
{
    using Strategy = FreehandStrokeStrategy;

    static_assert(std::is_class_v<Strategy>);
    static_assert(std::is_base_of_v<KisPainterBasedStrokeStrategy, Strategy>);
    static_assert(std::is_enum_v<Strategy::Flag>);
    static_assert(std::is_same_v<Strategy::Flags, QFlags<Strategy::Flag>>);

    static_assert(static_cast<int>(Strategy::None) == 0);
    static_assert(static_cast<int>(Strategy::SupportsContinuedInterstrokeData) == 1);
    static_assert(static_cast<int>(Strategy::SupportsTimedMergeId) == 2);

    QVERIFY(true);
}

void FreehandStrokeSchemaContractTest::freehandDabTypeSchemaRemainsStable()
{
    using Data = FreehandStrokeStrategy::Data;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_base_of_v<KisStrokeJobData, Data>);
    static_assert(std::is_enum_v<Data::DabType>);

    static_assert(static_cast<int>(Data::POINT) == 0);
    static_assert(static_cast<int>(Data::LINE) == 1);
    static_assert(static_cast<int>(Data::CURVE) == 2);
    static_assert(static_cast<int>(Data::POLYLINE) == 3);
    static_assert(static_cast<int>(Data::POLYGON) == 4);
    static_assert(static_cast<int>(Data::RECT) == 5);
    static_assert(static_cast<int>(Data::ELLIPSE) == 6);
    static_assert(static_cast<int>(Data::PAINTER_PATH) == 7);
    static_assert(static_cast<int>(Data::QPAINTER_PATH) == 8);
    static_assert(static_cast<int>(Data::QPAINTER_PATH_FILL) == 9);

    QVERIFY(true);
}

void FreehandStrokeSchemaContractTest::freehandDabPayloadMemberSchemaRemainsStable()
{
    using Data = FreehandStrokeStrategy::Data;

    static_assert(std::is_same_v<decltype(Data::strokeInfoId), int>);
    static_assert(std::is_same_v<decltype(Data::type), Data::DabType>);
    static_assert(std::is_same_v<decltype(Data::pi1), KisPaintInformation>);
    static_assert(std::is_same_v<decltype(Data::pi2), KisPaintInformation>);
    static_assert(std::is_same_v<decltype(Data::control1), QPointF>);
    static_assert(std::is_same_v<decltype(Data::control2), QPointF>);
    static_assert(std::is_same_v<decltype(Data::points), vQPointF>);
    static_assert(std::is_same_v<decltype(Data::rect), QRectF>);
    static_assert(std::is_same_v<decltype(Data::path), QPainterPath>);
    static_assert(std::is_same_v<decltype(Data::pen), QPen>);
    static_assert(std::is_same_v<decltype(Data::customColor), KoColor>);

    QVERIFY(true);
}

void FreehandStrokeSchemaContractTest::freehandDabConstructionAndCloneSignaturesRemainStable()
{
    using Data = FreehandStrokeStrategy::Data;

    static_assert(std::is_constructible_v<Data, int, const KisPaintInformation &>);
    static_assert(std::is_constructible_v<Data, int, const KisPaintInformation &, const KisPaintInformation &>);
    static_assert(std::is_constructible_v<Data,
                                          int,
                                          const KisPaintInformation &,
                                          const QPointF &,
                                          const QPointF &,
                                          const KisPaintInformation &>);
    static_assert(std::is_constructible_v<Data, int, Data::DabType, const vQPointF &>);
    static_assert(std::is_constructible_v<Data, int, Data::DabType, const QRectF &>);
    static_assert(std::is_constructible_v<Data, int, Data::DabType, const QPainterPath &>);
    static_assert(
        std::is_constructible_v<Data, int, Data::DabType, const QPainterPath &, const QPen &, const KoColor &>);
    ASSERT_FREEHAND_MEMBER(Data, createLodClone, KisStrokeJobData * (Data::*)(int));

    QVERIFY(true);
}

void FreehandStrokeSchemaContractTest::freehandStrategyLifecycleAndCallbackSignaturesRemainStable()
{
    using Strategy = FreehandStrokeStrategy;
    using Resources = KisResourcesSnapshotSP;
    using StrokeInfo = KisFreehandStrokeInfo *;
    using StrokeInfos = QVector<KisFreehandStrokeInfo *>;

    static_assert(std::is_constructible_v<Strategy, Resources, StrokeInfo, const KUndo2MagicString &>);
    static_assert(std::is_constructible_v<Strategy, Resources, StrokeInfo, const KUndo2MagicString &, Strategy::Flags>);
    static_assert(
        std::is_constructible_v<Strategy, Resources, StrokeInfo, const KUndo2MagicString &, Strategy::Flags, int>);
    static_assert(std::is_constructible_v<Strategy, Resources, StrokeInfos, const KUndo2MagicString &>);
    static_assert(
        std::is_constructible_v<Strategy, Resources, StrokeInfos, const KUndo2MagicString &, Strategy::Flags>);
    static_assert(std::has_virtual_destructor_v<Strategy>);

    ASSERT_FREEHAND_MEMBER(Strategy, initStrokeCallback, void (Strategy::*)());
    ASSERT_FREEHAND_MEMBER(Strategy, finishStrokeCallback, void (Strategy::*)());
    ASSERT_FREEHAND_MEMBER(Strategy, doStrokeCallback, void (Strategy::*)(KisStrokeJobData *));
    ASSERT_FREEHAND_MEMBER(Strategy, createLodClone, KisStrokeStrategy * (Strategy::*)(int));
    ASSERT_FREEHAND_MEMBER(Strategy, notifyUserStartedStroke, void (Strategy::*)());
    ASSERT_FREEHAND_MEMBER(Strategy, notifyUserEndedStroke, void (Strategy::*)());

    QVERIFY(true);
}

#undef ASSERT_FREEHAND_MEMBER

QTEST_GUILESS_MAIN(FreehandStrokeSchemaContractTest)

#include "FreehandStrokeSchemaContractTest.moc"
