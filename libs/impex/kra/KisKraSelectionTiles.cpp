/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKraSelectionTiles_p.h"
#include <tiles3/swap/kis_lzf_compression.h>
#include <QBuffer>
#include <QSet>
#include <QBitArray>
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

namespace {
bool ellipseContains(int x, int y, const QRect &rect)
{
    const qreal cx = rect.x() + rect.width() / 2.0, cy = rect.y() + rect.height() / 2.0;
    const qreal rx = rect.width() / 2.0, ry = rect.height() / 2.0;
    if (rx <= 0 || ry <= 0) return false;
    const qreal dx = (x + 0.5 - cx) / rx, dy = (y + 0.5 - cy) / ry;
    return dx * dx + dy * dy <= 1.0;
}
}

Outcome readSelectionTiles(const QByteArray &data, quint8 defaultPixel, QPoint offset,
                           QRect imageBounds, qsizetype maximumBytes, QRect &selection, bool &inverted,
                           LibrePaint::Image::Gpu::SelectionKind &kind,
                           LibrePaint::Image::Gpu::SelectionMaskSP &mask)
{
    selection = {};
    inverted = false;
    kind = LibrePaint::Image::Gpu::SelectionKind::Rectangle;
    mask.clear();
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
    qint32 holeMinimumX = std::numeric_limits<qint32>::max(), holeMinimumY = std::numeric_limits<qint32>::max();
    qint32 holeMaximumX = std::numeric_limits<qint32>::min(), holeMaximumY = std::numeric_limits<qint32>::min();
    qint64 selected = 0, unselectedInImage = 0;
    bool selectedOutsideImage = false;
    QBitArray imageBits;
    if (imageBounds.isValid() && !imageBounds.isEmpty() &&
        qint64(imageBounds.width()) * imageBounds.height() <= 64 * 1024 * 1024) {
        imageBits.resize(imageBounds.width() * imageBounds.height());
    }
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
                const qint64 worldX = qint64(x) + px + offset.x(), worldY = qint64(y) + py + offset.y();
                constexpr qint64 limit = RgbaTile::CoordinateLimit;
                if (worldX < -limit || worldY < -limit || worldX >= limit || worldY >= limit) return invalid();
                const auto value = quint8(pixels.at(py * FileTileSize + px));
                if (value == 0) {
                    if (imageBounds.contains(QPoint(int(worldX), int(worldY)))) {
                        holeMinimumX = qMin<qint32>(holeMinimumX, qint32(worldX));
                        holeMinimumY = qMin<qint32>(holeMinimumY, qint32(worldY));
                        holeMaximumX = qMax<qint32>(holeMaximumX, qint32(worldX));
                        holeMaximumY = qMax<qint32>(holeMaximumY, qint32(worldY));
                        ++unselectedInImage;
                    }
                    continue;
                }
                if (value != 255) return unsupported();
                minimumX = qMin<qint32>(minimumX, qint32(worldX));
                minimumY = qMin<qint32>(minimumY, qint32(worldY));
                maximumX = qMax<qint32>(maximumX, qint32(worldX));
                maximumY = qMax<qint32>(maximumY, qint32(worldY));
                if (imageBounds.contains(QPoint(int(worldX), int(worldY)))) {
                    if (!imageBits.isNull()) imageBits.setBit((int(worldY) - imageBounds.top()) * imageBounds.width() +
                                                              int(worldX) - imageBounds.left());
                } else {
                    selectedOutsideImage = true;
                }
                ++selected;
            }
        }
    }
    if (!input.atEnd()) return invalid();
    if (!selected) return {Status::Success, {}};
    const qint64 width = qint64(maximumX) - minimumX + 1;
    const qint64 height = qint64(maximumY) - minimumY + 1;
    if (width > 0 && height > 0 && width <= RgbaTile::CoordinateLimit && height <= RgbaTile::CoordinateLimit &&
        width * height == selected) {
        selection = QRect(minimumX, minimumY, int(width), int(height));
        return {Status::Success, {}};
    }
    if (imageBounds.isValid() && !imageBounds.isEmpty() && unselectedInImage > 0 &&
        qint64(imageBounds.width()) * imageBounds.height() == selected + unselectedInImage &&
        imageBounds.contains(QRect(minimumX, minimumY, int(width), int(height)))) {
        const qint64 holeWidth = qint64(holeMaximumX) - holeMinimumX + 1;
        const qint64 holeHeight = qint64(holeMaximumY) - holeMinimumY + 1;
        if (holeWidth > 0 && holeHeight > 0 && holeWidth * holeHeight == unselectedInImage) {
            selection = QRect(holeMinimumX, holeMinimumY, int(holeWidth), int(holeHeight));
            inverted = true;
            return {Status::Success, {}};
        }
    }
    if (!imageBits.isNull() && unselectedInImage > 0 &&
        qint64(imageBounds.width()) * imageBounds.height() == selected + unselectedInImage) {
        const qint64 holeWidth = qint64(holeMaximumX) - holeMinimumX + 1;
        const qint64 holeHeight = qint64(holeMaximumY) - holeMinimumY + 1;
        if (holeWidth > 0 && holeHeight > 0) {
            bool match = true;
            const QRect hole(holeMinimumX, holeMinimumY, int(holeWidth), int(holeHeight));
            for (int y = imageBounds.top(); y <= imageBounds.bottom() && match; ++y)
                for (int x = imageBounds.left(); x <= imageBounds.right(); ++x) {
                    const bool stored = imageBits.testBit((y - imageBounds.top()) * imageBounds.width() +
                                                           x - imageBounds.left());
                    if (stored == ellipseContains(x, y, hole)) {
                        match = false;
                        break;
                    }
                }
            if (match) {
                selection = hole;
                inverted = true;
                kind = LibrePaint::Image::Gpu::SelectionKind::Ellipse;
                return {Status::Success, {}};
            }
        }
    }
    if (!selectedOutsideImage && !imageBits.isNull() && imageBounds.contains(QRect(minimumX, minimumY, int(width), int(height)))) {
        bool ellipse = true;
        for (int y = minimumY; y <= maximumY && ellipse; ++y)
            for (int x = minimumX; x <= maximumX; ++x) {
                const bool stored = imageBits.testBit((y - imageBounds.top()) * imageBounds.width() +
                                                       x - imageBounds.left());
                if (stored != ellipseContains(x, y, QRect(minimumX, minimumY, int(width), int(height)))) {
                    ellipse = false;
                    break;
                }
            }
        if (ellipse) {
            selection = QRect(minimumX, minimumY, int(width), int(height));
            kind = LibrePaint::Image::Gpu::SelectionKind::Ellipse;
            return {Status::Success, {}};
        }
    }
    if (!selectedOutsideImage && !imageBits.isNull()) {
        QByteArray coverage(imageBounds.width() * imageBounds.height(), '\0');
        for (int y = imageBounds.top(); y <= imageBounds.bottom(); ++y)
            for (int x = imageBounds.left(); x <= imageBounds.right(); ++x)
                if (imageBits.testBit((y - imageBounds.top()) * imageBounds.width() + x - imageBounds.left()))
                    coverage[(y - imageBounds.top()) * imageBounds.width() + x - imageBounds.left()] = char(255);
        auto arbitrary = LibrePaint::Image::Gpu::SelectionMask::fromCoverage(imageBounds, coverage, imageBounds);
        if (arbitrary) {
            selection = arbitrary->bounds();
            kind = LibrePaint::Image::Gpu::SelectionKind::Freehand;
            mask = arbitrary;
            return {Status::Success, {}};
        }
    }
    return unsupported();
}

Outcome writeSelectionTiles(const QRect &selection, bool inverted, const QRect &imageBounds,
                            LibrePaint::Image::Gpu::SelectionKind kind,
                            const LibrePaint::Image::Gpu::SelectionMaskSP &mask,
                            qsizetype maximumBytes, QByteArray &output)
{
    if (kind == LibrePaint::Image::Gpu::SelectionKind::Freehand &&
        (!mask || mask->isEmpty() || mask->bounds() != selection)) return invalid();
    const QRect area = inverted ? imageBounds : selection;
    if (area.isEmpty()) {
        output = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 1\nDATA 0\n");
        return {Status::Success, {}};
    }
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (area.x() < -limit || area.y() < -limit ||
        qint64(area.x()) + area.width() > limit || qint64(area.y()) + area.height() > limit) return invalid();
    const int left = floorDivide(area.left(), FileTileSize);
    const int right = floorDivide(area.right(), FileTileSize);
    const int top = floorDivide(area.top(), FileTileSize);
    const int bottom = floorDivide(area.bottom(), FileTileSize);
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
            std::fill(pixels.begin(), pixels.end(), inverted ? char(255) : char(0));
            const QRect tile(x * FileTileSize, y * FileTileSize, FileTileSize, FileTileSize);
            if (kind == LibrePaint::Image::Gpu::SelectionKind::Freehand) {
                for (int py = tile.top(); py <= tile.bottom(); ++py) {
                    const int row = py - y * FileTileSize;
                    for (int px = tile.left(); px <= tile.right(); ++px) {
                        const bool covered = imageBounds.contains(px, py) && mask->contains(px, py);
                        if (inverted) {
                            if (covered) pixels[row * FileTileSize + px - x * FileTileSize] = char(0);
                        } else if (covered) {
                            pixels[row * FileTileSize + px - x * FileTileSize] = char(255);
                        }
                    }
                }
            } else if (inverted) {
                if (kind == LibrePaint::Image::Gpu::SelectionKind::Ellipse) {
                    for (int py = tile.top(); py <= tile.bottom(); ++py) {
                        const int row = py - y * FileTileSize;
                        for (int px = tile.left(); px <= tile.right(); ++px)
                            if (ellipseContains(px, py, selection))
                                pixels[row * FileTileSize + px - x * FileTileSize] = char(0);
                    }
                } else {
                    const QRect hole = tile.intersected(QRect(selection).intersected(area));
                    for (int py = hole.top(); py <= hole.bottom(); ++py) {
                        const int row = py - y * FileTileSize;
                        std::fill(pixels.begin() + row * FileTileSize + hole.left() - x * FileTileSize,
                                  pixels.begin() + row * FileTileSize + hole.right() - x * FileTileSize + 1, char(0));
                    }
                }
            } else if (kind == LibrePaint::Image::Gpu::SelectionKind::Ellipse) {
                for (int py = tile.top(); py <= tile.bottom(); ++py) {
                    const int row = py - y * FileTileSize;
                    for (int px = tile.left(); px <= tile.right(); ++px)
                        if (area.contains(px, py) && ellipseContains(px, py, area))
                            pixels[row * FileTileSize + px - x * FileTileSize] = char(255);
                }
            } else {
                const QRect selected = tile.intersected(area);
                for (int py = selected.top(); py <= selected.bottom(); ++py) {
                    const int row = py - y * FileTileSize;
                    std::fill(pixels.begin() + row * FileTileSize + selected.left() - x * FileTileSize,
                              pixels.begin() + row * FileTileSize + selected.right() - x * FileTileSize + 1, char(255));
                }
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
