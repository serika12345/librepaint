/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_TILE_PRESENTATION_H
#define KIS_GPU_TILE_PRESENTATION_H

#include <kritacanvas_export.h>
#include <gpu/KisGpuTileRenderer.h>
#include <QColor>
#include <QPolygonF>
#include <QTransform>
#include <memory>


namespace LibrePaint::Canvas::Gpu {

/** Borrowed native target; its owner and texture view remain alive through submission. */
struct PresentationTarget
{
    Image::Gpu::DevicePtr device;
    WGPUTextureView view = nullptr;
    QSize size;
    quint32 format = 0;
};

struct PresentationOutcome
{
    bool recorded = false;
    QString diagnostic;
    qsizetype tileCount = 0;
};

/** View-only RGBA bits 0 through 3. A single color component is gray by default,
 * preserving source alpha. Color display applies the selected alpha bit.
 */
struct DisplayChannels
{
    quint8 rgbaMask = 15;
    bool showSingleChannelAsColor = false;
};

/** Display-only contours in physical target pixels, independent of the document transform.
 * Thickness is measured in physical pixels; cursor RGB controls contrast against
 * the displayed image while preserving destination alpha.
 * One frame admits at most 65536 contour points and generated triangle vertices.
 */
struct ToolOutline
{
    QList<QPolygonF> contours;
    QColor color{Qt::white};
    qreal thickness = 1;
};

/**
 * Draws borrowed immutable tiles on their owning GPU thread and queue.
 * Display resources keep the shared device alive. record() joins an active frame and clears the
 * target before drawing, without submission, completion waits or CPU pixels.
 * The transform maps document coordinates to physical target pixels.
 * Retain the projection through frame submission. Invalid input and resource
 * failures leave the target untouched. Output colors are premultiplied RGBA.
 * Channel selection changes the presentation target while retaining projection pixels.
 */
class KRITACANVAS_EXPORT TilePresentation
{
public:
    explicit TilePresentation(Image::Gpu::DevicePtr device, qsizetype maximumTiles = 1024);
    ~TilePresentation();
    TilePresentation(const TilePresentation &) = delete;
    TilePresentation &operator=(const TilePresentation &) = delete;

    PresentationOutcome record(WGPUCommandEncoder commands,
                               const Image::Gpu::ProjectedSnapshot &projection,
                               const PresentationTarget &target, const QTransform &documentToPixels,
                               QColor clearColor, DisplayChannels channels = {},
                               const ToolOutline &outline = {});

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};
}
#endif
