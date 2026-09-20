/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisRollingMeanAccumulatorWrapper.h>

#include <QTest>

#include <cmath>

class KisRollingMeanAccumulatorWrapperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void distinguishesEmptyAndPopulatedWindow();
    void resetStartsAnEmptyWindow();
};

void KisRollingMeanAccumulatorWrapperTest::distinguishesEmptyAndPopulatedWindow()
{
    KisRollingMeanAccumulatorWrapper accumulator(3);
    QCOMPARE(accumulator.rollingCount(), 0);
    QVERIFY(std::isnan(accumulator.rollingMean()));
    QCOMPARE(accumulator.rollingMeanSafe(), 0.0);

    accumulator(1.0);
    accumulator(2.0);
    accumulator(3.0);
    QCOMPARE(accumulator.rollingCount(), 3);
    QCOMPARE(accumulator.rollingMean(), 2.0);
    QCOMPARE(accumulator.rollingMeanSafe(), 2.0);

    accumulator(7.0);
    QCOMPARE(accumulator.rollingCount(), 3);
    QCOMPARE(accumulator.rollingMean(), 4.0);
    QCOMPARE(accumulator.rollingMeanSafe(), 4.0);
}

void KisRollingMeanAccumulatorWrapperTest::resetStartsAnEmptyWindow()
{
    KisRollingMeanAccumulatorWrapper accumulator(3);
    accumulator(1.0);
    accumulator(2.0);
    accumulator(3.0);

    accumulator.reset(2);
    QCOMPARE(accumulator.rollingCount(), 0);
    QVERIFY(std::isnan(accumulator.rollingMean()));
    QCOMPARE(accumulator.rollingMeanSafe(), 0.0);

    accumulator(6.0);
    accumulator(10.0);
    accumulator(14.0);
    QCOMPARE(accumulator.rollingCount(), 2);
    QCOMPARE(accumulator.rollingMean(), 12.0);
    QCOMPARE(accumulator.rollingMeanSafe(), 12.0);
}

QTEST_GUILESS_MAIN(KisRollingMeanAccumulatorWrapperTest)

#include "KisRollingMeanAccumulatorWrapperTest.moc"
