/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuSelectionMask.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace LibrePaint::Image::Gpu {
namespace {
constexpr qint64 CoordinateLimit = RgbaTile::CoordinateLimit;

int floorDivide(qint64 value, int divisor)
{ return int(value / divisor - (value % divisor < 0)); }

bool validTileCoordinate(const QPoint &coordinate)
{
    const qint64 left = qint64(coordinate.x()) * SelectionMask::TileSize;
    const qint64 top = qint64(coordinate.y()) * SelectionMask::TileSize;
    return left >= -CoordinateLimit && top >= -CoordinateLimit &&
        left + SelectionMask::TileSize <= CoordinateLimit &&
        top + SelectionMask::TileSize <= CoordinateLimit;
}

struct CoverageBuilder
{
    SelectionTileMap tiles;
    QRect bounds;
    qsizetype bytes = 0;

    void addSpan(int y, int first, int last)
    {
        if (first > last) return;
        const int ty = floorDivide(y, SelectionMask::TileSize);
        const int row = y - ty * SelectionMask::TileSize;
        for (int firstX = first; firstX <= last;) {
            const int tx = floorDivide(firstX, SelectionMask::TileSize);
            const int tileLeft = tx * SelectionMask::TileSize;
            const int localFirst = std::max(firstX, tileLeft) - tileLeft;
            const int localLast = std::min(last, tileLeft + SelectionMask::TileSize - 1) - tileLeft;
            const SelectionTileKey coordinate(tx, ty);
            auto found = tiles.find(coordinate);
            if (found == tiles.end()) {
                if (bytes + SelectionMask::TileBytes > SelectionMask::MaximumBytes) {
                    tiles.clear();
                    bounds = {};
                    bytes = -1;
                    return;
                }
                found = tiles.insert(coordinate, QByteArray(SelectionMask::TileBytes, '\0'));
                bytes += SelectionMask::TileBytes;
            }
            const qsizetype base = qsizetype(row) * SelectionMask::TileSize / 8;
            for (int x = localFirst; x <= localLast; ++x)
                found.value()[base + (x >> 3)] = char(quint8(found.value().at(base + (x >> 3))) | (1u << (x & 7)));
            const int worldFirst = tileLeft + localFirst;
            const int worldLast = tileLeft + localLast;
            const QRect span(worldFirst, y, worldLast - worldFirst + 1, 1);
            bounds = bounds.isNull() ? span : bounds.united(span);
            firstX = worldLast + 1;
        }
    }
};

}

SelectionMask::SelectionMask(QRect bounds, SelectionTileMap tiles)
    : m_bounds(bounds), m_tiles(std::move(tiles))
{
}

QSharedPointer<const SelectionMask> SelectionMask::make(QRect bounds, SelectionTileMap tiles)
{
    if (tiles.isEmpty() || bounds.isEmpty() || bounds.width() <= 0 || bounds.height() <= 0) return {};
    if (qsizetype(tiles.size()) * TileBytes > MaximumBytes) return {};
    for (auto tile = tiles.cbegin(); tile != tiles.cend(); ++tile) {
        if (!validTileCoordinate(QPoint(tile.key().first, tile.key().second)) ||
            tile.value().size() != TileBytes) return {};
    }
    return SelectionMaskSP(new SelectionMask(bounds, std::move(tiles)));
}

QSharedPointer<const SelectionMask> SelectionMask::fromPolygon(const QList<QPointF> &points, const QRect &clip)
{
    if (points.size() < 3 || clip.isEmpty()) return {};
    qreal minimumX = std::numeric_limits<qreal>::max(), minimumY = std::numeric_limits<qreal>::max();
    qreal maximumX = std::numeric_limits<qreal>::lowest(), maximumY = std::numeric_limits<qreal>::lowest();
    for (const auto &point : points) {
        if (!std::isfinite(point.x()) || !std::isfinite(point.y()) ||
            std::abs(point.x()) > CoordinateLimit || std::abs(point.y()) > CoordinateLimit) return {};
        minimumX = std::min(minimumX, point.x());
        minimumY = std::min(minimumY, point.y());
        maximumX = std::max(maximumX, point.x());
        maximumY = std::max(maximumY, point.y());
    }
    const qint64 left = std::max<qint64>(clip.left(), qint64(std::floor(minimumX)));
    const qint64 top = std::max<qint64>(clip.top(), qint64(std::floor(minimumY)));
    const qint64 right = std::min<qint64>(qint64(clip.right()) + 1, qint64(std::ceil(maximumX)));
    const qint64 bottom = std::min<qint64>(qint64(clip.bottom()) + 1, qint64(std::ceil(maximumY)));
    if (left >= right || top >= bottom) return {};
    const qint64 width = right - left, height = bottom - top;
    if (width <= 0 || height <= 0) return {};
    const qint64 tilesWide = (width + TileSize - 1) / TileSize;
    const qint64 tilesHigh = (height + TileSize - 1) / TileSize;
    if (tilesWide > std::numeric_limits<qint32>::max() || tilesHigh > std::numeric_limits<qint32>::max() ||
        tilesWide * tilesHigh > MaximumBytes / TileBytes) return {};

    CoverageBuilder builder;
    QList<qreal> crossings;
    crossings.reserve(points.size());
    for (qint64 y = top; y < bottom && builder.bytes >= 0; ++y) {
        const qreal centerY = qreal(y) + 0.5;
        crossings.clear();
        for (qsizetype index = 0; index < points.size(); ++index) {
            const auto &first = points[index];
            const auto &second = points[(index + 1) % points.size()];
            if ((first.y() > centerY) == (second.y() > centerY)) continue;
            const qreal crossing = first.x() + (centerY - first.y()) * (second.x() - first.x()) / (second.y() - first.y());
            crossings.append(crossing);
        }
        if (crossings.isEmpty()) continue;
        std::sort(crossings.begin(), crossings.end());
        for (qsizetype index = 0; index + 1 < crossings.size(); index += 2) {
            const qint64 firstX = std::max<qint64>(left, qint64(std::ceil(crossings[index] - 0.5)));
            const qint64 lastX = std::min<qint64>(right - 1, qint64(std::ceil(crossings[index + 1] - 0.5)) - 1);
            if (firstX > lastX) continue;
            builder.addSpan(int(y), int(firstX), int(lastX));
        }
    }
    if (builder.bytes < 0) return {};
    return make(builder.bounds, std::move(builder.tiles));
}

QSharedPointer<const SelectionMask> SelectionMask::fromCoverage(const QRect &bounds, const QByteArray &coverage,
                                                                const QRect &clip)
{
    if (bounds.isEmpty() || bounds.width() <= 0 || bounds.height() <= 0 || clip.isEmpty()) return {};
    if (qint64(bounds.width()) * bounds.height() > coverage.size()) return {};
    const QRect area = bounds.intersected(clip);
    if (area.isEmpty()) return {};
    CoverageBuilder builder;
    for (int y = area.top(); y <= area.bottom() && builder.bytes >= 0; ++y) {
        const qsizetype row = qsizetype(y - bounds.top()) * bounds.width() - bounds.left();
        int runStart = -1;
        for (int x = area.left(); x <= area.right() + 1; ++x) {
            const bool selected = x <= area.right() && coverage.at(row + x) != 0;
            if (selected && runStart < 0) runStart = x;
            else if (!selected && runStart >= 0) {
                builder.addSpan(y, runStart, x - 1);
                runStart = -1;
            }
        }
    }
    if (builder.bytes < 0) return {};
    return make(builder.bounds, std::move(builder.tiles));
}

QSharedPointer<const SelectionMask> SelectionMask::fromTiles(const SelectionTileMap &tiles)
{
    if (tiles.isEmpty() || tiles.size() > MaximumBytes / TileBytes) return {};
    QRect bounds;
    for (auto tile = tiles.cbegin(); tile != tiles.cend(); ++tile) {
        const QPoint coordinate(tile.key().first, tile.key().second);
        if (!validTileCoordinate(coordinate) || tile.value().size() != TileBytes) return {};
        const QPoint origin = coordinate * TileSize;
        for (int y = 0; y < TileSize; ++y) {
            for (int x = 0; x < TileSize; ++x) {
                const qsizetype index = qsizetype(y) * TileSize + x;
                if ((quint8(tile.value().at(index >> 3)) & (1u << (index & 7))) == 0) continue;
                const QRect pixel(origin.x() + x, origin.y() + y, 1, 1);
                bounds = bounds.isNull() ? pixel : bounds.united(pixel);
            }
        }
    }
    return make(bounds, tiles);
}

QSharedPointer<const SelectionMask> SelectionMask::translated(QPoint delta, const QRect &clip) const
{
    if (isEmpty() || clip.isEmpty() ||
        std::abs(delta.x()) > CoordinateLimit || std::abs(delta.y()) > CoordinateLimit) return {};
    CoverageBuilder builder;
    for (auto tile = m_tiles.cbegin(); tile != m_tiles.cend(); ++tile) {
        const int tileX = tile.key().first * TileSize;
        const int tileY = tile.key().second * TileSize;
        for (int row = 0; row < TileSize; ++row) {
            const int worldY = tileY + row + delta.y();
            if (worldY < clip.top() || worldY > clip.bottom()) continue;
            const qsizetype base = qsizetype(row) * TileSize / 8;
            int runStart = -1;
            for (int x = 0; x <= TileSize; ++x) {
                const bool selected = x < TileSize &&
                    (quint8(tile.value().at(base + (x >> 3))) & (1u << (x & 7)));
                if (selected && runStart < 0) runStart = x;
                else if (!selected && runStart >= 0) {
                    const int first = std::max(clip.left(), tileX + runStart + delta.x());
                    const int last = std::min(clip.right(), tileX + x - 1 + delta.x());
                    if (first <= last) builder.addSpan(worldY, first, last);
                    runStart = -1;
                }
            }
        }
    }
    return make(builder.bounds, std::move(builder.tiles));
}

bool SelectionMask::contains(int x, int y) const
{
    if (!m_bounds.contains(x, y)) return false;
    const int tx = floorDivide(x, TileSize), ty = floorDivide(y, TileSize);
    const auto tile = m_tiles.constFind(SelectionTileKey(tx, ty));
    if (tile == m_tiles.cend()) return false;
    const int localX = x - tx * TileSize, localY = y - ty * TileSize;
    const qsizetype index = qsizetype(localY) * TileSize + localX;
    return (quint8(tile.value().at(index >> 3)) & (1u << (index & 7))) != 0;
}

bool SelectionMask::operator==(const SelectionMask &other) const
{ return m_bounds == other.m_bounds && m_tiles == other.m_tiles; }
}
