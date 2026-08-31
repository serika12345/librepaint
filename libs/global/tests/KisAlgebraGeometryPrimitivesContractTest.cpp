/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace
{

bool closeReal(qreal actual, qreal expected, qreal tolerance = 1e-12)
{
    return qAbs(actual - expected) < tolerance;
}

bool closePoint(const QPointF &actual, const QPointF &expected, qreal tolerance = 1e-12)
{
    return closeReal(actual.x(), expected.x(), tolerance) && closeReal(actual.y(), expected.y(), tolerance);
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
    void vectorPathPointTypesAndDefaultsRemainStable();
    void vectorPathPointConstructionOwnsGeometry();
    void vectorPathPointFactoriesAssignTypesAndControls();
    void segmentsProjectEndpointGeometryAndOwnValues();
    void rectanglePointTraitsAndBoundsAccumulateExtrema();
    void rectangleSizingAndClampingPreserveCoordinateSemantics();
    void rectangleSamplingAndApproximationCaptureExtrema();
    void rectangleClippingAndUnitMappingsPreserveArea();
    void rectangleComparisonsRecognizeToleranceAndPixelAlignment();
    void lineClippingAndIntersectionRespectExtentPolicies();
    void lineProjectionAndMovementPreserveMetricGeometry();
    void triangleConstructionAndElasticMovementPreserveConstraints();
    void polygonPredicatesAndPointComparisonsRespectTolerance();
    void scalarMinimizersConvergeWithinBounds();
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

void KisAlgebraGeometryPrimitivesContractTest::vectorPathPointTypesAndDefaultsRemainStable()
{
    using Point = KisAlgebra2D::VectorPath::VectorPathPoint;
    using Type = Point::Type;

    QCOMPARE(int(Point::MoveTo), 0);
    QCOMPARE(int(Point::LineTo), 1);
    QCOMPARE(int(Point::BezierTo), 2);

    const Point point;
    QCOMPARE(int(point.type), int(Type::MoveTo));
    QCOMPARE(point.endPoint, QPointF());
    QCOMPARE(point.controlPoint1, QPointF());
    QCOMPARE(point.controlPoint2, QPointF());
}

void KisAlgebraGeometryPrimitivesContractTest::vectorPathPointConstructionOwnsGeometry()
{
    using Point = KisAlgebra2D::VectorPath::VectorPathPoint;
    QPointF end(8.0, 5.0);
    QPointF control1(2.0, 3.0);
    QPointF control2(6.0, 7.0);
    const Point point(Point::BezierTo, end, control1, control2);

    end = QPointF(-1.0, -1.0);
    control1 = QPointF(-2.0, -2.0);
    control2 = QPointF(-3.0, -3.0);

    QCOMPARE(int(point.type), int(Point::BezierTo));
    QCOMPARE(point.endPoint, QPointF(8.0, 5.0));
    QCOMPARE(point.controlPoint1, QPointF(2.0, 3.0));
    QCOMPARE(point.controlPoint2, QPointF(6.0, 7.0));
}

void KisAlgebraGeometryPrimitivesContractTest::vectorPathPointFactoriesAssignTypesAndControls()
{
    using Point = KisAlgebra2D::VectorPath::VectorPathPoint;

    const Point move = Point::moveTo(QPointF(1.0, 2.0));
    QCOMPARE(int(move.type), int(Point::MoveTo));
    QCOMPARE(move.endPoint, QPointF(1.0, 2.0));
    QCOMPARE(move.controlPoint1, QPointF());
    QCOMPARE(move.controlPoint2, QPointF());

    const Point line = Point::lineTo(QPointF(3.0, 4.0));
    QCOMPARE(int(line.type), int(Point::LineTo));
    QCOMPARE(line.endPoint, QPointF(3.0, 4.0));
    QCOMPARE(line.controlPoint1, QPointF());
    QCOMPARE(line.controlPoint2, QPointF());

    const Point bezier = Point::bezierTo(QPointF(9.0, 8.0), QPointF(4.0, 5.0), QPointF(6.0, 7.0));
    QCOMPARE(int(bezier.type), int(Point::BezierTo));
    QCOMPARE(bezier.endPoint, QPointF(9.0, 8.0));
    QCOMPARE(bezier.controlPoint1, QPointF(4.0, 5.0));
    QCOMPARE(bezier.controlPoint2, QPointF(6.0, 7.0));
}

void KisAlgebraGeometryPrimitivesContractTest::segmentsProjectEndpointGeometryAndOwnValues()
{
    using Point = KisAlgebra2D::VectorPath::VectorPathPoint;
    using Segment = KisAlgebra2D::VectorPath::Segment;

    Point start = Point::moveTo(QPointF(1.0, 2.0));
    Point end = Point::bezierTo(QPointF(9.0, 8.0), QPointF(4.0, 5.0), QPointF(6.0, 7.0));
    const Segment segment(start, end);

    start.endPoint = QPointF(-1.0, -2.0);
    end.endPoint = QPointF(-9.0, -8.0);

    QCOMPARE(int(segment.start.type), int(Point::MoveTo));
    QCOMPARE(segment.start.endPoint, QPointF(1.0, 2.0));
    QCOMPARE(int(segment.end.type), int(Point::BezierTo));
    QCOMPARE(segment.end.endPoint, QPointF(9.0, 8.0));
    QCOMPARE(segment.startPoint, QPointF(1.0, 2.0));
    QCOMPARE(segment.endPoint, QPointF(9.0, 8.0));
    QCOMPARE(segment.controlPoint1, QPointF(4.0, 5.0));
    QCOMPARE(segment.controlPoint2, QPointF(6.0, 7.0));
    QCOMPARE(int(segment.type), int(Point::BezierTo));
}

void KisAlgebraGeometryPrimitivesContractTest::rectanglePointTraitsAndBoundsAccumulateExtrema()
{
    static_assert(std::is_same_v<KisAlgebra2D::PointTypeTraits<QPoint>::value_type, int>);
    static_assert(std::is_same_v<KisAlgebra2D::PointTypeTraits<QPoint>::calculation_type, qreal>);
    static_assert(std::is_same_v<KisAlgebra2D::PointTypeTraits<QPoint>::rect_type, QRect>);
    static_assert(std::is_same_v<KisAlgebra2D::PointTypeTraits<QPointF>::value_type, qreal>);
    static_assert(std::is_same_v<KisAlgebra2D::PointTypeTraits<QPointF>::calculation_type, qreal>);
    static_assert(std::is_same_v<KisAlgebra2D::PointTypeTraits<QPointF>::rect_type, QRectF>);

    QRect integerBounds;
    KisAlgebra2D::Private::resetEmptyRectangle(QPoint(3, -2), &integerBounds);
    QCOMPARE(integerBounds, QRect(3, -2, 1, 1));
    KisAlgebra2D::accumulateBounds(QPoint(-4, 8), &integerBounds);
    QCOMPARE(integerBounds, QRect(QPoint(-4, -2), QPoint(3, 8)));
    KisAlgebra2D::accumulateBoundsNonEmpty(QPoint(9, -6), &integerBounds);
    QCOMPARE(integerBounds, QRect(QPoint(-4, -6), QPoint(9, 8)));

    QRectF floatingBounds;
    KisAlgebra2D::Private::resetEmptyRectangle(QPointF(1.5, -2.5), &floatingBounds);
    QCOMPARE(floatingBounds.topLeft(), QPointF(1.5, -2.5));
    QCOMPARE(floatingBounds.size(), QSizeF(1e-10, 1e-10));

    const QVector<QPointF> points {
        QPointF(-2.5, 4.0),
        QPointF(7.0, -3.0),
        QPointF(1.0, 8.0),
    };
    KisAlgebra2D::accumulateBounds(points, &floatingBounds);
    QCOMPARE(floatingBounds, QRectF(QPointF(-2.5, -3.0), QPointF(7.0, 8.0)));
    QCOMPARE(KisAlgebra2D::accumulateBounds(points),
             QRectF(QPointF(-2.5, -3.0), QPointF(7.0, 8.0)));
}

void KisAlgebraGeometryPrimitivesContractTest::rectangleSizingAndClampingPreserveCoordinateSemantics()
{
    QCOMPARE(KisAlgebra2D::ensureRectNotSmaller(QRectF(3.0, 4.0, 2.0, 5.0), QSizeF(7.0, 3.0)),
             QRectF(3.0, 4.0, 7.0, 5.0));
    QCOMPARE(KisAlgebra2D::ensureRectNotSmaller(QRect(3, 4, 9, 8), QSize(7, 3)),
             QRect(3, 4, 9, 8));

    const QRect integerBounds(10, 20, 5, 7);
    QCOMPARE(KisAlgebra2D::ensureInRect(QPoint(6, 30), integerBounds), QPoint(10, 26));
    QCOMPARE(KisAlgebra2D::ensureInRect(QPoint(12, 23), integerBounds), QPoint(12, 23));

    const QRectF floatingBounds(10.0, 20.0, 5.0, 7.0);
    QCOMPARE(KisAlgebra2D::ensureInRect(QPointF(6.0, 30.0), floatingBounds), QPointF(10.0, 27.0));
    QCOMPARE(KisAlgebra2D::ensureInRect(QPointF(12.5, 23.5), floatingBounds), QPointF(12.5, 23.5));
}

void KisAlgebraGeometryPrimitivesContractTest::rectangleSamplingAndApproximationCaptureExtrema()
{
    const QVector<QPoint> expectedIntegerSamples {
        QPoint(0, 0), QPoint(4, 0), QPoint(8, 0),
        QPoint(0, 2), QPoint(4, 2), QPoint(8, 2),
        QPoint(0, 4), QPoint(4, 4), QPoint(8, 4),
    };
    QCOMPARE(KisAlgebra2D::sampleRectWithPoints(QRect(0, 0, 9, 5)), expectedIntegerSamples);

    const QVector<QPointF> expectedFloatingSamples {
        QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(8.0, 0.0),
        QPointF(0.0, 2.0), QPointF(4.0, 2.0), QPointF(8.0, 2.0),
        QPointF(0.0, 4.0), QPointF(4.0, 4.0), QPointF(8.0, 4.0),
    };
    QCOMPARE(KisAlgebra2D::sampleRectWithPoints(QRectF(0.0, 0.0, 8.0, 4.0)), expectedFloatingSamples);

    const QVector<QPoint> integerPoints {QPoint(-2, -3), QPoint(4, 6), QPoint(1, 2)};
    QCOMPARE(KisAlgebra2D::approximateRectFromPoints(integerPoints),
             QRect(QPoint(-2, -3), QPoint(4, 6)));

    const QVector<QPointF> floatingPoints {
        QPointF(-2.5, -3.25), QPointF(4.75, 6.5), QPointF(1.0, 2.0),
    };
    QCOMPARE(KisAlgebra2D::approximateRectFromPoints(floatingPoints),
             QRectF(QPointF(-2.5, -3.25), QPointF(4.75, 6.5)));

    const QRect transformedBounds = KisAlgebra2D::approximateRectWithPointTransform(
        QRect(0, 0, 9, 5),
        [] (QPointF point) {
            return point == QPointF(4.0, 2.0) ? QPointF(-20.0, 30.0) : point;
        });
    QCOMPARE(transformedBounds, QRect(QPoint(-20, 0), QPoint(8, 30)));
}

void KisAlgebraGeometryPrimitivesContractTest::rectangleClippingAndUnitMappingsPreserveArea()
{
    const QRectF rectangle(0.0, 0.0, 10.0, 10.0);
    const KisAlgebra2D::RightHalfPlane rightOfCenter(QPointF(5.0, 10.0), QPointF(5.0, 0.0));
    QCOMPARE(KisAlgebra2D::cutOffRect(rectangle, rightOfCenter), QRectF(5.0, 0.0, 5.0, 10.0));

    const QRectF destination(10.0, 20.0, 40.0, 30.0);
    const QTransform toDestination = KisAlgebra2D::mapToRect(destination);
    QCOMPARE(toDestination.map(QPointF(0.0, 0.0)), destination.topLeft());
    QCOMPARE(toDestination.map(QPointF(1.0, 1.0)), destination.bottomRight());

    const QTransform fromDestination = KisAlgebra2D::mapToRectInverse(destination);
    QVERIFY(closePoint(fromDestination.map(destination.topLeft()), QPointF(0.0, 0.0)));
    QVERIFY(closePoint(fromDestination.map(destination.bottomRight()), QPointF(1.0, 1.0)));
    QCOMPARE(KisAlgebra2D::mapToRectInverse(QRectF(4.0, 6.0, 0.0, 2.0)).map(QPointF(4.0, 8.0)),
             QPointF(0.0, 1.0));
}

void KisAlgebraGeometryPrimitivesContractTest::rectangleComparisonsRecognizeToleranceAndPixelAlignment()
{
    QCOMPARE(KisAlgebra2D::abs(QPointF(-2.5, 3.0)), QPointF(2.5, 3.0));

    const QRectF reference(1.0, 2.0, 8.0, 6.0);
    QVERIFY(KisAlgebra2D::fuzzyCompareRects(reference, reference.translated(0.05, -0.05), 0.1));
    QVERIFY(!KisAlgebra2D::fuzzyCompareRects(reference, reference.translated(0.1, 0.0), 0.1));

    const QTransform matrix(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    const QTransform closeMatrix(1.05, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    const QTransform farMatrix(1.1, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    QVERIFY(KisAlgebra2D::fuzzyMatrixCompare(matrix, closeMatrix, 0.1));
    QVERIFY(!KisAlgebra2D::fuzzyMatrixCompare(matrix, farMatrix, 0.1));

    const QPolygonF rectangle {
        QPointF(1.0, 2.0), QPointF(1.0, 8.0),
        QPointF(9.0, 8.0), QPointF(9.0, 2.0),
    };
    QVERIFY(KisAlgebra2D::isPolygonRect(rectangle, 1e-12));
    QPolygonF closedRectangle = rectangle;
    closedRectangle << rectangle.first();
    QVERIFY(KisAlgebra2D::isPolygonRect(closedRectangle, 1e-12));
    QVERIFY(KisAlgebra2D::isPolygonPixelAlignedRect(rectangle, 1e-12));

    const QPolygonF fractionalRectangle {
        QPointF(1.25, 2.0), QPointF(1.25, 8.0),
        QPointF(9.25, 8.0), QPointF(9.25, 2.0),
    };
    QVERIFY(KisAlgebra2D::isPolygonRect(fractionalRectangle, 1e-12));
    QVERIFY(!KisAlgebra2D::isPolygonPixelAlignedRect(fractionalRectangle, 1e-12));
}

void KisAlgebraGeometryPrimitivesContractTest::lineClippingAndIntersectionRespectExtentPolicies()
{
    const QRect bounds(0, 0, 10, 10);
    QLineF crossingLine(QPointF(-5.0, 5.0), QPointF(15.0, 5.0));
    QVERIFY(KisAlgebra2D::intersectLineRect(crossingLine, bounds, false));
    QCOMPARE(crossingLine, QLineF(QPointF(0.0, 5.0), QPointF(10.0, 5.0)));

    QLineF firstEndExtended(QPointF(2.0, 5.0), QPointF(8.0, 5.0));
    QVERIFY(KisAlgebra2D::intersectLineRect(firstEndExtended, bounds, true, false));
    QVERIFY(closePoint(firstEndExtended.p1(), QPointF(0.0, 5.0), 1e-6));
    QCOMPARE(firstEndExtended.p2(), QPointF(8.0, 5.0));

    QLineF lineOutsideBounds(QPointF(-5.0, -2.0), QPointF(15.0, -2.0));
    QVERIFY(!KisAlgebra2D::intersectLineRect(lineOutsideBounds, bounds, false));
    KisAlgebra2D::cropLineToRect(lineOutsideBounds, bounds, false, false);
    QCOMPARE(lineOutsideBounds, QLineF());

    const QPolygonF polygon {
        QPointF(0.0, 0.0), QPointF(10.0, 0.0),
        QPointF(10.0, 10.0), QPointF(0.0, 10.0),
    };
    QLineF crossingPolygon(QPointF(-5.0, 5.0), QPointF(15.0, 5.0));
    QVERIFY(KisAlgebra2D::intersectLineConvexPolygon(crossingPolygon, polygon, false, false));
    QCOMPARE(crossingPolygon, QLineF(QPointF(0.0, 5.0), QPointF(10.0, 5.0)));

    QLineF lineOutsidePolygon(QPointF(-5.0, -2.0), QPointF(15.0, -2.0));
    KisAlgebra2D::cropLineToConvexPolygon(lineOutsidePolygon, polygon, false, false);
    QCOMPARE(lineOutsidePolygon, QLineF());

    const QLineF boundedLine(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    const auto intersection = KisAlgebra2D::intersectLines(
        boundedLine, QLineF(QPointF(5.0, -2.0), QPointF(5.0, 2.0)));
    QVERIFY(intersection.has_value());
    QCOMPARE(*intersection, QPointF(5.0, 0.0));
    QVERIFY(!KisAlgebra2D::intersectLines(
        boundedLine, QLineF(QPointF(12.0, -2.0), QPointF(12.0, 2.0))).has_value());

    const auto pointIntersection = KisAlgebra2D::intersectLines(
        QPointF(0.0, 0.0), QPointF(10.0, 0.0),
        QPointF(3.0, -2.0), QPointF(3.0, 2.0));
    QVERIFY(pointIntersection.has_value());
    QVERIFY(closePoint(*pointIntersection, QPointF(3.0, 0.0), 1e-6));

    QVERIFY(KisAlgebra2D::isOnLine(boundedLine, QPointF(4.0, 0.0), 1e-12, true, true, false));
    QVERIFY(!KisAlgebra2D::isOnLine(boundedLine, QPointF(0.0, 0.0), 1e-12, true, true, false));
    QVERIFY(KisAlgebra2D::isOnLine(boundedLine, QPointF(0.0, 0.0), 1e-12, false, true, false));
    QVERIFY(!KisAlgebra2D::isOnLine(boundedLine, QPointF(12.0, 0.0), 1e-12, true, true, true));
    QVERIFY(KisAlgebra2D::isOnLine(boundedLine, QPointF(12.0, 0.0), 1e-12, true, false, true));
}

void KisAlgebraGeometryPrimitivesContractTest::lineProjectionAndMovementPreserveMetricGeometry()
{
    const QLineF horizontal(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    const QList<QLineF> parallelLines = KisAlgebra2D::getParallelLines(horizontal, 2.0);
    QCOMPARE(parallelLines.size(), 2);
    QCOMPARE(parallelLines[0], QLineF(QPointF(0.0, -2.0), QPointF(10.0, -2.0)));
    QCOMPARE(parallelLines[1], QLineF(QPointF(0.0, 2.0), QPointF(10.0, 2.0)));
    QVERIFY(KisAlgebra2D::getParallelLines(QLineF(QPointF(1.0, 1.0), QPointF(1.0, 1.0)), 2.0).isEmpty());

    QCOMPARE(KisAlgebra2D::findNearestPointOnLine(QPointF(3.0, 4.0), horizontal, false), QPointF(3.0, 0.0));
    QCOMPARE(KisAlgebra2D::findNearestPointOnLine(QPointF(12.0, 4.0), horizontal, false), QPointF(10.0, 0.0));
    QCOMPARE(KisAlgebra2D::findNearestPointOnLine(QPointF(12.0, 4.0), horizontal, true), QPointF(12.0, 0.0));
    QCOMPARE(KisAlgebra2D::pointToLineDistSquared(QPointF(3.0, 4.0), horizontal), 16.0);

    QCOMPARE(KisAlgebra2D::movePointInTheDirection(QPointF(1.0, 2.0), QPointF(3.0, 4.0), 10.0),
             QPointF(7.0, 10.0));
    QCOMPARE(KisAlgebra2D::movePointInTheDirection(QPointF(1.0, 2.0), QPointF(), 10.0),
             QPointF(1.0, 2.0));
    QCOMPARE(KisAlgebra2D::movePointAlongTheLine(QPointF(3.0, 4.0), horizontal, 2.0, true),
             QPointF(5.0, 0.0));
    QCOMPARE(KisAlgebra2D::movePointAlongTheLine(QPointF(3.0, 4.0), horizontal, 2.0, false),
             QPointF(5.0, 4.0));
}

void KisAlgebraGeometryPrimitivesContractTest::triangleConstructionAndElasticMovementPreserveConstraints()
{
    qreal firstRoot = 0.0;
    qreal secondRoot = 0.0;
    QCOMPARE(KisAlgebra2D::quadraticEquation(1.0, -3.0, 2.0, &firstRoot, &secondRoot), 2);
    QCOMPARE(firstRoot, 2.0);
    QCOMPARE(secondRoot, 1.0);
    QCOMPARE(KisAlgebra2D::quadraticEquation(1.0, -2.0, 1.0, &firstRoot, &secondRoot), 1);
    QCOMPARE(firstRoot, 1.0);
    QCOMPARE(KisAlgebra2D::quadraticEquation(1.0, 0.0, 1.0, &firstRoot, &secondRoot), 0);

    const QVector<QPointF> trianglePoints = KisAlgebra2D::findTrianglePoint(
        QPointF(0.0, 0.0), QPointF(4.0, 0.0), std::sqrt(5.0), std::sqrt(5.0));
    QCOMPARE(trianglePoints, QVector<QPointF>({QPointF(2.0, 1.0), QPointF(2.0, -1.0)}));
    const auto nearestTrianglePoint = KisAlgebra2D::findTrianglePointNearest(
        QPointF(0.0, 0.0), QPointF(4.0, 0.0), std::sqrt(5.0), std::sqrt(5.0), QPointF(2.0, -5.0));
    QVERIFY(nearestTrianglePoint.has_value());
    QVERIFY(closePoint(*nearestTrianglePoint, QPointF(2.0, -1.0)));

    const QPointF elasticPoint = KisAlgebra2D::moveElasticPoint(
        QPointF(0.0, 0.0), QPointF(0.0, 2.0), QPointF(0.0, 2.0),
        QPointF(-2.0, -1.0), QPointF(3.0, -1.0));
    QVERIFY(closePoint(elasticPoint, QPointF(0.0, 0.0)));

    QVERIFY(closePoint(KisAlgebra2D::transformAsBase(
                           QPointF(1.0, 0.0), QPointF(1.0, 0.0), QPointF(0.0, 2.0)),
                       QPointF(0.0, 2.0)));
    QCOMPARE(KisAlgebra2D::transformAsBase(
                 QPointF(3.0, 4.0), QPointF(1e-8, 0.0), QPointF(2.0, 0.0)),
             QPointF(3.0, 4.0));
    QCOMPARE(KisAlgebra2D::transformAsBase(
                 QPointF(3.0, 4.0), QPointF(2.0, 0.0), QPointF()),
             QPointF());
}

void KisAlgebraGeometryPrimitivesContractTest::polygonPredicatesAndPointComparisonsRespectTolerance()
{
    QVERIFY(KisAlgebra2D::fuzzyPointCompare(QPointF(2.0, 3.0), QPointF(2.0, 3.0)));
    QVERIFY(!KisAlgebra2D::fuzzyPointCompare(QPointF(2.0, 3.0), QPointF(2.1, 3.0)));
    QVERIFY(KisAlgebra2D::fuzzyPointCompare(QPointF(2.0, 3.0), QPointF(2.05, 2.95), 0.1));
    QVERIFY(!KisAlgebra2D::fuzzyPointCompare(QPointF(2.0, 3.0), QPointF(2.1, 3.0), 0.1));

    const QVector<QPointF> firstPoints {QPointF(1.0, 2.0), QPointF(3.0, 4.0)};
    const QVector<QPointF> closePoints {QPointF(1.05, 1.95), QPointF(3.0, 4.0)};
    QVERIFY(KisAlgebra2D::fuzzyPointCompare(firstPoints, closePoints, 0.1));
    QVector<QPointF> extraPoint = closePoints;
    extraPoint << QPointF(5.0, 6.0);
    QVERIFY(!KisAlgebra2D::fuzzyPointCompare(firstPoints, extraPoint, 0.1));

    const QVector<QPointF> counterClockwise {
        QPointF(0.0, 0.0), QPointF(4.0, 0.0),
        QPointF(4.0, 3.0), QPointF(0.0, 3.0),
    };
    QVector<QPointF> clockwise = counterClockwise;
    std::reverse(clockwise.begin(), clockwise.end());
    QCOMPARE(KisAlgebra2D::polygonDirection(counterClockwise), -1);
    QCOMPARE(KisAlgebra2D::polygonDirection(clockwise), 1);
    QVERIFY(KisAlgebra2D::isPolygonTrulyConvex(counterClockwise));

    const QVector<QPointF> concave {
        QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(2.0, 1.0),
        QPointF(4.0, 3.0), QPointF(0.0, 3.0),
    };
    QVERIFY(!KisAlgebra2D::isPolygonTrulyConvex(concave));

    const QVector<QPointF> repeatedLoop {
        QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(4.0, 3.0), QPointF(0.0, 3.0),
        QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(4.0, 3.0), QPointF(0.0, 3.0),
    };
    QVERIFY(!KisAlgebra2D::isPolygonTrulyConvex(repeatedLoop, true));
    QVERIFY(KisAlgebra2D::isPolygonTrulyConvex(repeatedLoop, false));
}

void KisAlgebraGeometryPrimitivesContractTest::scalarMinimizersConvergeWithinBounds()
{
    const qreal goldenMinimum = KisAlgebra2D::findMinimumGoldenSection(
        [] (qreal value) { return (value - 3.0) * (value - 3.0); },
        0.0, 10.0, 1e-7, 100);
    QVERIFY(closeReal(goldenMinimum, 3.0, 1e-4));

    const qreal ternaryMinimum = KisAlgebra2D::findMinimumTernarySection(
        [] (qreal value) { return (value + 2.0) * (value + 2.0); },
        5.0, -10.0, 1e-7, 100);
    QVERIFY(closeReal(ternaryMinimum, -2.0, 1e-4));

    const qreal iterationLimited = KisAlgebra2D::findMinimumGoldenSection(
        [] (qreal value) { return (value - 3.0) * (value - 3.0); },
        0.0, 10.0, 1e-12, 0);
    QVERIFY(iterationLimited >= 0.0);
    QVERIFY(iterationLimited <= 10.0);
}

QTEST_GUILESS_MAIN(KisAlgebraGeometryPrimitivesContractTest)

#include "KisAlgebraGeometryPrimitivesContractTest.moc"
