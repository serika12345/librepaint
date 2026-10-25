/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuSelectionMask.h"
#include <QVector>
#include <algorithm>
#include <cmath>
#include <limits>

namespace LibrePaint::Image::Gpu {
namespace {
constexpr qint64 CoordinateLimit = RgbaTile::CoordinateLimit;
constexpr int MaximumFeatherRadius = 512;

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

qsizetype bytesPerTile(SelectionMask::CoverageFormat format)
{
    return format == SelectionMask::CoverageFormat::Alpha ? SelectionMask::AlphaTileBytes : SelectionMask::TileBytes;
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

/** Collects byte-per-pixel alpha coverage in 256 x 256 tiles. */
struct AlphaBuilder
{
    SelectionTileMap tiles;
    QRect bounds;
    qsizetype bytes = 0;

    bool add(int x, int y, quint8 value)
    {
        if (value == 0) return true;
        const int tx = floorDivide(x, SelectionMask::TileSize);
        const int ty = floorDivide(y, SelectionMask::TileSize);
        const SelectionTileKey coordinate(tx, ty);
        auto found = tiles.find(coordinate);
        if (found == tiles.end()) {
            if (bytes + SelectionMask::AlphaTileBytes > SelectionMask::MaximumBytes) {
                tiles.clear();
                bounds = {};
                bytes = -1;
                return false;
            }
            found = tiles.insert(coordinate, QByteArray(SelectionMask::AlphaTileBytes, '\0'));
            bytes += SelectionMask::AlphaTileBytes;
        }
        const int localX = x - tx * SelectionMask::TileSize;
        const int localY = y - ty * SelectionMask::TileSize;
        found.value()[qsizetype(localY) * SelectionMask::TileSize + localX] = char(value);
        const QRect pixel(x, y, 1, 1);
        bounds = bounds.isNull() ? pixel : bounds.united(pixel);
        return true;
    }
};

bool fitsPixelBudget(const QRect &area)
{
    return !area.isEmpty() && qint64(area.width()) * qint64(area.height()) <= SelectionMask::MaximumBytes;
}

/** Separable maximum or minimum filter over byte coverage; the result is alpha. */
QSharedPointer<const SelectionMask> morph(const SelectionMask &source, int xRadius, int yRadius,
                                          const QRect &clip, bool maximum)
{
    const int radiusX = std::min(std::max(xRadius, 0), MaximumFeatherRadius);
    const int radiusY = std::min(std::max(yRadius, 0), MaximumFeatherRadius);
    const QRect area = source.bounds().adjusted(-radiusX, -radiusY, radiusX, radiusY).intersected(clip);
    if (!fitsPixelBudget(area)) return {};
    const qsizetype width = area.width(), height = area.height();
    QVector<quint8> horizontal(width * height);
    for (qint64 y = 0; y < height; ++y) {
        for (qint64 x = 0; x < width; ++x) {
            quint8 value = 0;
            for (int offset = -radiusX; offset <= radiusX; ++offset) {
                const qint64 sourceX = x + offset;
                if (sourceX < 0 || sourceX >= width) continue;
                const quint8 candidate = source.coverage(area.left() + int(sourceX), area.top() + int(y));
                value = maximum ? std::max(value, candidate) : std::min(value, candidate);
            }
            horizontal[y * width + x] = value;
        }
    }
    QVector<quint8> vertical(width * height);
    for (qint64 x = 0; x < width; ++x) {
        for (qint64 y = 0; y < height; ++y) {
            quint8 value = 0;
            for (int offset = -radiusY; offset <= radiusY; ++offset) {
                const qint64 sourceY = y + offset;
                if (sourceY < 0 || sourceY >= height) continue;
                const quint8 candidate = horizontal[sourceY * width + x];
                value = maximum ? std::max(value, candidate) : std::min(value, candidate);
            }
            vertical[y * width + x] = value;
        }
    }
    AlphaBuilder builder;
    for (qint64 y = 0; y < height; ++y)
        for (qint64 x = 0; x < width; ++x)
            if (!builder.add(area.left() + int(x), area.top() + int(y), vertical[y * width + x])) return {};
    return SelectionMask::fromAlphaTiles(std::move(builder.tiles));
}

} // namespace

SelectionMask::SelectionMask(QRect bounds, SelectionTileMap tiles, CoverageFormat format)
    : m_bounds(bounds), m_tiles(std::move(tiles)), m_format(format)
{
}

QSharedPointer<const SelectionMask> SelectionMask::make(QRect bounds, SelectionTileMap tiles, CoverageFormat format)
{
    if (tiles.isEmpty() || bounds.isEmpty() || bounds.width() <= 0 || bounds.height() <= 0) return {};
    const qsizetype expected = bytesPerTile(format);
    if (qsizetype(tiles.size()) * expected > MaximumBytes) return {};
    for (auto tile = tiles.cbegin(); tile != tiles.cend(); ++tile) {
        if (!validTileCoordinate(QPoint(tile.key().first, tile.key().second)) ||
            tile.value().size() != expected) return {};
    }
    return SelectionMaskSP(new SelectionMask(bounds, std::move(tiles), format));
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
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
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
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::fromRectangle(const QRect &rect, const QRect &clip)
{
    const QRect normalized = rect.normalized();
    if (normalized.isEmpty() || clip.isEmpty()) return {};
    const QRect area = normalized.intersected(clip);
    if (area.isEmpty()) return {};
    CoverageBuilder builder;
    for (int y = area.top(); y <= area.bottom() && builder.bytes >= 0; ++y)
        builder.addSpan(y, area.left(), area.right());
    if (builder.bytes < 0) return {};
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::fromEllipse(const QRect &rect, const QRect &clip)
{
    const QRect normalized = rect.normalized();
    if (normalized.isEmpty() || clip.isEmpty()) return {};
    const QRect area = normalized.intersected(clip);
    if (area.isEmpty()) return {};
    const qreal centerX = normalized.x() + normalized.width() / 2.0;
    const qreal centerY = normalized.y() + normalized.height() / 2.0;
    const qreal radiusX = normalized.width() / 2.0;
    const qreal radiusY = normalized.height() / 2.0;
    if (radiusX <= 0 || radiusY <= 0) return {};
    CoverageBuilder builder;
    for (int y = area.top(); y <= area.bottom() && builder.bytes >= 0; ++y) {
        const qreal deltaY = (y + 0.5 - centerY) / radiusY;
        if (std::abs(deltaY) > 1.0) continue;
        const qreal deltaX = radiusX * std::sqrt(1.0 - deltaY * deltaY);
        const qint64 first = std::max<qint64>(area.left(), qint64(std::ceil(centerX - deltaX - 0.5)));
        const qint64 last = std::min<qint64>(area.right(), qint64(std::floor(centerX + deltaX - 0.5)));
        if (first <= last) builder.addSpan(y, int(first), int(last));
    }
    if (builder.bytes < 0) return {};
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::fromAlphaCoverage(const QRect &bounds, const QByteArray &coverage,
                                                                     const QRect &clip)
{
    if (bounds.isEmpty() || bounds.width() <= 0 || bounds.height() <= 0 || clip.isEmpty()) return {};
    if (qint64(bounds.width()) * bounds.height() > coverage.size()) return {};
    const QRect area = bounds.intersected(clip);
    if (area.isEmpty()) return {};
    AlphaBuilder builder;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        const qsizetype row = qsizetype(y - bounds.top()) * bounds.width() - bounds.left();
        for (int x = area.left(); x <= area.right(); ++x)
            if (!builder.add(x, y, quint8(coverage.at(row + x)))) return {};
    }
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
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
    return make(bounds, tiles, CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::fromAlphaTiles(const SelectionTileMap &tiles)
{
    if (tiles.isEmpty() || tiles.size() > MaximumBytes / AlphaTileBytes) return {};
    QRect bounds;
    for (auto tile = tiles.cbegin(); tile != tiles.cend(); ++tile) {
        const QPoint coordinate(tile.key().first, tile.key().second);
        if (!validTileCoordinate(coordinate) || tile.value().size() != AlphaTileBytes) return {};
        const QPoint origin = coordinate * TileSize;
        for (int y = 0; y < TileSize; ++y) {
            for (int x = 0; x < TileSize; ++x) {
                if (quint8(tile.value().at(qsizetype(y) * TileSize + x)) == 0) continue;
                const QRect pixel(origin.x() + x, origin.y() + y, 1, 1);
                bounds = bounds.isNull() ? pixel : bounds.united(pixel);
            }
        }
    }
    return make(bounds, tiles, CoverageFormat::Alpha);
}

QSharedPointer<const SelectionMask> SelectionMask::translated(QPoint delta, const QRect &clip) const
{
    if (isEmpty() || clip.isEmpty() ||
        std::abs(delta.x()) > CoordinateLimit || std::abs(delta.y()) > CoordinateLimit) return {};
    if (m_format == CoverageFormat::Alpha) {
        AlphaBuilder builder;
        for (auto tile = m_tiles.cbegin(); tile != m_tiles.cend(); ++tile) {
            const int tileX = tile.key().first * TileSize;
            const int tileY = tile.key().second * TileSize;
            for (int row = 0; row < TileSize; ++row) {
                const int worldY = tileY + row + delta.y();
                if (worldY < clip.top() || worldY > clip.bottom()) continue;
                for (int x = 0; x < TileSize; ++x) {
                    const quint8 value = quint8(tile.value().at(qsizetype(row) * TileSize + x));
                    if (value == 0) continue;
                    const int worldX = tileX + x + delta.x();
                    if (worldX < clip.left() || worldX > clip.right()) continue;
                    if (!builder.add(worldX, worldY, value)) return {};
                }
            }
        }
        return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
    }
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
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::inverted(const QRect &clip) const
{
    if (clip.isEmpty()) return {};
    if (m_format == CoverageFormat::Alpha) {
        if (!fitsPixelBudget(clip)) return {};
        AlphaBuilder builder;
        for (int y = clip.top(); y <= clip.bottom(); ++y)
            for (int x = clip.left(); x <= clip.right(); ++x)
                if (!builder.add(x, y, quint8(255u - coverage(x, y)))) return {};
        return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
    }
    CoverageBuilder builder;
    for (int y = clip.top(); y <= clip.bottom(); ++y) {
        int runStart = -1;
        for (int x = clip.left(); x <= clip.right() + 1; ++x) {
            const bool selected = x <= clip.right() && !contains(x, y);
            if (selected && runStart < 0) runStart = x;
            else if (!selected && runStart >= 0) {
                builder.addSpan(y, runStart, x - 1);
                runStart = -1;
            }
        }
    }
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::grown(int xRadius, int yRadius, const QRect &clip) const
{
    if (isEmpty() || clip.isEmpty() || xRadius < 0 || yRadius < 0) return {};
    if (m_format == CoverageFormat::Alpha) return morph(*this, xRadius, yRadius, clip, true);
    CoverageBuilder builder;
    for (auto tile = m_tiles.cbegin(); tile != m_tiles.cend(); ++tile) {
        const int tileX = tile.key().first * TileSize;
        const int tileY = tile.key().second * TileSize;
        for (int row = 0; row < TileSize; ++row) {
            const int worldY = tileY + row;
            const qsizetype base = qsizetype(row) * TileSize / 8;
            for (int x = 0; x < TileSize; ++x) {
                if ((quint8(tile.value().at(base + (x >> 3))) & (1u << (x & 7))) == 0) continue;
                const int first = std::max(clip.left(), tileX + x - xRadius);
                const int last = std::min(clip.right(), tileX + x + xRadius);
                for (int y = std::max(clip.top(), worldY - yRadius);
                     y <= std::min(clip.bottom(), worldY + yRadius); ++y)
                    builder.addSpan(y, first, last);
            }
        }
    }
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::shrunk(int xRadius, int yRadius, const QRect &clip) const
{
    if (isEmpty() || clip.isEmpty() || xRadius < 0 || yRadius < 0) return {};
    if (m_format == CoverageFormat::Alpha) {
        if (xRadius == 0 && yRadius == 0) {
            AlphaBuilder builder;
            const QRect area = m_bounds.intersected(clip);
            for (int y = area.top(); y <= area.bottom(); ++y)
                for (int x = area.left(); x <= area.right(); ++x)
                    if (!builder.add(x, y, coverage(x, y))) return {};
            return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
        }
        return morph(*this, xRadius, yRadius, clip, false);
    }
    const auto complement = inverted(clip);
    if (!complement) {
        if (xRadius != 0 || yRadius != 0) return {};
        return make(m_bounds.intersected(clip), m_tiles, CoverageFormat::Binary);
    }
    const auto grownComplement = complement->grown(xRadius, yRadius, clip);
    const auto result = grownComplement ? grownComplement->inverted(clip) : inverted(clip);
    return result;
}

QSharedPointer<const SelectionMask> SelectionMask::subtracted(const SelectionMask &other, const QRect &clip) const
{
    if (isEmpty() || clip.isEmpty()) return {};
    if (m_format == CoverageFormat::Alpha) {
        AlphaBuilder builder;
        const QRect area = m_bounds.intersected(clip);
        for (int y = area.top(); y <= area.bottom(); ++y)
            for (int x = area.left(); x <= area.right(); ++x) {
                const quint8 value = coverage(x, y), removed = other.coverage(x, y);
                if (!builder.add(x, y, value > removed ? quint8(value - removed) : quint8(0))) return {};
            }
        return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
    }
    CoverageBuilder builder;
    const QRect area = m_bounds.intersected(clip);
    for (int y = area.top(); y <= area.bottom(); ++y) {
        int runStart = -1;
        for (int x = area.left(); x <= area.right() + 1; ++x) {
            const bool selected = x <= area.right() && contains(x, y) && !other.contains(x, y);
            if (selected && runStart < 0) runStart = x;
            else if (!selected && runStart >= 0) {
                builder.addSpan(y, runStart, x - 1);
                runStart = -1;
            }
        }
    }
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::border(int xRadius, int yRadius, const QRect &clip) const
{
    const auto inner = shrunk(xRadius, yRadius, clip);
    if (m_format == CoverageFormat::Alpha) {
        if (inner) return subtracted(*inner, clip);
        AlphaBuilder builder;
        const QRect area = m_bounds.intersected(clip);
        for (int y = area.top(); y <= area.bottom(); ++y)
            for (int x = area.left(); x <= area.right(); ++x)
                if (!builder.add(x, y, coverage(x, y))) return {};
        return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
    }
    return inner ? subtracted(*inner, clip) : make(m_bounds.intersected(clip), m_tiles, CoverageFormat::Binary);
}

QSharedPointer<const SelectionMask> SelectionMask::feathered(int radius, const QRect &clip) const
{
    if (isEmpty() || clip.isEmpty() || radius <= 0) return {};
    const int reach = std::min(radius, MaximumFeatherRadius);
    const QRect area = m_bounds.adjusted(-reach, -reach, reach, reach).intersected(clip);
    if (!fitsPixelBudget(area)) return {};
    const qint64 width = area.width(), height = area.height();
    QVector<quint8> plane(width * height);
    for (qint64 y = 0; y < height; ++y)
        for (qint64 x = 0; x < width; ++x)
            plane[y * width + x] = coverage(area.left() + int(x), area.top() + int(y));
    QVector<quint32> horizontal(width * height);
    QVector<quint32> prefix(width + 1);
    for (qint64 y = 0; y < height; ++y) {
        prefix[0] = 0;
        for (qint64 x = 0; x < width; ++x) prefix[x + 1] = prefix[x] + plane[y * width + x];
        for (qint64 x = 0; x < width; ++x) {
            const qint64 first = std::max<qint64>(0, x - reach);
            const qint64 last = std::min<qint64>(width - 1, x + reach);
            horizontal[y * width + x] = prefix[last + 1] - prefix[first];
        }
    }
    const qint64 window = qint64(2 * reach + 1) * qint64(2 * reach + 1);
    QVector<quint32> columns(width, 0);
    for (qint64 y = 0; y <= std::min<qint64>(reach, height - 1); ++y)
        for (qint64 x = 0; x < width; ++x) columns[x] += horizontal[y * width + x];
    AlphaBuilder builder;
    for (qint64 y = 0; y < height; ++y) {
        for (qint64 x = 0; x < width; ++x) {
            const qint64 average = (qint64(columns[x]) + window / 2) / window;
            const quint8 value = quint8(std::min<qint64>(255, average));
            if (!builder.add(area.left() + int(x), area.top() + int(y), value)) return {};
        }
        const qint64 add = y + reach + 1;
        if (add < height)
            for (qint64 x = 0; x < width; ++x) columns[x] += horizontal[add * width + x];
        const qint64 remove = y - reach;
        if (remove >= 0)
            for (qint64 x = 0; x < width; ++x) columns[x] -= horizontal[remove * width + x];
    }
    return make(builder.bounds, std::move(builder.tiles), CoverageFormat::Alpha);
}

quint8 SelectionMask::coverage(int x, int y) const
{
    if (!m_bounds.contains(x, y)) return 0;
    const int tx = floorDivide(x, TileSize), ty = floorDivide(y, TileSize);
    const auto tile = m_tiles.constFind(SelectionTileKey(tx, ty));
    if (tile == m_tiles.cend()) return 0;
    const int localX = x - tx * TileSize, localY = y - ty * TileSize;
    if (m_format == CoverageFormat::Alpha)
        return quint8(tile.value().at(qsizetype(localY) * TileSize + localX));
    const qsizetype index = qsizetype(localY) * TileSize + localX;
    return (quint8(tile.value().at(index >> 3)) & (1u << (index & 7))) ? 255 : 0;
}

bool SelectionMask::operator==(const SelectionMask &other) const
{ return m_format == other.m_format && m_bounds == other.m_bounds && m_tiles == other.m_tiles; }
}
