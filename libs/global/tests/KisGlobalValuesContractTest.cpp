/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_global.h"

#include <QTest>

#include <limits>

class KisGlobalValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void numericLimitsAndSelectionConstantsRemainStable();
    void outlineAndCursorValuesRemainStable();
    void angleAndPowerHelpersNormalizeValues();
    void geometryHelpersTransformValues();
    void trimmingHelpersSplitAndAdvanceRectangles();
};

void KisGlobalValuesContractTest::numericLimitsAndSelectionConstantsRemainStable()
{
    QCOMPARE(quint8_MAX, std::numeric_limits<quint8>::max());
    QCOMPARE(quint16_MAX, std::numeric_limits<quint16>::max());
    QCOMPARE(qint16_MIN, std::numeric_limits<qint16>::min());
    QCOMPARE(qint16_MAX, std::numeric_limits<qint16>::max());
    QCOMPARE(qint32_MIN, std::numeric_limits<qint32>::min());
    QCOMPARE(qint32_MAX, std::numeric_limits<qint32>::max());

    QCOMPARE(MIN_SELECTED, quint8(0));
    QCOMPARE(MAX_SELECTED, quint8(255));
    QCOMPARE(SELECTION_THRESHOLD, quint8(1));

    QCOMPARE(PRESSURE_MIN, 0.0);
    QCOMPARE(PRESSURE_MAX, 1.0);
    QCOMPARE(PRESSURE_DEFAULT, PRESSURE_MAX);
    QCOMPARE(PRESSURE_THRESHOLD, 5.0 / 255.0);
    QCOMPARE(currentUnderlyingStyleNameProperty, "currentUnderlyingStyleName");
}

void KisGlobalValuesContractTest::outlineAndCursorValuesRemainStable()
{
    QCOMPARE(int(OUTLINE_NONE), 0);
    QCOMPARE(int(OUTLINE_CIRCLE), 1);
    QCOMPARE(int(OUTLINE_FULL), 2);
    QCOMPARE(int(OUTLINE_TILT), 3);
    QCOMPARE(int(N_OUTLINE_STYLE_SIZE), 4);

    QCOMPARE(int(CURSOR_STYLE_NO_CURSOR), 0);
    QCOMPARE(int(CURSOR_STYLE_TOOLICON), 1);
    QCOMPARE(int(CURSOR_STYLE_POINTER), 2);
    QCOMPARE(int(CURSOR_STYLE_SMALL_ROUND), 3);
    QCOMPARE(int(CURSOR_STYLE_CROSSHAIR), 4);
    QCOMPARE(int(CURSOR_STYLE_TRIANGLE_RIGHTHANDED), 5);
    QCOMPARE(int(CURSOR_STYLE_TRIANGLE_LEFTHANDED), 6);
    QCOMPARE(int(CURSOR_STYLE_BLACK_PIXEL), 7);
    QCOMPARE(int(CURSOR_STYLE_WHITE_PIXEL), 8);
    QCOMPARE(int(CURSOR_STYLE_ERASER), 9);
    QCOMPARE(int(N_CURSOR_STYLE_SIZE), 10);

    QCOMPARE(int(OLD_CURSOR_STYLE_TOOLICON), 0);
    QCOMPARE(int(OLD_CURSOR_STYLE_CROSSHAIR), 1);
    QCOMPARE(int(OLD_CURSOR_STYLE_POINTER), 2);
    QCOMPARE(int(OLD_CURSOR_STYLE_OUTLINE), 3);
    QCOMPARE(int(OLD_CURSOR_STYLE_NO_CURSOR), 4);
    QCOMPARE(int(OLD_CURSOR_STYLE_SMALL_ROUND), 5);
    QCOMPARE(int(OLD_CURSOR_STYLE_OUTLINE_CENTER_DOT), 6);
    QCOMPARE(int(OLD_CURSOR_STYLE_OUTLINE_CENTER_CROSS), 7);
    QCOMPARE(int(OLD_CURSOR_STYLE_TRIANGLE_RIGHTHANDED), 8);
    QCOMPARE(int(OLD_CURSOR_STYLE_TRIANGLE_LEFTHANDED), 9);
    QCOMPARE(int(OLD_CURSOR_STYLE_OUTLINE_TRIANGLE_RIGHTHANDED), 10);
    QCOMPARE(int(OLD_CURSOR_STYLE_OUTLINE_TRIANGLE_LEFTHANDED), 11);
}

void KisGlobalValuesContractTest::angleAndPowerHelpersNormalizeValues()
{
    const qreal pi = M_PI;
    QCOMPARE(normalizeAngle(-0.5 * pi), 1.5 * pi);
    QCOMPARE(normalizeAngle(5.0 * pi), pi);
    QCOMPARE(normalizeAngleDegrees(-90.0), 270.0);
    QCOMPARE(normalizeAngleDegrees(450.0), 90.0);
    QCOMPARE(shortestAngularDistance(0.0, 1.5 * pi), 0.5 * pi);
    QCOMPARE(incrementInDirection(0.0, 0.25 * pi, 0.5 * pi), 0.25 * pi);
    QCOMPARE(bisectorAngle(0.0, 0.5 * pi), 0.25 * pi);

    QCOMPARE(pow2(4), 16);
    QCOMPARE(pow3(4), 64);
    QCOMPARE(kisDegreesToRadians(180.0), pi);
    QCOMPARE(kisRadiansToDegrees(pi), 180.0);
    QCOMPARE(nextPowerOfTwo(quint32(5)), quint32(8));
    QCOMPARE(nextPowerOfTwo(quint64(16)), quint64(32));
}

void KisGlobalValuesContractTest::geometryHelpersTransformValues()
{
    QCOMPARE(kisBoundFast(0, -2, 10), 0);
    QCOMPARE(kisBoundFast(0, 5, 10), 5);
    QCOMPARE(kisBoundFast(0, 12, 10), 10);

    const QPointF origin;
    const QPointF point(3.0, 4.0);
    QCOMPARE(kisDistance(origin, point), 5.0);
    QCOMPARE(kisSquareDistance(origin, point), 25.0);

    const QLineF horizontal(QPointF(0.0, 2.0), QPointF(8.0, 2.0));
    QCOMPARE(kisDistanceToLine(QPointF(3.0, 5.0), horizontal), 3.0);
    QCOMPARE(kisSquareDistanceToLine(QPointF(3.0, 5.0), horizontal), 9.0);

    QCOMPARE(kisGrowRect(QRect(1, 2, 3, 4), 2), QRect(-1, 0, 7, 8));
    QCOMPARE(kisEnsureInRect(QRect(8, 8, 4, 4), QRect(0, 0, 10, 10)),
             QRect(6, 6, 4, 4));
    QCOMPARE(kisProjectOnVector(QPointF(2.0, 0.0), QPointF(3.0, 4.0)),
             QPointF(3.0, 0.0));
    QCOMPARE(snapToClosestAxis(QPointF(2.0, 5.0)), QPointF(0.0, 5.0));
    QCOMPARE(snapToClosestAxis(QPointF(5.0, 2.0)), QPointF(5.0, 0.0));

    const QPointF snapped = snapToClosestNiceAngle(QPointF(3.0, 2.0),
                                                    QPointF(1.0, 1.0),
                                                    M_PI / 2.0);
    QVERIFY(qFuzzyCompare(snapped.x(), 1.0 + std::sqrt(5.0)));
    QCOMPARE(snapped.y(), 1.0);
}

void KisGlobalValuesContractTest::trimmingHelpersSplitAndAdvanceRectangles()
{
    QRectF floatingLeftSource(0.0, 0.0, 10.0, 8.0);
    QCOMPARE(kisTrimLeft(3, floatingLeftSource), QRectF(0.0, 0.0, 3.0, 8.0));
    QCOMPARE(floatingLeftSource, QRectF(3.0, 0.0, 7.0, 8.0));

    QRect integerLeftSource(0, 0, 10, 8);
    QCOMPARE(kisTrimLeft(3, integerLeftSource), QRect(0, 0, 3, 8));
    QCOMPARE(integerLeftSource, QRect(3, 0, 7, 8));

    QRectF floatingTopSource(0.0, 0.0, 10.0, 8.0);
    QCOMPARE(kisTrimTop(2, floatingTopSource), QRectF(0.0, 0.0, 10.0, 2.0));
    QCOMPARE(floatingTopSource, QRectF(0.0, 2.0, 10.0, 6.0));

    QRect integerTopSource(0, 0, 10, 8);
    QCOMPARE(kisTrimTop(2, integerTopSource), QRect(0, 0, 10, 2));
    QCOMPARE(integerTopSource, QRect(0, 2, 10, 6));
}

QTEST_GUILESS_MAIN(KisGlobalValuesContractTest)

#include "KisGlobalValuesContractTest.moc"
