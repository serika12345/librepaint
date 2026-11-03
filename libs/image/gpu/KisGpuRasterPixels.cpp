/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuRasterPixels.h"
#include <zlib.h>
#include <utility>
#include <QtEndian>

namespace LibrePaint::Image::Gpu {
QByteArray rgbaBytes(const RgbaPixel &pixel, PixelFormat format)
{
    if (!validPixel(pixel, format)) return {};
    QByteArray bytes(pixelByteCount(format), Qt::Uninitialized);
    for (int i = 0; i < 4; ++i) {
        if (format == PixelFormat::Rgba16) qToLittleEndian(pixel[i], bytes.data() + i * 2);
        else bytes[i] = char(pixel[i]);
    }
    return bytes;
}

std::optional<RgbaPixel> rgbaPixel(QByteArrayView bytes, PixelFormat format)
{
    if (!pixelByteCount(format) || bytes.size() != pixelByteCount(format)) return {};
    RgbaPixel pixel;
    for (int i = 0; i < 4; ++i)
        pixel[i] = format == PixelFormat::Rgba16 ? qFromLittleEndian<quint16>(bytes.data() + i * 2) : quint8(bytes[i]);
    return pixel;
}

namespace {
bool validCoordinate(QPoint coordinate)
{
    for (const auto component : {coordinate.x(), coordinate.y()}) {
        const qint64 origin = qint64(component) * RgbaTile::Size;
        if (origin < -RgbaTile::CoordinateLimit || origin + RgbaTile::Size > RgbaTile::CoordinateLimit) return false;
    }
    return true;
}

QByteArray decode(const QByteArray &data, PixelFormat format)
{
    const auto byteCount = RgbaTile::byteCount(format);
    if (!byteCount || data.isEmpty() || quint64(data.size()) > compressBound(byteCount)) return {};
    QByteArray pixels(byteCount, Qt::Uninitialized);
    uLongf size = pixels.size();
    uLong consumed = data.size();
    const auto result = uncompress2(reinterpret_cast<Bytef *>(pixels.data()), &size,
                                   reinterpret_cast<const Bytef *>(data.constData()), &consumed);
    if (result != Z_OK || size != quint64(byteCount) || consumed != quint64(data.size())) return {};
    return pixels;
}
}

RasterTile::RasterTile(QPoint coordinate, QByteArray data, PixelFormat format)
    : m_coordinate(coordinate), m_compressed(std::move(data)), m_format(format) {}

qsizetype RasterTile::compressionBufferBytes(PixelFormat format)
{
    const auto bytes = RgbaTile::byteCount(format);
    return bytes ? qsizetype(compressBound(bytes)) : 0;
}

QSharedPointer<const RasterTile> RasterTile::fromPixels(const RgbaTile &tile)
{
    const auto byteCount = RgbaTile::byteCount(tile.format);
    if (!byteCount || !validCoordinate(tile.coordinate) || tile.rgba.size() != byteCount) return {};
    uLongf size = compressionBufferBytes(tile.format);
    QByteArray compressed(qsizetype(size), Qt::Uninitialized);
    const auto result = compress2(reinterpret_cast<Bytef *>(compressed.data()), &size,
                                 reinterpret_cast<const Bytef *>(tile.rgba.constData()),
                                 byteCount, Z_DEFAULT_COMPRESSION);
    if (result != Z_OK) return {};
    compressed.resize(size);
    return QSharedPointer<const RasterTile>(new RasterTile(tile.coordinate, std::move(compressed), tile.format));
}

QByteArray RasterTile::rgba() const { return decode(m_compressed, m_format); }
}
