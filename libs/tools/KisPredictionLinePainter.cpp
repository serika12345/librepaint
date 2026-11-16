/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPredictionLinePainter.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

#include <algorithm>
#include <cmath>

namespace
{

bool hasNonFinitePoint(const QVector<QPointF> &polyline)
{
    for (const QPointF &point : polyline) {
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
            return true;
        }
    }
    return false;
}

QPainterPath makePath(const QVector<QPointF> &polyline, int first, int last)
{
    QPainterPath path;
    path.moveTo(polyline.at(first));
    for (int i = first + 1; i <= last; ++i) {
        path.lineTo(polyline.at(i));
    }
    return path;
}

QPen makePen(const QColor &color, qreal width)
{
    QPen pen(color);
    pen.setWidthF(width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

} // namespace

namespace KisPredictionLinePainter
{

void paint(QPainter &gc, const QVector<QPointF> &polyline, int latestIndex, const KisPredictionLineStyle &style)
{
    if (polyline.size() < 2 || hasNonFinitePoint(polyline)) {
        return;
    }

    const int lastIndex = static_cast<int>(polyline.size()) - 1;

    gc.save();
    gc.setRenderHint(QPainter::Antialiasing, true);

    if (style.testColorRgb == -1) {
        gc.setPen(makePen(style.color, style.diameter));
        gc.drawPath(makePath(polyline, 0, lastIndex));
    } else {
        // Test colour: only the predicted part is drawn. The part that overlaps
        // the displayed line is left out, so the displayed line keeps its own
        // pixels and a capture sees where it ends and where the prediction starts.
        if (latestIndex >= 0 && latestIndex < lastIndex) {
            QColor color = QColor::fromRgb(static_cast<QRgb>(style.testColorRgb));
            color.setAlpha(255);
            gc.setPen(makePen(color, style.diameter));
            gc.drawPath(makePath(polyline, latestIndex, lastIndex));
        }
    }

    gc.restore();
}

QRectF bounds(const QVector<QPointF> &polyline, qreal diameter)
{
    if (polyline.size() < 2) {
        return QRectF();
    }

    qreal minX = polyline.first().x();
    qreal maxX = minX;
    qreal minY = polyline.first().y();
    qreal maxY = minY;

    for (const QPointF &point : polyline) {
        minX = std::min(minX, point.x());
        maxX = std::max(maxX, point.x());
        minY = std::min(minY, point.y());
        maxY = std::max(maxY, point.y());
    }

    const qreal grow = diameter / 2.0 + 2.0;
    return QRectF(QPointF(minX - grow, minY - grow),
                  QPointF(maxX + grow, maxY + grow));
}

} // namespace KisPredictionLinePainter
