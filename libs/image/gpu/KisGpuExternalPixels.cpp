/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintImage_p.h"
#include "KisGpuPaintGeometry_p.h"
#include "KisGpuAutoBrush_p.h"
#include "KisGpuMarkerBrush_p.h"
#include <QHash>
#include <QMap>
#include <QRegion>
#include <tuple>
#include <algorithm>
#include <cstring>
#include <new>

namespace LibrePaint::Image::Gpu {
namespace {
using Lineage = QList<QPair<quint64, quint64>>;
struct GeometryCache
{
    PixelFormat format;
    QHash<const PaintOperation *, std::optional<QList<Detail::MarkerDab>>> markers;
    QHash<const PaintOperation *, std::optional<QList<Detail::FigureFillEdge>>> fills;
    QHash<const PaintOperation *, std::optional<QList<Detail::AutoBrushDab>>> gaussian;
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
    if (paint.compositionBounds && !paint.compositionBounds->intersects(block)) return false;
    if (paint.brush.autoBrush) {
        auto found = geometry.gaussian.constFind(&paint);
        if (found == geometry.gaussian.cend()) found = geometry.gaussian.insert(&paint, Detail::autoBrushDabs(paint, false));
        if (!*found) return std::nullopt;
        for (const auto &dab : **found) if (dab.bounds().intersects(block)) return true;
    } else {
        auto found = geometry.markers.constFind(&paint);
        if (found == geometry.markers.cend()) {
            constexpr int limit = RgbaTile::CoordinateLimit;
            found = geometry.markers.insert(&paint, Detail::markerDabs(paint, false, QRect(-limit, -limit, 2 * limit, 2 * limit)));
        }
        if (!*found) return std::nullopt;
        for (const auto &dab : **found)
            if (Detail::usesAllocatedSourceRegion(paint, geometry.format) ? dab.bounds().intersects(block) : Detail::markerDabIntersects(dab, block)) return true;
    }
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
    if (filter.kind >= FilterKind::MaximizeChannel && !filter.applyRect.intersects(block)) return false;
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
            if ((*translation)->selectedPixels) {
                if ((*translation)->clearRect.intersects(block) ||
                    (*translation)->copyRect.translated((*translation)->offset).intersects(block)) return true;
            } else block.translate(-(*translation)->offset);
        } else if (const auto affine = std::get_if<AffineSnapshot>(&operation)) {
            if ((*affine)->allocatedBlocks) {
                return std::any_of((*affine)->allocatedBlocks->cbegin(), (*affine)->allocatedBlocks->cend(), [&](const auto &area) { return area.intersects(block); });
            }
            if (!(*affine)->selection) return true;
            auto transformed = (*affine)->selection->copyRect;
            for (const auto &pass : (*affine)->plan->passes) transformed = affineOutputRegion(pass, transformed);
            if (transformed.intersects(block) || (*affine)->selection->clearRect.intersects(block)) return true;
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
    return layer.initialProjection && std::any_of(layer.initialProjection->allocatedBlocks.cbegin(),
        layer.initialProjection->allocatedBlocks.cend(), [&](const auto &area) { return area.intersects(block); });
}

std::optional<Lineage> pixelLineage(const Layer &layer, QRect block, GeometryCache &geometry, qsizetype firstOperation = 0)
{
    Lineage result;
    QRect historyBlock = block;
    for (qsizetype index = layer.operations.size(); index-- > firstOperation;) {
        const auto &operation = layer.operations[index];
        if (const auto value = std::get_if<TileWriteSnapshot>(&operation)) {
            const auto &write = **value;
            if (writeIntersects(write, block)) {
                result.append({write.writeGroup ? write.writeGroup : write.id.sequence, 0});
                return result;
            }
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            if ((*translation)->selectedPixels) {
                if ((*translation)->clearRect.intersects(block) ||
                    (*translation)->copyRect.translated((*translation)->offset).intersects(block)) {
                    result.append({(*translation)->id.sequence, (*translation)->version});
                    return result;
                }
            } else {
                block.translate(-(*translation)->offset);
                historyBlock.translate(-(*translation)->offset);
            }
        } else if (const auto affine = std::get_if<AffineSnapshot>(&operation)) {
            bool changed = !(*affine)->selection;
            if ((*affine)->selection) {
                auto transformed = (*affine)->selection->copyRect;
                for (const auto &pass : (*affine)->plan->passes) transformed = affineOutputRegion(pass, transformed);
                changed = transformed.intersects(block) || (*affine)->selection->clearRect.intersects(block);
            }
            if (changed) { result.append({(*affine)->id.sequence, (*affine)->version}); return result; }
        } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
            const auto keep = (*crop)->rect.normalized();
            if (!keep.contains(block)) {
                const auto exists = blockExistsBefore(layer, index, historyBlock, geometry);
                if (!exists) return std::nullopt;
                if (!*exists) continue;
                result.append({(*crop)->id.sequence, (*crop)->version});
                block = block.intersected(keep);
                if (block.isEmpty()) return result;
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
    if (firstOperation == 0 && layer.initialProjection && std::any_of(layer.initialProjection->allocatedBlocks.cbegin(),
        layer.initialProjection->allocatedBlocks.cend(), [&](const auto &area) { return area.intersects(block); }))
        result.append({0, quint64(quintptr(layer.initialProjection.data()))});
    return result;
}

using MaskLineage = QList<std::tuple<QUuid, quint64, quint64>>;

std::optional<MaskLineage> maskLineage(SelectionMaskSP mask, QRect block, GeometryCache &geometry)
{
    MaskLineage result;
    while (mask) {
        if (const auto &translation = mask->translationSource()) {
            block.translate(-translation->offset);
            mask = translation->source;
            continue;
        }
        const auto &paint = mask->paintSource();
        if (!paint || !paint->previous || paint->firstEditOperation < 0) {
            result.append({{}, quint64(quintptr(mask.data())), 0});
            return result;
        }
        if (paint->cleared && std::any_of(paint->previous->allocatedBlocks().cbegin(),
            paint->previous->allocatedBlocks().cend(), [&](const auto &area) { return area.intersects(block); })) {
            result.append({paint->source->documentId(), 0, paint->source->revision()});
            return result;
        }
        const auto &layers = paint->source->layers;
        const auto source = std::find_if(layers.cbegin(), layers.cend(), [&](const auto &layer) { return layer.id == paint->layer; });
        if (source == layers.cend()) return std::nullopt;
        const auto edits = pixelLineage(*source, block, geometry, paint->firstEditOperation);
        if (!edits) return std::nullopt;
        if (!edits->isEmpty() && !paint->externalWriteUuid.isNull()) {
            result.append({paint->externalWriteUuid, 0, 0});
            return result;
        }
        for (const auto &edit : *edits) result.append({paint->source->documentId(), edit.first, edit.second});
        mask = paint->previous;
    }
    return std::nullopt;
}

Result readFailure(ExecutionCode code)
{
    if (code == ExecutionCode::CapacityExceeded) return Result::CapacityExceeded;
    if (code == ExecutionCode::Busy) return Result::Busy;
    return Result::DeviceFailure;
}
}

PaintImageSP PaintImage::cloneRasterLayer(const Snapshot &version, LayerId layer, QRect region,
                                        const QUuid &destinationUuid, qsizetype maximumWorkingBytes,
                                        const SelectionMaskSP &mask, bool invertMask) const
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
    if (!layer) {
        const auto direct = version->directProjectionLayer();
        QUuid uuid;
        for (const auto &node : version->layers) if (node.id == direct) { uuid = node.uuid; break; }
        const bool filtered = std::any_of(version->layers.cbegin(), version->layers.cend(), [&](const auto &node) {
            return node.kind == LayerKind::FilterMask && node.parentUuid == uuid && node.visible;
        });
        if (!filtered) layer = direct;
    }
    auto pixels = m_d->execution.readRgbaPixels(m_d->handle, version, region, layer, pixelBytes, mask, invertMask);
    if (pixels.code != ExecutionCode::Success) return {};
    RasterLayer properties{QStringLiteral("Projection"), true, destinationUuid, {}};
    RasterImage input;
    input.size = region.size();
    input.format = version->format;
    input.colorProfile = version->colorProfile;
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
        GeometryCache geometry{current->format, {}, {}, {}};
        QHash<quint64, Lineage> lineages;
        QList<OperationDocument::VersionPixelWrite> writes;
        qsizetype compressedBytes = 0;
        auto versions = recordHistory ? QList<Snapshot>{current} : document.m_history;
        if (retainedVersion && !versions.contains(current)) versions.append(current);
        for (const auto &version : versions) {
            const Layer *candidate = nullptr;
            for (const auto &value : version->layers) if (value.id == layer) candidate = &value;
            if (version->format != current->format || version->colorProfile != current->colorProfile) continue;
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
                    if (candidate->operations != selected.operations || candidate->initialProjection != selected.initialProjection) {
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
Result PaintImage::writeMaskPosition(const QUuid &maskUuid, QPoint position,
                                    SelectionMaskSP *retainedCoverage, QPoint retainedPosition)
{ return applyMaskPosition(maskUuid, position, retainedCoverage, retainedPosition, true); }

Result PaintImage::applyMaskPosition(const QUuid &maskUuid, QPoint position,
                                    SelectionMaskSP *retainedCoverage, QPoint retainedPosition, bool request)
{
    if (!onThread() || maskUuid.isNull() || !retainedCoverage || !*retainedCoverage) return Result::InvalidInput;
    auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success && status != Result::Closed) return status;
    if (position == retainedPosition) return Result::Success;
    const auto selected = *retainedCoverage;
    Snapshot source;
    LayerId layerId = 0;
    bool child = false;
    if (status != Result::Closed) {
        for (const auto &version : document.m_history) {
            for (const auto &layer : version->layers) {
                if (editableMaskCoverage(layer, maskUuid) != selected) continue;
                source = version;
                layerId = layer.id;
                child = layer.transparencyMaskUuid == maskUuid;
                break;
            }
            if (source == document.snapshot()) break;
        }
    }
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    const auto move = [&](const SelectionMaskSP &coverage, QPoint before) -> SelectionMaskSP {
        const qint64 x = qint64(position.x()) - before.x(), y = qint64(position.y()) - before.y();
        if (x < -2 * limit || x > 2 * limit || y < -2 * limit || y > 2 * limit) return {};
        const auto bounds = coverage->allocatedBounds() | coverage->bounds();
        if (!bounds.isEmpty() && (qint64(bounds.left()) + x < -limit || qint64(bounds.right()) + x >= limit ||
            qint64(bounds.top()) + y < -limit || qint64(bounds.bottom()) + y >= limit)) return {};
        return translatedSelectionMask(coverage, QPoint(int(x), int(y)), domain);
    };
    try {
        QList<OperationDocument::VersionMaskWrite> writes;
        SelectionMaskSP output;
        if (source) {
            QHash<QPair<quintptr, QPoint>, SelectionMaskSP> prepared;
            for (const auto &version : document.layerPositionVersions(source, layerId, child)) {
                for (const auto &layer : version->layers) {
                    if (layer.id != layerId) continue;
                    const auto coverage = editableMaskCoverage(layer, maskUuid);
                    if (!coverage) return Result::LayerUnavailable;
                    const auto before = child ? layer.transparencyMaskPosition : layer.position;
                    const auto key = qMakePair(quintptr(coverage.data()), before);
                    if (!prepared.contains(key)) {
                        const auto moved = move(coverage, before);
                        if (!moved) return Result::CapacityExceeded;
                        prepared.insert(key, moved);
                    }
                    const auto moved = prepared.value(key);
                    writes.append({version, layerId, maskUuid, moved, position});
                    if (version == source) output = moved;
                }
            }
        } else output = move(selected, retainedPosition);
        if (!output) return Result::CapacityExceeded;
        const auto result = writes.isEmpty() ? Result::Success : document.applyMaskVersions(writes);
        if (result == Result::Success) {
            *retainedCoverage = output;
            if (request && !writes.isEmpty()) requestCurrentVersion();
        }
        return result;
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}

Result PaintImage::writeMaskPixels(LayerId layer, const QUuid &maskUuid, QRect region, const QByteArray &bytes,
                                   SelectionMaskSP *retainedCoverage, QSize retainedCanvasSize, QPoint retainedPosition)
{
    if (!onThread() || maskUuid.isNull()) return Result::InvalidInput;
    auto &d = *m_d;
    auto &document = *d.document;
    const auto status = document.checkMutation();
    const bool closed = status == Result::Closed;
    if (status != Result::Success && !(closed && retainedCoverage && *retainedCoverage)) return status;
    const auto current = closed ? Snapshot{} : document.snapshot();
    const auto findTarget = [&](const Snapshot &version) -> QPair<LayerId, SelectionMaskSP> {
        if (!version) return {};
        for (const auto &node : version->layers) {
            const auto coverage = editableMaskCoverage(node, maskUuid);
            if (coverage) return {node.id, coverage};
        }
        return {};
    };
    const auto maskPosition = [&](const Snapshot &version) {
        if (version) for (const auto &node : version->layers)
            if (editableMaskCoverage(node, maskUuid)) return node.uuid == maskUuid ? node.position : node.transparencyMaskPosition;
        return retainedPosition;
    };
    const auto currentTarget = findTarget(current);
    if (layer && currentTarget.first && currentTarget.first != layer) return Result::LayerUnavailable;
    const auto selected = retainedCoverage ? *retainedCoverage : currentTarget.second;
    if (!selected) return Result::LayerUnavailable;
    const auto canvas = !retainedCanvasSize.isEmpty() ? retainedCanvasSize : current ? current->size : QSize{};
    if (canvas.isEmpty() || canvas.width() > RgbaTile::CoordinateLimit || canvas.height() > RgbaTile::CoordinateLimit)
        return Result::InvalidInput;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    const qint64 count = qint64(region.width()) * region.height();
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit || bytes.size() < count)
        return Result::InvalidInput;
    if (count > SelectionMask::MaximumBytes || count > d.limits.rasterBytes) return Result::CapacityExceeded;
    if (region.isEmpty()) return Result::Success;
    try {
        RasterImage contextInput;
        contextInput.size = canvas;
        OperationDocument context(contextInput, d.limits);
        const auto sourceContext = context.snapshot();
        GeometryCache geometry{PixelFormat::Rgba8, {}, {}, {}};
        QHash<quint64, MaskLineage> selectedLineages;
        QMap<std::tuple<int, int, int, int>, SelectionMaskSP> inputs;
        qsizetype retainedBytes = 0;
        const QUuid writeUuid = QUuid::createUuid();
        QList<OperationDocument::VersionMaskWrite> writes;
        constexpr int blockSize = Detail::PixelHistoryBlockSize;
        const auto blockIndex = [](qint64 value) { return value >= 0 ? value / blockSize : (value - blockSize + 1) / blockSize; };
        const auto selectedPosition = retainedCoverage ? retainedPosition : maskPosition(current);
        const auto origin = selectedPosition;
        auto versions = closed ? QList<Snapshot>{} : document.m_history;
        bool selectedInHistory = false;
        for (const auto &version : versions) if (findTarget(version).second == selected) selectedInHistory = true;
        if (!selectedInHistory) versions.append(sourceContext);
        SelectionMaskSP selectedOutput;
        for (qsizetype index = 0; index < versions.size(); ++index) {
            const auto &version = versions[index];
            const bool retainedOnly = !selectedInHistory && index == versions.size() - 1;
            const auto target = findTarget(version);
            const auto previous = retainedOnly ? selected : target.second;
            if (!previous) continue;
            const auto previousPosition = retainedOnly ? selectedPosition : maskPosition(version);
            const qint64 dx = qint64(previousPosition.x()) - selectedPosition.x(), dy = qint64(previousPosition.y()) - selectedPosition.y();
            if (qint64(region.left()) + dx < -limit || qint64(region.right()) + dx >= limit ||
                qint64(region.top()) + dy < -limit || qint64(region.bottom()) + dy >= limit) return Result::InvalidInput;
            const QPoint placement{int(dx), int(dy)};
            QRegion allowed;
            for (qint64 y = blockIndex(qint64(region.top()) - origin.y()); y <= blockIndex(qint64(region.bottom()) - origin.y()); ++y)
                for (qint64 x = blockIndex(qint64(region.left()) - origin.x()); x <= blockIndex(qint64(region.right()) - origin.x()); ++x) {
                    const auto key = Detail::key(int(x), int(y));
                    const QRect block(int(x * blockSize + origin.x()), int(y * blockSize + origin.y()), blockSize, blockSize);
                    if (previous != selected) {
                        auto selectedLineage = selectedLineages.constFind(key);
                        if (selectedLineage == selectedLineages.cend()) {
                            const auto lineage = maskLineage(selected, block, geometry);
                            if (!lineage) return Result::CapacityExceeded;
                            selectedLineage = selectedLineages.insert(key, *lineage);
                        }
                        const auto lineage = maskLineage(previous, block.translated(placement), geometry);
                        if (!lineage) return Result::CapacityExceeded;
                        if (*lineage != *selectedLineage) continue;
                    }
                    allowed += block.intersected(region).translated(placement);
                }
            if (allowed.isEmpty()) continue;
            const auto image = createMaskImage(sourceContext, previous);
            if (!image) return readFailure(d.lastRequest.code);
            const auto sourceLayer = image->operations().activeLayer();
            const auto firstEdit = image->operations().snapshot()->layers.first().operations.size();
            if (image->clearSelection() != Result::Success) return Result::CapacityExceeded;
            for (const auto &area : allowed) {
                const auto key = std::make_tuple(area.x(), area.y(), area.width(), area.height());
                auto imported = inputs.constFind(key);
                if (imported == inputs.cend()) {
                    QByteArray alpha(qsizetype(area.width()) * area.height(), Qt::Uninitialized);
                    for (int y = 0; y < area.height(); ++y)
                        std::memcpy(alpha.data() + qsizetype(y) * area.width(),
                            bytes.constData() + qsizetype(area.y() + y - placement.y() - region.y()) * region.width() + area.x() - placement.x() - region.x(), size_t(area.width()));
                    auto input = SelectionMask::fromAlphaCoverage(area, alpha, area);
                    if (!input) {
                        if (std::any_of(alpha.cbegin(), alpha.cend(), [](char value) { return value != 0; }))
                            return Result::CapacityExceeded;
                        input = SelectionMask::empty();
                    }
                    if (input->byteCount() > d.limits.rasterBytes - retainedBytes) return Result::CapacityExceeded;
                    retainedBytes += input->byteCount();
                    imported = inputs.insert(key, input);
                }
                const QPointF first(area.topLeft()), last(area.x() + area.width(), area.y() + area.height());
                const QList<InputSample> samples{{first, 1}, {last, 1}};
                const Shape rectangle{ShapeKind::Rectangle, first, last, 0};
                Brush copy; copy.blendMode = LayerBlendMode::Copy;
                FigureStyle clear; clear.outline = false; clear.fill = RgbaPixel{};
                clear.fillBlendMode = LayerBlendMode::Copy; clear.ignoreSelection = true;
                auto result = image->publish(image->m_d->document->paintFigureWithin(
                    sourceLayer, copy, samples, rectangle, clear, 0, area));
                if (result != Result::Success) return result;
                if (!(*imported)->isEmpty()) {
                    if (image->setSelectionMask(*imported) != Result::Success) return Result::CapacityExceeded;
                    Brush white; white.rgba = {255, 255, 255, 255};
                    FigureStyle fill; fill.outline = false; fill.fill = white.rgba;
                    result = image->publish(image->m_d->document->paintFigureWithin(
                        sourceLayer, white, samples, rectangle, fill, 0, area));
                    if (result != Result::Success) return result;
                }
            }
            const auto source = image->operations().snapshot();
            if (source->sampleCount > (d.limits.rasterBytes - retainedBytes) / qsizetype(sizeof(InputSample)))
                return Result::CapacityExceeded;
            retainedBytes += source->sampleCount * qsizetype(sizeof(InputSample));
            const auto operationCount = source->layers.first().operations.size();
            if (operationCount > (d.limits.rasterBytes - retainedBytes) / qsizetype(sizeof(PaintOperation)))
                return Result::CapacityExceeded;
            retainedBytes += operationCount * qsizetype(sizeof(PaintOperation));
            SelectionMaskSP output;
            const auto generated = d.execution.paintedSelectionCoverage(image->handle(), source, sourceLayer, &output);
            if (generated.code != ExecutionCode::Success) return readFailure(generated.code);
            output = output->withPaintHistory(previous, firstEdit, writeUuid);
            if (!output || !document.selectionFitsRetainedInputs(output)) return Result::CapacityExceeded;
            if (previous == selected && previousPosition == selectedPosition) selectedOutput = output;
            if (!retainedOnly) writes.append({version, target.first, maskUuid, output});
        }
        if (!selectedOutput) return Result::LayerUnavailable;
        const auto adopted = writes.isEmpty() ? Result::Success : document.applyMaskVersions(writes);
        if (adopted == Result::Success) {
            if (retainedCoverage) *retainedCoverage = selectedOutput;
            if (!writes.isEmpty()) requestCurrentVersion();
        }
        return adopted;
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}

}
