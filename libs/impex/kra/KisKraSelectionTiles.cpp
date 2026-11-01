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
    const auto adoptFiniteCoverage = [&]() {
        LibrePaint::Image::Gpu::SelectionMaskSP coverage;
        const auto result = readTransparencyMask(data, offset, imageBounds, maximumBytes, coverage);
        if (result.code != Status::Success) return result;
        selection = coverage->bounds(); inverted = false;
        kind = LibrePaint::Image::Gpu::SelectionKind::Freehand; mask = coverage;
        return result;
    };
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
    QByteArray imageAlpha;
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
                const bool inImage = imageBounds.contains(QPoint(int(worldX), int(worldY)));
                if (value == 0) {
                    if (inImage) {
                        holeMinimumX = qMin<qint32>(holeMinimumX, qint32(worldX));
                        holeMinimumY = qMin<qint32>(holeMinimumY, qint32(worldY));
                        holeMaximumX = qMax<qint32>(holeMaximumX, qint32(worldX));
                        holeMaximumY = qMax<qint32>(holeMaximumY, qint32(worldY));
                        ++unselectedInImage;
                    }
                    continue;
                }
                if (value != 255) {
                    // Fractional selectedness requires the complete image mirror
                    // to distinguish soft edges from shape boundaries.
                    if (imageBits.isNull() || !inImage) return adoptFiniteCoverage();
                    if (imageAlpha.isNull()) {
                        imageAlpha = QByteArray(imageBounds.width() * imageBounds.height(), '\0');
                        for (int y = 0; y < imageBounds.height(); ++y)
                            for (int x = 0; x < imageBounds.width(); ++x)
                                if (imageBits.testBit(y * imageBounds.width() + x))
                                    imageAlpha[y * imageBounds.width() + x] = char(255);
                    }
                }
                minimumX = qMin<qint32>(minimumX, qint32(worldX));
                minimumY = qMin<qint32>(minimumY, qint32(worldY));
                maximumX = qMax<qint32>(maximumX, qint32(worldX));
                maximumY = qMax<qint32>(maximumY, qint32(worldY));
                if (inImage) {
                    const auto index = (int(worldY) - imageBounds.top()) * imageBounds.width() +
                        int(worldX) - imageBounds.left();
                    if (!imageBits.isNull()) imageBits.setBit(index);
                    if (!imageAlpha.isNull()) imageAlpha[index] = char(value);
                } else {
                    selectedOutsideImage = true;
                }
                ++selected;
            }
        }
    }
    if (!input.atEnd()) return invalid();
    if (!selected) return {Status::Success, {}};
    if (!imageAlpha.isNull()) {
        if (selectedOutsideImage) return adoptFiniteCoverage();
        const auto arbitrary = LibrePaint::Image::Gpu::SelectionMask::fromAlphaCoverage(
            imageBounds, imageAlpha, imageBounds);
        if (!arbitrary) return unsupported();
        selection = arbitrary->bounds();
        kind = LibrePaint::Image::Gpu::SelectionKind::Freehand;
        mask = arbitrary;
        return {Status::Success, {}};
    }
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
    return adoptFiniteCoverage();
}

Outcome writeSelectionTiles(const QRect &selection, bool inverted, const QRect &imageBounds,
                            LibrePaint::Image::Gpu::SelectionKind kind,
                            const LibrePaint::Image::Gpu::SelectionMaskSP &mask,
                            qsizetype maximumBytes, QByteArray &output)
{
    if (mask && mask->isGpuGenerated())
        return {Status::UnsupportedData, QStringLiteral("KRA selection writing requires explicit external Alpha8 pixels")};
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
    const qint64 denseCount = qint64(right - left + 1) * (bottom - top + 1);
    QList<Coordinates> coordinates;
    if (denseCount > maximumBytes / FileTileBytes && kind == SelectionKind::Freehand && !inverted) {
        // Finite masks can contain distant islands. Native KRA defaults to zero
        // between stored tiles, so capacity follows the occupied tile set.
        QSet<Coordinates> occupied;
        for (auto source = mask->tiles().cbegin(); source != mask->tiles().cend(); ++source) {
            const auto origin = QPoint(source.key().first, source.key().second) * SelectionMask::TileSize;
            for (int y = 0; y < SelectionMask::TileSize / FileTileSize; ++y)
                for (int x = 0; x < SelectionMask::TileSize / FileTileSize; ++x) {
                    const QPoint point = origin + QPoint(x, y) * FileTileSize;
                    if (!QRect(point, QSize(FileTileSize, FileTileSize)).intersects(area)) continue;
                    occupied.insert({floorDivide(point.y(), FileTileSize), floorDivide(point.x(), FileTileSize)});
                    if (occupied.size() > maximumBytes / FileTileBytes) return capacity();
                }
        }
        coordinates = occupied.values();
        std::sort(coordinates.begin(), coordinates.end());
    } else {
        if (denseCount <= 0 || denseCount > maximumBytes / FileTileBytes || denseCount > std::numeric_limits<quint32>::max()) return capacity();
        coordinates.reserve(qsizetype(denseCount));
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) coordinates.append({y, x});
    }
    const auto count = coordinates.size();
    if (count <= 0 || quint64(count) > std::numeric_limits<quint32>::max()) return capacity();
    QByteArray bytes = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 1\nDATA ") +
        QByteArray::number(count) + '\n';
    KisLzfCompression compression;
    const auto encodedCapacity = compression.outputBufferSize(FileTileBytes);
    QByteArray encoded(encodedCapacity, Qt::Uninitialized);
    QByteArray pixels(FileTileBytes, '\0');
    for (const auto &coordinate : coordinates) {
        const int y = coordinate.first, x = coordinate.second;
        std::fill(pixels.begin(), pixels.end(), inverted ? char(255) : char(0));
        const QRect tile(x * FileTileSize, y * FileTileSize, FileTileSize, FileTileSize);
        if (kind == LibrePaint::Image::Gpu::SelectionKind::Freehand) {
            for (int py = tile.top(); py <= tile.bottom(); ++py) {
                const int row = py - y * FileTileSize;
                for (int px = tile.left(); px <= tile.right(); ++px) {
                    if (inverted && !imageBounds.contains(px, py)) continue;
                    const quint8 value = mask->coverage(px, py);
                    pixels[row * FileTileSize + px - x * FileTileSize] = char(inverted ? 255u - value : value);
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
    output = std::move(bytes);
    return {Status::Success, {}};
}

Outcome readTransparencyMask(const QByteArray &data, QPoint offset, QRect imageBounds,
                             qsizetype maximumBytes,
                             LibrePaint::Image::Gpu::SelectionMaskSP &mask, quint8 defaultCoverage)
{
    mask.clear();
    if (imageBounds.isEmpty() || maximumBytes <= 0) return invalid();
    QBuffer input;
    input.setData(data);
    input.open(QIODevice::ReadOnly);
    if (line(input) != QByteArrayLiteral("VERSION 2"))
        return {Status::UnsupportedData, QStringLiteral("KRA transparency mask requires version 2")};
    if (line(input) != QByteArrayLiteral("TILEWIDTH 64") || line(input) != QByteArrayLiteral("TILEHEIGHT 64") ||
        line(input) != QByteArrayLiteral("PIXELSIZE 1")) return invalid();
    const auto countLine = line(input).split(' ');
    bool ok = false;
    const auto count = countLine.size() == 2 ? countLine[1].toUInt(&ok) : 0;
    if (!ok || countLine[0] != QByteArrayLiteral("DATA")) return invalid();
    if (count > quint64(maximumBytes / FileTileBytes)) return capacity();
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    QSet<QPoint> seen;
    KisLzfCompression compression;
    SelectionTileMap tiles;
    QList<QRect> allocated;
    qsizetype bytes = 0;
    QByteArray pixels;
    for (quint32 index = 0; index < count; ++index) {
        const auto fields = line(input).split(',');
        if (fields.size() != 4 || fields[2] != QByteArrayLiteral("LZF")) return invalid();
        const int x = fields[0].toInt(&ok);
        if (!ok || x % FileTileSize) return invalid();
        const int y = fields[1].toInt(&ok);
        if (!ok || y % FileTileSize || seen.contains({x, y})) return invalid();
        seen.insert({x, y});
        const qint64 left = qint64(x) + offset.x(), top = qint64(y) + offset.y();
        if (left < -limit || top < -limit || left + FileTileSize > limit || top + FileTileSize > limit) return invalid();
        allocated.append(QRect(int(left), int(top), FileTileSize, FileTileSize));
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
        for (int row = 0; row < FileTileSize; ++row) {
            for (int column = 0; column < FileTileSize; ++column) {
                const quint8 value = quint8(pixels.at(row * FileTileSize + column));
                if (value == defaultCoverage) continue;
                const qint64 worldX = qint64(x) + column + offset.x();
                const qint64 worldY = qint64(y) + row + offset.y();
                if (worldX < -limit || worldY < -limit || worldX >= limit || worldY >= limit) return invalid();
                const int tileX = floorDivide(worldX, SelectionMask::TileSize);
                const int tileY = floorDivide(worldY, SelectionMask::TileSize);
                const SelectionTileKey key(tileX, tileY);
                auto tile = tiles.find(key);
                if (tile == tiles.end()) {
                    if (bytes + SelectionMask::AlphaTileBytes >
                        std::min<qsizetype>(SelectionMask::MaximumBytes, maximumBytes)) return capacity();
                    tile = tiles.insert(key, QByteArray(SelectionMask::AlphaTileBytes, char(defaultCoverage)));
                    bytes += SelectionMask::AlphaTileBytes;
                }
                const int localX = int(worldX) - tileX * SelectionMask::TileSize;
                const int localY = int(worldY) - tileY * SelectionMask::TileSize;
                tile.value()[qsizetype(localY) * SelectionMask::TileSize + localX] = char(value);
            }
        }
    }
    if (!input.atEnd()) return invalid();
    const auto coverage = SelectionMask::fromAlphaTiles(tiles, defaultCoverage);
    mask = coverage ? coverage->withAllocatedBlocks(allocated) : SelectionMaskSP{};
    if (!mask)
        return {Status::UnsupportedData, QStringLiteral("KRA transparency mask coverage could not be adopted")};
    return {Status::Success, {}};
}

Outcome writeTransparencyMask(const LibrePaint::Image::Gpu::SelectionMaskSP &mask,
                              const QRect &imageBounds, qsizetype maximumBytes,
                              QByteArray &output, QPoint &offset)
{
    if (!mask || mask->isGpuGenerated() || imageBounds.isEmpty())
        return {Status::InvalidData, QStringLiteral("KRA transparency mask requires coverage and image bounds")};
    const auto &blocks = mask->allocatedBlocks();
    if (maximumBytes < 0 || blocks.size() > maximumBytes / FileTileBytes) return capacity();
    if (!mask->bounds().isEmpty() && blocks.isEmpty()) return invalid();
    offset = blocks.isEmpty() ? QPoint{} : QPoint(blocks.first().x() % FileTileSize, blocks.first().y() % FileTileSize);
    QByteArray bytes = QByteArrayLiteral("VERSION 2\nTILEWIDTH 64\nTILEHEIGHT 64\nPIXELSIZE 1\nDATA ") +
        QByteArray::number(blocks.size()) + '\n';
    KisLzfCompression compression;
    const auto capacity = compression.outputBufferSize(FileTileBytes);
    QByteArray encoded(capacity, Qt::Uninitialized), pixels(FileTileBytes, Qt::Uninitialized);
    for (const auto &block : blocks) {
        if (block.size() != QSize(FileTileSize, FileTileSize) ||
            (block.x() - offset.x()) % FileTileSize || (block.y() - offset.y()) % FileTileSize) return invalid();
        for (int y = 0; y < FileTileSize; ++y) for (int x = 0; x < FileTileSize; ++x)
            pixels[y * FileTileSize + x] = char(mask->coverage(block.x() + x, block.y() + y));
        const auto size = compression.compress(reinterpret_cast<const quint8 *>(pixels.constData()), FileTileBytes,
            reinterpret_cast<quint8 *>(encoded.data()), capacity);
        if (size <= 0) return {Status::IoFailure, QStringLiteral("KRA transparency compression failed")};
        const bool compressed = size < FileTileBytes;
        const int payload = compressed ? size : FileTileBytes;
        bytes += QByteArray::number(block.x() - offset.x()) + ',' + QByteArray::number(block.y() - offset.y()) +
            QByteArrayLiteral(",LZF,") + QByteArray::number(payload + 1) + '\n';
        bytes += char(compressed ? 1 : 0);
        bytes.append(compressed ? encoded.constData() : pixels.constData(), payload);
    }
    output = std::move(bytes);
    return {Status::Success, {}};
}
}
