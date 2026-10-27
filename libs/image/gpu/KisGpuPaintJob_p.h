/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PAINT_JOB_P_H
#define KIS_GPU_PAINT_JOB_P_H

#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include "KisGpuGaussianBrush_p.h"
#include "KisGpuMarkerBrush_p.h"
#include <variant>
#include <array>
#include <vector>

namespace LibrePaint::Image::Gpu::Detail {
struct PaintParameters
{
    qint32 x = 0, y = 0;
    quint32 color = 0, canvasOpacityBits = 0;
    quint32 opacityValue = 255, erase = 0, count = 0, paintFlags = 30;
    qint32 mirrorX = 0, mirrorY = 0;
    quint32 fillColor = 0, figureFlags = 0;
    qint32 selectionX = 0, selectionY = 0, selectionWidth = 0, selectionHeight = 0;
    quint32 patternWidth = 0, patternHeight = 0, fillBlendMode = 0, strokeBlendMode = 0;
    quint32 colorHigh = 0, fillColorHigh = 0, pixelFormat = 0, padding = 0;
};
static_assert(sizeof(PaintParameters) == 96);
constexpr quint32 UniformPixelFill = 16384u;

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
    std::variant<QList<MarkerDab>, QList<GaussianDab>> geometry;
    QList<MarkerDab> appendedGeometry;
    quint64 coverageCoordinate = 0;
    bool retainCoverage = false;
    bool extendsCoverage = false;
    PaintParameters values{};
};

quint32 packedPixel(const RgbaPixel &rgba);
std::array<quint32, 2> packedPixelWords(const RgbaPixel &rgba, PixelFormat format);
bool preparePaintJobs(Device &device, WGPUBindGroupLayout layout, std::vector<PaintJob> &jobs,
                      std::size_t first, WGPUBuffer zero, WGPUBuffer unusedCoverage,
                      WGPUBuffer unusedSelection, WGPUBuffer unusedPattern);
}
#endif
