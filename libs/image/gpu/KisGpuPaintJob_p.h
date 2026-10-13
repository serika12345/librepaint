/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PAINT_JOB_P_H
#define KIS_GPU_PAINT_JOB_P_H

#include "KisGpuTileStorage_p.h"
#include <array>
#include <optional>

namespace Krita::Image::Gpu::Detail {
struct PaintSegment
{
    qint32 ax, ay, bx, by;
    quint32 p0, p1, phase, length;
};
static_assert(sizeof(PaintSegment) == 32);

struct PaintParameters
{
    qint32 x, y, width, height;
    quint32 color, opacity, erase, step;
    quint32 count, diameter, uniformFill = 0, reserved = 0;
};
static_assert(sizeof(PaintParameters) == 48);

struct PaintJob
{
    std::shared_ptr<Tile> input, output;
    Resource<QRhiBuffer> samples, parameters;
    Resource<QRhiShaderResourceBindings> bindings;
    QList<PaintSegment> geometry;
    PaintParameters values{};
};

QList<PaintSegment> paintGeometry(const PaintOperation &operation, quint32 step);
std::optional<QList<quint64>> paintSegmentTiles(const PaintSegment &segment, quint32 diameter,
                                               QRect region, QSize canvas, qsizetype maximumCoordinates);
quint32 packedPixel(const std::array<quint8, 4> &rgba);
bool preparePaintJob(QRhi &rhi, PaintJob &job, QRhiBuffer *zero);
}
#endif
