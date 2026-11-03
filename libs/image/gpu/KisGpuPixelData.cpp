/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPixelData.h"
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

}

UploadTile::UploadTile(QPoint coordinate, QByteArray data, PixelFormat format)
    : m_coordinate(coordinate), m_rgba(std::move(data)), m_format(format) {}

QSharedPointer<const UploadTile> UploadTile::fromPixels(const RgbaTile &tile)
{
    const auto byteCount = RgbaTile::byteCount(tile.format);
    if (!byteCount || !validCoordinate(tile.coordinate) || tile.rgba.size() != byteCount) return {};
    return QSharedPointer<const UploadTile>(new UploadTile(tile.coordinate, tile.rgba, tile.format));
}
}
