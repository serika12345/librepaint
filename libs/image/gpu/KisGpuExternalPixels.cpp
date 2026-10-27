/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintImage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include "KisGpuGaussianBrush_p.h"
#include "KisGpuMarkerBrush_p.h"
#include <QHash>
#include <QMap>
#include <algorithm>
#include <cstring>
#include <new>

namespace LibrePaint::Image::Gpu {
namespace {
using Lineage = QList<QPair<quint64, quint64>>;
struct GeometryCache
{
    QHash<const PaintOperation *, std::optional<QList<Detail::MarkerDab>>> markers;
    QHash<const PaintOperation *, std::optional<QList<Detail::FigureFillEdge>>> fills;
    QHash<const PaintOperation *, std::optional<QList<Detail::GaussianDab>>> gaussian;
};

bool writeIntersects(const TileWriteOperation &write, const QRect &block)
{
    if (write.writtenAreas.isEmpty()) {
        for (const auto &tile : write.tiles)
            if (QRect(tile->coordinate() * RgbaTile::Size, QSize(RgbaTile::Size, RgbaTile::Size)).intersects(block)) return true;
    } else {
        for (const auto &area : write.writtenAreas) if (area.intersects(block)) return true;
    }
    return false;
}

std::optional<bool> paintIntersects(const PaintOperation &paint, const QRect &block, GeometryCache &geometry)
{
    if (paint.brush.gaussianWash) {
        auto found = geometry.gaussian.constFind(&paint);
        if (found == geometry.gaussian.cend()) found = geometry.gaussian.insert(&paint, Detail::gaussianDabs(paint, false));
        if (!*found) return std::nullopt;
        if (!paint.figureStyle.outline) return false;
        for (const auto &dab : **found) if (dab.bounds().intersects(block)) return true;
        return false;
    }
    auto found = geometry.markers.constFind(&paint);
    if (found == geometry.markers.cend()) {
        constexpr int limit = RgbaTile::CoordinateLimit;
        found = geometry.markers.insert(&paint, Detail::markerDabs(paint, false, QRect(-limit, -limit, 2 * limit, 2 * limit)));
    }
    if (!*found) return std::nullopt;
    for (const auto &dab : **found) if (Detail::markerDabIntersects(dab, block)) return true;
    if (hasFigureFill(paint.figureStyle)) {
        auto fill = geometry.fills.constFind(&paint);
        if (fill == geometry.fills.cend()) fill = geometry.fills.insert(&paint, Detail::figureFillEdges(paint));
        if (!*fill) return std::nullopt;
        const auto tiles = Detail::figureFillTiles(**fill, paint.brush, block, 1, Detail::PixelHistoryBlockSize);
        if (!tiles || !tiles->isEmpty()) return true;
    }
    return false;
}

bool filterIntersects(const FilterOperation &filter, const QRect &block)
{
    if (filter.selectionRect.isEmpty()) return true;
    return filter.selectionInverted ? !filter.selectionRect.contains(block) :
        filter.selectionRect.intersects(block);
}

std::optional<bool> blockExistsBefore(const Layer &layer, qsizetype before, QRect block, GeometryCache &geometry)
{
    while (before-- > 0) {
        const auto &operation = layer.operations[before];
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            if (writeIntersects(**write, block)) return true;
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            block.translate(-(*translation)->offset);
        } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
            if (!(*crop)->rect.normalized().intersects(block)) return false;
            // A partially cropped history block stays allocated even when its remaining pixels are defaults.
        } else if (std::holds_alternative<FilterSnapshot>(operation)) {
            // A filter rewrites existing pixels and allocates no source block.
            continue;
        } else {
            const auto intersects = paintIntersects(*std::get<PaintSnapshot>(operation), block, geometry);
            if (!intersects || *intersects) return intersects;
        }
    }
    return false;
}

std::optional<Lineage> pixelLineage(const Layer &layer, QRect block, GeometryCache &geometry)
{
    Lineage result;
    QRect historyBlock = block;
    for (qsizetype index = layer.operations.size(); index-- > 0;) {
        const auto &operation = layer.operations[index];
        if (const auto value = std::get_if<TileWriteSnapshot>(&operation)) {
            const auto &write = **value;
            if (writeIntersects(write, block)) {
                result.append({write.writeGroup ? write.writeGroup : write.id.sequence, 0});
                break;
            }
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            block.translate(-(*translation)->offset);
            historyBlock.translate(-(*translation)->offset);
        } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
            const auto keep = (*crop)->rect.normalized();
            if (!keep.contains(block)) {
                const auto exists = blockExistsBefore(layer, index, historyBlock, geometry);
                if (!exists) return std::nullopt;
                if (!*exists) continue;
                result.append({(*crop)->id.sequence, (*crop)->version});
                block = block.intersected(keep);
                if (block.isEmpty()) break;
            }
        } else if (const auto filter = std::get_if<FilterSnapshot>(&operation)) {
            // Filters retain their frozen coverage; a block inside that coverage
            // gains one lineage entry so later external writes can distinguish it.
            if (filterIntersects(**filter, block)) result.append({(*filter)->id.sequence, (*filter)->version});
        } else {
            const auto &paint = *std::get<PaintSnapshot>(operation);
            const auto intersects = paintIntersects(paint, block, geometry);
            if (!intersects) return std::nullopt;
            if (*intersects) result.append({paint.id.sequence, paint.version});
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

PaintImageSP PaintImage::cloneRasterLayer(const Snapshot &version, LayerId layer, QRect region,
                                        const QUuid &destinationUuid, qsizetype maximumWorkingBytes) const
{
    if (!onThread() || !m_d->handle || !version || m_d->document->hasActiveStroke() ||
        version->documentId() != m_d->document->snapshot()->documentId() || destinationUuid.isNull() ||
        maximumWorkingBytes < 0) return {};
    constexpr int limit = RgbaTile::CoordinateLimit, size = RgbaTile::Size;
    if (region.isEmpty() || region.width() > limit || region.height() > limit ||
        region.x() < -limit || region.y() < -limit || qint64(region.x()) + region.width() > limit ||
        qint64(region.y()) + region.height() > limit) return {};
    const qint64 columns = (qint64(region.width()) + size - 1) / size;
    const qint64 rows = (qint64(region.height()) + size - 1) / size;
    const int pixelSize = pixelByteCount(version->format);
    const int bytesPerTile = RgbaTile::byteCount(version->format);
    const qint64 pixelBytes = qint64(region.width()) * region.height() * pixelSize;
    const qint64 tileBytes = columns * rows * bytesPerTile;
    if (pixelBytes + 2 * tileBytes > maximumWorkingBytes) return {};
    if (!layer) layer = version->directProjectionLayer();
    auto pixels = m_d->execution.readRgbaPixels(m_d->handle, version, region, layer, pixelBytes);
    if (pixels.code != ExecutionCode::Success) return {};
    RasterLayer properties{QStringLiteral("Projection"), true, destinationUuid, {}};
    RasterImage input;
    input.size = region.size();
    input.format = version->format;
    input.layers = {properties};
    qsizetype retainedBytes = 0;
    for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
        QByteArray tile(bytesPerTile, '\0');
        const int width = std::min(size, region.width() - x * size);
        const int height = std::min(size, region.height() - y * size);
        for (int row = 0; row < height; ++row) {
            const auto source = pixels.bytes.constData() + (qint64(y * size + row) * region.width() + x * size) * pixelSize;
            std::memcpy(tile.data() + row * size * pixelSize, source, width * pixelSize);
        }
        if (std::none_of(tile.cbegin(), tile.cend(), [](char value) { return value != 0; })) continue;
        if (bytesPerTile > m_d->limits.rasterBytes - retainedBytes) return {};
        retainedBytes += bytesPerTile;
        input.layers.first().tiles.append({{x, y}, std::move(tile), version->format});
    }
    OperationDocument prepared(input, m_d->limits);
    pixels.bytes.clear();
    input.layers.clear();
    return cloneLayerForest({{prepared.snapshot(), prepared.activeLayer(), properties}}, region.size());
}

Result PaintImage::writeRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes)
{ return applyRgbaPixels(layer, region, bytes, false); }

Result PaintImage::writeRgbaPixels(const LayerPixelReferenceSP &pixels, QRect region, const QByteArray &bytes)
{
    if (!onThread() || !pixels || m_d->layerPixels.value(pixels->m_uuid).toStrongRef() != pixels)
        return Result::InvalidInput;
    if (pixels->m_version->documentId() != m_d->document->snapshot()->documentId()) return Result::StaleVersion;
    return applyRgbaPixels(pixels->m_layer, region, bytes, false, &pixels->m_version);
}

Result PaintImage::replaceRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes)
{ return applyRgbaPixels(layer, region, bytes, true); }

Result PaintImage::applyRgbaPixels(LayerId layer, QRect region, const QByteArray &bytes, bool recordHistory,
                                 Snapshot *retainedVersion)
{
    if (!onThread()) return Result::InvalidInput;
    auto &d = *m_d;
    auto &document = *d.document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    const auto current = retainedVersion ? *retainedVersion : document.snapshot();
    const auto selectedLayer = std::find_if(current->layers.cbegin(), current->layers.cend(),
        [layer](const auto &value) { return value.id == layer; });
    if (selectedLayer == current->layers.cend() || selectedLayer->kind != LayerKind::Paint) return Result::LayerUnavailable;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit)
        return Result::InvalidInput;
    const int pixelSize = pixelByteCount(current->format);
    const int bytesPerTile = RgbaTile::byteCount(current->format);
    const qint64 byteCount = qint64(region.width()) * region.height() * pixelSize;
    if (bytes.size() < byteCount) return Result::InvalidInput;
    if (byteCount > d.limits.rasterBytes) return Result::CapacityExceeded;
    if (region.isEmpty()) return Result::Success;
    const auto &selected = *selectedLayer;
    constexpr int size = RgbaTile::Size;
    const qint64 columns = Detail::pixelTileIndex(region.right(), size) - Detail::pixelTileIndex(region.left(), size) + 1;
    const qint64 rows = Detail::pixelTileIndex(region.bottom(), size) - Detail::pixelTileIndex(region.top(), size) + 1;
    if (columns * rows > (d.limits.rasterBytes - current->rasterBytes) / bytesPerTile)
        return Result::CapacityExceeded;
    Result adopted;
    try {
        GeometryCache geometry;
        QHash<quint64, Lineage> lineages;
        QList<OperationDocument::VersionPixelWrite> writes;
        qsizetype compressedBytes = 0;
        auto versions = recordHistory ? QList<Snapshot>{current} : document.m_history;
        if (retainedVersion && !versions.contains(current)) versions.append(current);
        for (const auto &version : versions) {
            const Layer *candidate = nullptr;
            for (const auto &value : version->layers) if (value.id == layer) candidate = &value;
            if (!candidate || candidate->uuid != selected.uuid || candidate->defaultPixel != selected.defaultPixel) continue;
            const qint64 dx = qint64(candidate->position.x()) - selected.position.x();
            const qint64 dy = qint64(candidate->position.y()) - selected.position.y();
            if (qint64(region.left()) + dx < -limit || qint64(region.right()) + dx >= limit ||
                qint64(region.top()) + dy < -limit || qint64(region.bottom()) + dy >= limit)
                return Result::InvalidInput;
            const QPoint placement{int(dx), int(dy)};
            QMap<quint64, QList<QRect>> areas;
            constexpr int blockSize = Detail::PixelHistoryBlockSize;
            const auto blockIndex = [](qint64 value) { return value >= 0 ? value / blockSize : (value - blockSize + 1) / blockSize; };
            for (qint64 y = blockIndex(qint64(region.top()) - selected.position.y()); y <= blockIndex(qint64(region.bottom()) - selected.position.y()); ++y) {
                for (qint64 x = blockIndex(qint64(region.left()) - selected.position.x()); x <= blockIndex(qint64(region.right()) - selected.position.x()); ++x) {
                    const auto blockKey = Detail::key(int(x), int(y));
                    const QRect block(int(x * blockSize + selected.position.x()), int(y * blockSize + selected.position.y()), blockSize, blockSize);
                    if (candidate->operations != selected.operations) {
                        auto lineage = lineages.constFind(blockKey);
                        if (lineage == lineages.cend()) {
                            const auto selectedLineage = pixelLineage(selected, block, geometry);
                            if (!selectedLineage) return Result::CapacityExceeded;
                            lineage = lineages.insert(blockKey, *selectedLineage);
                        }
                        const auto candidateLineage = pixelLineage(*candidate, block.translated(placement), geometry);
                        if (!candidateLineage) return Result::CapacityExceeded;
                        if (*candidateLineage != *lineage) continue;
                    }
                    const auto area = block.intersected(region).translated(placement);
                    for (int ty = Detail::pixelTileIndex(area.top(), size); ty <= Detail::pixelTileIndex(area.bottom(), size); ++ty)
                        for (int tx = Detail::pixelTileIndex(area.left(), size); tx <= Detail::pixelTileIndex(area.right(), size); ++tx)
                            areas[Detail::key(tx, ty)].append(area.intersected(QRect(tx * size, ty * size, size, size)));
                }
            }
            if (areas.isEmpty()) continue;
            if (areas.size() > (d.limits.rasterBytes - version->rasterBytes) / bytesPerTile)
                return Result::CapacityExceeded;
            OperationDocument::VersionPixelWrite write;
            write.before = version;
            write.layer = layer;
            for (auto tile = areas.cbegin(); tile != areas.cend(); ++tile) {
                const auto coordinate = Detail::coordinate(tile.key());
                const QRect tileRegion(coordinate * size, QSize(size, size));
                auto pixels = d.execution.readRgbaPixels(d.handle, version, tileRegion, layer, bytesPerTile);
                if (pixels.code != ExecutionCode::Success) return readFailure(pixels.code);
                for (const auto &area : tile.value()) {
                    for (int y = area.top(); y <= area.bottom(); ++y) {
                        const qsizetype source = (qsizetype(y - placement.y() - region.y()) * region.width() + area.x() - placement.x() - region.x()) * pixelSize;
                        const qsizetype destination = ((y - tileRegion.y()) * size + area.x() - tileRegion.x()) * pixelSize;
                        std::memcpy(pixels.bytes.data() + destination, bytes.constData() + source, size_t(area.width()) * pixelSize);
                    }
                    write.areas.append(area);
                }
                const auto checkpoint = RasterTile::fromPixels({coordinate, pixels.bytes, version->format});
                if (!checkpoint) return Result::CapacityExceeded;
                if (checkpoint->compressedByteCount() > d.limits.compressedWriteBytes - compressedBytes)
                    return Result::CapacityExceeded;
                compressedBytes += checkpoint->compressedByteCount();
                write.tiles.append(checkpoint);
            }
            writes.append(write);
        }
        adopted = document.applyPixelVersions(writes, recordHistory, retainedVersion);
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
    if (recordHistory) return publish(adopted);
    if (adopted == Result::Success) requestCurrentVersion();
    return adopted;
}
}
