/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_RASTER_PIXELS_H
#define KIS_GPU_RASTER_PIXELS_H

#include <kritaimagegpuoperations_export.h>
#include <QByteArray>
#include <QPoint>
#include <QSharedPointer>

namespace LibrePaint::Image::Gpu {
/** Full tile pixels in straight, byte-ordered RGBA8. Coordinates identify tiles. */
struct RgbaTile
{
    static constexpr int Size = 256;
    static constexpr int ByteCount = Size * Size * 4;
    static constexpr int CoordinateLimit = 8388608;
    QPoint coordinate;
    QByteArray rgba;
};

/**
 * Immutable lossless CPU checkpoint. Factories reject invalid tile extents,
 * byte counts and compressed streams. Every tile covers exactly 256 x 256
 * pixels inside [-CoordinateLimit, CoordinateLimit) on each axis.
 * Compression uses one zlib stream; decoding is bounded to ByteCount bytes
 * and consumes the complete stream. Returned byte values have independent
 * lifetimes and retain all channels, including colors under zero alpha.
 */
class KRITAIMAGEGPUOPERATIONS_EXPORT RasterTile
{
public:
    static QSharedPointer<const RasterTile> fromPixels(const RgbaTile &tile);
    static QSharedPointer<const RasterTile> fromCompressed(QPoint coordinate, const QByteArray &data);
    static qsizetype maximumCompressedBytes();
    QPoint coordinate() const { return m_coordinate; }
    QByteArray compressedData() const { return m_compressed; }
    QByteArray rgba() const;

private:
    RasterTile(QPoint coordinate, QByteArray data);
    QPoint m_coordinate;
    QByteArray m_compressed;
};
}
#endif
