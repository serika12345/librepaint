/*
 * SPDX-FileCopyrightText: 2008-2009 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2020 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierUtils.h"

namespace KisBezierUtils
{

qreal curveLength(const QPointF p0, const QPointF p1, const QPointF p2, const QPointF p3, const qreal error)
{
    /*
     * This algorithm is implemented based on an idea by Jens Gravesen:
     * "Adaptive subdivision and the length of Bezier curves" mat-report no. 1992-10, Mathematical Institute,
     * The Technical University of Denmark.
     *
     * By subdividing the curve at parameter value t you only have to find the length of a full Bezier curve.
     * If you denote the length of the control polygon by L1 i.e.:
     *   L1 = |P0 P1| +|P1 P2| +|P2 P3|
     *
     * and the length of the cord by L0 i.e.:
     *   L0 = |P0 P3|
     *
     * then
     *   L = 1/2*L0 + 1/2*L1
     *
     * is a good approximation to the length of the curve, and the difference
     *   ERR = L1-L0
     *
     * is a measure of the error. If the error is to large, then you just subdivide curve at parameter value
     * 1/2, and find the length of each half.
     * If m is the number of subdivisions then the error goes to zero as 2^-4m.
     * If you don't have a cubic curve but a curve of degree n then you put
     *   L = (2*L0 + (n-1)*L1)/(n+1)
     */

    const int deg = bezierDegree(p0, p1, p2, p3);

    if (deg == -1) {
        return 0.0;
    }

    const qreal chordLen = kisDistance(p0, p3);

    if (deg == 1) {
        return chordLen;
    }

    qreal polyLength = 0.0;

    polyLength += kisDistance(p0, p1);
    polyLength += kisDistance(p1, p2);
    polyLength += kisDistance(p2, p3);

    if ((polyLength - chordLen) > error) {
        QPointF q0, q1, q2, q3, q4;
        deCasteljau(p0, p1, p2, p3, 0.5, &q0, &q1, &q2, &q3, &q4);

        return curveLength(p0, q0, q1, q2, error) + curveLength(q2, q3, q4, p3, error);
    } else {
        if (deg == 3) {
            return 0.5 * chordLen + 0.5 * polyLength;
        } else {
            return (2.0 * chordLen + polyLength) / 3.0;
        }
    }
}

qreal curveLengthAtPoint(const QPointF p0,
                         const QPointF p1,
                         const QPointF p2,
                         const QPointF p3,
                         qreal t,
                         const qreal error)
{
    QPointF q0, q1, q2, q3, q4;
    deCasteljau(p0, p1, p2, p3, t, &q0, &q1, &q2, &q3, &q4);

    return curveLength(p0, q0, q1, q2, error);
}

} // namespace KisBezierUtils
