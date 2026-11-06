/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisQuickShapePreview.h"
#include <QPainter>
#include <kis_quick_shape.h>
#include <cmath>

QRectF KisQuickShapePreview::update(const KisQuickShape &shape)
{
    const QRectF previous = bounds();
    QPainterPath path;
    m_closed = shape.type() == KisQuickShape::Ellipse;
    if (shape.type() == KisQuickShape::Line) {
        path.moveTo(shape.lineStart());
        path.lineTo(shape.lineEnd());
    } else if (m_closed) {
        path.addEllipse(QPointF(), shape.radiusX(), shape.radiusY());
        QTransform transform;
        transform.translate(shape.center().x(), shape.center().y());
        transform.rotateRadians(shape.rotation());
        path = transform.map(path);
    }
    m_path = path;
    m_frame = QImage();
    return previous.united(bounds());
}

QRectF KisQuickShapePreview::setStrokeSample(const QImage &sample, const QLineF &sampleLine, qreal sampleScale)
{
    const QRectF previous = bounds();
    // Expand the sampling grid without changing coverage. Qt then filters the
    // image at subpixel positions, including on displays with pixel ratio 2.
    const QImage image = sample.scaled(sample.size() * 4, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    const QLineF line(sampleLine.p1() * 4, sampleLine.p2() * 4);
    const qreal scale = sampleScale / 4;
    m_sampleScale = scale;
    m_halfWidth = image.height() * scale / 2.0;
    const int cap = image.height();
    const int start = qRound(line.x1() + cap / 2.0);
    const int end = qRound(line.x2() - cap / 2.0);
    m_body = image.copy(QRect(start, 0, qMax(1, end - start), image.height()))
                 .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_head = image.copy(QRect(qRound(line.x1() - cap / 2.0), 0, cap, cap))
                 .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_tail = image.copy(QRect(qRound(line.x2() - cap / 2.0), 0, cap, cap))
                 .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_frame = QImage();
    return previous.united(bounds());
}

QRectF KisQuickShapePreview::clear()
{
    const QRectF previous = bounds();
    m_path = QPainterPath();
    m_body = m_head = m_tail = m_frame = QImage();
    m_halfWidth = 0;
    return previous;
}

QRectF KisQuickShapePreview::bounds() const
{
    if (m_path.elementCount() == 0 || m_body.isNull()) return QRectF();
    return m_path.boundingRect().adjusted(-m_halfWidth, -m_halfWidth, m_halfWidth, m_halfWidth);
}

const QPainterPath &KisQuickShapePreview::path() const { return m_path; }

void KisQuickShapePreview::rasterize(const QTransform &transform, const QRect &viewport, qreal dpr) const
{
    m_frameTransform = transform;
    m_frameViewport = viewport;
    m_frameDpr = dpr;
    const qreal zoom = std::hypot(transform.m11(), transform.m12());
    const qreal halfWidth = m_halfWidth * zoom;
    const qreal sampleScale = m_sampleScale * zoom;
    const QPainterPath path = transform.map(m_path);
    m_frameBounds = path.boundingRect().adjusted(-halfWidth - 1, -halfWidth - 1, halfWidth + 1, halfWidth + 1)
                       .toAlignedRect().intersected(viewport);
    if (m_frameBounds.isEmpty() || sampleScale <= 0) return;
    /**
     * The raster covers the whole figure, so its cost grows with the on-screen
     * area. Bound the device pixels of the preview; a figure that fills the
     * viewport is drawn slightly softer instead of dropping frames.
     */
    const qint64 maxFrameDevicePixels = 1000 * 1000;
    qreal effectiveDpr = dpr;
    const qint64 requestedDevicePixels =
        qint64(m_frameBounds.width()) * m_frameBounds.height() * qRound64(dpr * dpr * 1000) / 1000;
    if (requestedDevicePixels > maxFrameDevicePixels) {
        effectiveDpr = dpr * std::sqrt(qreal(maxFrameDevicePixels) / qreal(requestedDevicePixels));
    }
    m_frame = QImage(QSize(qCeil(m_frameBounds.width() * effectiveDpr), qCeil(m_frameBounds.height() * effectiveDpr)),
                     QImage::Format_ARGB32_Premultiplied);
    m_frame.setDevicePixelRatio(effectiveDpr);
    m_frame.fill(Qt::transparent);
    QPainter raster(&m_frame);
    raster.translate(-m_frameBounds.topLeft());
    // Sample edges already contain coverage. Internal mesh edges are shared;
    // replacing pixels avoids darker seams where neighboring strips meet.
    raster.setCompositionMode(QPainter::CompositionMode_Source);
    raster.setRenderHint(QPainter::SmoothPixmapTransform);
    const auto normalized = [](QPointF v) {
        const qreal length = std::hypot(v.x(), v.y());
        return length > 0 ? v / length : QPointF(1, 0);
    };
    for (QPolygonF points : path.toSubpathPolygons()) {
        if (points.size() < 2) continue;
        if (m_closed && points.first() == points.last()) points.removeLast();
        const int count = points.size();
        QVector<QPointF> normals;
        for (int i = 0; i < count; ++i) {
            const QPointF previous = points[m_closed ? (i + count - 1) % count : qMax(0, i - 1)];
            const QPointF next = points[m_closed ? (i + 1) % count : qMin(count - 1, i + 1)];
            const QPointF tangent = normalized(next - previous);
            normals.append(QPointF(-tangent.y(), tangent.x()) * halfWidth);
        }
        qreal offset = 0;
        for (int i = 0; i < count - (m_closed ? 0 : 1); ++i) {
            const int next = (i + 1) % count;
            const qreal length = QLineF(points[i], points[next]).length();
            if (length <= 0) continue;
            QPolygonF destination {points[i] - normals[i], points[next] - normals[next],
                                   points[next] + normals[next], points[i] + normals[i]};
            QPolygonF source {QPointF(offset, 0), QPointF(offset + length / sampleScale, 0),
                              QPointF(offset + length / sampleScale, m_body.height()), QPointF(offset, m_body.height())};
            QTransform mapping;
            if (QTransform::quadToQuad(source, destination, mapping)) {
                QPainterPath strip;
                strip.addPolygon(destination);
                strip.closeSubpath();
                raster.save();
                raster.setClipPath(strip, Qt::IntersectClip);
                raster.setTransform(mapping, true);
                const int firstTile = qFloor(offset / m_body.width());
                const int lastTile = qFloor((offset + length / sampleScale) / m_body.width());
                for (int tile = firstTile; tile <= lastTile; ++tile) {
                    raster.drawImage(QPointF(tile * m_body.width(), 0), m_body);
                }
                raster.restore();
            }
            offset += length / sampleScale;
        }
        if (!m_closed) {
            const auto cap = [&](const QPointF &point, const QPointF &tangent, const QImage &image) {
                raster.save();
                raster.translate(point);
                raster.rotate(std::atan2(tangent.y(), tangent.x()) * 180 / M_PI);
                raster.drawImage(QRectF(-halfWidth, -halfWidth, halfWidth * 2, halfWidth * 2), image);
                raster.restore();
            };
            cap(points.first(), points[1] - points[0], m_head);
            cap(points.last(), points.last() - points[count - 2], m_tail);
        }
    }

}

void KisQuickShapePreview::paint(QPainter &painter, const QTransform &transform, const QRect &viewport) const
{
    if (m_body.isNull() || m_path.elementCount() == 0) return;
    const qreal dpr = painter.device()->devicePixelRatioF();
    if (m_frame.isNull() || m_frameTransform != transform || m_frameViewport != viewport || m_frameDpr != dpr) {
        rasterize(transform, viewport, dpr);
    }
    if (m_frame.isNull()) return;
    painter.save();
    painter.setOpacity(1.0);
    painter.drawImage(m_frameBounds.topLeft(), m_frame);
    painter.restore();
}

qint64 KisQuickShapePreview::frameDevicePixelCount() const
{
    return qint64(m_frame.width()) * m_frame.height();
}
