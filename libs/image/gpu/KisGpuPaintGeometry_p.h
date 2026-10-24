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

inline quint64 key(int x, int y) { return (quint64(quint32(y)) << 32) | quint32(x); }
inline QPoint coordinate(quint64 key) { return {int(quint32(key)), int(quint32(key >> 32))}; }

struct PaintGeometry
{
    QList<PaintSegment> outline;
    QList<PaintSegment> fill;
};
constexpr qsizetype PathGeometrySegmentLimit = 1024 * 1024;
/** Derived positions/pressure preserve raw input; excessive replay returns nullopt. */
std::optional<QList<InputSample>> weightedInput(const QList<InputSample> &input,
                                               const WeightedSmoothing &settings);
std::optional<PaintGeometry> paintGeometry(const PaintOperation &operation, quint32 step);
std::optional<QList<quint64>> figureFillTiles(const QList<PaintSegment> &edges, const Brush &brush,
                                             QRect region, qsizetype maximumCoordinates,
                                             int tileSize = RgbaTile::Size);
std::optional<QList<quint64>> paintSegmentTiles(const PaintSegment &segment, quint32 diameter, const Brush &brush,
                                               QRect region, qsizetype maximumCoordinates,
                                               int tileSize = RgbaTile::Size);
}
#endif
