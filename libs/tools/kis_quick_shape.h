/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_QUICK_SHAPE_H
#define KIS_QUICK_SHAPE_H

#include <QPointF>
#include <QVector>

#include <kritatools_export.h>

/**
 * Shape fitted to a freehand gesture the user held in place.
 *
 * Positions use image pixel coordinates. An ellipse is described by its
 * center, two radii, and a rotation in radians so that the caller can render
 * the fitted shape without repeating the sample analysis. Enabling the circle
 * snap keeps the fitted radii and reports the larger one on both axes.
 */
class KRITATOOLS_EXPORT KisQuickShape
{
public:
    enum ShapeType {
        NoShape,
        Line,
        Ellipse
    };

    /**
     * Fits a line or an ellipse to \p points. Returns an invalid shape when
     * the samples do not form a recognizable line or closed ellipse.
     */
    static KisQuickShape recognize(const QVector<QPointF> &points);

    bool isValid() const;
    ShapeType type() const;

    QPointF lineStart() const;
    QPointF lineEnd() const;

    QPointF center() const;
    qreal radiusX() const;
    qreal radiusY() const;
    qreal rotation() const;

    void setSnappedToCircle(bool value);
    bool isSnappedToCircle() const;

    /**
     * Sets the line endpoint to \p position while keeping its start fixed.
     * Returns true when the endpoint changed.
     */
    bool setLineEnd(const QPointF &position);

    /**
     * Scales the fitted ellipse about its center. Returns true when the
     * resulting size changed.
     */
    bool setScale(qreal factor);
    qreal scale() const;

    /** Sets the ellipse rotation in radians. Returns true when it changed. */
    bool setRotation(qreal angle);

private:
    ShapeType m_type {NoShape};
    QPointF m_start;
    QPointF m_end;
    QPointF m_center;
    qreal m_radiusX {0.0};
    qreal m_radiusY {0.0};
    qreal m_rotation {0.0};
    qreal m_scale {1.0};
    bool m_snappedToCircle {false};
};

/**
 * Collects the pointer samples of one gesture and detects the hold that
 * requests a quick shape.
 *
 * The UI owner feeds every converted pointer position to extend() and restarts
 * its hold timer when that call reports that the pointer moved beyond the hold
 * slop. When the timer elapses without further motion, recognize() fits the
 * collected samples.
 */
class KRITATOOLS_EXPORT KisQuickShapeTracker
{
public:
    static constexpr qreal DefaultHoldSlop = 3.0;

    void begin(const QPointF &position);

    /**
     * Adds one sample and returns true when the pointer moved further than the
     * hold slop from the position where the hold was last restarted.
     */
    bool extend(const QPointF &position);
    void reset();

    bool isTracking() const;

    /**
     * Fits a shape to the collected samples and stores it. Returns true when a
     * shape was recognized; tracking then stops.
     */
    bool recognize();

    const KisQuickShape &shape() const;

    /**
     * Switches the recognized ellipse between the fitted radii and a circle.
     * Returns true when a recognized ellipse changed.
     */
    bool toggleCircle();

    /**
     * Applies the pointer position to the recognized shape: a line keeps its
     * start fixed and follows the pointer with its endpoint. An ellipse scales
     * and rotates about its center relative to the place where the hold was detected.
     * Returns true when the shape changed.
     */
    bool adjustTo(const QPointF &position);

    void setHoldSlop(qreal slop);
    int pointCount() const;

private:
    QVector<QPointF> m_points;
    QPointF m_holdAnchor;
    QPointF m_referencePoint;
    QPointF m_lastAdjustPosition;
    qreal m_referenceRotation {0.0};
    qreal m_holdSlop {DefaultHoldSlop};
    KisQuickShape m_shape;
    bool m_tracking {false};
};

#endif // KIS_QUICK_SHAPE_H
