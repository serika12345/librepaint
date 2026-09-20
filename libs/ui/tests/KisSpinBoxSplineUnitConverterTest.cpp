/*
 *  SPDX-FileCopyrightText: 2019 Agata Cacko <cacko.azg@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisSpinBoxSplineUnitConverterTest.h>

#include <QTest>

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationToCurve_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("max");
    QTest::addColumn<double>("expected");

    QTest::newRow("normal lower endpoint") << 0 << 0 << 10 << 0.0;
    QTest::newRow("normal interior") << 6 << 0 << 10 << 0.6;
    QTest::newRow("normal upper endpoint") << 10 << 0 << 10 << 1.0;
    QTest::newRow("offset range") << -5 << -10 << 10 << 0.25;
    QTest::newRow("reversed lower endpoint") << 10 << 10 << 0 << 0.0;
    QTest::newRow("reversed upper endpoint") << 0 << 10 << 0 << 1.0;
    QTest::newRow("reversed interior") << 64 << 90 << 0 << 26.0 / 90.0;
}

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationToCurve()
{
    QFETCH(int, x);
    QFETCH(int, min);
    QFETCH(int, max);
    QFETCH(double, expected);

    qreal result = converter.io2sp(x, min, max);
    QCOMPARE(result, expected);
}

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationToSpinBox_data()
{
    QTest::addColumn<double>("x");
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("max");
    QTest::addColumn<int>("expected");

    QTest::newRow("normal lower endpoint") << 0.0 << 0 << 10 << 0;
    QTest::newRow("normal interior") << 0.4 << 0 << 10 << 4;
    QTest::newRow("normal upper endpoint") << 1.0 << 0 << 10 << 10;
    QTest::newRow("offset range") << 0.25 << -10 << 10 << -5;
    QTest::newRow("normal half value rounds upward") << 0.25 << 0 << 10 << 3;
    QTest::newRow("reversed half value rounds downward") << 0.25 << 10 << 0 << 7;
    QTest::newRow("reversed lower interior") << 0.3 << 10 << 0 << 7;
    QTest::newRow("reversed upper interior") << 0.7 << 10 << 0 << 3;
}

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationToSpinBox()
{
    QFETCH(double, x);
    QFETCH(int, min);
    QFETCH(int, max);
    QFETCH(int, expected);

    int result = converter.sp2io(x, min, max);
    QCOMPARE(result, expected);
}

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationTwoWay_data()
{
    QTest::addColumn<double>("xDouble");
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("max");
    QTest::addColumn<int>("xInt");

    QTest::newRow("0.5 in (0, 10) = 5") << 0.5 << 0 << 10 << 5;
    QTest::newRow("0.3 in (0, 10) = 3") << 0.3 << 0 << 10 << 3;
    QTest::newRow("0.7 in (0, 10) = 7") << 0.7 << 0 << 10 << 7;

    QTest::newRow("0.5 in (10, 0) = 5") << 0.5 << 10 << 0 << 5;
    QTest::newRow("0.7 in (10, 0) = 3") << 0.7 << 10 << 0 << 3;
    QTest::newRow("0.3 in (10, 0) = 7") << 0.3 << 10 << 0 << 7;
}

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationTwoWay()
{
    QFETCH(double, xDouble);
    QFETCH(int, min);
    QFETCH(int, max);
    QFETCH(int, xInt);

    int resultInt = converter.sp2io(xDouble, min, max);
    QCOMPARE(resultInt, xInt);

    double resultDouble = converter.io2sp(xInt, min, max);
    QCOMPARE(resultDouble, xDouble);
}

void KisSpinBoxSplineUnitConverterTest::testCurveCalculationCase64()
{
    int inX = 64;
    int min = 90, max = 0;

    double resultDouble = converter.io2sp(inX, min, max);
    int resultInt = converter.sp2io(resultDouble, min, max);

    QCOMPARE(resultInt, inX);
}

QTEST_GUILESS_MAIN(KisSpinBoxSplineUnitConverterTest)
