/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <Eigen/Eigenvalues>
#include <QVector2D>

namespace KisAlgebra2D
{

qreal angleBetweenVectors(const QPointF &v1, const QPointF &v2)
{
    qreal a1 = std::atan2(v1.y(), v1.x());
    qreal a2 = std::atan2(v2.y(), v2.x());

    return a2 - a1;
}

qreal directionBetweenPoints(const QPointF &p1, const QPointF &p2, qreal defaultAngle)
{
    if (fuzzyPointCompare(p1, p2)) {
        return defaultAngle;
    }

    const QVector2D diff(p2 - p1);
    return std::atan2(diff.y(), diff.x());
}

bool fuzzyPointCompare(const QPointF &p1, const QPointF &p2)
{
    return qFuzzyCompare(p1.x(), p2.x()) && qFuzzyCompare(p1.y(), p2.y());
}

bool fuzzyPointCompare(const QPointF &p1, const QPointF &p2, qreal delta)
{
    return qAbs(p1.x() - p2.x()) < delta && qAbs(p1.y() - p2.y()) < delta;
}

boost::optional<QPointF> intersectLines(const QLineF &boundedLine, const QLineF &unboundedLine)
{
    const QPointF B1 = unboundedLine.p1();
    const QPointF A1 = unboundedLine.p2() - unboundedLine.p1();

    const QPointF B2 = boundedLine.p1();
    const QPointF A2 = boundedLine.p2() - boundedLine.p1();

    Eigen::Matrix<float, 2, 2> A;
    A << A1.x(), -A2.x(),
         A1.y(), -A2.y();

    Eigen::Matrix<float, 2, 1> B;
    B << B2.x() - B1.x(),
         B2.y() - B1.y();

    if (qFuzzyIsNull(A.determinant())) {
        return boost::none;
    }

    Eigen::Matrix<float, 2, 1> T;

    T = A.inverse() * B;

    const qreal t2 = T(1,0);
    double epsilon = 1e-06; // to avoid precision issues

    if (t2 < 0.0 || t2 > 1.0) {
        if (qAbs(t2) > epsilon && qAbs(t2 - 1.0) > epsilon) {
            return boost::none;
        }
    }

    return t2 * A2 + B2;
}

boost::optional<QPointF> intersectLines(const QPointF &p1, const QPointF &p2,
                                        const QPointF &q1, const QPointF &q2)
{
    return intersectLines(QLineF(p1, p2), QLineF(q1, q2));
}

} // namespace KisAlgebra2D
