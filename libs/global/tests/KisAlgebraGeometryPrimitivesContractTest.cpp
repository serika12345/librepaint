/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <QTest>

#include <cmath>

namespace
{

bool closeReal(qreal actual, qreal expected, qreal tolerance = 1e-12)
{
    return qAbs(actual - expected) < tolerance;
}

bool closePoint(const QPointF &actual, const QPointF &expected)
{
    return closeReal(actual.x(), expected.x()) && closeReal(actual.y(), expected.y());
}

} // namespace

class KisAlgebraGeometryPrimitivesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rightHalfPlaneReportsSignedDistanceAndLine();
    void outerCircleReportsBoundaryDistanceAndFade();
    void haltonSequenceAdvancesAndRoundsRanges();
    void vectorProductsAndLengthsUseCartesianCoordinates();
    void unitNormalsPreserveOrientation();
    void anglesReportRotationDirectionAndCoincidentDefault();
    void signHelpersDefineZeroAndCopiedSign();
    void interpolationAndRoundingPreserveScalarAndPointValues();
    void floorDivisionAndRangeHelpersHandleBoundaries();
    void wrappingNormalizesIntegralFloatingAndShiftedRanges();
    void rectangleAndDimensionHelpersPreserveGeometry();
    void relativeAndAbsoluteConversionsRoundTripValues();
};

void KisAlgebraGeometryPrimitivesContractTest::rightHalfPlaneReportsSignedDistanceAndLine()
{
    const QPointF start(1.0, 2.0);
    const QPointF end(5.0, 2.0);
    const KisAlgebra2D::RightHalfPlane plane(start, end);

    QCOMPARE(plane.getLine(), QLineF(start, end));
    QCOMPARE(plane.value(QPointF(3.0, 5.0)), 3.0);
    QCOMPARE(plane.valueSq(QPointF(3.0, 5.0)), 9.0);
    QCOMPARE(plane.pos(QPointF(3.0, 5.0)), 1);
    QCOMPARE(plane.value(QPointF(3.0, -2.0)), -4.0);
    QCOMPARE(plane.valueSq(QPointF(3.0, -2.0)), -16.0);
    QCOMPARE(plane.pos(QPointF(3.0, -2.0)), -1);
    QCOMPARE(plane.value(QPointF(3.0, 2.0)), 0.0);
    QCOMPARE(plane.valueSq(QPointF(3.0, 2.0)), 0.0);
    QCOMPARE(plane.pos(QPointF(3.0, 2.0)), 0);

    const QLineF verticalLine(QPointF(2.0, 1.0), QPointF(2.0, 5.0));
    const KisAlgebra2D::RightHalfPlane verticalPlane(verticalLine);
    QCOMPARE(verticalPlane.getLine(), verticalLine);
    QCOMPARE(verticalPlane.value(QPointF(-1.0, 3.0)), 3.0);
}

void KisAlgebraGeometryPrimitivesContractTest::outerCircleReportsBoundaryDistanceAndFade()
{
    const QPointF center(2.0, -1.0);
    const KisAlgebra2D::OuterCircle circle(center, 5.0);

    QCOMPARE(circle.value(QPointF(2.0, 2.0)), -2.0);
    QCOMPARE(circle.valueSq(QPointF(2.0, 2.0)), -4.0);
    QCOMPARE(circle.pos(QPointF(2.0, 2.0)), -1);
    QCOMPARE(circle.value(QPointF(5.0, 3.0)), 0.0);
    QCOMPARE(circle.valueSq(QPointF(5.0, 3.0)), 0.0);
    QCOMPARE(circle.pos(QPointF(5.0, 3.0)), 0);
    QCOMPARE(circle.value(QPointF(2.0, 7.0)), 3.0);
    QCOMPARE(circle.valueSq(QPointF(2.0, 7.0)), 9.0);
    QCOMPARE(circle.pos(QPointF(2.0, 7.0)), 1);

    QCOMPARE(circle.fadeSq(QPointF(5.0, 3.0)), 0.0);
    QCOMPARE(circle.fadeSq(QPointF(2.0, 5.0)), 1.0);
    QVERIFY(closeReal(circle.fadeSq(center), -25.0 / 11.0));
}

void KisAlgebraGeometryPrimitivesContractTest::haltonSequenceAdvancesAndRoundsRanges()
{
    KisAlgebra2D::HaltonSequenceGenerator sequence(2);

    QCOMPARE(sequence.currentValue(), 0.0);
    QCOMPARE(sequence.currentValue(10), 0);
    QCOMPARE(sequence.generate(), 0.5);
    QCOMPARE(sequence.currentValue(), 0.5);
    QCOMPARE(sequence.currentValue(10), 5);

    sequence.step();
    QCOMPARE(sequence.currentValue(), 0.25);
    QCOMPARE(sequence.currentValue(10), 3);
    QCOMPARE(sequence.generate(), 0.75);
    QCOMPARE(sequence.generate(10), 1);
    QCOMPARE(sequence.currentValue(), 0.125);
}

void KisAlgebraGeometryPrimitivesContractTest::vectorProductsAndLengthsUseCartesianCoordinates()
{
    const QPointF first(3.0, 4.0);
    const QPointF second(-2.0, 5.0);

    QCOMPARE(KisAlgebra2D::dotProduct(first, second), 14.0);
    QCOMPARE(KisAlgebra2D::crossProduct(first, second), 23.0);
    QCOMPARE(KisAlgebra2D::normSquared(first), 25.0);
    QCOMPARE(KisAlgebra2D::norm(first), 5.0);
    QVERIFY(closePoint(KisAlgebra2D::normalize(first), QPointF(0.6, 0.8)));
}

void KisAlgebraGeometryPrimitivesContractTest::unitNormalsPreserveOrientation()
{
    const QPointF direction(3.0, 4.0);
    const QPointF left = KisAlgebra2D::leftUnitNormal(direction);
    const QPointF right = KisAlgebra2D::rightUnitNormal(direction);

    QVERIFY(closePoint(left, QPointF(0.8, -0.6)));
    QVERIFY(closePoint(right, QPointF(-0.8, 0.6)));
    QVERIFY(closeReal(KisAlgebra2D::norm(left), 1.0));
    QVERIFY(closeReal(KisAlgebra2D::norm(right), 1.0));
    QVERIFY(KisAlgebra2D::crossProduct(direction, left) < 0.0);
    QVERIFY(KisAlgebra2D::crossProduct(direction, right) > 0.0);
    QVERIFY(closePoint(KisAlgebra2D::inwardUnitNormal(direction, 1), left));
    QVERIFY(closePoint(KisAlgebra2D::inwardUnitNormal(direction, -1), right));
}

void KisAlgebraGeometryPrimitivesContractTest::anglesReportRotationDirectionAndCoincidentDefault()
{
    const qreal quarterTurn = std::acos(-1.0) / 2.0;

    QVERIFY(closeReal(KisAlgebra2D::angleBetweenVectors(QPointF(1.0, 0.0), QPointF(0.0, 1.0)), quarterTurn));
    QVERIFY(closeReal(KisAlgebra2D::angleBetweenVectors(QPointF(0.0, 1.0), QPointF(1.0, 0.0)), -quarterTurn));
    QVERIFY(closeReal(KisAlgebra2D::directionBetweenPoints(QPointF(2.0, 3.0), QPointF(2.0, 8.0), -0.75),
                      quarterTurn,
                      1e-6));
    QCOMPARE(KisAlgebra2D::directionBetweenPoints(QPointF(2.0, 3.0), QPointF(2.0, 3.0), -0.75), -0.75);
}

void KisAlgebraGeometryPrimitivesContractTest::signHelpersDefineZeroAndCopiedSign()
{
    const qreal positiveZero = 0.0;
    const qreal negativeZero = -0.0;

    QCOMPARE(KisAlgebra2D::signPZ(positiveZero), 1.0);
    QCOMPARE(KisAlgebra2D::signPZ(negativeZero), 1.0);
    QCOMPARE(KisAlgebra2D::signPZ(-2.0), -1.0);

    const qreal zeroSign = KisAlgebra2D::signZZ(negativeZero);
    QCOMPARE(zeroSign, 0.0);
    QVERIFY(!std::signbit(zeroSign));
    QCOMPARE(KisAlgebra2D::signZZ(2.0), 1.0);
    QCOMPARE(KisAlgebra2D::signZZ(-2.0), -1.0);

    QCOMPARE(KisAlgebra2D::copysign(-3.0, positiveZero), 3.0);
    QCOMPARE(KisAlgebra2D::copysign(-3.0, negativeZero), 3.0);
    QCOMPARE(KisAlgebra2D::copysign(3.0, -2.0), -3.0);
    const qreal negativeFromPositiveZero = KisAlgebra2D::copysign(positiveZero, -2.0);
    QCOMPARE(negativeFromPositiveZero, 0.0);
    QVERIFY(std::signbit(negativeFromPositiveZero));
    const qreal positiveFromNegativeZero = KisAlgebra2D::copysign(negativeZero, -2.0);
    QCOMPARE(positiveFromNegativeZero, 0.0);
    QVERIFY(!std::signbit(positiveFromNegativeZero));
}

void KisAlgebraGeometryPrimitivesContractTest::interpolationAndRoundingPreserveScalarAndPointValues()
{
    QCOMPARE(KisAlgebra2D::lerp(QPointF(2.0, -4.0), QPointF(10.0, 8.0), 0.25), QPointF(4.0, -1.0));
    QCOMPARE(KisAlgebra2D::linearReshapeFunc(4.0, 2.0, 6.0, 10.0, 30.0), 20.0);
    QCOMPARE(KisAlgebra2D::lazyRound<int>(2.49), 2);
    QCOMPARE(KisAlgebra2D::lazyRound<int>(-2.5), -3);
    QCOMPARE(KisAlgebra2D::lazyRound<qreal>(2.49), 2.49);
}

void KisAlgebraGeometryPrimitivesContractTest::floorDivisionAndRangeHelpersHandleBoundaries()
{
    QCOMPARE(KisAlgebra2D::divideFloor(7, 3), 2);
    QCOMPARE(KisAlgebra2D::divideFloor(-7, 3), -3);
    QCOMPARE(KisAlgebra2D::divideFloor(7, -3), -3);
    QCOMPARE(KisAlgebra2D::divideFloor(-7, -3), 2);
    QVERIFY(KisAlgebra2D::isInRange(5, 2, 5));
    QVERIFY(KisAlgebra2D::isInRange(3, 5, 2));
    QVERIFY(!KisAlgebra2D::isInRange(6, 2, 5));
    QCOMPARE(KisAlgebra2D::clampPoint(QPoint(-4, 20), QRect(1, 2, 7, 9)), QPoint(1, 10));
}

void KisAlgebraGeometryPrimitivesContractTest::wrappingNormalizesIntegralFloatingAndShiftedRanges()
{
    QCOMPARE(KisAlgebra2D::wrapValue(-1, 8), 7);
    QCOMPARE(KisAlgebra2D::wrapValue(17, 8), 1);
    QVERIFY(closeReal(KisAlgebra2D::wrapValue(-0.5, 2.0), 1.5));
    QCOMPARE(KisAlgebra2D::wrapValue(12, 3, 8), 7);
    QCOMPARE(KisAlgebra2D::wrapValue(2, 3, 8), 7);
}

void KisAlgebraGeometryPrimitivesContractTest::rectangleAndDimensionHelpersPreserveGeometry()
{
    QCOMPARE(KisAlgebra2D::createRectFromCorners(QPoint(8, 3), QPoint(2, 11)), QRect(2, 3, 6, 8));
    QCOMPARE(KisAlgebra2D::createRectFromCorners(QLineF(QPointF(8.0, 3.0), QPointF(2.0, 11.0))),
             QRectF(2.0, 3.0, 6.0, 8.0));
    QCOMPARE(KisAlgebra2D::blowRect(QRectF(10.0, 20.0, 4.0, 6.0), 0.5), QRectF(8.0, 17.0, 8.0, 12.0));
    QCOMPARE(KisAlgebra2D::ensureSizeNotSmaller(QSize(-2, 9), QSize(5, 7)), QSize(-5, 9));
    QCOMPARE(KisAlgebra2D::maxDimension(QSize(4, 9)), 9);
    QCOMPARE(KisAlgebra2D::minDimension(QSize(4, 9)), 4);
}

void KisAlgebraGeometryPrimitivesContractTest::relativeAndAbsoluteConversionsRoundTripValues()
{
    const QRectF bounds(10.0, 20.0, 40.0, 20.0);
    const QPointF relativePoint(0.25, 0.75);
    const QRectF relativeRect(0.1, 0.2, 0.4, 0.5);
    const qreal relativeValue = 0.5;

    const QPointF absolutePoint = KisAlgebra2D::relativeToAbsolute(relativePoint, bounds);
    QCOMPARE(absolutePoint, QPointF(20.0, 35.0));
    QVERIFY(closePoint(KisAlgebra2D::absoluteToRelative(absolutePoint, bounds), relativePoint));

    const QRectF absoluteRect = KisAlgebra2D::relativeToAbsolute(relativeRect, bounds);
    QCOMPARE(absoluteRect, QRectF(14.0, 24.0, 16.0, 10.0));
    QCOMPARE(KisAlgebra2D::absoluteToRelative(absoluteRect, bounds), relativeRect);

    const qreal absoluteValue = KisAlgebra2D::relativeToAbsolute(relativeValue, bounds);
    QVERIFY(closeReal(absoluteValue, 5.0 * std::sqrt(10.0)));
    QVERIFY(closeReal(KisAlgebra2D::absoluteToRelative(absoluteValue, bounds), relativeValue));
}

QTEST_GUILESS_MAIN(KisAlgebraGeometryPrimitivesContractTest)

#include "KisAlgebraGeometryPrimitivesContractTest.moc"
