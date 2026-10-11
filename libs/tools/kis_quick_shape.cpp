/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_quick_shape.h>

#include <QtGlobal>

#include <cmath>
#include <limits>

namespace {

// Gestures shorter than this cannot be told apart from a dot and are left
// unsnapped.
const qreal kMinLineLength = 8.0;

// A gesture counts as a line when every sample stays inside the larger of
// these two distances from the chord.
const qreal kLineResidualAbsolute = 3.0;
const qreal kLineResidualRatio = 0.06;

const qreal kMinEllipseRadius = 4.0;

// Mean absolute deviation of the normalized ellipse radius. The samples of a
// hand-drawn ellipse scatter around a value of one.
const qreal kEllipseResidualTolerance = 0.25;

// Largest start-to-end gap, relative to the major diameter, that still counts
// as a closed ellipse.
const qreal kEllipseClosureRatio = 0.5;

QPointF meanPosition(const QVector<QPointF> &points)
{
    QPointF sum;
    for (const QPointF &point : points) {
        sum += point;
    }
    return sum / points.size();
}

qreal dotProduct(const QPointF &a, const QPointF &b)
{
    return a.x() * b.x() + a.y() * b.y();
}

qreal meanEllipseResidual(const QVector<QPointF> &points,
                          const QPointF &center,
                          const QPointF &axis,
                          qreal radiusX,
                          qreal radiusY)
{
    const QPointF perpendicular(-axis.y(), axis.x());
    qreal sum = 0.0;

    for (const QPointF &point : points) {
        const QPointF offset = point - center;
        const qreal u = dotProduct(offset, axis) / radiusX;
        const qreal v = dotProduct(offset, perpendicular) / radiusY;
        sum += qAbs(std::sqrt(u * u + v * v) - 1.0);
    }

    return sum / points.size();
}

} // namespace

KisQuickShape KisQuickShape::recognize(const QVector<QPointF> &points)
{
    KisQuickShape result;

    if (points.size() < 2) {
        return result;
    }

    const QPointF start = points.first();
    const QPointF end = points.last();
    const QPointF chord = end - start;
    const qreal chordLength = std::hypot(chord.x(), chord.y());

    // Compare against the chord first so that a nearly straight gesture keeps
    // its endpoints instead of the slightly different principal axis.
    if (chordLength >= kMinLineLength) {
        const QPointF direction = chord / chordLength;
        qreal maxDeviation = 0.0;

        for (const QPointF &point : points) {
            const QPointF offset = point - start;
            const qreal perpendicular = qAbs(-offset.x() * direction.y() + offset.y() * direction.x());
            maxDeviation = qMax(maxDeviation, perpendicular);
        }

        const qreal tolerance = qMax(kLineResidualAbsolute, kLineResidualRatio * chordLength);
        if (maxDeviation <= tolerance) {
            result.m_type = Line;
            result.m_start = start;
            result.m_end = end;
            return result;
        }
    }

    // An ellipse needs enough samples to describe its curvature.
    if (points.size() < 5) {
        return result;
    }

    // Principal axes of the samples give a rotated ellipse that follows the
    // direction the user actually drew in.
    const QPointF mean = meanPosition(points);
    qreal sxx = 0.0;
    qreal sxy = 0.0;
    qreal syy = 0.0;

    for (const QPointF &point : points) {
        const QPointF offset = point - mean;
        sxx += offset.x() * offset.x();
        sxy += offset.x() * offset.y();
        syy += offset.y() * offset.y();
    }

    sxx /= points.size();
    sxy /= points.size();
    syy /= points.size();

    const qreal angle = 0.5 * std::atan2(2.0 * sxy, sxx - syy);
    const QPointF axis(std::cos(angle), std::sin(angle));
    const QPointF perpendicular(-axis.y(), axis.x());

    qreal minU = std::numeric_limits<qreal>::max();
    qreal maxU = -std::numeric_limits<qreal>::max();
    qreal minV = std::numeric_limits<qreal>::max();
    qreal maxV = -std::numeric_limits<qreal>::max();

    for (const QPointF &point : points) {
        const QPointF offset = point - mean;
        const qreal u = dotProduct(offset, axis);
        const qreal v = dotProduct(offset, perpendicular);
        minU = qMin(minU, u);
        maxU = qMax(maxU, u);
        minV = qMin(minV, v);
        maxV = qMax(maxV, v);
    }

    const qreal radiusX = (maxU - minU) / 2.0;
    const qreal radiusY = (maxV - minV) / 2.0;

    if (radiusX < kMinEllipseRadius || radiusY < kMinEllipseRadius) {
        return result;
    }

    const qreal size = 2.0 * qMax(radiusX, radiusY);
    if (chordLength > kEllipseClosureRatio * size) {
        return result;
    }

    const QPointF center = mean
        + axis * ((minU + maxU) / 2.0)
        + perpendicular * ((minV + maxV) / 2.0);

    if (meanEllipseResidual(points, center, axis, radiusX, radiusY) > kEllipseResidualTolerance) {
        return result;
    }

    result.m_type = Ellipse;
    result.m_center = center;
    result.m_radiusX = radiusX;
    result.m_radiusY = radiusY;
    result.m_rotation = angle;
    return result;
}

bool KisQuickShape::isValid() const
{
    return m_type != NoShape;
}

KisQuickShape::ShapeType KisQuickShape::type() const
{
    return m_type;
}

QPointF KisQuickShape::lineStart() const
{
    return m_start;
}

QPointF KisQuickShape::lineEnd() const
{
    return m_end;
}

QPointF KisQuickShape::center() const
{
    return m_center;
}

qreal KisQuickShape::radiusX() const
{
    return (m_snappedToCircle ? qMax(m_radiusX, m_radiusY) : m_radiusX) * m_scale;
}

qreal KisQuickShape::radiusY() const
{
    return (m_snappedToCircle ? qMax(m_radiusX, m_radiusY) : m_radiusY) * m_scale;
}

qreal KisQuickShape::rotation() const
{
    return m_rotation;
}

void KisQuickShape::setSnappedToCircle(bool value)
{
    m_snappedToCircle = value;
}

bool KisQuickShape::isSnappedToCircle() const
{
    return m_snappedToCircle;
}

bool KisQuickShape::setLineEnd(const QPointF &position)
{
    if (m_type != Line || position == m_end) {
        return false;
    }

    m_end = position;
    return true;
}

bool KisQuickShape::setScale(qreal factor)
{
    if (m_type != Ellipse || !(factor > 0.0)) {
        return false;
    }

    /**
     * The smallest fitted radius defines how far the ellipse may shrink, so
     * that the shape never collapses into nothing.
     */
    const qreal smallestRadius = qMin(m_radiusX, m_radiusY);
    const qreal minFactor = smallestRadius > 0.0
        ? qMin(qreal(1.0), qreal(2.0) / smallestRadius)
        : qreal(1.0);
    const qreal scale = qBound(minFactor, factor, qreal(100.0));

    if (qFuzzyCompare(scale, m_scale)) {
        return false;
    }

    m_scale = scale;
    return true;
}

qreal KisQuickShape::scale() const
{
    return m_scale;
}

bool KisQuickShape::setRotation(qreal angle)
{
    if (m_type != Ellipse || !std::isfinite(angle)) {
        return false;
    }

    const qreal rotation = std::remainder(angle, 2.0 * M_PI);
    if (qFuzzyCompare(rotation, m_rotation)) {
        return false;
    }

    m_rotation = rotation;
    return true;
}

void KisQuickShapeTracker::begin(const QPointF &position)
{
    reset();
    m_points.append(position);
    m_holdAnchor = position;
    m_referencePoint = position;
    m_lastAdjustPosition = position;
    m_tracking = true;
}

bool KisQuickShapeTracker::extend(const QPointF &position)
{
    if (!m_tracking) {
        return false;
    }

    m_points.append(position);

    const QPointF moved = position - m_holdAnchor;
    if (std::hypot(moved.x(), moved.y()) <= m_holdSlop) {
        return false;
    }

    m_holdAnchor = position;
    return true;
}

void KisQuickShapeTracker::reset()
{
    m_points.clear();
    m_holdAnchor = QPointF();
    m_referencePoint = QPointF();
    m_lastAdjustPosition = QPointF();
    m_referenceRotation = 0.0;
    m_shape = KisQuickShape();
    m_tracking = false;
}

bool KisQuickShapeTracker::isTracking() const
{
    return m_tracking;
}

bool KisQuickShapeTracker::recognize()
{
    if (!m_tracking) {
        return false;
    }

    const KisQuickShape shape = KisQuickShape::recognize(m_points);
    if (!shape.isValid()) {
        return false;
    }

    m_shape = shape;
    m_referencePoint = m_points.last();
    m_lastAdjustPosition = m_points.last();
    m_referenceRotation = shape.rotation();
    m_tracking = false;
    return true;
}

const KisQuickShape &KisQuickShapeTracker::shape() const
{
    return m_shape;
}

bool KisQuickShapeTracker::toggleCircle()
{
    if (!m_shape.isValid() || m_shape.type() != KisQuickShape::Ellipse) {
        return false;
    }

    m_shape.setSnappedToCircle(!m_shape.isSnappedToCircle());
    return true;
}

bool KisQuickShapeTracker::adjustTo(const QPointF &position)
{
    if (!m_shape.isValid()) {
        return false;
    }

    if (m_shape.type() == KisQuickShape::Line) {
        return m_shape.setLineEnd(position);
    }

    // Ignore the hand tremor that keeps the pointer inside the hold slop, so
    // that the shape is not rebuilt for every incoming event.
    const QPointF moved = position - m_lastAdjustPosition;
    if (std::hypot(moved.x(), moved.y()) <= m_holdSlop) {
        return false;
    }

    m_lastAdjustPosition = position;

    const QPointF center = m_shape.center();
    const QPointF reference = m_referencePoint - center;
    const QPointF current = position - center;
    const qreal referenceDistance = std::hypot(reference.x(), reference.y());
    const qreal currentDistance = std::hypot(current.x(), current.y());

    if (referenceDistance < 1.0) {
        return false;
    }

    const bool scaled = m_shape.setScale(currentDistance / referenceDistance);
    bool rotated = false;
    if (currentDistance > 1e-3) {
        const qreal angle = std::atan2(reference.x() * current.y() - reference.y() * current.x(),
                                      dotProduct(reference, current));
        rotated = m_shape.setRotation(m_referenceRotation + angle);
    }
    return scaled || rotated;
}

void KisQuickShapeTracker::setHoldSlop(qreal slop)
{
    m_holdSlop = qMax(qreal(0.0), slop);
}

int KisQuickShapeTracker::pointCount() const
{
    return m_points.size();
}
