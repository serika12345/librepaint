/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_QPAINTER_CANVAS_DRAW_IMAGE_H
#define KIS_QPAINTER_CANVAS_DRAW_IMAGE_H

#include <QImage>
#include <QPainter>
#include <QRect>

#include <kis_coordinates_converter.h>

namespace KisQPainterCanvasImage
{
inline void draw(QPainter &painter,
                 const KisCoordinatesConverter &converter,
                 const QImage &projection,
                 const QRect &updateWidgetRect)
{
    painter.setTransform(converter.viewportToWidgetTransform());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF viewportRect = converter.widgetToViewport(updateWidgetRect);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(viewportRect, projection, viewportRect);
}
} // namespace KisQPainterCanvasImage

#endif
