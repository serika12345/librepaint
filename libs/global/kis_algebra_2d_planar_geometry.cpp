/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_algebra_2d.h"

#include <cmath>
#include <limits>

#include <Eigen/Eigenvalues>

namespace KisAlgebra2D {

QPointF transformAsBase(const QPointF &pt, const QPointF &base1, const QPointF &base2) {
    qreal len1 = norm(base1);
    if (len1 < 1e-5) return pt;
    qreal sin1 = base1.y() / len1;
    qreal cos1 = base1.x() / len1;

    qreal len2 = norm(base2);
    if (len2 < 1e-5) return QPointF();
    qreal sin2 = base2.y() / len2;
    qreal cos2 = base2.x() / len2;

    qreal sinD = sin2 * cos1 - cos2 * sin1;
    qreal cosD = cos1 * cos2 + sin1 * sin2;
    qreal scaleD = len2 / len1;

    QPointF result;
    result.rx() = scaleD * (pt.x() * cosD - pt.y() * sinD);
    result.ry() = scaleD * (pt.x() * sinD + pt.y() * cosD);

    return result;
}


bool intersectLineRect(QLineF &line, const QRect rect, bool extend)
{
    return intersectLineRect(line, rect, extend, extend);
}


bool intersectLineRect(QLineF &line, const QRect rect, bool extendFirst, bool extendSecond)
{
    // using Liang-Barsky algorithm
    // using parametric equation for a line:
    // X = x1 + t(x2-x1) = x1 + t*p2
    // Y = y1 + t(y2-y1) = y1 + t*p4
    // note that we have a line *segment* here, not a line
    // t1 = 0, t2 = 1, no matter what points we got

    double p1 = -(line.x2() - line.x1());
    double p2 = -p1;
    double p3 = -(line.y2() - line.y1());
    double p4 = -p3;

    QVector<double> p = QVector<double>({p1, p2, p3, p4});
    QVector<double> q = QVector<double>({line.x1() - rect.x(), rect.x() + rect.width() - line.x1(), line.y1() - rect.y(), rect.y() + rect.height() - line.y1()});

    float tmin = extendFirst ? -std::numeric_limits<float>::infinity() : 0; // line.p1() - first point, or infinity
    float tmax = extendSecond ? std::numeric_limits<float>::infinity() : 1; // line.p2() - second point, or infinity

    for (int i = 0; i < p.length(); i++) {
        // now clipping
        if (p[i] == 0 && q[i] < 0) {
            // line is parallel to the boundary and outside of it
            return false;
        } else if (p[i] < 0) {
            // line entry point (should be tmin)
            double t = q[i]/p[i];
            if (t > tmax) {
                if (extendSecond) {
                    tmax = t;
                } else {
                    return false;
                }
            }
            if (t > tmin) {
                tmin = t;
            }


        } else if (p[i] > 0) {
            // line leaving point (should be tmax)
            double t = q[i]/p[i];
            if (t < tmin) {
                if (extendFirst) {
                    tmin = t;
                } else {
                    return false;
                }
            }
            if (t < tmax) {
                tmax = t;
            }
        }
    }

    QPointF pt1 = line.p1();
    QPointF pt2 = line.p2();

    if (extendFirst || tmin > 0) {
        pt1 = QPointF(line.x1() + tmin*(line.x2() - line.x1()), line.y1() + tmin*(line.y2() - line.y1()));
    }
    if (extendSecond || tmax < 1) {
        pt2 = QPointF(line.x1() + tmax*(line.x2() - line.x1()), line.y1() + tmax*(line.y2() - line.y1()));
    }

    line.setP1(pt1);
    line.setP2(pt2);

    return true;

}


bool intersectLineConvexPolygon(QLineF &line, const QPolygonF polygon, bool extendFirst, bool extendSecond)
{
    qreal epsilon = 1e-07;

    if (polygon.size() == 0) {
        return false;
    }

    // trivial case: no extends, all points inside the polygon
    if (!extendFirst && !extendSecond && polygon.containsPoint(line.p1(), Qt::WindingFill) && polygon.containsPoint(line.p2(), Qt::WindingFill)) {
        return true;
    }

    // Cyrus-Beck algorithm: https://en.wikipedia.org/wiki/Cyrus%E2%80%93Beck_algorithm

    // parametric equation for the line:
    // p(t) = t*P1 + (1-t)*P2

    // we can't use infinity here, because the points would all end up as (+/- inf, +/- inf)
    // which would be useless if we have a very specific direction
    // hence we use just "a big number"
    float aBigNumber = 100000; // that will keep t*Px still on the line // this is LIMITATION of the algorithm
    float tmin = extendFirst ? -aBigNumber : 0; // line.p1() - first point, or infinity
    float tmax = extendSecond ? aBigNumber : 1; // line.p2() - second point, or infinity



    QList<QLineF> clippingLines;
    QList<QPointF> normals;

    bool clockwise = false;
    {
        // only temporary values to check whether the polygon is clockwise or counterclockwise
        QPointF vec1 = polygon[1] - polygon[0];
        QPointF vec2 = polygon[2] - polygon[0];

        QLineF line1(QPointF(0, 0), vec1);
        QLineF line2(QPointF(0, 0), vec2);

        qreal angle = line1.angleTo(line2); // range: <0, 360) // <0, 180) means counter-clockwise
        if (angle >= 180) {
            clockwise = true;
        }
    }


    for (int i = 0; i < polygon.size() - 1; i++) {
        clippingLines.append(QLineF(polygon[i], polygon[i + 1]));
        float xdiff = polygon[i].x() - polygon[i + 1].x();
        float ydiff = polygon[i].y() - polygon[i + 1].y();

        if (!clockwise) {
            normals.append(QPointF(ydiff, -xdiff));
        } else {
            normals.append(QPointF(-ydiff, xdiff));
        }

    }

    if (!polygon.isClosed()) {
        int i = polygon.size() - 1;
        clippingLines.append(QLineF(polygon[i], polygon[0]));
        float xdiff = polygon[i].x() - polygon[0].x();
        float ydiff = polygon[i].y() - polygon[0].y();

        if (!clockwise) {
            normals.append(QPointF(ydiff, -xdiff));
        } else {
            normals.append(QPointF(-ydiff, xdiff));
        }

    }

    QPointF lineVec = line.p2() - line.p1();
    // ax + by + c = 0
    // c = -ax - by
//    qreal cFromStandardEquation = -lineVec.x()*line.p1().x() - lineVec.y()*line.p1().y();


    QPointF p1 = line.p1();
    QPointF p2 = line.p2();

    qreal pA, pB, pC; // standard equation for the line: ax + by + c = 0
    if (qAbs(lineVec.x()) < epsilon) {
        // x1 ~= x2, line looks like: x = -pC
        pB = 0;
        pA = 1;
        pC = -p1.x();
    } else {
        pB = 1; // let b = 1 to simplify
        qreal tmp = (p1.y() - p2.y())/(p1.x() - p2.x());
        pA = -tmp;
        pC = tmp * p1.x() - p1.y();
    }


    int pEFound = 0;


    for (int i = 0; i < clippingLines.size(); i++) {

        // is the clipping line parallel to the line?
        QPointF clipVec = clippingLines[i].p2() - clippingLines[i].p1();

        if (qFuzzyCompare(clipVec.x()*lineVec.y(), clipVec.y()*lineVec.x())) {

            // vectors are parallel
            // let's check if one of the clipping points is on the line (extended) to see if it's the same line; if not, proceed normally

            qreal distanceBetweenLines = qAbs(pA*clippingLines[i].p1().x() + pB*clippingLines[i].p1().y() + pC)
                    /(sqrt(pA*pA + pB*pB));


            if (qAbs(distanceBetweenLines) < epsilon) {
                // they are the same line

                qreal t1;
                qreal t2;

                if (qAbs(p2.x() - p1.x()) > epsilon) {
                    t1 = (clippingLines[i].p1().x() - p1.x())/(p2.x() - p1.x());
                    t2 = (clippingLines[i].p2().x() - p1.x())/(p2.x() - p1.x());
                } else {
                    t1 = (clippingLines[i].p1().y() - p1.y())/(p2.y() - p1.y());
                    t2 = (clippingLines[i].p2().y() - p1.y())/(p2.y() - p1.y());
                }

                qreal tmin1 = qMin(t1, t2);
                qreal tmax2 = qMax(t1, t2);



                if (tmin < tmin1) {
                    tmin = tmin1;
                }
                if (tmax > tmax2) {
                    tmax = tmax2;
                }
                continue;
            }
            else {
                // if clipping line points are P1 and P2, and some other point is P(t) (given by parametric equation),
                // and N is the normal vector
                // and one of the points of the line we're intersecting is K,
                // then we have a following equation:
                // K = P(t) + a * N
                // where t and a are unknown.
                // after simplifying we get:
                // t*(p2 - p1) + a*(n) = k - p1
                // and since we have two axis, we get a linear system of two equations
                // A * [t; a] = B

                Eigen::Matrix2f A;
                Eigen::Vector2f b;
                A << p2.x() - p1.x(), normals[i].x(),
                     p2.y() - p1.y(), normals[i].y();
                b << clippingLines[i].p1().x() - p1.x(),
                     clippingLines[i].p1().y() - p1.y();

                Eigen::Vector2f ta = A.colPivHouseholderQr().solve(b);

                qreal tt = ta(1);
                if (tt < 0) {
                    return false;
                }
            }

        }


        boost::optional<QPointF> pEOptional = intersectLines(clippingLines[i], line); // bounded, unbounded


        if (pEOptional) {
            pEFound++;

            QPointF pE = pEOptional.value();
            QPointF n = normals[i];

            QPointF A = line.p2() - line.p1();
            QPointF B = line.p1() - pE;

            qreal over = (n.x()*B.x() + n.y()*B.y());
            qreal under = (n.x()*A.x() + n.y()*A.y());
            qreal t = -over/under;

//            if (pE.x() != p2.x()) {
//                qreal maybet = (pE.x() - p1.x())/(p2.x() - p1.x());
//            }

            qreal tminvalue = tmin * under + over;
            qreal tmaxvalue = tmax * under + over;

            if (tminvalue > 0 && tmaxvalue > 0) {
                // both outside of the edge
                return false;
            }
            if (tminvalue <= 0 && tmaxvalue <= 0) {
                // noop, both inside
            }
            if (tminvalue*tmaxvalue < 0) {

                // on two different sides
                if (tminvalue > 0) {
                    // tmin outside, tmax inside
                    if (t > tmin) {
                        tmin = t;
                    }
                } else {
                    if (t < tmax) {
                        tmax = t;
                    }
                }
            }



            if (tmax < tmin) {
                return false;
            }
        }
        else {
        }

    }

    if (pEFound == 0) {
        return false;
    }

    QLineF response = QLineF(tmin*line.p2() + (1-tmin)*line.p1(), tmax*line.p2() + (1-tmax)*line.p1());
    line = response;
    return true;
}


int quadraticEquation(qreal a, qreal b, qreal c, qreal *x1, qreal *x2)
{
    int numSolutions = 0;

    const qreal D = pow2(b) - 4 * a * c;
    const qreal eps = 1e-14;

    if (qAbs(D) <= eps) {
        *x1 = -b / (2 * a);
        numSolutions = 1;
    } else if (D < 0) {
        return 0;
    } else {
        const qreal sqrt_D = std::sqrt(D);

        *x1 = (-b + sqrt_D) / (2 * a);
        *x2 = (-b - sqrt_D) / (2 * a);
        numSolutions = 2;
    }

    return numSolutions;
}


QVector<QPointF> findTrianglePoint(const QPointF &p1, const QPointF &p2, qreal a, qreal b)
{
    using KisAlgebra2D::norm;
    using KisAlgebra2D::dotProduct;

    QVector<QPointF> result;

    const QPointF p = p2 - p1;

    const qreal pSq = dotProduct(p, p);

    const qreal T =  0.5 * (-pow2(b) + pow2(a) + pSq);

    if (p.isNull()) return result;

    if (qAbs(p.x()) > qAbs(p.y())) {
        const qreal A = 1.0;
        const qreal B2 = -T * p.y() / pSq;
        const qreal C = pow2(T) / pSq - pow2(a * p.x()) / pSq;

        const qreal D4 = pow2(B2) - A * C;

        if (D4 > 0 || qFuzzyIsNull(D4)) {
            if (qFuzzyIsNull(D4)) {
                const qreal y = -B2 / A;
                const qreal x = (T - y * p.y()) / p.x();
                result << p1 + QPointF(x, y);
            } else {
                const qreal y1 = (-B2 + std::sqrt(D4)) / A;
                const qreal x1 = (T - y1 * p.y()) / p.x();
                result << p1 + QPointF(x1, y1);

                const qreal y2 = (-B2 - std::sqrt(D4)) / A;
                const qreal x2 = (T - y2 * p.y()) / p.x();
                result << p1 + QPointF(x2, y2);
            }
        }
    } else {
        const qreal A = 1.0;
        const qreal B2 = -T * p.x() / pSq;
        const qreal C = pow2(T) / pSq - pow2(a * p.y()) / pSq;

        const qreal D4 = pow2(B2) - A * C;

        if (D4 > 0 || qFuzzyIsNull(D4)) {
            if (qFuzzyIsNull(D4)) {
                const qreal x = -B2 / A;
                const qreal y = (T - x * p.x()) / p.y();
                result << p1 + QPointF(x, y);
            } else {
                const qreal x1 = (-B2 + std::sqrt(D4)) / A;
                const qreal y1 = (T - x1 * p.x()) / p.y();
                result << p1 + QPointF(x1, y1);

                const qreal x2 = (-B2 - std::sqrt(D4)) / A;
                const qreal y2 = (T - x2 * p.x()) / p.y();
                result << p1 + QPointF(x2, y2);
            }
        }
    }

    return result;
}


boost::optional<QPointF> findTrianglePointNearest(const QPointF &p1, const QPointF &p2, qreal a, qreal b, const QPointF &nearest)
{
    const QVector<QPointF> points = findTrianglePoint(p1, p2, a, b);

    boost::optional<QPointF> result;

    if (points.size() == 1) {
        result = points.first();
    } else if (points.size() > 1) {
        result = kisDistance(points.first(), nearest) < kisDistance(points.last(), nearest) ? points.first() : points.last();
    }

    return result;
}


QPointF moveElasticPoint(const QPointF &pt, const QPointF &base, const QPointF &newBase, const QPointF &wingA, const QPointF &wingB)
{
    using KisAlgebra2D::norm;
    using KisAlgebra2D::dotProduct;
    using KisAlgebra2D::crossProduct;

    const QPointF vecL = base - pt;
    const QPointF vecLa = wingA - pt;
    const QPointF vecLb = wingB - pt;

    const qreal L = norm(vecL);
    const qreal La = norm(vecLa);
    const qreal Lb = norm(vecLb);

    const qreal sinMuA = crossProduct(vecLa, vecL) / (La * L);
    const qreal sinMuB = crossProduct(vecL, vecLb) / (Lb * L);

    const qreal cosMuA = dotProduct(vecLa, vecL) / (La * L);
    const qreal cosMuB = dotProduct(vecLb, vecL) / (Lb * L);

    const qreal dL = dotProduct(newBase - base, -vecL) / L;


    const qreal divB = (cosMuB + La / Lb * sinMuB * cosMuA / sinMuA) / L +
            (cosMuB + sinMuB * cosMuA / sinMuA) / Lb;
    const qreal dLb = dL / ( L * divB);

    const qreal divA = (cosMuA + Lb / La * sinMuA * cosMuB / sinMuB) / L +
            (cosMuA + sinMuA * cosMuB / sinMuB) / La;
    const qreal dLa = dL / ( L * divA);

    boost::optional<QPointF> result =
        KisAlgebra2D::findTrianglePointNearest(wingA, wingB, La + dLa, Lb + dLb, pt);

    const QPointF resultPoint = result ? *result : pt;

    return resultPoint;
}


void cropLineToRect(QLineF &line, const QRect rect, bool extendFirst, bool extendSecond)
{
    bool intersects = intersectLineRect(line, rect, extendFirst, extendSecond);
    if (!intersects) {
        line = QLineF(); // empty line to help with drawing
    }
}


void cropLineToConvexPolygon(QLineF &line, const QPolygonF polygon, bool extendFirst, bool extendSecond)
{
    bool intersects = intersectLineConvexPolygon(line, polygon, extendFirst, extendSecond);
    if (!intersects) {
        line = QLineF(); // empty line to help with drawing
    }
}


qreal findMinimumGoldenSection(std::function<qreal(qreal)> f, qreal xA, qreal xB, qreal eps, int maxIter = 100)
{
    // requirements:
    // only one local minimum between xA and xB

    int i = 0;
    const double phi = 0.618033988749894; // 1/(golden ratio)
    qreal a = xA;
    qreal b = xB;
    qreal c = b - (b - a)*phi;
    qreal d = a + (b - a)*phi;

    while (qAbs(b - a) > eps) {
        if (f(c) < f(d)) {
            b = d;
        } else {
            a = c;
        }

        // Wikipedia says to recompute both c and d here to avoid loss of precision which may lead to incorrect results or infinite loop
        c = b - (b - a) * phi;
        d = a + (b - a) * phi;

        i++;
        if (i > maxIter) {
            break;
        }

    }

    return (b + a)/2;
}


qreal findMinimumTernarySection(std::function<qreal(qreal)> f, qreal xA, qreal xB, qreal eps, int maxIter = 100)
{
    // requirements:
    // only one local minimum between xA and xB

    int i = 0;
    qreal l = qMin(xA, xB);
    qreal r = qMax(xA, xB);


    qreal m1 = l + (r - l)/3;
    qreal m2 = r - (r - l)/3;

    while ((r - l) > eps) {
        qreal f1 = f(m1);
        qreal f2 = f(m2);

        if (f1 > f2) {
            l = m1;
        } else {
            r = m2;
        }

        // In Golden Section, Wikipedia says to recompute both c and d here to avoid loss of precision which may lead to incorrect results or infinite loop
        // so it's probably a good idea to do it here too
        m1 = l + (r - l)/3;
        m2 = r - (r - l)/3;

        i++;

        if (i > maxIter) {
            break;
        }
    }

    return (l + r)/2;
}


qreal pointToLineDistSquared(const QPointF &pt, const QLineF &line)
{
    // distance = |(p2 - p1) x (p1 - pt)| / |p2 - p1|

    // magnitude of (p2 - p1) x (p1 - pt)
    const qreal cross = (line.dx() * (line.y1() - pt.y()) - line.dy() * (line.x1() - pt.x()));

    return cross * cross / (line.dx() * line.dx() + line.dy() * line.dy());
}


QList<QLineF> getParallelLines(const QLineF& line, const qreal distance) {

    QPointF lineNormalVector = QPointF(line.dy(), -line.dx());
    QLineF line1 = QLineF(movePointInTheDirection(line.p1(), lineNormalVector, distance), movePointInTheDirection(line.p2(), lineNormalVector, distance));
    QLineF line2 = QLineF(movePointInTheDirection(line.p1(), -lineNormalVector, distance), movePointInTheDirection(line.p2(), -lineNormalVector, distance));

    if (line1.isNull() || line2.isNull()) {
        return QList<QLineF>();
    }
    return {line1, line2};
}


QPointF findNearestPointOnLine(const QPointF &point, const QLineF &line, bool unbounded)
{
    // TODO: can we reuse (or deduplicate with) KisBezierUtils.cpp:nearestPoint()?

    if (line.length() == 0) {
        return point;
    }

    QPointF lineVector = line.p2() - line.p1();
    QPointF lineNormalVector = QPointF(-lineVector.y(), lineVector.x());

    QLineF pointToLineNormalLine = QLineF(point, point + lineNormalVector);
    QPointF result;
    QLineF::IntersectionType intersectionType = pointToLineNormalLine.intersects(line, &result);
    if (unbounded || intersectionType == QLineF::IntersectionType::BoundedIntersection) {
        return result;
    }
    qreal distance1 = kisDistance(line.p1(), result);
    qreal distance2 = kisDistance(line.p2(), result);
    if (distance1 + distance2 <= line.length() || qFuzzyCompare(distance1 + distance2, line.length())) {
        return result; // it's still on the line
    }
    if (distance1 < distance2) {
        return line.p1();
    }
    return line.p2();
}


QPointF movePointInTheDirection(const QPointF &point, const QPointF &direction, qreal distance)
{
    QPointF response = point;
    QLineF line = QLineF(QPointF(0, 0), direction);
    if (qFuzzyCompare(line.length(), 0)) {
        return point;
    }
    return response + distance*direction/line.length();
}


QPointF movePointAlongTheLine(const QPointF &point, const QLineF &direction, qreal distance, bool ensureOnLine)
{
    QPointF response = point;
    if (ensureOnLine) {
        response = findNearestPointOnLine(point, direction);
    }
    return movePointInTheDirection(response, direction.p2() - direction.p1(), distance);
}




bool isOnLine(const QLineF &line, const QPointF &point, const qreal eps, bool boundedStart, bool boundedEnd, bool includeEnds)
{
    if (fuzzyPointCompare(point, line.p1(), eps)) {
        return includeEnds || !boundedStart;
    }
    if (fuzzyPointCompare(point, line.p2(), eps)) {
        return includeEnds || !boundedEnd;
    }

    if (kisDistanceToLine(point, line) > eps) {
        return false;
    }

    QPointF lineVector = line.p2() - line.p1();
    QPointF pointVector = point - line.p1();
    qreal t = -1;
    if (qAbs(pointVector.x()) < eps) {
        // use y
        t = pointVector.y()/lineVector.y();
    } else {
        t = pointVector.x()/lineVector.x();
    }

    if (t < 0) {
        return !boundedStart;
    }
    if (t > 1.0) {
        return !boundedEnd;
    }

    return true;
}



} // namespace KisAlgebra2D
