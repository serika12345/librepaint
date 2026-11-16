/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PREDICTION_DECORATION_H
#define KIS_PREDICTION_DECORATION_H

#include "canvas/kis_canvas_decoration.h"

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <QVector>

class KisPredictionDecoration : public KisCanvasDecoration
{
public:
    static const QString idTag;   // "stroke_prediction_decoration"

    explicit KisPredictionDecoration(QPointer<KisView> view);

    /// Places the polyline (document coordinates) and returns the document
    /// rectangle that needs repainting: the union of the previous and the new
    /// bounding rectangle.
    QRectF setPrediction(const QVector<QPointF> &polyline, int latestIndex, qreal diameter, const QColor &color);
    /// Removes the polyline and returns the document rectangle that needs
    /// repainting: the previous bounding rectangle, empty when there was none.
    QRectF clearPrediction();
    bool hasPrediction() const;

    void drawDecoration(QPainter &gc, const QRectF &updateArea, const KisCoordinatesConverter *converter, KisCanvas2 *canvas) override;

private:
    QVector<QPointF> m_polyline;
    int m_latestIndex = -1;
    qreal m_diameter = 1.0;
    QColor m_color;
    QRectF m_bounds;
};

#endif // KIS_PREDICTION_DECORATION_H
