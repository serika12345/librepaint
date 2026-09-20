/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <QTransform>
#include <QPainterPath>
#include <kis_debug.h>

#include <QtMath>

#include <config-gsl.h>

#ifdef HAVE_GSL
#include <gsl/gsl_multimin.h>
#endif /*HAVE_GSL*/

#include <Eigen/Eigenvalues>
#include <KisBezierUtils.h>

#define SANITY_CHECKS

namespace KisAlgebra2D {

void adjustIfOnPolygonBoundary(const QPolygonF &poly, int polygonDirection, QPointF *pt)
{
    const int numPoints = poly.size();
    for (int i = 0; i < numPoints; i++) {
        int nextI = i + 1;
        if (nextI >= numPoints) {
            nextI = 0;
        }

        const QPointF &p0 = poly[i];
        const QPointF &p1 = poly[nextI];

        QPointF edge = p1 - p0;

        qreal cross = crossProduct(edge, *pt - p0)
            / (0.5 * edge.manhattanLength());

        if (cross < 1.0 &&
            isInRange(pt->x(), p0.x(), p1.x()) &&
            isInRange(pt->y(), p0.y(), p1.y())) {

            QPointF salt = 1.0e-3 * inwardUnitNormal(edge, polygonDirection);

            QPointF adjustedPoint = *pt + salt;

            // in case the polygon is self-intersecting, polygon direction
            // might not help
            if (kisDistanceToLine(adjustedPoint, QLineF(p0, p1)) < 1e-4) {
                adjustedPoint = *pt - salt;

#ifdef SANITY_CHECKS
                if (kisDistanceToLine(adjustedPoint, QLineF(p0, p1)) < 1e-4) {
                    dbgKrita << ppVar(*pt);
                    dbgKrita << ppVar(adjustedPoint);
                    dbgKrita << ppVar(QLineF(p0, p1));
                    dbgKrita << ppVar(salt);

                    dbgKrita << ppVar(poly.containsPoint(*pt, Qt::OddEvenFill));

                    dbgKrita << ppVar(kisDistanceToLine(*pt, QLineF(p0, p1)));
                    dbgKrita << ppVar(kisDistanceToLine(adjustedPoint, QLineF(p0, p1)));
                }

                *pt = adjustedPoint;

                KIS_ASSERT_RECOVER_NOOP(kisDistanceToLine(*pt, QLineF(p0, p1)) > 1e-4);
#endif /* SANITY_CHECKS */
            }
        }
    }
}

QVector<QPointF> intersectTwoCircles(const QPointF &center1, qreal r1,
                                     const QPointF &center2, qreal r2)
{
    QVector<QPointF> points;

    const QPointF diff = (center2 - center1);
    const QPointF c1;
    const QPointF c2 = diff;

    const qreal centerDistance = norm(diff);

    if (centerDistance > r1 + r2) return points;
    if (centerDistance < qAbs(r1 - r2)) return points;

    if (centerDistance < qAbs(r1 - r2) + 0.001) {
        dbgKrita << "Skipping intersection" << ppVar(center1) << ppVar(center2) << ppVar(r1) << ppVar(r2) << ppVar(centerDistance) << ppVar(qAbs(r1-r2));
        return points;
    }

    const qreal x_kp1 = diff.x();
    const qreal y_kp1 = diff.y();

    const qreal F2 =
        0.5 * (pow2(x_kp1) +
               pow2(y_kp1) + pow2(r1) - pow2(r2));

    const qreal eps = 1e-6;

    if (qAbs(diff.y()) < eps) {
        qreal x = F2 / diff.x();
        qreal y1, y2;
        int result = KisAlgebra2D::quadraticEquation(
            1, 0,
            pow2(x) - pow2(r2),
            &y1, &y2);

        KIS_SAFE_ASSERT_RECOVER(result > 0) { return points; }

        if (result == 1) {
            points << QPointF(x, y1);
        } else if (result == 2) {
            KisAlgebra2D::RightHalfPlane p(c1, c2);

            QPointF p1(x, y1);
            QPointF p2(x, y2);

            if (p.pos(p1) >= 0) {
                points << p1;
                points << p2;
            } else {
                points << p2;
                points << p1;
            }
        }
    } else {
        const qreal A = diff.x() / diff.y();
        const qreal C = F2 / diff.y();

        qreal x1, x2;
        int result = KisAlgebra2D::quadraticEquation(
            1 + pow2(A), -2 * A * C,
            pow2(C) - pow2(r1),
            &x1, &x2);

        KIS_SAFE_ASSERT_RECOVER(result > 0) { return points; }

        if (result == 1) {
            points << QPointF(x1, C - x1 * A);
        } else if (result == 2) {
            KisAlgebra2D::RightHalfPlane p(c1, c2);

            QPointF p1(x1, C - x1 * A);
            QPointF p2(x2, C - x2 * A);

            if (p.pos(p1) >= 0) {
                points << p1;
                points << p2;
            } else {
                points << p2;
                points << p1;
            }
        }
    }

    for (int i = 0; i < points.size(); i++) {
        points[i] = center1 + points[i];
    }

    return points;
}

inline QTransform toQTransformStraight(const Eigen::Matrix3d &m)
{
    return QTransform(m(0,0), m(0,1), m(0,2),
                      m(1,0), m(1,1), m(1,2),
                      m(2,0), m(2,1), m(2,2));
}

inline Eigen::Matrix3d fromQTransformStraight(const QTransform &t)
{
    Eigen::Matrix3d m;

    m << t.m11() , t.m12() , t.m13()
        ,t.m21() , t.m22() , t.m23()
        ,t.m31() , t.m32() , t.m33();

    return m;
}

std::pair<QPointF, QTransform> transformEllipse(const QPointF &axes, const QTransform &fullLocalToGlobal)
{
    KisAlgebra2D::DecomposedMatrix decomposed(fullLocalToGlobal);
    const QTransform localToGlobal =
            decomposed.scaleTransform() *
            decomposed.shearTransform() *

            /* decomposed.projectTransform() * */
            /*decomposed.translateTransform() * */

            decomposed.rotateTransform();

    const QTransform localEllipse = QTransform(1.0 / pow2(axes.x()), 0.0, 0.0,
                                               0.0, 1.0 / pow2(axes.y()), 0.0,
                                               0.0, 0.0, 1.0);


    const QTransform globalToLocal = localToGlobal.inverted();

    Eigen::Matrix3d eqM =
        fromQTransformStraight(globalToLocal *
                               localEllipse *
                               globalToLocal.transposed());

//    std::cout << "eqM:" << std::endl << eqM << std::endl;

    Eigen::EigenSolver<Eigen::Matrix3d> eigenSolver(eqM);

    const Eigen::Matrix3d T = eigenSolver.eigenvalues().real().asDiagonal();
    const Eigen::Matrix3d U = eigenSolver.eigenvectors().real();

    const Eigen::Matrix3d Ti = eigenSolver.eigenvalues().imag().asDiagonal();
    const Eigen::Matrix3d Ui = eigenSolver.eigenvectors().imag();

    KIS_SAFE_ASSERT_RECOVER_NOOP(Ti.isZero());
    KIS_SAFE_ASSERT_RECOVER_NOOP(Ui.isZero());
    KIS_SAFE_ASSERT_RECOVER_NOOP((U * U.transpose()).isIdentity());

//    std::cout << "T:" << std::endl << T << std::endl;
//    std::cout << "U:" << std::endl << U << std::endl;

//    std::cout << "Ti:" << std::endl << Ti << std::endl;
//    std::cout << "Ui:" << std::endl << Ui << std::endl;

//    std::cout << "UTU':" << std::endl << U * T * U.transpose() << std::endl;

    const qreal newA = 1.0 / std::sqrt(T(0,0) * T(2,2));
    const qreal newB = 1.0 / std::sqrt(T(1,1) * T(2,2));

    const QTransform newGlobalToLocal = toQTransformStraight(U);
    const QTransform newLocalToGlobal = QTransform::fromScale(-1,-1) *
            newGlobalToLocal.inverted() *
            decomposed.translateTransform();

    return std::make_pair(QPointF(newA, newB), newLocalToGlobal);
}

QPointF alignForZoom(const QPointF &pt, qreal zoom)
{
    return QPointF((pt * zoom).toPoint()) / zoom;
}

#ifdef HAVE_GSL

struct ElasticMotionData
{
    QPointF oldBasePos;
    QPointF newBasePos;
    QVector<QPointF> anchorPoints;

    QPointF oldResultPoint;
};

double elasticMotionError(const gsl_vector * x, void *paramsPtr)
{
    using KisAlgebra2D::norm;
    using KisAlgebra2D::dotProduct;
    using KisAlgebra2D::crossProduct;

    const QPointF newResultPoint(gsl_vector_get(x, 0), gsl_vector_get(x, 1));

    const ElasticMotionData *p = static_cast<const ElasticMotionData*>(paramsPtr);

    const QPointF vecL = newResultPoint - p->newBasePos;
    const qreal L = norm(vecL);

    const qreal deltaL = L - kisDistance(p->oldBasePos, p->oldResultPoint);

    QVector<qreal> deltaLi;
    QVector<qreal> Li;
    QVector<qreal> cosMuI;
    QVector<qreal> sinMuI;
    Q_FOREACH (const QPointF &anchorPoint, p->anchorPoints) {
        const QPointF vecLi = newResultPoint - anchorPoint;
        const qreal _Li = norm(vecLi);

        Li << _Li;
        deltaLi << _Li - kisDistance(p->oldResultPoint, anchorPoint);
        cosMuI << dotProduct(vecLi, vecL) / (_Li * L);
        sinMuI << crossProduct(vecL, vecLi) / (_Li * L);
    }

    qreal finalError = 0;

    qreal tangentialForceSum = 0;
    for (int i = 0; i < p->anchorPoints.size(); i++) {
        const qreal sum = deltaLi[i] * sinMuI[i] / Li[i];
        tangentialForceSum += sum;
    }

    finalError += pow2(tangentialForceSum);

    qreal normalForceSum = 0;
    {
        qreal sum = 0;
        for (int i = 0; i < p->anchorPoints.size(); i++) {
            sum += deltaLi[i] * cosMuI[i] / Li[i];
        }
        normalForceSum = (-deltaL) / L - sum;
    }

    finalError += pow2(normalForceSum);

    return finalError;
}


#endif /* HAVE_GSL */

QPointF moveElasticPoint(const QPointF &pt,
                         const QPointF &base, const QPointF &newBase,
                         const QVector<QPointF> &anchorPoints)
{
    const QPointF offset = newBase - base;

    QPointF newResultPoint = pt + offset;

#ifdef HAVE_GSL

    ElasticMotionData data;
    data.newBasePos = newBase;
    data.oldBasePos = base;
    data.anchorPoints = anchorPoints;
    data.oldResultPoint = pt;

    const gsl_multimin_fminimizer_type *T =
        gsl_multimin_fminimizer_nmsimplex2;
    gsl_multimin_fminimizer *s = 0;
    gsl_vector *ss, *x;
    gsl_multimin_function minex_func;

    size_t iter = 0;
    int status;
    double size;

    /* Starting point */
    x = gsl_vector_alloc (2);
    gsl_vector_set (x, 0, newResultPoint.x());
    gsl_vector_set (x, 1, newResultPoint.y());

    /* Set initial step sizes to 0.1 */
    ss = gsl_vector_alloc (2);
    gsl_vector_set (ss, 0, 0.1 * offset.x());
    gsl_vector_set (ss, 1, 0.1 * offset.y());

    /* Initialize method and iterate */
    minex_func.n = 2;
    minex_func.f = elasticMotionError;
    minex_func.params = (void*)&data;

    s = gsl_multimin_fminimizer_alloc (T, 2);
    gsl_multimin_fminimizer_set (s, &minex_func, x, ss);

    do
    {
        iter++;
        status = gsl_multimin_fminimizer_iterate(s);

        if (status)
            break;

        size = gsl_multimin_fminimizer_size (s);
        status = gsl_multimin_test_size (size, 1e-6);

        /**
         * Sometimes the algorithm may converge to a wrong point,
         * then just try to force it search better or return invalid
         * result.
         */
        if (status == GSL_SUCCESS && elasticMotionError(s->x, &data) > 0.5) {
            status = GSL_CONTINUE;
        }

        if (status == GSL_SUCCESS)
        {
//             qDebug() << "*******Converged to minimum";
//             qDebug() << gsl_vector_get (s->x, 0)
//                      << gsl_vector_get (s->x, 1)
//                      << "|" << s->fval << size;
//             qDebug() << ppVar(iter);

             newResultPoint.rx() = gsl_vector_get (s->x, 0);
             newResultPoint.ry() = gsl_vector_get (s->x, 1);
        }
    }
    while (status == GSL_CONTINUE && iter < 10000);

    if (status != GSL_SUCCESS) {
        ENTER_FUNCTION() << "failed to find point" << ppVar(pt) << ppVar(base) << ppVar(newBase);
    }

    gsl_vector_free(x);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);
#endif

    return newResultPoint;
}

QList<QLineF> intersectLineConcavePolygon(const QPolygonF polygon, const QLineF& line, bool extendFirst, bool extendSecond) {

    // should use kis_convex_hull instead, that one uses boost
    QPolygonF convexHull = calculateConvexHull(polygon);
    if (convexHull.count() == polygon.count()) {
        QLineF resultLine = line;
        bool result = intersectLineConvexPolygon(resultLine, polygon, extendFirst, extendSecond);
        if (result) {
            return QList<QLineF>() << resultLine;
        } else {
            return QList<QLineF>();
        }
    }

    // it was a concave polygon
    // intersectLines

    // should be as easy as cutting every line, then sorting the points and creating the lines, except gotta take care of exceptions

    KIS_ASSERT(false && "Not implemented yet");

    return QList<QLineF>();
}

QList<int> getLineSegmentCrossingLineIndexes(const QLineF &line, const QPainterPath &shape)
{
    VectorPath path(shape);
    QList<int> indexes;

    for (int i = 0; i < path.segmentsCount(); i++) {
        QList<VectorPath::VectorPathPoint> points = path.segmentAt(i);
        if (points.count() < 2)
            continue;
        if (points[1].type == VectorPath::VectorPathPoint::BezierTo) {

            qreal eps = 1e-5;
            QVector<qreal> intersections = KisBezierUtils::intersectWithLine(points[0].endPoint, points[1].controlPoint1, points[1].controlPoint2, points[1].endPoint, line, eps);
            for (int i = 0; i < intersections.length(); i++) {
                //
                QPointF p = KisBezierUtils::bezierCurve(points[0].endPoint, points[1].controlPoint1, points[1].controlPoint2, points[1].endPoint, intersections[i]);

                bool onLine = isOnLine(line, p, eps, true, true, true);
                if (onLine) {
                    indexes << path.segmentIndexToPathIndex(i);
                    break; // we need just one
                }
            }

        } else {

            QLineF lineSegment = path.segmentAtAsLine(i);
            QPointF intersection;
            QLineF::IntersectionType type = lineSegment.intersects(line, &intersection);
            if (type == QLineF::IntersectionType::BoundedIntersection) {
                indexes << path.segmentIndexToPathIndex(i);
            }
        }
    }
    return indexes;
}

QList<QPointF> VectorPath::intersectSegmentWithLineBounded(const QLineF &line, const Segment &segment)
{
    qreal eps = 1e-5;
    if (segment.type != VectorPath::VectorPathPoint::BezierTo) {
        QLineF segLine = QLineF(segment.startPoint, segment.endPoint);
        QPointF intersection;
        if (segLine.intersects(line, &intersection) == QLineF::BoundedIntersection) {
            return QList<QPointF> {intersection};
        } else {
            return QList<QPointF>();
        }

    } else {
        QList<QPointF> result;
        // check for intersections
        QVector<qreal> intersections = KisBezierUtils::intersectWithLine(segment.startPoint, segment.controlPoint1, segment.controlPoint2, segment.endPoint, line, eps);
        for (int i = 0; i < intersections.count(); i++) {
            QPointF p = KisBezierUtils::bezierCurve(segment.startPoint, segment.controlPoint1, segment.controlPoint2, segment.endPoint, intersections[i]);
            bool onLine = isOnLine(line, p, eps, true, true, true);
            if (onLine) {
                result << p;
            }
        }

        return result;
    }

}

QList<QPointF> VectorPath::intersectSegmentWithLineBounded(const QLineF &line, const VectorPathPoint &p1, const VectorPathPoint &p2)
{
    return intersectSegmentWithLineBounded(line, VectorPath::Segment(p1, p2));
}

} // namespace
