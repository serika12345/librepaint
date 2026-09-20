/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisRollingSumAccumulatorWrapper.h>

#include <QTest>

class KisRollingSumAccumulatorWrapperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsSumAndCountInsideWindow();
    void resetStartsAnEmptyWindow();
};

void KisRollingSumAccumulatorWrapperTest::keepsSumAndCountInsideWindow()
{
    KisRollingSumAccumulatorWrapper accumulator(3);
    QCOMPARE(accumulator.rollingCount(), 0);
    QCOMPARE(accumulator.rollingSum(), 0.0);

    accumulator(1.0);
    accumulator(2.0);
    accumulator(3.0);
    QCOMPARE(accumulator.rollingCount(), 3);
    QCOMPARE(accumulator.rollingSum(), 6.0);

    accumulator(4.0);
    QCOMPARE(accumulator.rollingCount(), 3);
    QCOMPARE(accumulator.rollingSum(), 9.0);
}

void KisRollingSumAccumulatorWrapperTest::resetStartsAnEmptyWindow()
{
    KisRollingSumAccumulatorWrapper accumulator(3);
    accumulator(1.0);
    accumulator(2.0);
    accumulator(3.0);

    accumulator.reset(2);
    QCOMPARE(accumulator.rollingCount(), 0);
    QCOMPARE(accumulator.rollingSum(), 0.0);

    accumulator(5.0);
    accumulator(7.0);
    accumulator(9.0);
    QCOMPARE(accumulator.rollingCount(), 2);
    QCOMPARE(accumulator.rollingSum(), 16.0);
}

QTEST_GUILESS_MAIN(KisRollingSumAccumulatorWrapperTest)

#include "KisRollingSumAccumulatorWrapperTest.moc"
