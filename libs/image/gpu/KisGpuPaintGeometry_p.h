/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_GEOMETRY_P_H
#define KIS_GPU_PAINT_GEOMETRY_P_H

#include "KisGpuOperationDocument.h"
#include "KisGpuBasicCurveControls.h"
#include "KisGpuRgba8Compositing_p.h"
#include "KisGpuRgba16Compositing_p.h"
#include <QRect>
#include <optional>

namespace LibrePaint::Image::Gpu::Detail {
constexpr int PixelHistoryBlockSize = 64;
inline bool hasCopyBlend(const PaintOperation &operation)
{
    return operation.brush.blendMode == LayerBlendMode::Copy ||
        (hasFigureFill(operation.figureStyle) && operation.figureStyle.fillBlendMode == LayerBlendMode::Copy);
}
inline bool usesAllocatedSourceRegion(const PaintOperation &operation, PixelFormat format)
{
    const auto replacesAlpha = [](LayerBlendMode mode) {
        return mode == LayerBlendMode::Copy || mode == LayerBlendMode::DestinationIn || mode == LayerBlendMode::DestinationAtop ||
            transparentSourceAffectsBlend(mode);
    };
    const bool partialWordChannels = format == PixelFormat::Rgba16 && usesSeparableRgba16Blend(operation.brush.blendMode) &&
        (operation.brush.alphaLocked || operation.brush.channelWriteMask != 15);
    return !operation.brush.erase && (partialWordChannels || replacesAlpha(operation.brush.blendMode) ||
        (hasFigureFill(operation.figureStyle) && replacesAlpha(operation.figureStyle.fillBlendMode)));
}
inline int pixelTileIndex(int value, int size)
{ return value >= 0 ? value / size : (value - size + 1) / size; }

struct FigureFillEdge
{
    qint32 ax, ay, bx, by;
    /** Per-coordinate correction for reflecting raw positions before rounding. */
    std::array<qint32, 4> mirroredRounding;
};
static_assert(sizeof(FigureFillEdge) == 32);

inline quint64 key(int x, int y) { return (quint64(quint32(y)) << 32) | quint32(x); }
inline QPoint coordinate(quint64 key) { return {int(quint32(key)), int(quint32(key >> 32))}; }

constexpr qsizetype PaintGeometryElementLimit = 1024 * 1024;
/** Derived positions/pressure preserve raw input; excessive replay returns nullopt. */
std::optional<QList<InputSample>> weightedInput(const QList<InputSample> &input,
                                               const WeightedSmoothing &settings);
/** Retains raw tap/sensors; subsequent pixel centers hold adjacent corners. */
QList<InputSample> pixelPerfectInput(const QList<InputSample> &input);
std::optional<QList<FigureFillEdge>> figureFillEdges(const PaintOperation &operation);
std::optional<QList<quint64>> figureFillTiles(const QList<FigureFillEdge> &edges, const Brush &brush,
                                             QRect region, qsizetype maximumCoordinates,
                                             int tileSize = RgbaTile::Size);

}
#endif
