/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_GEOMETRY_P_H
#define KIS_GPU_PAINT_GEOMETRY_P_H

#include "KisGpuOperationDocument.h"
#include "KisGpuBasicCurveControls.h"
#include <QRect>
#include <optional>

namespace LibrePaint::Image::Gpu::Detail {
constexpr int PixelHistoryBlockSize = 64;
inline int pixelTileIndex(int value, int size)
{ return value >= 0 ? value / size : (value - size + 1) / size; }
struct PaintSegment
{
    qint32 ax, ay, bx, by;
    quint32 p0, p1, phase, length;
};
static_assert(sizeof(PaintSegment) == 32);

struct FigureFillEdge
{
    qint32 ax, ay, bx, by;
    /** Per-coordinate correction for reflecting raw positions before rounding. */
    std::array<qint32, 4> mirroredRounding;
};
static_assert(sizeof(FigureFillEdge) == 32);

inline quint64 key(int x, int y) { return (quint64(quint32(y)) << 32) | quint32(x); }
inline QPoint coordinate(quint64 key) { return {int(quint32(key)), int(quint32(key >> 32))}; }

struct PaintGeometry
{
    /** Brush dabs use 1/16-pixel positions. */
    QList<PaintSegment> outline;
    /** Closed figure edges use 26.6 positions for integer GPU cell coverage. */
    QList<FigureFillEdge> fill;
};
constexpr qsizetype PathGeometrySegmentLimit = 1024 * 1024;
/** Derived positions/pressure preserve raw input; excessive replay returns nullopt. */
std::optional<QList<InputSample>> weightedInput(const QList<InputSample> &input,
                                               const WeightedSmoothing &settings);
/** Retains raw tap/sensors; subsequent pixel centers hold adjacent corners. */
QList<InputSample> pixelPerfectInput(const QList<InputSample> &input);
std::optional<PaintGeometry> paintGeometry(const PaintOperation &operation, quint32 step);
std::optional<QList<quint64>> figureFillTiles(const QList<FigureFillEdge> &edges, const Brush &brush,
                                             QRect region, qsizetype maximumCoordinates,
                                             int tileSize = RgbaTile::Size);
std::optional<QList<quint64>> paintSegmentTiles(const PaintSegment &segment, quint32 diameter, const Brush &brush,
                                               QRect region, qsizetype maximumCoordinates,
                                               int tileSize = RgbaTile::Size);
}
#endif
