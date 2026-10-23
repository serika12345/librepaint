/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKraRgbaTiles_p.h"
#include <tiles3/swap/kis_lzf_compression.h>
#include <QBuffer>
#include <QSet>
#include <cstring>
#include <limits>
#include <algorithm>

namespace LibrePaint::ImportExport::Kra::Detail {
using namespace Image::Gpu;
namespace {
constexpr int FileTileSize = 64;
constexpr int FileTileBytes = FileTileSize * FileTileSize * 4;
using Coordinates = QPair<int, int>; // row, column
int floorDivide(qint64 value, int divisor) { return int(value / divisor - (value % divisor < 0)); }
Outcome invalid() { return {Status::InvalidData, QStringLiteral("Invalid KRA RGBA tile stream")}; }
Outcome capacity() { return {Status::CapacityExceeded, QStringLiteral("KRA raster pixel capacity exceeded")}; }
QByteArray line(QBuffer &input)
{
    const auto value = input.readLine(128);
    return value.endsWith('\n') ? value.chopped(1) : QByteArray{};
}
}

QByteArray rgba(const std::array<quint8, 4> &pixel)
{
    QByteArray result;
    for (const auto component : pixel) result.append(char(component));
    return result;
}
QByteArray bgra(const std::array<quint8, 4> &pixel)
{ return rgba({pixel[2], pixel[1], pixel[0], pixel[3]}); }

Outcome readTiles(const QByteArray &data, const std::array<quint8, 4> &defaultPixel,
                  QPoint offset, qsizetype maximumBytes, QList<RgbaTile> &output, QList<QRect> *writtenAreas)
{
    QBuffer input;
    input.setData(data);
    input.open(QIODevice::ReadOnly);
    if (line(input) != QByteArrayLiteral("VERSION 2")) {
        return {Status::UnsupportedData, QStringLiteral("KRA tile stream requires version 2")};
    }
    if (line(input) != QByteArrayLiteral("TILEWIDTH 64") || line(input) != QByteArrayLiteral("TILEHEIGHT 64") ||
        line(input) != QByteArrayLiteral("PIXELSIZE 4")) return invalid();
    const auto countLine = line(input).split(' ');
    bool ok = false;
    const auto count = countLine.size() == 2 ? countLine[1].toUInt(&ok) : 0;
    if (!ok || countLine[0] != QByteArrayLiteral("DATA")) return invalid();
    if (count > quint64(maximumBytes / FileTileBytes)) return capacity();
    QMap<Coordinates, QByteArray> result;
    QList<QRect> areas;
    QSet<QPoint> seen;
    const auto fallback = rgba(defaultPixel);
    KisLzfCompression compression;
    for (quint32 tile = 0; tile < count; ++tile) {
        const auto fields = line(input).split(',');
        if (fields.size() != 4 || fields[2] != QByteArrayLiteral("LZF")) return invalid();
        const int x = fields[0].toInt(&ok);
        if (!ok || x % FileTileSize) return invalid();
        const int y = fields[1].toInt(&ok);
        if (!ok || y % FileTileSize || seen.contains({x, y})) return invalid();
        seen.insert({x, y});
        const int size = fields[3].toInt(&ok);
        if (!ok || size < 1 || size > FileTileBytes + 1 || size > input.bytesAvailable()) return invalid();
        const auto encoded = input.read(size);
        QByteArray pixels;
        if (encoded[0] == 0) {
            if (size != FileTileBytes + 1) return invalid();
            pixels = encoded.mid(1);
        } else if (encoded[0] == 1) {
            QByteArray planar(FileTileBytes, Qt::Uninitialized);
            if (compression.decompress(reinterpret_cast<const quint8 *>(encoded.constData() + 1), size - 1,
                                       reinterpret_cast<quint8 *>(planar.data()), planar.size()) != FileTileBytes) return invalid();
            pixels.resize(FileTileBytes);
            KisAbstractCompression::delinearizeColors(reinterpret_cast<quint8 *>(planar.data()),
                                                      reinterpret_cast<quint8 *>(pixels.data()), pixels.size(), 4);
        } else return invalid();
        const qint64 originX = qint64(x) + offset.x(), originY = qint64(y) + offset.y();
        const qint64 left = std::max<qint64>(originX, -RgbaTile::CoordinateLimit);
        const qint64 top = std::max<qint64>(originY, -RgbaTile::CoordinateLimit);
        const qint64 right = std::min<qint64>(originX + FileTileSize, RgbaTile::CoordinateLimit);
        const qint64 bottom = std::min<qint64>(originY + FileTileSize, RgbaTile::CoordinateLimit);
        if (writtenAreas && left < right && top < bottom) {
            const QRect area(int(left), int(top), int(right - left), int(bottom - top));
            for (int ty = floorDivide(top, RgbaTile::Size); ty <= floorDivide(bottom - 1, RgbaTile::Size); ++ty)
                for (int tx = floorDivide(left, RgbaTile::Size); tx <= floorDivide(right - 1, RgbaTile::Size); ++tx)
                    areas.append(area.intersected(QRect(tx * RgbaTile::Size, ty * RgbaTile::Size, RgbaTile::Size, RgbaTile::Size)));
        }
        for (int py = 0; py < FileTileSize; ++py) {
            for (int px = 0; px < FileTileSize; ++px) {
                const auto source = pixels.constData() + (py * FileTileSize + px) * 4;
                const char pixel[]{source[2], source[1], source[0], source[3]};
                const qint64 worldX = originX + px, worldY = originY + py;
                if (worldX < -RgbaTile::CoordinateLimit || worldY < -RgbaTile::CoordinateLimit ||
                    worldX >= RgbaTile::CoordinateLimit || worldY >= RgbaTile::CoordinateLimit) {
                    if (std::memcmp(pixel, fallback.constData(), 4)) return invalid();
                    continue;
                }
                const int tx = floorDivide(worldX, RgbaTile::Size), ty = floorDivide(worldY, RgbaTile::Size);
                const Coordinates coordinate{ty, tx};
                if (!result.contains(coordinate)) {
                    if (result.size() >= maximumBytes / RgbaTile::ByteCount) return capacity();
                    result.insert(coordinate, fallback.repeated(RgbaTile::ByteCount / 4));
                }
                const auto index = ((worldY - qint64(ty) * 256) * 256 + worldX - qint64(tx) * 256) * 4;
                std::memcpy(result[coordinate].data() + index, pixel, 4);
            }
        }
    }
    if (!input.atEnd()) return invalid();
    QList<RgbaTile> values;
    for (auto tile = result.cbegin(); tile != result.cend(); ++tile) values.append({{tile.key().second, tile.key().first}, tile.value()});
    output = std::move(values);
    if (writtenAreas) *writtenAreas = std::move(areas);
    return {Status::Success, {}};
}

Outcome writeTiles(const QList<RgbaTile> &tiles, const std::array<quint8, 4> &defaultPixel,
                   QPoint offset, qsizetype maximumBytes, QByteArray &output, const QList<QRect> &writtenAreas)
{
    if (tiles.size() > maximumBytes / RgbaTile::ByteCount) return capacity();
    QSet<QPoint> seen;
    QMap<Coordinates, QByteArray> result;
    const auto fallback = rgba(defaultPixel), fileFallback = bgra(defaultPixel);
    if (writtenAreas.size() > maximumBytes / qsizetype(sizeof(QRect))) return capacity();
    for (const auto &area : writtenAreas) {
        if (area.isEmpty() || area.x() < -RgbaTile::CoordinateLimit || area.y() < -RgbaTile::CoordinateLimit ||
            qint64(area.x()) + area.width() > RgbaTile::CoordinateLimit ||
            qint64(area.y()) + area.height() > RgbaTile::CoordinateLimit) return invalid();
        const int left = floorDivide(qint64(area.left()) - offset.x(), FileTileSize);
        const int right = floorDivide(qint64(area.right()) - offset.x(), FileTileSize);
        const int top = floorDivide(qint64(area.top()) - offset.y(), FileTileSize);
        const int bottom = floorDivide(qint64(area.bottom()) - offset.y(), FileTileSize);
        if (qint64(right - left + 1) * (bottom - top + 1) > maximumBytes / FileTileBytes) return capacity();
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
            const Coordinates coordinate{y, x};
            if (!result.contains(coordinate)) {
                if (result.size() >= maximumBytes / FileTileBytes) return capacity();
                result.insert(coordinate, fileFallback.repeated(FileTileBytes / 4));
            }
        }
    }
    for (const auto &tile : tiles) {
        if (tile.rgba.size() != RgbaTile::ByteCount || seen.contains(tile.coordinate)) return invalid();
        seen.insert(tile.coordinate);
        const qint64 originX = qint64(tile.coordinate.x()) * 256, originY = qint64(tile.coordinate.y()) * 256;
        if (originX < -RgbaTile::CoordinateLimit || originY < -RgbaTile::CoordinateLimit ||
            originX + 256 > RgbaTile::CoordinateLimit || originY + 256 > RgbaTile::CoordinateLimit) return invalid();
        for (int y = 0; y < 256; ++y) {
            for (int x = 0; x < 256; ++x) {
                const auto source = tile.rgba.constData() + (y * 256 + x) * 4;
                if (!std::memcmp(source, fallback.constData(), 4)) continue;
                const qint64 localX = originX + x - offset.x(), localY = originY + y - offset.y();
                if (localX < std::numeric_limits<int>::min() || localY < std::numeric_limits<int>::min() ||
                    localX > std::numeric_limits<int>::max() || localY > std::numeric_limits<int>::max()) return invalid();
                const int tx = floorDivide(localX, 64), ty = floorDivide(localY, 64);
                const Coordinates coordinate{ty, tx};
                if (!result.contains(coordinate)) {
                    if (result.size() >= maximumBytes / FileTileBytes) return capacity();
                    result.insert(coordinate, fileFallback.repeated(FileTileBytes / 4));
                }
                auto destination = result[coordinate].data() + ((localY - qint64(ty) * 64) * 64 + localX - qint64(tx) * 64) * 4;
                destination[0] = source[2]; destination[1] = source[1]; destination[2] = source[0]; destination[3] = source[3];
            }
        }
    }
    QByteArray bytes = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 4\nDATA ") + QByteArray::number(result.size()) + '\n';
    KisLzfCompression compression;
    QByteArray planar(FileTileBytes, Qt::Uninitialized);
    const auto encodedCapacity = compression.outputBufferSize(FileTileBytes);
    QByteArray encoded(encodedCapacity, Qt::Uninitialized);
    for (auto tile = result.begin(); tile != result.end(); ++tile) {
        KisAbstractCompression::linearizeColors(reinterpret_cast<quint8 *>(tile.value().data()),
                                               reinterpret_cast<quint8 *>(planar.data()), FileTileBytes, 4);
        const auto compressedBytes = compression.compress(reinterpret_cast<const quint8 *>(planar.constData()), FileTileBytes,
                                                         reinterpret_cast<quint8 *>(encoded.data()), encodedCapacity);
        if (compressedBytes <= 0) return {Status::IoFailure, QStringLiteral("KRA layer pixel compression failed")};
        const bool useCompressed = compressedBytes < FileTileBytes;
        const auto payloadSize = useCompressed ? compressedBytes : FileTileBytes;
        bytes += QByteArray::number(tile.key().second * 64) + ',' + QByteArray::number(tile.key().first * 64) +
                 QByteArrayLiteral(",LZF,") + QByteArray::number(payloadSize + 1) + '\n';
        bytes += char(useCompressed ? 1 : 0);
        bytes.append(useCompressed ? encoded.constData() : tile.value().constData(), payloadSize);
    }
    output = std::move(bytes);
    return {Status::Success, {}};
}
}
