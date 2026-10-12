/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_TILE_PRESENTATION_H
#define KIS_GPU_TILE_PRESENTATION_H

#include <kritacanvas_export.h>
#include <gpu/KisGpuTileRenderer.h>
#include <QColor>
#include <QTransform>
#include <memory>

class QRhiRenderTarget;

namespace Krita::Canvas::Gpu {

struct PresentationOutcome
{
    bool recorded = false;
    QString diagnostic;
    qsizetype tileCount = 0;
};

/**
 * Draws borrowed immutable tiles on their owning GPU thread and queue.
 * The QRhi outlives this owner. record() joins an active frame and clears the
 * target before drawing, without submission, completion waits or CPU pixels.
 * The transform maps document coordinates to physical target pixels.
 * Retain the projection through frame submission. Invalid input and resource
 * failures leave the target untouched. Output colors are premultiplied RGBA.
 */
class KRITACANVAS_EXPORT TilePresentation
{
public:
    explicit TilePresentation(QRhi &rhi, qsizetype maximumTiles = 1024);
    ~TilePresentation();
    TilePresentation(const TilePresentation &) = delete;
    TilePresentation &operator=(const TilePresentation &) = delete;

    PresentationOutcome record(QRhiCommandBuffer &commands,
                               const Image::Gpu::ProjectedSnapshot &projection,
                               QRhiRenderTarget &target, const QTransform &documentToPixels,
                               QColor clearColor);

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};
}
#endif
