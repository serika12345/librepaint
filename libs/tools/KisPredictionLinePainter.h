/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PREDICTION_LINE_PAINTER_H
#define KIS_PREDICTION_LINE_PAINTER_H

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include <kritatools_export.h>

class QPainter;

/**
 * Appearance of the display-only prediction line.
 *
 * The diameter is in the same unit as the polyline coordinates, which are
 * document pixels. When testColorRgb is not -1 it holds an opaque 0xRRGGBB
 * test color and the painter draws the overlap interval thin and the
 * prediction interval in the test color, so that a screen capture can tell
 * the prediction line from the committed stroke.
 */
struct KisPredictionLineStyle {
    QColor color;               // color of the committed stroke
    qreal diameter = 1.0;       // width in document pixels
    int testColorRgb = -1;      // 0xRRGGBB, -1 for the normal appearance
};

/**
 * Draws the prediction line as one polyline. The painter keeps its own
 * transform, so the polyline is expected in document coordinates. This
 * painter does not touch the paint device; the caller decides where the
 * painter draws.
 */
namespace KisPredictionLinePainter {
/// gc already carries the transform from polyline coordinates to screen
/// coordinates. This function does not change the transform.
KRITATOOLS_EXPORT void paint(QPainter &gc, const QVector<QPointF> &polyline, int latestIndex, const KisPredictionLineStyle &style);
/// Bounding rectangle of the polyline grown by half the diameter plus 2.
/// Empty when there are fewer than two points.
KRITATOOLS_EXPORT QRectF bounds(const QVector<QPointF> &polyline, qreal diameter);
}

#endif // KIS_PREDICTION_LINE_PAINTER_H
