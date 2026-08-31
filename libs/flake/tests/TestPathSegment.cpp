/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008-2009 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "TestPathSegment.h"

#include <KisHandlePainterHelper.h>
#include <KoPathPoint.h>
#include <KoPathSegment.h>
#include <KoShape.h>

#include <QTest>
#include <QTransform>

void KoShape::notifyChanged()
{
    qFatal("KoShape::notifyChanged must not be used by this detached contract");
}

QRectF KoShape::shapeToDocument(const QRectF &) const
{
    qFatal("KoShape::shapeToDocument must not be used by this detached contract");
    return {};
}

void KisHandlePainterHelper::drawConnectionLine(const QPointF &, const QPointF &)
{
    qFatal("Handle painting must not be used by this path-segment contract");
}

void KisHandlePainterHelper::drawGradientHandle(const QPointF &)
{
    qFatal("Handle painting must not be used by this path-segment contract");
}

void KisHandlePainterHelper::drawHandleCircle(const QPointF &)
{
    qFatal("Handle painting must not be used by this path-segment contract");
}

void KisHandlePainterHelper::drawHandleRect(const QPointF &)
{
    qFatal("Handle painting must not be used by this path-segment contract");
}

void KisHandlePainterHelper::drawHandleSmallCircle(const QPointF &)
{
    qFatal("Handle painting must not be used by this path-segment contract");
}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    qFatal("Unexpected Bezier assertion in path-segment contract");
}

namespace KisAlgebra2D
{
bool fuzzyPointCompare(const QPointF &, const QPointF &, qreal)
{
    qFatal("Unexpected algebra comparison in path-segment contract");
    return false;
}

boost::optional<QPointF> intersectLines(const QPointF &, const QPointF &, const QPointF &, const QPointF &)
{
    qFatal("Unexpected algebra intersection in path-segment contract");
    return boost::none;
}
} // namespace KisAlgebra2D

namespace
{
bool closeScalar(qreal actual, qreal expected, qreal tolerance = 1e-6)
{
    return qAbs(actual - expected) <= tolerance;
}

bool closePoint(const QPointF &actual, const QPointF &expected, qreal tolerance = 1e-6)
{
    return closeScalar(actual.x(), expected.x(), tolerance) && closeScalar(actual.y(), expected.y(), tolerance);
}
} // namespace

void TestPathSegment::constructsCopiesAssignsAndOwnsDetachedEndpoints()
{
    KoPathSegment invalid;
    QVERIFY(!invalid.isValid());
    QCOMPARE(invalid.first(), nullptr);
    QCOMPARE(invalid.second(), nullptr);
    QCOMPARE(invalid.degree(), -1);

    KoPathSegment line(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    KoPathSegment quadratic(QPointF(0.0, 0.0), QPointF(5.0, 10.0), QPointF(10.0, 0.0));
    KoPathSegment cubic(QPointF(0.0, 0.0), QPointF(0.0, 10.0), QPointF(10.0, 10.0), QPointF(10.0, 0.0));
    QVERIFY(line.isValid());
    QCOMPARE(line.degree(), 1);
    QCOMPARE(quadratic.degree(), 2);
    QCOMPARE(cubic.degree(), 3);

    auto *first = new KoPathPoint(nullptr, QPointF(1.0, 2.0));
    auto *second = new KoPathPoint(nullptr, QPointF(3.0, 4.0));
    KoPathSegment owned(first, second);
    QCOMPARE(owned.first(), first);
    QCOMPARE(owned.second(), second);

    KoPathSegment copied(owned);
    QVERIFY(copied == owned);
    QVERIFY(copied.first() != owned.first());
    QVERIFY(copied.second() != owned.second());

    KoPathSegment assigned;
    assigned = owned;
    QVERIFY(assigned == owned);
    QVERIFY(assigned.first() != owned.first());
    QVERIFY(assigned.second() != owned.second());

    assigned.setFirst(new KoPathPoint(nullptr, QPointF(-1.0, -2.0)));
    assigned.setSecond(new KoPathPoint(nullptr, QPointF(-3.0, -4.0)));
    QCOMPARE(assigned.first()->point(), QPointF(-1.0, -2.0));
    QCOMPARE(assigned.second()->point(), QPointF(-3.0, -4.0));
    QVERIFY(!(assigned == owned));

    auto *const parentToken = reinterpret_cast<KoPathShape *>(quintptr(0x1));
    KoPathPoint borrowedFirst(nullptr, QPointF(20.0, 30.0));
    KoPathPoint borrowedSecond(nullptr, QPointF(40.0, 50.0));
    borrowedFirst.setParent(parentToken);
    borrowedSecond.setParent(parentToken);
    {
        KoPathSegment borrowed(&borrowedFirst, &borrowedSecond);
        KoPathSegment borrowedCopy(borrowed);
        QCOMPARE(borrowedCopy.first(), &borrowedFirst);
        QCOMPARE(borrowedCopy.second(), &borrowedSecond);

        KoPathSegment borrowedAssignment;
        borrowedAssignment = borrowed;
        QCOMPARE(borrowedAssignment.first(), &borrowedFirst);
        QCOMPARE(borrowedAssignment.second(), &borrowedSecond);
    }
    borrowedFirst.setParent(nullptr);
    borrowedSecond.setParent(nullptr);
}

void TestPathSegment::evaluatesStableLineQuadraticAndCubicGeometry()
{
    KoPathSegment line(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    QCOMPARE(line.pointAt(0.0), QPointF(0.0, 0.0));
    QCOMPARE(line.pointAt(0.5), QPointF(5.0, 0.0));
    QCOMPARE(line.pointAt(1.0), QPointF(10.0, 0.0));
    QCOMPARE(line.controlPoints(), QList<QPointF>({QPointF(0.0, 0.0), QPointF(10.0, 0.0)}));
    QCOMPARE(line.boundingRect(), QRectF(0.0, 0.0, 10.0, 0.1));
    QCOMPARE(line.controlPointRect(), QRectF(0.0, 0.0, 10.0, 0.1));
    QCOMPARE(line.convexHull(), QList<QPointF>({QPointF(0.0, 0.0), QPointF(10.0, 0.0)}));

    KoPathSegment quadratic(QPointF(0.0, 0.0), QPointF(5.0, 10.0), QPointF(10.0, 0.0));
    QCOMPARE(quadratic.pointAt(0.5), QPointF(5.0, 5.0));
    QCOMPARE(quadratic.controlPoints(), QList<QPointF>({QPointF(0.0, 0.0), QPointF(5.0, 10.0), QPointF(10.0, 0.0)}));
    QCOMPARE(quadratic.boundingRect(), QRectF(0.0, 0.0, 10.0, 5.0));
    QCOMPARE(quadratic.controlPointRect(), QRectF(0.0, 0.0, 10.0, 10.0));
    QCOMPARE(quadratic.convexHull(), QList<QPointF>({QPointF(0.0, 0.0), QPointF(5.0, 10.0), QPointF(10.0, 0.0)}));

    KoPathSegment cubic(QPointF(0.0, 0.0), QPointF(0.0, 10.0), QPointF(10.0, 10.0), QPointF(10.0, 0.0));
    QCOMPARE(cubic.pointAt(0.5), QPointF(5.0, 7.5));
    QCOMPARE(cubic.controlPoints(),
             QList<QPointF>({QPointF(0.0, 0.0), QPointF(0.0, 10.0), QPointF(10.0, 10.0), QPointF(10.0, 0.0)}));
    QCOMPARE(cubic.boundingRect(), QRectF(0.0, 0.0, 10.0, 7.5));
    QCOMPARE(cubic.controlPointRect(), QRectF(0.0, 0.0, 10.0, 10.0));
    QCOMPARE(cubic.convexHull(),
             QList<QPointF>({QPointF(0.0, 0.0), QPointF(0.0, 10.0), QPointF(10.0, 10.0), QPointF(10.0, 0.0)}));
}

void TestPathSegment::transformsConvertsSplitsAndInterpolatesWithoutMutatingSource()
{
    KoPathSegment source(QPointF(0.0, 0.0), QPointF(0.0, 10.0), QPointF(10.0, 10.0), QPointF(10.0, 0.0));
    const KoPathSegment original(source);

    KoPathSegment mapped = source.mapped(QTransform::fromTranslate(4.0, -2.0));
    QCOMPARE(mapped.controlPoints(),
             QList<QPointF>({QPointF(4.0, -2.0), QPointF(4.0, 8.0), QPointF(14.0, 8.0), QPointF(14.0, -2.0)}));
    QVERIFY(source == original);

    KoPathSegment line(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    KoPathSegment cubicLine = line.toCubic();
    QCOMPARE(cubicLine.degree(), 3);
    QCOMPARE(cubicLine.controlPoints(),
             QList<QPointF>({QPointF(0.0, 0.0), QPointF(3.0, 0.0), QPointF(7.0, 0.0), QPointF(10.0, 0.0)}));
    QCOMPARE(line.degree(), 1);

    const auto halves = source.splitAt(0.5);
    QCOMPARE(halves.first.first()->point(), source.first()->point());
    QCOMPARE(halves.first.second()->point(), source.pointAt(0.5));
    QCOMPARE(halves.second.first()->point(), source.pointAt(0.5));
    QCOMPARE(halves.second.second()->point(), source.second()->point());
    QCOMPARE(halves.first.degree(), 3);
    QCOMPARE(halves.second.degree(), 3);
    QVERIFY(source == original);

    KoPathSegment interpolated =
        KoPathSegment::interpolate(QPointF(0.0, 0.0), QPointF(5.0, 5.0), QPointF(10.0, 0.0), 0.5);
    QCOMPARE(interpolated.degree(), 2);
    QCOMPARE(interpolated.controlPoints(), QList<QPointF>({QPointF(0.0, 0.0), QPointF(5.0, 10.0), QPointF(10.0, 0.0)}));
    QCOMPARE(interpolated.pointAt(0.5), QPointF(5.0, 5.0));
    QVERIFY(!KoPathSegment::interpolate(QPointF(), QPointF(1.0, 1.0), QPointF(2.0, 0.0), 0.0).isValid());
}

void TestPathSegment::measuresLengthAndMapsArcParameters()
{
    KoPathSegment line(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    QCOMPARE(line.length(), 10.0);
    QCOMPARE(line.length(0.001), 10.0);
    QCOMPARE(line.lengthAt(0.0), 0.0);
    QCOMPARE(line.lengthAt(0.25), 2.5);
    QCOMPARE(line.lengthAt(1.0, 0.001), 10.0);
    QCOMPARE(line.paramAtLength(-1.0), 0.0);
    QCOMPARE(line.paramAtLength(2.5), 0.25);
    QCOMPARE(line.paramAtLength(12.0), 1.0);

    KoPathSegment curve(QPointF(0.0, 0.0), QPointF(5.0, 10.0), QPointF(10.0, 0.0));
    const qreal totalLength = curve.length();
    QVERIFY(totalLength > 10.0);
    QVERIFY(totalLength < 20.0);
    QVERIFY(closeScalar(curve.lengthAt(0.5), totalLength * 0.5, 0.01));
    QVERIFY(closeScalar(curve.paramAtLength(totalLength * 0.5), 0.5, 0.001));
}

void TestPathSegment::findsIntersectionsAndNearestParameters()
{
    KoPathSegment horizontal(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    KoPathSegment vertical(QPointF(5.0, -5.0), QPointF(5.0, 5.0));
    QCOMPARE(horizontal.intersections(vertical), QList<QPointF>({QPointF(5.0, 0.0)}));
    QVERIFY(horizontal.intersections(KoPathSegment(QPointF(0.0, 2.0), QPointF(10.0, 2.0))).isEmpty());

    KoPathSegment firstCurve(QPointF(0.0, 0.0), QPointF(2.5, 5.0), QPointF(7.5, 5.0), QPointF(10.0, 0.0));
    KoPathSegment secondCurve(QPointF(0.0, 3.0), QPointF(2.5, -2.0), QPointF(7.5, -2.0), QPointF(10.0, 3.0));
    QCOMPARE(firstCurve.intersections(secondCurve).size(), 2);

    QCOMPARE(horizontal.nearestPoint(QPointF(-2.0, 0.0)), 0.0);
    QCOMPARE(horizontal.nearestPoint(QPointF(2.5, 4.0)), 0.25);
    QCOMPARE(horizontal.nearestPoint(QPointF(12.0, 0.0)), 1.0);

    KoPathSegment quadratic(QPointF(0.0, 0.0), QPointF(5.0, 5.0), QPointF(10.0, 0.0));
    QVERIFY(closeScalar(quadratic.nearestPoint(quadratic.pointAt(0.25)), 0.25));
    QVERIFY(closeScalar(quadratic.nearestPoint(QPointF(5.0, 5.0)), 0.5));

    KoPathSegment cubic(QPointF(0.0, 0.0), QPointF(2.5, 5.0), QPointF(7.5, 5.0), QPointF(10.0, 0.0));
    QVERIFY(closeScalar(cubic.nearestPoint(cubic.pointAt(0.75)), 0.75));
}

void TestPathSegment::classifiesFlatnessAndReportsNormalizedDirection()
{
    KoPathSegment line(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    QVERIFY(line.isFlat());

    KoPathSegment flatQuadratic(QPointF(0.0, 0.0), QPointF(5.0, 0.0), QPointF(10.0, 0.0));
    KoPathSegment curvedQuadratic(QPointF(0.0, 0.0), QPointF(5.0, 5.0), QPointF(10.0, 0.0));
    QVERIFY(flatQuadratic.isFlat());
    QVERIFY(!curvedQuadratic.isFlat());
    QVERIFY(curvedQuadratic.isFlat(5.0));

    KoPathSegment flatCubic(QPointF(0.0, 0.0), QPointF(2.5, 0.0), QPointF(7.5, 0.0), QPointF(10.0, 0.0));
    KoPathSegment curvedCubic(QPointF(0.0, 0.0), QPointF(2.5, 5.0), QPointF(7.5, 5.0), QPointF(10.0, 0.0));
    QVERIFY(flatCubic.isFlat());
    QVERIFY(!curvedCubic.isFlat());
    QVERIFY(closePoint(flatCubic.angleVectorAtParam(0.0), QPointF(1.0, 0.0)));
    QVERIFY(closePoint(flatCubic.angleVectorAtParam(0.5), QPointF(1.0, 0.0)));
    QVERIFY(closePoint(flatCubic.angleVectorAtParam(1.0), QPointF(1.0, 0.0)));
}

QTEST_GUILESS_MAIN(TestPathSegment)
