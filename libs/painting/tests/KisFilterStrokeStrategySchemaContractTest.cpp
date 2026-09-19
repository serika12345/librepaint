/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "strokes/kis_filter_stroke_strategy.h"
#include "strokes/move_stroke_strategy.h"

#include <memory>

#include <QTest>

namespace
{


} // namespace

class KisFilterStrokeStrategySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveStrokeDataValuesAndLodCloneRemainStable();
    void moveStrokePickLayerDataValuesAndLodCloneRemainStable();
    void moveStrokeBarrierUpdateDataValuesAndLodCloneRemainStable();
};

void KisFilterStrokeStrategySchemaContractTest::moveStrokeDataValuesAndLodCloneRemainStable()
{
    using Data = MoveStrokeStrategy::Data;


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

QTEST_GUILESS_MAIN(KisFilterStrokeStrategySchemaContractTest)

#include "KisFilterStrokeStrategySchemaContractTest.moc"
