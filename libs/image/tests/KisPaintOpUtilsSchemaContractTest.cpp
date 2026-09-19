/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_paintop_utils.h"

#include <QTest>


namespace
{

struct PaintOpProbe {
};


} // namespace

class KisPaintOpUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void positionHistoryTypeAndUpdateBehaviorRemainStable();
    void sizeAndAutomaticSpacingBehaviorRemainStable();
};

// clang-format off
void KisPaintOpUtilsSchemaContractTest::positionHistoryTypeAndUpdateBehaviorRemainStable()
// clang-format on
{
    using History = KisPaintOpUtils::PositionHistory;


    History history;
    const QPointF initialPoint(10.0, 20.0);
    const QPointF secondPoint(20.0, 30.0);
    const QPointF thirdPoint(30.0, 40.0);

    history.reset(initialPoint);
    QCOMPARE(history.pushThroughHistory(initialPoint, 1.0), initialPoint);
    QCOMPARE(history.pushThroughHistory(secondPoint, 1.0), initialPoint);
    QCOMPARE(history.pushThroughHistory(thirdPoint, 1.0), secondPoint);
}

// clang-format off
void KisPaintOpUtilsSchemaContractTest::sizeAndAutomaticSpacingBehaviorRemainStable()
// clang-format on
{

    QVERIFY(KisPaintOpUtils::checkSizeTooSmall(1.0, 0.009, 10.0));
    QVERIFY(KisPaintOpUtils::checkSizeTooSmall(1.0, 10.0, 0.009));
    QVERIFY(!KisPaintOpUtils::checkSizeTooSmall(1.0, 0.01, 0.01));
    QCOMPARE(KisPaintOpUtils::calcAutoSpacing(0.25, 0.5), 0.125);
    QCOMPARE(KisPaintOpUtils::calcAutoSpacing(4.0, 0.5), 1.0);
    QCOMPARE(KisPaintOpUtils::calcAutoSpacing(QPointF(4.0, 9.0), 0.5, 1.0), QPointF(1.0, 1.5));
}

// clang-format off
// clang-format off
// clang-format off
QTEST_GUILESS_MAIN(KisPaintOpUtilsSchemaContractTest)

#include "KisPaintOpUtilsSchemaContractTest.moc"
