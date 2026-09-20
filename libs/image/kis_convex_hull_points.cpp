/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_convex_hull.h"

#include <boost/geometry.hpp>

namespace boost
{
namespace geometry
{
namespace traits
{

template<>
struct tag<QPoint> {
    using type = point_tag;
};

template<>
struct coordinate_type<QPoint> {
    using type = int;
};

template<>
struct coordinate_system<QPoint> {
    using type = cs::cartesian;
};

template<>
struct dimension<QPoint> : boost::mpl::int_<2> {
};

template<>
struct access<QPoint, 0> {
    static int get(const QPoint &point)
    {
        return point.x();
    }

    static void set(QPoint &point, const int &value)
    {
        point.rx() = value;
    }
};

template<>
struct access<QPoint, 1> {
    static int get(const QPoint &point)
    {
        return point.y();
    }

    static void set(QPoint &point, const int &value)
    {
        point.ry() = value;
    }
};

template<>
struct tag<QPolygon> {
    using type = linestring_tag;
};

} // namespace traits
} // namespace geometry

template<>
struct range_iterator<QPolygon> {
    using type = QPolygon::iterator;
};

template<>
struct range_const_iterator<QPolygon> {
    using type = QPolygon::const_iterator;
};

} // namespace boost

namespace KisConvexHull
{

QPolygon findConvexHull(const QVector<QPoint> &points)
{
    QPolygon hull;
    boost::geometry::convex_hull(QPolygon(points), hull);
    return hull;
}

} // namespace KisConvexHull
