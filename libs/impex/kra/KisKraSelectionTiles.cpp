/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKraSelectionTiles_p.h"
#include <tiles3/swap/kis_lzf_compression.h>
#include <QBuffer>
#include <QSet>
#include <algorithm>
#include <limits>

namespace LibrePaint::ImportExport::Kra::Detail {
using namespace Image::Gpu;
namespace {
constexpr int FileTileSize = 64;
constexpr int FileTileBytes = FileTileSize * FileTileSize;
using Coordinates = QPair<int, int>; // row, column
int floorDivide(qint64 value, int divisor) { return int(value / divisor - (value % divisor < 0)); }
Outcome invalid() { return {Status::InvalidData, QStringLiteral("Invalid KRA selection tile stream")}; }
Outcome capacity() { return {Status::CapacityExceeded, QStringLiteral("KRA selection pixel capacity exceeded")}; }
Outcome unsupported() { return {Status::UnsupportedData, QStringLiteral("KRA requires a rectangular pixel selection")}; }
QByteArray line(QBuffer &input)
{
    const auto value = input.readLine(128);
    return value.endsWith('\n') ? value.chopped(1) : QByteArray{};
}
}

Outcome readSelectionTiles(const QByteArray &data, quint8 defaultPixel, QPoint offset,
                           qsizetype maximumBytes, QRect &selection)
{
    selection = {};
    if (defaultPixel != 0) return unsupported();
    QBuffer input;
    input.setData(data);
    input.open(QIODevice::ReadOnly);
    if (line(input) != QByteArrayLiteral("VERSION 2")) {
        return {Status::UnsupportedData, QStringLiteral("KRA selection stream requires version 2")};
    }
    if (line(input) != QByteArrayLiteral("TILEWIDTH 64") || line(input) != QByteArrayLiteral("TILEHEIGHT 64") ||
        line(input) != QByteArrayLiteral("PIXELSIZE 1")) return invalid();
    const auto countLine = line(input).split(' ');
    bool ok = false;
    const auto count = countLine.size() == 2 ? countLine[1].toUInt(&ok) : 0;
    if (!ok || countLine[0] != QByteArrayLiteral("DATA")) return invalid();
    if (count > quint64(maximumBytes / FileTileBytes)) return capacity();
    QSet<QPoint> seen;
    KisLzfCompression compression;
    qint32 minimumX = std::numeric_limits<qint32>::max(), minimumY = std::numeric_limits<qint32>::max();
    qint32 maximumX = std::numeric_limits<qint32>::min(), maximumY = std::numeric_limits<qint32>::min();
    qint64 selected = 0;
    QByteArray pixels;
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
        if (encoded[0] == 0) {
            if (size != FileTileBytes + 1) return invalid();
            pixels = encoded.mid(1);
        } else if (encoded[0] == 1) {
            pixels.resize(FileTileBytes);
            if (compression.decompress(reinterpret_cast<const quint8 *>(encoded.constData() + 1), size - 1,
                                       reinterpret_cast<quint8 *>(pixels.data()), pixels.size()) != FileTileBytes) return invalid();
        } else return invalid();
        for (int py = 0; py < FileTileSize; ++py) {
            for (int px = 0; px < FileTileSize; ++px) {
                const auto value = quint8(pixels.at(py * FileTileSize + px));
                if (value == 0) continue;
                if (value != 255) return unsupported();
                const qint64 worldX = qint64(x) + px + offset.x(), worldY = qint64(y) + py + offset.y();
                constexpr qint64 limit = RgbaTile::CoordinateLimit;
                if (worldX < -limit || worldY < -limit || worldX >= limit || worldY >= limit) return invalid();
                minimumX = qMin<qint32>(minimumX, qint32(worldX));
                minimumY = qMin<qint32>(minimumY, qint32(worldY));
                maximumX = qMax<qint32>(maximumX, qint32(worldX));
                maximumY = qMax<qint32>(maximumY, qint32(worldY));
                ++selected;
            }
        }
    }
    if (!input.atEnd()) return invalid();
    if (!selected) return {Status::Success, {}};
    const qint64 width = qint64(maximumX) - minimumX + 1;
    const qint64 height = qint64(maximumY) - minimumY + 1;
    if (width <= 0 || height <= 0 || width > RgbaTile::CoordinateLimit || height > RgbaTile::CoordinateLimit ||
        width * height != selected) return unsupported();
    selection = QRect(minimumX, minimumY, int(width), int(height));
    return {Status::Success, {}};
}

Outcome writeSelectionTiles(const QRect &selection, qsizetype maximumBytes, QByteArray &output)
{
    if (selection.isEmpty()) {
        output = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 1\nDATA 0\n");
        return {Status::Success, {}};
    }
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (selection.x() < -limit || selection.y() < -limit ||
        qint64(selection.x()) + selection.width() > limit ||
        qint64(selection.y()) + selection.height() > limit) return invalid();
    const int left = floorDivide(selection.left(), FileTileSize);
    const int right = floorDivide(selection.right(), FileTileSize);
    const int top = floorDivide(selection.top(), FileTileSize);
    const int bottom = floorDivide(selection.bottom(), FileTileSize);
    const qint64 count = qint64(right - left + 1) * (bottom - top + 1);
    if (count <= 0 || count > maximumBytes / FileTileBytes || count > std::numeric_limits<quint32>::max()) return capacity();
    QByteArray bytes = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 1\nDATA ") +
        QByteArray::number(count) + '\n';
    KisLzfCompression compression;
    const auto encodedCapacity = compression.outputBufferSize(FileTileBytes);
    QByteArray encoded(encodedCapacity, Qt::Uninitialized);
    QByteArray pixels(FileTileBytes, '\0');
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            std::fill(pixels.begin(), pixels.end(), '\0');
            const QRect tile(x * FileTileSize, y * FileTileSize, FileTileSize, FileTileSize);
            const QRect selected = tile.intersected(selection);
            for (int py = selected.top(); py <= selected.bottom(); ++py) {
                const int row = py - y * FileTileSize;
                std::fill(pixels.begin() + row * FileTileSize + selected.left() - x * FileTileSize,
                          pixels.begin() + row * FileTileSize + selected.right() - x * FileTileSize + 1, char(255));
            }
            const auto compressedBytes = compression.compress(reinterpret_cast<const quint8 *>(pixels.constData()), FileTileBytes,
                                                              reinterpret_cast<quint8 *>(encoded.data()), encodedCapacity);
            if (compressedBytes <= 0) return {Status::IoFailure, QStringLiteral("KRA selection compression failed")};
            const bool useCompressed = compressedBytes < FileTileBytes;
            const auto payloadSize = useCompressed ? compressedBytes : FileTileBytes;
            bytes += QByteArray::number(x * FileTileSize) + ',' + QByteArray::number(y * FileTileSize) +
                     QByteArrayLiteral(",LZF,") + QByteArray::number(payloadSize + 1) + '\n';
            bytes += char(useCompressed ? 1 : 0);
            bytes.append(useCompressed ? encoded.constData() : pixels.constData(), payloadSize);
        }
    }
    output = std::move(bytes);
    return {Status::Success, {}};
}
}
