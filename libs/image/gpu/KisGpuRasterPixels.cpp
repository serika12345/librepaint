/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuRasterPixels.h"
#include <zlib.h>
#include <utility>

namespace LibrePaint::Image::Gpu {
namespace {
bool validCoordinate(QPoint coordinate)
{
    for (const auto component : {coordinate.x(), coordinate.y()}) {
        const qint64 origin = qint64(component) * RgbaTile::Size;
        if (origin < -RgbaTile::CoordinateLimit || origin + RgbaTile::Size > RgbaTile::CoordinateLimit) return false;
    }
    return true;
}

QByteArray decode(const QByteArray &data)
{
    if (data.isEmpty() || data.size() > RasterTile::maximumCompressedBytes()) return {};
    QByteArray pixels(RgbaTile::ByteCount, Qt::Uninitialized);
    uLongf size = pixels.size();
    uLong consumed = data.size();
    const auto result = uncompress2(reinterpret_cast<Bytef *>(pixels.data()), &size,
                                   reinterpret_cast<const Bytef *>(data.constData()), &consumed);
    if (result != Z_OK || size != RgbaTile::ByteCount || consumed != quint64(data.size())) return {};
    return pixels;
}
}

RasterTile::RasterTile(QPoint coordinate, QByteArray data)
    : m_coordinate(coordinate), m_compressed(std::move(data)) {}

QSharedPointer<const RasterTile> RasterTile::fromPixels(const RgbaTile &tile)
{
    if (!validCoordinate(tile.coordinate) || tile.rgba.size() != RgbaTile::ByteCount) return {};
    uLongf size = compressBound(RgbaTile::ByteCount);
    QByteArray compressed(qsizetype(size), Qt::Uninitialized);
    const auto result = compress2(reinterpret_cast<Bytef *>(compressed.data()), &size,
                                 reinterpret_cast<const Bytef *>(tile.rgba.constData()),
                                 RgbaTile::ByteCount, Z_DEFAULT_COMPRESSION);
    if (result != Z_OK) return {};
    compressed.resize(size);
    return QSharedPointer<const RasterTile>(new RasterTile(tile.coordinate, std::move(compressed)));
}

QSharedPointer<const RasterTile> RasterTile::fromCompressed(QPoint coordinate, const QByteArray &data)
{
    if (!validCoordinate(coordinate) || decode(data).size() != RgbaTile::ByteCount) return {};
    return QSharedPointer<const RasterTile>(new RasterTile(coordinate, data));
}

QByteArray RasterTile::rgba() const { return decode(m_compressed); }
qsizetype RasterTile::maximumCompressedBytes() { return compressBound(RgbaTile::ByteCount); }
}
