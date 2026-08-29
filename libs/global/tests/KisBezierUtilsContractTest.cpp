/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierUtils.h"

#include <QLineF>
#include <QTest>

#include <array>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace KisAlgebra2D
{
// Keep the contract target independent of kritaglobal while exercising the
// Bezier implementation against the same algebraic leaf behavior.
bool fuzzyPointCompare(const QPointF &p1, const QPointF &p2, qreal delta)
{
    return qAbs(p1.x() - p2.x()) < delta && qAbs(p1.y() - p2.y()) < delta;
}

boost::optional<QPointF> intersectLines(const QPointF &p1, const QPointF &p2, const QPointF &q1, const QPointF &q2)
{
    const QPointF boundedDirection = p2 - p1;
    const QPointF unboundedDirection = q2 - q1;
    const qreal determinant =
        unboundedDirection.x() * boundedDirection.y() - unboundedDirection.y() * boundedDirection.x();
    if (qFuzzyIsNull(determinant)) {
        return boost::none;
    }

    const QPointF originsDifference = p1 - q1;
    const qreal boundedParameter =
        (originsDifference.x() * unboundedDirection.y() - originsDifference.y() * unboundedDirection.x()) / determinant;
    constexpr qreal epsilon = 1e-6;
    if (boundedParameter < -epsilon || boundedParameter > 1.0 + epsilon) {
        return boost::none;
    }

    return p1 + boundedParameter * boundedDirection;
}
} // namespace KisAlgebra2D

namespace
{
bool closeScalar(qreal actual, qreal expected, qreal epsilon = 1e-6)
{
    return qAbs(actual - expected) <= epsilon;
}

bool closePoint(const QPointF &actual, const QPointF &expected, qreal epsilon = 1e-6)
{
    return closeScalar(actual.x(), expected.x(), epsilon) && closeScalar(actual.y(), expected.y(), epsilon);
}

std::array<QPointF, 12> makeLinearPatch()
{
    return {
        QPointF(0.0, 0.0),
        QPointF(40.0, 0.0),
        QPointF(0.0, 20.0),
        QPointF(120.0, 0.0),
        QPointF(80.0, 0.0),
        QPointF(120.0, 20.0),
        QPointF(0.0, 60.0),
        QPointF(40.0, 60.0),
        QPointF(0.0, 40.0),
        QPointF(120.0, 60.0),
        QPointF(80.0, 60.0),
        QPointF(120.0, 40.0),
    };
}
} // namespace

class KisBezierUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void evaluatesCurveAndDerivatives()
    {
        const QPointF p0(0.0, 0.0);
        const QPointF p1(1.0, 2.0);
        const QPointF p2(3.0, 2.0);
        const QPointF p3(4.0, 0.0);

        QVERIFY(closePoint(KisBezierUtils::bezierCurve(p0, p1, p2, p3, 0.5), QPointF(2.0, 1.5)));
        QVERIFY(closePoint(KisBezierUtils::bezierCurveDeriv(p0, p1, p2, p3, 0.5), QPointF(4.5, 0.0)));
        QVERIFY(closePoint(KisBezierUtils::bezierCurveDeriv2(p0, p1, p2, p3, 0.5), QPointF(0.0, -12.0)));

        QVERIFY(closePoint(KisBezierUtils::bezierCurve(QList<QPointF>{p0, p3}, 0.25), QPointF(1.0, 0.0)));
        QVERIFY(
            closePoint(KisBezierUtils::bezierCurve(QList<QPointF>{p0, QPointF(2.0, 2.0), p3}, 0.5), QPointF(2.0, 1.5)));
        QVERIFY(closePoint(KisBezierUtils::bezierCurve(QList<QPointF>{p0, p1, p2, p3}, 0.5), QPointF(2.0, 1.5)));
    }

    void subdividesAndClassifiesCurves()
    {
        const QPointF p0(0.0, 0.0);
        const QPointF p1(1.0, 2.0);
        const QPointF p2(3.0, 2.0);
        const QPointF p3(4.0, 0.0);
        QPointF leftControl1;
        QPointF leftControl2;
        QPointF splitPoint;
        QPointF rightControl1;
        QPointF rightControl2;

        KisBezierUtils::deCasteljau(p0,
                                    p1,
                                    p2,
                                    p3,
                                    0.5,
                                    &leftControl1,
                                    &leftControl2,
                                    &splitPoint,
                                    &rightControl1,
                                    &rightControl2);

        QVERIFY(closePoint(leftControl1, QPointF(0.5, 1.0)));
        QVERIFY(closePoint(leftControl2, QPointF(1.25, 1.5)));
        QVERIFY(closePoint(splitPoint, QPointF(2.0, 1.5)));
        QVERIFY(closePoint(rightControl1, QPointF(2.75, 1.5)));
        QVERIFY(closePoint(rightControl2, QPointF(3.5, 1.0)));

        const QPointF lineP0(0.0, 0.0);
        const QPointF lineP1(1.0, 0.0);
        const QPointF lineP2(2.0, 0.0);
        const QPointF lineP3(3.0, 0.0);
        QVERIFY(KisBezierUtils::isLinearSegmentByControlPoints(lineP0, lineP1, lineP2, lineP3));
        QVERIFY(KisBezierUtils::isLinearSegmentByDerivatives(lineP0, QPointF(3.0, 0.0), lineP3, QPointF(3.0, 0.0)));
        QVERIFY(!KisBezierUtils::isLinearSegmentByControlPoints(p0, p1, p2, p3));
        QVERIFY(!KisBezierUtils::isLinearSegmentByDerivatives(p0, QPointF(3.0, 6.0), p3, QPointF(3.0, -6.0)));

        QCOMPARE(KisBezierUtils::bezierDegree(lineP0, lineP1, lineP2, lineP3), 1);
        QCOMPARE(KisBezierUtils::bezierDegree(p0, QPointF(2.0, 2.0), QPointF(2.0, 2.0), p3), 2);
        QCOMPARE(KisBezierUtils::bezierDegree(p0, p1, p2, p3), 3);
    }

    void linearizesAndMergesOrderedSteps()
    {
        const QVector<qreal> lineSteps = KisBezierUtils::linearizeCurve(QPointF(0.0, 0.0),
                                                                        QPointF(1.0, 0.0),
                                                                        QPointF(2.0, 0.0),
                                                                        QPointF(3.0, 0.0),
                                                                        1e-4);
        QCOMPARE(lineSteps, QVector<qreal>({0.0, 1.0}));

        const QVector<qreal> curveSteps = KisBezierUtils::linearizeCurve(QPointF(0.0, 0.0),
                                                                         QPointF(0.0, 10.0),
                                                                         QPointF(10.0, 10.0),
                                                                         QPointF(10.0, 0.0),
                                                                         0.01);
        QVERIFY(curveSteps.size() > 2);
        QCOMPARE(curveSteps.first(), 0.0);
        QCOMPARE(curveSteps.last(), 1.0);
        for (qsizetype i = 1; i < curveSteps.size(); ++i) {
            QVERIFY(curveSteps[i - 1] < curveSteps[i]);
        }

        const QVector<qreal> merged = KisBezierUtils::mergeLinearizationSteps({0.0, 0.5, 1.0}, {0.25, 0.5, 0.75});
        QCOMPARE(merged, QVector<qreal>({0.0, 0.25, 0.5, 0.75, 1.0}));

        QCOMPARE(KisBezierUtils::controlPolygonZeros(
                     {QPointF(0.0, 1.0), QPointF(1.0, -1.0), QPointF(2.0, 2.0), QPointF(3.0, -3.0)}),
                 3);
        QCOMPARE(KisBezierUtils::controlPolygonZeros({QPointF(0.0, 1.0)}), 0);
    }

    void measuresLengthAndMapsArcProportions()
    {
        const QPointF p0(0.0, 0.0);
        const QPointF p1(1.0, 0.0);
        const QPointF p2(2.0, 0.0);
        const QPointF p3(3.0, 0.0);

        QVERIFY(closeScalar(KisBezierUtils::curveLength(p0, p1, p2, p3, 1e-6), 3.0));
        QVERIFY(closeScalar(KisBezierUtils::curveLengthAtPoint(p0, p1, p2, p3, 0.25, 1e-6), 0.75));
        QVERIFY(closeScalar(KisBezierUtils::curveParamByProportion(p0, p1, p2, p3, 0.25, 1e-6), 0.25));
        QVERIFY(closeScalar(KisBezierUtils::curveProportionByParam(p0, p1, p2, p3, 0.25, 1e-6), 0.25));
    }

    void nearestPointReportsParametersButLineShortcutLeavesOutputsUntouched()
    {
        const QList<QPointF> line{QPointF(0.0, 0.0), QPointF(10.0, 0.0)};
        qreal distance = -1.0;
        QPointF nearest(-1.0, -1.0);

        QVERIFY(closeScalar(KisBezierUtils::nearestPoint(line, QPointF(4.0, 3.0), &distance, &nearest), 0.4));
        QCOMPARE(distance, -1.0);
        QCOMPARE(nearest, QPointF(-1.0, -1.0));
        QVERIFY(closeScalar(KisBezierUtils::nearestPoint(line, QPointF(8.0, -2.0)), 0.8));

        const QList<QPointF> cubicLine{QPointF(0.0, 0.0),
                                       QPointF(10.0 / 3.0, 0.0),
                                       QPointF(20.0 / 3.0, 0.0),
                                       QPointF(10.0, 0.0)};
        QVERIFY(
            closeScalar(KisBezierUtils::nearestPoint(cubicLine, QPointF(4.0, 3.0), &distance, &nearest), 0.4, 1e-4));
        QVERIFY(closeScalar(distance, 3.0, 1e-4));
        QVERIFY(closePoint(nearest, QPointF(4.0, 0.0), 1e-4));
    }

    void findsAllLineIntersectionsAndNearestAnchor()
    {
        const QPointF p0(0.0, -24.0);
        const QPointF p1(1.0, 46.0);
        const QPointF p2(2.0, -59.0);
        const QPointF p3(3.0, 36.0);
        const QLineF axis(QPointF(-1.0, 0.0), QPointF(4.0, 0.0));
        const QVector<qreal> intersections = KisBezierUtils::intersectWithLine(p0, p1, p2, p3, axis, 1e-4);

        QCOMPARE(intersections.size(), 3);
        QVERIFY(closeScalar(intersections[0], 0.2, 1e-3));
        QVERIFY(closeScalar(intersections[1], 0.4, 1e-3));
        QVERIFY(closeScalar(intersections[2], 0.8, 1e-3));

        const auto leftIntersection =
            KisBezierUtils::intersectWithLineNearest(p0, p1, p2, p3, axis, QPointF(0.0, 0.0), 1e-4);
        const auto rightIntersection =
            KisBezierUtils::intersectWithLineNearest(p0, p1, p2, p3, axis, QPointF(3.0, 0.0), 1e-4);
        QVERIFY(leftIntersection);
        QVERIFY(rightIntersection);
        QVERIFY(*leftIntersection < 0.5);
        QVERIFY(*rightIntersection > 0.5);
        QVERIFY(qAbs(KisBezierUtils::bezierCurve(p0, p1, p2, p3, *leftIntersection).y()) < 1e-3);
        QVERIFY(qAbs(KisBezierUtils::bezierCurve(p0, p1, p2, p3, *rightIntersection).y()) < 1e-3);
    }

    void mapsPatchesAndEditsControlPoints()
    {
        const std::array<QPointF, 12> patch = makeLinearPatch();
        const QPointF localPoint(0.25, 0.75);
        const QPointF globalPoint(30.0, 45.0);

        const QPointF mapped = KisBezierUtils::calculateGlobalPos(patch, localPoint);
        const QPointF mappedSvg = KisBezierUtils::calculateGlobalPosSVG2(patch, localPoint);
        QVERIFY(closePoint(mapped, globalPoint, 1e-4));
        QVERIFY(closePoint(mappedSvg, globalPoint, 1e-4));
        QVERIFY(closePoint(KisBezierUtils::calculateLocalPos(patch, mapped), localPoint, 1e-3));
        QVERIFY(closePoint(KisBezierUtils::calculateLocalPosSVG2(patch, mappedSvg), localPoint, 1e-3));

        const QPointF quadraticControl =
            KisBezierUtils::interpolateQuadric(QPointF(0.0, 0.0), QPointF(4.0, 0.0), QPointF(2.0, 2.0), 0.5);
        QVERIFY(closePoint(quadraticControl, QPointF(2.0, 4.0)));

        const auto offsets = KisBezierUtils::offsetSegment(0.5, QPointF(3.0, 6.0));
        QVERIFY(closePoint(offsets.first, QPointF(4.0, 8.0)));
        QVERIFY(closePoint(offsets.second, QPointF(4.0, 8.0)));

        const QPointF originalP0(0.0, 0.0);
        const QPointF originalP1(2.0, 4.0);
        const QPointF originalP2(8.0, 4.0);
        const QPointF originalP3(10.0, 0.0);
        QPointF leftControl1;
        QPointF leftControl2;
        QPointF splitPoint;
        QPointF rightControl1;
        QPointF rightControl2;
        KisBezierUtils::deCasteljau(originalP0,
                                    originalP1,
                                    originalP2,
                                    originalP3,
                                    0.5,
                                    &leftControl1,
                                    &leftControl2,
                                    &splitPoint,
                                    &rightControl1,
                                    &rightControl2);

        const auto mergedControls = KisBezierUtils::removeBezierNode(originalP0,
                                                                     leftControl1,
                                                                     leftControl2,
                                                                     splitPoint,
                                                                     rightControl1,
                                                                     rightControl2,
                                                                     originalP3);
        QVERIFY(closePoint(mergedControls.first, originalP1, 1e-4));
        QVERIFY(closePoint(mergedControls.second, originalP2, 1e-4));
    }
};

QTEST_GUILESS_MAIN(KisBezierUtilsContractTest)

#include "KisBezierUtilsContractTest.moc"
