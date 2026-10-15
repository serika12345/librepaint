/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PAINT_JOB_P_H
#define KIS_GPU_PAINT_JOB_P_H

#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <array>

namespace LibrePaint::Image::Gpu::Detail {
struct PaintParameters
{
    qint32 x, y, width, height;
    quint32 color, opacity, erase, step;
    quint32 count, diameter, uniformFill = 0, alphaLocked = 0;
};
static_assert(sizeof(PaintParameters) == 48);

struct PaintJob
{
    std::shared_ptr<Tile> input, output;
    NativeBuffer samples, parameters;
    NativeBindings bindings;
    QList<PaintSegment> geometry;
    PaintParameters values{};
};

quint32 packedPixel(const std::array<quint8, 4> &rgba);
bool preparePaintJob(Device &device, WGPUBindGroupLayout layout, PaintJob &job, WGPUBuffer zero);
}
#endif
