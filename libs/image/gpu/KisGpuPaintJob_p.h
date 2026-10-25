/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PAINT_JOB_P_H
#define KIS_GPU_PAINT_JOB_P_H

#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include "KisGpuGaussianBrush_p.h"
#include <array>
#include <vector>

namespace LibrePaint::Image::Gpu::Detail {
struct PaintParameters
{
    qint32 x, y, paddingX, paddingY;
    quint32 color, opacity, erase, step;
    quint32 count, diameter, uniformFill = 0, paintFlags = 30;
    qint32 mirrorX = 0, mirrorY = 0;
    quint32 fillColor = 0, figureFlags = 0;
    qint32 selectionX = 0, selectionY = 0, selectionWidth = 0, selectionHeight = 0;
    quint32 patternWidth = 0, patternHeight = 0, fillBlendMode = 0, strokeBlendMode = 0;
};
static_assert(sizeof(PaintParameters) == 96);

struct PaintJob
{
    std::shared_ptr<Tile> input, output;
    std::shared_ptr<Tile> coverageInput, coverageOutput;
    NativeBuffer inputs;
    std::shared_ptr<NativeBuffer> fillEdges;
    /** Frozen selection coverage for this tile; null binds the zero tile. */
    std::shared_ptr<NativeBuffer> selectionCoverage;
    /** Frozen pattern pixels for this operation; null binds the unused tile. */
    std::shared_ptr<NativeBuffer> pattern;
    NativeBindings bindings;
    QList<PaintSegment> geometry;
    QList<GaussianDab> gaussianGeometry;
    QList<PaintSegment> appendedGeometry;
    quint64 coverageCoordinate = 0;
    bool retainCoverage = false;
    bool extendsCoverage = false;
    PaintParameters values{};
};

quint32 packedPixel(const std::array<quint8, 4> &rgba);
bool preparePaintJobs(Device &device, WGPUBindGroupLayout layout, std::vector<PaintJob> &jobs,
                      std::size_t first, WGPUBuffer zero, WGPUBuffer unusedCoverage,
                      WGPUBuffer unusedSelection, WGPUBuffer unusedPattern);
}
#endif
