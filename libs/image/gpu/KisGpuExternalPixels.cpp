/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintImage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QHash>
#include <QMap>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace LibrePaint::Image::Gpu {
namespace {
using Lineage = QList<QPair<quint64, quint64>>;
using GeometryCache = QHash<const PaintOperation *, QList<Detail::PaintSegment>>;

Lineage pixelLineage(const Layer &layer, QRect block, QSize canvas, GeometryCache &geometry)
{
    Lineage result;
    for (auto it = layer.operations.crbegin(); it != layer.operations.crend(); ++it) {
        if (const auto value = std::get_if<TileWriteSnapshot>(&*it)) {
            const auto &write = **value;
            bool intersects = false;
            if (write.writtenAreas.isEmpty()) {
                for (const auto &tile : write.tiles)
                    intersects |= QRect(tile->coordinate() * RgbaTile::Size, QSize(RgbaTile::Size, RgbaTile::Size)).intersects(block);
            } else {
                for (const auto &area : write.writtenAreas) intersects |= area.intersects(block);
            }
            if (intersects) {
                result.append({write.writeGroup ? write.writeGroup : write.id.sequence, 0});
                break;
            }
        } else {
            const auto &paint = *std::get<PaintSnapshot>(*it);
            auto found = geometry.constFind(&paint);
            if (found == geometry.cend()) {
                const auto step = quint32(std::max<qint64>(1, std::llround(paint.brush.diameter * paint.brush.spacing * 16)));
                found = geometry.insert(&paint, Detail::paintGeometry(paint, step));
            }
            const auto diameter = quint32(std::max<qint64>(1, std::llround(paint.brush.diameter * 16)));
            for (const auto &segment : *found) {
                const auto tiles = Detail::paintSegmentTiles(segment, diameter, block.intersected(QRect(QPoint(), canvas)),
                                                             canvas, 1, Detail::PixelHistoryBlockSize);
                if (!tiles || !tiles->isEmpty()) {
                    result.append({paint.id.sequence, paint.version});
                    break;
                }
            }
        }
    }
    return result;
}

Result readFailure(ExecutionCode code)
{
    if (code == ExecutionCode::CapacityExceeded) return Result::CapacityExceeded;
    if (code == ExecutionCode::Busy) return Result::Busy;
    return Result::DeviceFailure;
}
}

Result PaintImage::writeRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes)
{ return applyRgbaPixels(layer, region, bytes, false); }

Result PaintImage::replaceRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes)
{ return applyRgbaPixels(layer, region, bytes, true); }

Result PaintImage::applyRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes, bool recordHistory)
{
    if (!onThread()) return Result::InvalidInput;
    auto &d = *m_d;
    auto &document = *d.document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    const auto current = document.snapshot();
    const auto index = document.layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit)
        return Result::InvalidInput;
    const qint64 byteCount = qint64(region.width()) * region.height() * 4;
    if (bytes.size() < byteCount) return Result::InvalidInput;
    if (byteCount > d.limits.rasterBytes) return Result::CapacityExceeded;
    if (region.isEmpty()) return Result::Success;
    const auto &selected = current->layers[index];
    constexpr int size = RgbaTile::Size;
    const qint64 columns = Detail::pixelTileIndex(region.right(), size) - Detail::pixelTileIndex(region.left(), size) + 1;
    const qint64 rows = Detail::pixelTileIndex(region.bottom(), size) - Detail::pixelTileIndex(region.top(), size) + 1;
    if (columns * rows > (d.limits.rasterBytes - current->rasterBytes) / RgbaTile::ByteCount)
        return Result::CapacityExceeded;
    Result adopted;
    try {
        GeometryCache geometry;
        QHash<quint64, Lineage> lineages;
        QList<OperationDocument::VersionPixelWrite> writes;
        qsizetype compressedBytes = 0;
        const auto versions = recordHistory ? QList<Snapshot>{current} : document.m_history;
        for (const auto &version : versions) {
            const Layer *candidate = nullptr;
            for (const auto &value : version->layers) if (value.id == layer) candidate = &value;
            if (!candidate || candidate->defaultPixel != selected.defaultPixel) continue;
            QMap<quint64, QList<QRect>> areas;
            constexpr int blockSize = Detail::PixelHistoryBlockSize;
            for (int y = Detail::pixelTileIndex(region.top(), blockSize); y <= Detail::pixelTileIndex(region.bottom(), blockSize); ++y) {
                for (int x = Detail::pixelTileIndex(region.left(), blockSize); x <= Detail::pixelTileIndex(region.right(), blockSize); ++x) {
                    const auto blockKey = Detail::key(x, y);
                    const QRect block(x * blockSize, y * blockSize, blockSize, blockSize);
                    if (candidate->operations != selected.operations) {
                        auto lineage = lineages.constFind(blockKey);
                        if (lineage == lineages.cend()) lineage = lineages.insert(blockKey, pixelLineage(selected, block, current->size, geometry));
                        if (pixelLineage(*candidate, block, version->size, geometry) != *lineage) continue;
                    }
                    const auto area = block.intersected(region);
                    areas[Detail::key(Detail::pixelTileIndex(area.x(), size), Detail::pixelTileIndex(area.y(), size))].append(area);
                }
            }
            if (areas.isEmpty()) continue;
            if (areas.size() > (d.limits.rasterBytes - version->rasterBytes) / RgbaTile::ByteCount)
                return Result::CapacityExceeded;
            OperationDocument::VersionPixelWrite write;
            write.before = version;
            write.layer = layer;
            for (auto tile = areas.cbegin(); tile != areas.cend(); ++tile) {
                const auto coordinate = Detail::coordinate(tile.key());
                const QRect tileRegion(coordinate * size, QSize(size, size));
                auto pixels = d.execution.readRgbaPixels(d.handle, version, tileRegion, layer, RgbaTile::ByteCount);
                if (pixels.code != ExecutionCode::Success) return readFailure(pixels.code);
                for (const auto &area : tile.value()) {
                    for (int y = area.top(); y <= area.bottom(); ++y) {
                        const qsizetype source = (qsizetype(y - region.y()) * region.width() + area.x() - region.x()) * 4;
                        const qsizetype destination = ((y - tileRegion.y()) * size + area.x() - tileRegion.x()) * 4;
                        std::memcpy(pixels.bytes.data() + destination, bytes.constData() + source, size_t(area.width()) * 4);
                    }
                    write.areas.append(area);
                }
                const auto checkpoint = RasterTile::fromPixels({coordinate, pixels.bytes});
                if (!checkpoint) return Result::CapacityExceeded;
                if (checkpoint->compressedData().size() > d.limits.recoveryBytes - compressedBytes)
                    return Result::CapacityExceeded;
                compressedBytes += checkpoint->compressedData().size();
                write.tiles.append(checkpoint);
            }
            writes.append(write);
        }
        adopted = document.applyPixelVersions(writes, recordHistory);
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
    if (recordHistory) return publish(adopted);
    if (adopted == Result::Success) requestCurrentVersion();
    return adopted;
}
}
