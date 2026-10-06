/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_QUICK_SHAPE_PREVIEW_H
#define KIS_QUICK_SHAPE_PREVIEW_H
#include <QPainterPath>
#include <QImage>
#include <QLineF>
#include <QTransform>
class KisQuickShape;
class QPainter;

/**
 * Display-only figure overlay. A captured brush sample supplies color, opacity
 * and texture. Geometry changes map that sample along the outline at constant
 * width. The screen-space raster is bounded by the viewport and cached between
 * paints. This owner depends only on geometry and Qt GUI values.
 */
class KisQuickShapePreview
{
public:
    QRectF update(const KisQuickShape &shape);
    QRectF setStrokeSample(const QImage &image, const QLineF &sampleLine, qreal imageUnitsPerSamplePixel);
    QRectF clear();
    const QPainterPath &path() const;
    QRectF bounds() const;
    void paint(QPainter &painter, const QTransform &imageToView, const QRect &viewport) const;

    /**
     * Device pixels of the raster painted last. The value stays inside the
     * internal cap, so a figure that fills the viewport does not scale the
     * preview work with the screen area.
     */
    qint64 frameDevicePixelCount() const;

private:
    void rasterize(const QTransform &imageToView, const QRect &viewport, qreal devicePixelRatio) const;
    QPainterPath m_path;
    QImage m_body;
    QImage m_head;
    QImage m_tail;
    qreal m_sampleScale {1.0};
    qreal m_halfWidth {0.0};
    bool m_closed {false};
    mutable QImage m_frame;
    mutable QRect m_frameBounds;
    mutable QTransform m_frameTransform;
    mutable QRect m_frameViewport;
    mutable qreal m_frameDpr {0.0};
};
#endif
