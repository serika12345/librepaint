/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <QTransform>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>

namespace KisAlgebra2D {

template <class Point, class Rect>
inline Point ensureInRectImpl(Point pt, const Rect &bounds)
{
    if (pt.x() > bounds.right()) {
        pt.rx() = bounds.right();
    } else if (pt.x() < bounds.left()) {
        pt.rx() = bounds.left();
    }

    if (pt.y() > bounds.bottom()) {
        pt.ry() = bounds.bottom();
    } else if (pt.y() < bounds.top()) {
        pt.ry() = bounds.top();
    }

    return pt;
}

QPoint ensureInRect(QPoint pt, const QRect &bounds)
{
    return ensureInRectImpl(pt, bounds);
}

QPointF ensureInRect(QPointF pt, const QRectF &bounds)
{
    return ensureInRectImpl(pt, bounds);
}

template <class Rect, class Point>
QVector<Point> sampleRectWithPoints(const Rect &rect)
{
    QVector<Point> points;

    Point m1 = 0.5 * (rect.topLeft() + rect.topRight());
    Point m2 = 0.5 * (rect.bottomLeft() + rect.bottomRight());

    points << rect.topLeft();
    points << m1;
    points << rect.topRight();

    points << 0.5 * (rect.topLeft() + rect.bottomLeft());
    points << 0.5 * (m1 + m2);
    points << 0.5 * (rect.topRight() + rect.bottomRight());

    points << rect.bottomLeft();
    points << m2;
    points << rect.bottomRight();

    return points;
}

QVector<QPoint> sampleRectWithPoints(const QRect &rect)
{
    return sampleRectWithPoints<QRect, QPoint>(rect);
}

QVector<QPointF> sampleRectWithPoints(const QRectF &rect)
{
    return sampleRectWithPoints<QRectF, QPointF>(rect);
}

template <class Rect, class Point, bool alignPixels>
Rect approximateRectFromPointsImpl(const QVector<Point> &points)
{
    using namespace boost::accumulators;
    accumulator_set<qreal, stats<tag::min, tag::max > > accX;
    accumulator_set<qreal, stats<tag::min, tag::max > > accY;

    Q_FOREACH (const Point &pt, points) {
        accX(pt.x());
        accY(pt.y());
    }

    Rect resultRect;

    if (alignPixels) {
        resultRect.setCoords(std::floor(min(accX)), std::floor(min(accY)),
                             std::ceil(max(accX)), std::ceil(max(accY)));
    } else {
        resultRect.setCoords(min(accX), min(accY),
                             max(accX), max(accY));
    }

    return resultRect;
}

QRect approximateRectFromPoints(const QVector<QPoint> &points)
{
    return approximateRectFromPointsImpl<QRect, QPoint, true>(points);
}

QRectF approximateRectFromPoints(const QVector<QPointF> &points)
{
    return approximateRectFromPointsImpl<QRectF, QPointF, false>(points);
}

QRect approximateRectWithPointTransform(const QRect &rect, std::function<QPointF(QPointF)> func)
{
    QVector<QPoint> points = sampleRectWithPoints(rect);

    using namespace boost::accumulators;
    accumulator_set<qreal, stats<tag::min, tag::max > > accX;
    accumulator_set<qreal, stats<tag::min, tag::max > > accY;

    Q_FOREACH (const QPoint &pt, points) {
        QPointF dstPt = func(pt);

        accX(dstPt.x());
        accY(dstPt.y());
    }

    QRect resultRect;
    resultRect.setCoords(std::floor(min(accX)), std::floor(min(accY)),
                         std::ceil(max(accX)), std::ceil(max(accY)));

    return resultRect;
}

QRectF cutOffRect(const QRectF &rc, const KisAlgebra2D::RightHalfPlane &p)
{
    QVector<QPointF> points;

    const QLineF cutLine = p.getLine();

    points << rc.topLeft();
    points << rc.topRight();
    points << rc.bottomRight();
    points << rc.bottomLeft();

    QPointF p1 = points[3];
    bool p1Valid = p.pos(p1) >= 0;

    QVector<QPointF> resultPoints;

    for (int i = 0; i < 4; i++) {
        const QPointF p2 = points[i];
        const bool p2Valid = p.pos(p2) >= 0;

        if (p1Valid != p2Valid) {
            QPointF intersection;
            cutLine.intersects(QLineF(p1, p2), &intersection);
            resultPoints << intersection;
        }

        if (p2Valid) {
            resultPoints << p2;
        }

        p1 = p2;
        p1Valid = p2Valid;
    }

    return approximateRectFromPoints(resultPoints);
}

QTransform mapToRect(const QRectF &rect)
{
    return
        QTransform(rect.width(), 0, 0, rect.height(),
                   rect.x(), rect.y());
}

QTransform mapToRectInverse(const QRectF &rect)
{
    return
        QTransform::fromTranslate(-rect.x(), -rect.y()) *
        QTransform::fromScale(rect.width() != 0 ? 1.0 / rect.width() : 0.0,
                              rect.height() != 0 ? 1.0 / rect.height() : 0.0);
}

bool fuzzyMatrixCompare(const QTransform &t1, const QTransform &t2, qreal delta) {
    return
            qAbs(t1.m11() - t2.m11()) < delta &&
            qAbs(t1.m12() - t2.m12()) < delta &&
            qAbs(t1.m13() - t2.m13()) < delta &&
            qAbs(t1.m21() - t2.m21()) < delta &&
            qAbs(t1.m22() - t2.m22()) < delta &&
            qAbs(t1.m23() - t2.m23()) < delta &&
            qAbs(t1.m31() - t2.m31()) < delta &&
            qAbs(t1.m32() - t2.m32()) < delta &&
            qAbs(t1.m33() - t2.m33()) < delta;
}

} // namespace KisAlgebra2D
