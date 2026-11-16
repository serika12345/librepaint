/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "canvas/KisPredictionDecoration.h"

#include <QPainter>

#include <KisLatencyProbe.h>
#include <KisPredictionLinePainter.h>
#include <KisPredictionTestSwitches.h>

#include "canvas/kis_canvas2.h"
#include "kis_coordinates_converter.h"

const QString KisPredictionDecoration::idTag = "stroke_prediction_decoration";

KisPredictionDecoration::KisPredictionDecoration(QPointer<KisView> view)
    : KisCanvasDecoration(idTag, view)
{
    setVisible(true);
}

QRectF KisPredictionDecoration::setPrediction(const QVector<QPointF> &polyline, int latestIndex, qreal diameter, const QColor &color)
{
    const QRectF oldBounds = m_bounds;

    m_polyline = polyline;
    m_latestIndex = latestIndex;
    m_diameter = diameter;
    m_color = color;
    m_bounds = KisPredictionLinePainter::bounds(polyline, diameter);

    if (oldBounds.isEmpty()) {
        return m_bounds;
    }
    return oldBounds.united(m_bounds);
}

QRectF KisPredictionDecoration::clearPrediction()
{
    const QRectF oldBounds = m_bounds;

    m_polyline.clear();
    m_latestIndex = -1;
    m_bounds = QRectF();

    return oldBounds;
}

bool KisPredictionDecoration::hasPrediction() const
{
    return !m_bounds.isEmpty();
}

void KisPredictionDecoration::drawDecoration(QPainter &gc, const QRectF &updateArea, const KisCoordinatesConverter *converter, KisCanvas2 *canvas)
{
    Q_UNUSED(canvas);

    if (!hasPrediction()) {
        return;
    }

    if (!updateArea.intersects(m_bounds)) {
        return;
    }

    KisPredictionLineStyle style;
    style.color = m_color;
    style.diameter = m_diameter;
    style.testColorRgb = KisPredictionTestSwitches::testColorRgb();

    gc.save();
    gc.setTransform(converter->documentToWidgetTransform(), true);
    KisPredictionLinePainter::paint(gc, m_polyline, m_latestIndex, style);
    gc.restore();

    KisLatencyProbe::mark("prediction_paint", m_bounds.width(), m_bounds.height());
}
