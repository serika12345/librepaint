/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_PIXEL_DATA_H
#define KIS_GPU_PIXEL_DATA_H

#include <kritaimagegpuoperations_export.h>
#include <QByteArray>
#include <QPoint>
#include <QSharedPointer>
#include <array>
#include <optional>

namespace LibrePaint::Image::Gpu {
enum class PixelFormat : quint8 { Rgba8, Rgba16 };
/** Numeric RGBA components in the range declared by the document's format. */
using RgbaPixel = std::array<quint16, 4>;

/** Straight RGBA components; U16 components use little-endian byte order. */
constexpr int pixelByteCount(PixelFormat format)
{
    return format == PixelFormat::Rgba8 ? 4 : format == PixelFormat::Rgba16 ? 8 : 0;
}
constexpr quint16 maximumComponent(PixelFormat format)
{ return format == PixelFormat::Rgba8 ? 255 : format == PixelFormat::Rgba16 ? 65535 : 0; }
constexpr bool validPixel(const RgbaPixel &pixel, PixelFormat format)
{
    if (!pixelByteCount(format)) return false;
    for (const auto component : pixel) if (component > maximumComponent(format)) return false;
    return true;
}
/** Encoding preserves raw component values; it performs no color conversion. */
KRITAIMAGEGPUOPERATIONS_EXPORT QByteArray rgbaBytes(const RgbaPixel &pixel, PixelFormat format);
KRITAIMAGEGPUOPERATIONS_EXPORT std::optional<RgbaPixel> rgbaPixel(QByteArrayView bytes, PixelFormat format);

/** Full tile pixels in straight RGBA with the declared precision. Coordinates identify tiles. */
struct RgbaTile
{
    static constexpr int Size = 256;
    static constexpr int ByteCount = Size * Size * 4;
    static constexpr int CoordinateLimit = 8388608;
    QPoint coordinate;
    QByteArray rgba;
    PixelFormat format = PixelFormat::Rgba8;
    static constexpr int byteCount(PixelFormat format) { return Size * Size * pixelByteCount(format); }
};

/** Validated CPU tile upload, released after its GPU pixels are published. */
class KRITAIMAGEGPUOPERATIONS_EXPORT UploadTile
{
public:
    static QSharedPointer<const UploadTile> fromPixels(const RgbaTile &tile);
    QPoint coordinate() const { return m_coordinate; }
    PixelFormat format() const { return m_format; }
    qsizetype byteCount() const { return RgbaTile::byteCount(m_format); }
    QByteArray rgba() const { return m_rgba; }

private:
    UploadTile(QPoint coordinate, QByteArray data, PixelFormat format);
    QPoint m_coordinate;
    QByteArray m_rgba;
    PixelFormat m_format;
};
}
#endif
