/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuExecution_p.h"
#include "KisGpuAutoBrush_p.h"
#include "KisGpuMarkerBrush_p.h"
#include "KisGpuReadbackBatch_p.h"
#include "KisGpuPaintJob_p.h"
#include <QColorSpace>
#include <QSysInfo>
#include <QSet>
#include <QRegion>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <new>
#include <chrono>
#include <mutex>

namespace LibrePaint::Image::Gpu {
namespace {
PixelReadbackOutcome failure(ExecutionCode code, const QString &diagnostic)
{
    PixelReadbackOutcome outcome;
    outcome.code = code;
    outcome.diagnostic = diagnostic;
    return outcome;
}
std::future<PixelReadbackOutcome> readyReadback(PixelReadbackOutcome outcome)
{
    std::promise<PixelReadbackOutcome> result;
    auto future = result.get_future();
    result.set_value(std::move(outcome));
    return future;
}

struct PixelReadCompletion
{
    std::promise<PixelReadbackOutcome> result;
    PixelReadbackOutcome outcome;
    qsizetype remaining = 0;
    bool transparencyOnly = false;
    bool boundsOnly = false;
    QList<std::shared_ptr<Detail::Tile>> selectionPins;
    QList<std::shared_ptr<Detail::CoverageTile>> selectionCoveragePins;
    std::mutex mutex;

    void complete(const Detail::ReadbackBatch::Result &read, const QList<qsizetype> &outputs)
    {
        std::lock_guard<std::mutex> guard(mutex);
        const qsizetype expectedBytes = boundsOnly ? 4 * sizeof(qint32) :
                                        transparencyOnly ? sizeof(quint32) : RgbaTile::byteCount(outcome.document->format);
        if (read.data.size() != expectedBytes) {
            outcome.code = ExecutionCode::DeviceFailure;
            outcome.diagnostic = read.diagnostic.isEmpty() ?
                (boundsOnly ? QStringLiteral("GPU bounds readback did not return the complete value") :
                 transparencyOnly ? QStringLiteral("GPU transparency readback did not return the complete value") :
                                    QStringLiteral("GPU pixel readback did not return a complete tile")) : read.diagnostic;
        } else if (outcome.code == ExecutionCode::Success) {
            if (boundsOnly) {
                qint32 values[4];
                std::memcpy(values, read.data.constData(), sizeof(values));
                outcome.bounds = values[0] <= values[2] && values[1] <= values[3] ?
                    QRect(QPoint(values[0], values[1]), QPoint(values[2], values[3])) : QRect();
            } else if (transparencyOnly) {
                quint32 value = 0;
                std::memcpy(&value, read.data.constData(), sizeof(value));
                outcome.containsTransparency = value != 0;
            } else {
                for (const auto index : outputs) outcome.tiles[index].rgba = read.data;
            }
        }
        if (--remaining == 0) {
            if (outcome.code != ExecutionCode::Success) outcome = failure(outcome.code, outcome.diagnostic);
            result.set_value(std::move(outcome));
        }
    }
};
ExecutionCode executionCode(RenderOutcome::Code code)
{
    return code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded :
        code == RenderOutcome::InvalidVersion ? ExecutionCode::InvalidRequest : ExecutionCode::DeviceFailure;
}

bool validPixelRegion(QRect region)
{
    constexpr qint64 limit = qint64(OperationDocument::CoordinateLimit);
    return region.width() >= 0 && region.height() >= 0 &&
        region.x() >= -limit && region.y() >= -limit &&
        qint64(region.x()) + region.width() <= limit &&
        qint64(region.y()) + region.height() <= limit;
}

std::optional<QList<quint64>> paintLayerCoordinates(const Layer &layer, QRect region, qsizetype maximum,
                                                  OperationId pendingOperation, QSize documentSize, PixelFormat format)
{
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    QRect needed = region.intersected(domain);
    QHash<quint64, QRect> afterSpatialOperations;
    qsizetype remainingTranslations = 0;
    bool selectedTranslation = false;
    bool affinePresent = false;
    for (auto operation = layer.operations.crbegin(); operation != layer.operations.crend(); ++operation) {
        if (const auto translation = std::get_if<TranslationSnapshot>(&*operation)) {
            afterSpatialOperations.insert((*translation)->id.sequence, needed);
            ++remainingTranslations;
            const auto sourceRegion = needed.translated(-(*translation)->offset).intersected(domain);
            selectedTranslation |= (*translation)->selectedPixels;
            needed = (*translation)->selectedPixels ? needed.united(sourceRegion.intersected((*translation)->copyRect)) : sourceRegion;
        } else if (const auto affine = std::get_if<AffineSnapshot>(&*operation)) {
            affinePresent = true;
            afterSpatialOperations.insert((*affine)->id.sequence, needed);
            auto transformed = needed;
            for (qsizetype i = (*affine)->plan->passes.size(); i-- > 0;)
                transformed = affineNeededRegion((*affine)->plan->passes[i], transformed);
            needed = (*affine)->selection ? needed.united(transformed.intersected((*affine)->selection->copyRect)) : transformed;
            needed &= domain;
        } else if (const auto crop = std::get_if<CropSnapshot>(&*operation)) {
            afterSpatialOperations.insert((*crop)->id.sequence, needed);
            needed = needed.intersected((*crop)->rect.normalized());
        }
    }
    QSet<quint64> coordinates;
    // An unaligned translation can read four source tiles for one output tile.
    // Bound coordinate discovery separately from the caller's final tile budget.
    const qsizetype expansion = selectedTranslation ? 5 : 4;
    qsizetype stageMaximum = remainingTranslations
        ? (maximum <= std::numeric_limits<qsizetype>::max() / expansion ? maximum * expansion : std::numeric_limits<qsizetype>::max())
        : maximum;
    if (affinePresent) stageMaximum = std::max(stageMaximum, qsizetype(256 * 1024 * 1024 / RgbaTile::byteCount(format)));
    for (const auto &operation : layer.operations) {
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            for (const auto &tile : (*write)->tiles) {
                if (!needed.intersects(QRect(tile->coordinate() * TileRenderer::TileSize,
                                             QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                coordinates.insert(Detail::key(tile->coordinate().x(), tile->coordinate().y()));
                if (coordinates.size() > stageMaximum) return std::nullopt;
            }
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            if (--remainingTranslations == 0 && !affinePresent) stageMaximum = maximum;
            const auto target = afterSpatialOperations.value((*translation)->id.sequence);
            QSet<quint64> moved;
            const auto append = [&](QRect pixels) {
                if (pixels.isEmpty()) return true;
                for (int y = Detail::pixelTileIndex(pixels.top(), TileRenderer::TileSize);
                     y <= Detail::pixelTileIndex(pixels.bottom(), TileRenderer::TileSize); ++y)
                    for (int x = Detail::pixelTileIndex(pixels.left(), TileRenderer::TileSize);
                         x <= Detail::pixelTileIndex(pixels.right(), TileRenderer::TileSize); ++x) {
                        moved.insert(Detail::key(x, y));
                        if (moved.size() > stageMaximum) return false;
                    }
                return true;
            };
            for (const auto tile : coordinates) {
                const QRect original(Detail::coordinate(tile) * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                if ((*translation)->selectedPixels) {
                    if (!append(original.intersected(target)) ||
                        !append(original.intersected((*translation)->copyRect).translated((*translation)->offset).intersected(target)))
                        return std::nullopt;
                } else if (!append(original.translated((*translation)->offset).intersected(target))) return std::nullopt;
            }
            if ((*translation)->selectedPixels && layer.defaultPixel[3]) {
                if (!append((*translation)->clearRect.intersected(target)) ||
                    !append((*translation)->copyRect.translated((*translation)->offset).intersected(target))) return std::nullopt;
            }
            coordinates.swap(moved);
            needed = target;
        } else if (const auto affine = std::get_if<AffineSnapshot>(&operation)) {
            const auto target = afterSpatialOperations.value((*affine)->id.sequence);
            QRegion generated;
            for (const auto tile : coordinates) {
                const QRect original(Detail::coordinate(tile) * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                QRect moved = (*affine)->selection ? original.intersected((*affine)->selection->copyRect) : original;
                for (const auto &pass : (*affine)->plan->passes) moved = affineOutputRegion(pass, moved);
                generated += moved.intersected(target);
                if ((*affine)->selection) generated += original.intersected(target);
            }
            for (const auto &pass : (*affine)->plan->passes)
                if (!pass.mapWholeLayer) generated += pass.destination.intersected(target);
            coordinates.clear();
            for (const auto &area : generated) {
                if (area.isEmpty()) continue;
                for (int y = Detail::pixelTileIndex(area.top(), TileRenderer::TileSize); y <= Detail::pixelTileIndex(area.bottom(), TileRenderer::TileSize); ++y)
                    for (int x = Detail::pixelTileIndex(area.left(), TileRenderer::TileSize); x <= Detail::pixelTileIndex(area.right(), TileRenderer::TileSize); ++x) {
                        coordinates.insert(Detail::key(x, y)); if (coordinates.size() > stageMaximum) return std::nullopt;
                    }
            }
            needed = target;
        } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
            const auto target = afterSpatialOperations.value((*crop)->id.sequence);
            const auto keep = (*crop)->rect.normalized();
            for (auto tile = coordinates.begin(); tile != coordinates.end();) {
                const QRect bounds(Detail::coordinate(*tile) * TileRenderer::TileSize,
                                   QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                if (!keep.intersects(bounds) || !target.intersects(bounds)) tile = coordinates.erase(tile);
                else ++tile;
            }
            needed = target;
        } else if (const auto filter = std::get_if<FilterSnapshot>(&operation)) {
            // A filter over canvas-wide default content synthesizes tiles in the
            // renderer, so its coverage participates in the retained layer extent.
            if (layer.defaultPixel == RgbaPixel{}) continue;
            QRect area = needed;
            if (!(*filter)->selectionRect.isEmpty() && !(*filter)->selectionInverted)
                area = area.intersected((*filter)->selectionRect);
            area = area.intersected(QRect(QPoint(), documentSize));
            if (area.isEmpty()) continue;
            const int left = Detail::pixelTileIndex(area.left(), TileRenderer::TileSize);
            const int right = Detail::pixelTileIndex(area.right(), TileRenderer::TileSize);
            const int top = Detail::pixelTileIndex(area.top(), TileRenderer::TileSize);
            const int bottom = Detail::pixelTileIndex(area.bottom(), TileRenderer::TileSize);
            if (qint64(right - left + 1) * (bottom - top + 1) > stageMaximum) return std::nullopt;
            for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                coordinates.insert(Detail::key(x, y));
                if (coordinates.size() > stageMaximum) return std::nullopt;
            }
        } else {
            if (needed.isEmpty()) continue;
            const auto &paint = **std::get_if<PaintSnapshot>(&operation);
            if (paint.brush.autoBrush) {
                const auto dabs = Detail::autoBrushDabs(paint, paint.id == pendingOperation);
                if (!dabs) return std::nullopt;
                for (const auto &dab : *dabs) {
                    const auto bounds = dab.bounds().intersected(needed);
                    if (bounds.isEmpty()) continue;
                    const int left = Detail::pixelTileIndex(bounds.left(), TileRenderer::TileSize);
                    const int right = Detail::pixelTileIndex(bounds.right(), TileRenderer::TileSize);
                    const int top = Detail::pixelTileIndex(bounds.top(), TileRenderer::TileSize);
                    const int bottom = Detail::pixelTileIndex(bounds.bottom(), TileRenderer::TileSize);
                    if (qint64(right - left + 1) * (bottom - top + 1) > stageMaximum) return std::nullopt;
                    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                        coordinates.insert(Detail::key(x, y));
                        if (coordinates.size() > stageMaximum) return std::nullopt;
                    }
                }
            } else {
                const bool allocatedSource = Detail::usesAllocatedSourceRegion(paint, format);
                const auto dabs = Detail::markerDabs(paint, paint.id == pendingOperation, needed, allocatedSource);
                if (!dabs) return std::nullopt;
                for (const auto &dab : *dabs) {
                    const auto bounds = dab.bounds().intersected(needed);
                    const int left = Detail::pixelTileIndex(bounds.left(), TileRenderer::TileSize);
                    const int right = Detail::pixelTileIndex(bounds.right(), TileRenderer::TileSize);
                    const int top = Detail::pixelTileIndex(bounds.top(), TileRenderer::TileSize);
                    const int bottom = Detail::pixelTileIndex(bounds.bottom(), TileRenderer::TileSize);
                    if (qint64(right - left + 1) * (bottom - top + 1) > stageMaximum) return std::nullopt;
                    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                        const QRect tile(QPoint(x, y) * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                        if (!allocatedSource && !Detail::markerDabIntersects(dab, tile.intersected(needed))) continue;
                        coordinates.insert(Detail::key(x, y));
                        if (coordinates.size() > stageMaximum) return std::nullopt;
                    }
                }
            }
            if (hasFigureFill(paint.figureStyle)) {
                const auto edges = Detail::figureFillEdges(paint);
                if (!edges) return std::nullopt;
                const auto tiles = Detail::figureFillTiles(*edges, paint.brush, needed, stageMaximum);
                if (!tiles) return std::nullopt;
                for (const auto tile : *tiles) {
                    coordinates.insert(tile);
                    if (coordinates.size() > stageMaximum) return std::nullopt;
                }
            }
        }
    }
    auto result = coordinates.values();
    std::sort(result.begin(), result.end());
    return result;
}

std::optional<QList<quint64>> layerCoordinates(const Snapshot &document, const Layer &layer, QRect region, qsizetype maximum)
{
    if (layer.kind == LayerKind::Paint) return paintLayerCoordinates(layer, region, maximum, document->pendingOperation(), document->size, document->format);
    QHash<QUuid, QList<const Layer *>> children;
    for (const auto &value : document->layers) children[value.parentUuid].append(&value);
    QList<const Layer *> pending{&layer};
    QSet<quint64> coordinates;
    for (qsizetype i = 0; i < pending.size(); ++i) {
        for (const auto child : children.value(pending[i]->uuid)) {
            if (!child->visible || !child->opacity) continue;
            if (child->kind == LayerKind::Group) pending.append(child);
            else {
                const auto tiles = paintLayerCoordinates(*child, region, maximum, document->pendingOperation(), document->size, document->format);
                if (!tiles) return std::nullopt;
                for (const auto tile : *tiles) {
                    coordinates.insert(tile);
                    if (coordinates.size() > maximum) return std::nullopt;
                }
            }
        }
    }
    auto result = coordinates.values();
    std::sort(result.begin(), result.end());
    return result;
}
}

ExecutionOutcome Execution::retireDisplayCaches()
{
    auto &d = *m_d;
    if (!d.device->finish())
        return {ExecutionCode::DeviceFailure, QStringLiteral("GPU completion failed before pixel streaming")};
    retireCompletedReadback();
    for (auto &document : d.documents) {
        document.projected.clear();
        document.rendered.clear();
    }
    return {ExecutionCode::Success, {}};
}

LayerTileCoordinatesOutcome Execution::layerTileCoordinates(DocumentHandle handle, const Snapshot &pinned,
                                                              LayerId layer, qsizetype maximumTiles)
{
    const auto reject = [](ExecutionCode code, const QString &diagnostic) {
        return LayerTileCoordinatesOutcome{code, {}, {}, diagnostic};
    };
    if (!pinned || maximumTiles < 0 || maximumTiles > std::numeric_limits<qsizetype>::max() / RgbaTile::byteCount(pinned->format))
        return reject(ExecutionCode::InvalidRequest, QStringLiteral("Invalid sparse tile capacity"));
    const qsizetype maximumBytes = maximumTiles * RgbaTile::byteCount(pinned ? pinned->format : PixelFormat::Rgba16);
    if (!layer || maximumBytes < 0) return reject(ExecutionCode::InvalidRequest, QStringLiteral("Layer pixel capacity and identity are invalid"));
    auto outcome = readPixels(handle, pinned, {}, {layer}, false, PixelReadArea::LayerCoordinates);
    if (outcome.code != ExecutionCode::Success) return reject(outcome.code, outcome.diagnostic);
    constexpr int limit = RgbaTile::CoordinateLimit;
    outcome.region = {-limit, -limit, 2 * limit, 2 * limit};
    const Layer *selected = nullptr;
    for (const auto &value : pinned->layers) if (value.id == layer) selected = &value;
    LayerTileCoordinatesOutcome result{ExecutionCode::Success, {}, {}, {}};
    if (selected->kind == LayerKind::Paint) {
        const auto allocation = Detail::paintLayerAllocation(pinned, *selected,
            qMin(Detail::PaintGeometryElementLimit, maximumBytes / qsizetype(sizeof(QRect))));
        if (!allocation) return reject(ExecutionCode::CapacityExceeded, QStringLiteral("Layer allocation geometry capacity exceeded"));
        QSet<quint64> coordinates;
        for (const auto &area : allocation->blocks) {
            result.writtenAreas.append(area);
            const int left = Detail::pixelTileIndex(area.left(), TileRenderer::TileSize);
            const int top = Detail::pixelTileIndex(area.top(), TileRenderer::TileSize);
            const int right = Detail::pixelTileIndex(area.right(), TileRenderer::TileSize);
            const int bottom = Detail::pixelTileIndex(area.bottom(), TileRenderer::TileSize);
            if (qint64(right - left + 1) * (bottom - top + 1) > maximumTiles)
                return reject(ExecutionCode::CapacityExceeded, QStringLiteral("Complete layer pixel capacity exceeded"));
            for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                coordinates.insert(Detail::key(x, y));
                if (coordinates.size() > maximumTiles)
                    return reject(ExecutionCode::CapacityExceeded, QStringLiteral("Complete layer pixel capacity exceeded"));
            }
        }
        auto ordered = coordinates.values();
        std::sort(ordered.begin(), ordered.end());
        for (const auto coordinate : ordered) result.coordinates.append(Detail::coordinate(coordinate));
    } else {
        const auto coordinates = layerCoordinates(pinned, *selected, outcome.region, maximumTiles);
        if (!coordinates) return reject(ExecutionCode::CapacityExceeded, QStringLiteral("Complete group pixel capacity exceeded"));
        for (const auto coordinate : *coordinates) result.coordinates.append(Detail::coordinate(coordinate));
    }
    return result;
}

PixelReadbackOutcome Execution::readLayerPixels(DocumentHandle handle, const Snapshot &pinned,
                                               LayerId layer, qsizetype maximumBytes)
{
    if (!pinned || maximumBytes < 0) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Invalid layer pixel capacity"));
    const auto plan = layerTileCoordinates(handle, pinned, layer, maximumBytes / RgbaTile::byteCount(pinned->format));
    if (plan.code != ExecutionCode::Success) return failure(plan.code, plan.diagnostic);
    auto outcome = readPixels(handle, pinned, {}, {layer}, false, PixelReadArea::LayerCoordinates);
    if (outcome.code != ExecutionCode::Success) return outcome;
    constexpr int limit = RgbaTile::CoordinateLimit;
    outcome.region = {-limit, -limit, 2 * limit, 2 * limit};
    outcome.writtenAreas = plan.writtenAreas;
    if (plan.coordinates.isEmpty()) return outcome;
    for (const auto coordinate : plan.coordinates) {
        const QRect region(coordinate * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        auto pixels = readPixels(handle, pinned, region, {layer}, false, PixelReadArea::LayerCoordinates);
        if (pixels.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches();
            if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
            pixels = readPixels(handle, pinned, region, {layer}, false, PixelReadArea::LayerCoordinates);
        }
        if (pixels.code != ExecutionCode::Success) return failure(pixels.code, pixels.diagnostic);
        outcome.tiles.append(pixels.tiles);
    }
    return outcome;
}

ImageReadbackOutcome Execution::readProjectionImage(DocumentHandle handle, const Snapshot &pinned, qsizetype maximumBytes)
{
    const auto rejected = [](ExecutionCode code, const QString &diagnostic) {
        return ImageReadbackOutcome{code, {}, 0, {}, diagnostic};
    };
    if (maximumBytes < 0) return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Projection pixel capacity is invalid"));
    const auto checked = readPixels(handle, pinned, {}, {}, true);
    if (checked.code != ExecutionCode::Success) return rejected(checked.code, checked.diagnostic);
    const int pixelSize = pixelByteCount(pinned->format);
    if (qint64(pinned->size.width()) * pinned->size.height() * pixelSize > maximumBytes)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Complete projection pixel capacity exceeded"));
    const QRect canvas(QPoint(0, 0), pinned->size);
    constexpr int size = TileRenderer::TileSize;
    const qsizetype columns = (pinned->size.width() + size - 1) / size;
    const qsizetype rows = (pinned->size.height() + size - 1) / size;
    QSet<quint64> coordinates;
    const auto defaults = pinned->projectionDefaultPixels();
    bool uniform = pinned->projectionBackground != RgbaPixel{};
    QSet<LayerId> projectionLayers;
    for (const auto &input : pinned->projectionInputs()) projectionLayers.insert(input.layer);
    for (const auto &layer : pinned->layers) {
        if (!projectionLayers.contains(layer.id)) continue;
        uniform |= defaults.value(layer.id) != RgbaPixel{};
        const auto tiles = layerCoordinates(pinned, layer, canvas, columns * rows);
        if (!tiles) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection tile coordinates exceed the canvas"));
        for (const auto tile : *tiles) coordinates.insert(tile);
    }
    if (uniform) {
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x) coordinates.insert(Detail::key(x, y));
    }
    ImageReadbackOutcome outcome;
    outcome.document = pinned;
    outcome.generation = checked.generation;
    outcome.image = QImage(pinned->size, pinned->format == PixelFormat::Rgba16 ? QImage::Format_RGBA64 : QImage::Format_RGBA8888);
    if (outcome.image.isNull()) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection image allocation failed"));
    outcome.image.fill(Qt::transparent);
    outcome.image.setColorSpace(pinned->format == PixelFormat::Rgba16 ? QColorSpace::SRgbLinear : QColorSpace::SRgb);
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const QRect region(Detail::coordinate(coordinate) * size, QSize(size, size));
        auto pixels = readPixels(handle, pinned, region);
        if (pixels.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches();
            if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
            pixels = readPixels(handle, pinned, region);
        }
        if (pixels.code != ExecutionCode::Success) return rejected(pixels.code, pixels.diagnostic);
        for (const auto &tile : pixels.tiles) {
            const auto origin = tile.coordinate * size;
            const auto clipped = QRect(origin, QSize(size, size)).intersected(canvas);
            for (int y = clipped.top(); y <= clipped.bottom(); ++y) {
                const auto source = tile.rgba.constData() + ((y - origin.y()) * size + clipped.x() - origin.x()) * pixelSize;
                auto destination = outcome.image.scanLine(y) + clipped.x() * pixelSize;
                std::memcpy(destination, source, size_t(clipped.width()) * pixelSize);
                if (pinned->format == PixelFormat::Rgba16 && QSysInfo::ByteOrder == QSysInfo::BigEndian)
                    for (int i = 0; i < clipped.width() * pixelSize; i += 2) std::swap(destination[i], destination[i + 1]);
            }
        }
    }
    outcome.code = ExecutionCode::Success;
    return outcome;
}

PixelBytesOutcome Execution::readRgbaPixels(DocumentHandle handle, const Snapshot &pinned,
                                           QRect region, LayerId layer, qsizetype maximumBytes,
                                           const SelectionMaskSP &copyMask, bool invertMask)
{
    const auto rejected = [](ExecutionCode code, const QString &diagnostic) {
        PixelBytesOutcome outcome;
        outcome.code = code;
        outcome.diagnostic = diagnostic;
        return outcome;
    };
    if (!validPixelRegion(region) || maximumBytes < 0)
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Packed pixel region or capacity is invalid"));
    const QList<LayerId> layers = layer ? QList<LayerId>{layer} : QList<LayerId>{};
    const auto checked = readPixels(handle, pinned, {}, layers, !layer, PixelReadArea::LayerCoordinates);
    if (checked.code != ExecutionCode::Success) return rejected(checked.code, checked.diagnostic);
    const int pixelSize = pixelByteCount(pinned->format);
    const qint64 byteCount = qint64(region.width()) * region.height() * pixelSize;
    if (byteCount > maximumBytes)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Packed pixel output capacity exceeded"));
    const auto clipped = layer ? region : region.intersected(QRect(QPoint(), pinned->size));
    constexpr int size = TileRenderer::TileSize;
    QSet<quint64> coordinates;
    RgbaPixel defaultPixel{};
    const auto defaults = pinned->projectionDefaultPixels();
    bool uniformProjection = !layer && pinned->projectionBackground != RgbaPixel{};
    QSet<LayerId> projectionLayers;
    if (!layer) for (const auto &input : pinned->projectionInputs()) projectionLayers.insert(input.layer);
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !projectionLayers.contains(value.id)) continue;
        if (layer) defaultPixel = defaults.value(value.id);
        else uniformProjection |= defaults.value(value.id) != RgbaPixel{};
        const auto tiles = layerCoordinates(pinned, value, clipped, std::numeric_limits<qsizetype>::max());
        if (!tiles) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Packed pixel coordinates exceed capacity"));
        for (const auto tile : *tiles) coordinates.insert(tile);
    }
    if ((uniformProjection || (copyMask && defaultPixel != RgbaPixel{})) && !clipped.isEmpty()) {
        for (int y = Detail::pixelTileIndex(clipped.top(), size); y <= Detail::pixelTileIndex(clipped.bottom(), size); ++y)
            for (int x = Detail::pixelTileIndex(clipped.left(), size); x <= Detail::pixelTileIndex(clipped.right(), size); ++x)
                coordinates.insert(Detail::key(x, y));
    }
    PixelBytesOutcome outcome;
    outcome.document = pinned;
    outcome.generation = checked.generation;
    outcome.region = region;
    try {
        outcome.bytes.resize(qsizetype(byteCount));
    } catch (const std::bad_alloc &) {
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Packed pixel allocation failed"));
    }
    if (copyMask || defaultPixel == RgbaPixel{}) outcome.bytes.fill('\0');
    else {
        const auto pixel = rgbaBytes(defaultPixel, pinned->format);
        for (qsizetype offset = 0; offset < outcome.bytes.size(); offset += pixelSize)
            std::memcpy(outcome.bytes.data() + offset, pixel.constData(), pixelSize);
    }
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const QRect tileRegion(Detail::coordinate(coordinate) * size, QSize(size, size));
        const auto readPart = [&]() -> PixelReadbackOutcome {
            if (!copyMask) return readPixels(handle, pinned, tileRegion, layers, !layer, PixelReadArea::LayerCoordinates);
            auto pending = readPixelsAsyncImpl(handle, pinned, tileRegion, layers, !layer,
                PixelReadArea::LayerCoordinates, false, false, false, false, copyMask, invertMask);
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
                return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU selection copy completion failed"));
            retireCompletedReadback();
            return pending.get();
        };
        auto pixels = readPart();
        if (pixels.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches();
            if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
            pixels = readPart();
        }
        if (pixels.code != ExecutionCode::Success) return rejected(pixels.code, pixels.diagnostic);
        for (const auto &tile : pixels.tiles) {
            const auto origin = tile.coordinate * size;
            const auto portion = QRect(origin, QSize(size, size)).intersected(clipped);
            for (int y = portion.top(); y <= portion.bottom(); ++y) {
                const auto source = tile.rgba.constData() + ((y - origin.y()) * size + portion.x() - origin.x()) * pixelSize;
                const qsizetype destination = (qsizetype(y - region.y()) * region.width() + portion.x() - region.x()) * pixelSize;
                std::memcpy(outcome.bytes.data() + destination, source, size_t(portion.width()) * pixelSize);
            }
        }
    }
    outcome.code = ExecutionCode::Success;
    return outcome;
}

PixelReadbackOutcome Execution::readPixels(DocumentHandle handle, const Snapshot &pinned,
                                         QRect region, const QList<LayerId> &layers, bool includeProjection, PixelReadArea area)
{
    if (!m_d->onThread()) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read requires the GPU thread"));
    if (!m_d->active && !m_d->device->isRecordingFrame() && m_d->device->readbackPending()) {
        if (!m_d->device->finish())
            return failure(ExecutionCode::DeviceFailure, QStringLiteral("Earlier GPU pixel completion failed"));
        retireCompletedReadback();
    }
    auto pending = readPixelsAsync(handle, pinned, region, layers, includeProjection, area);
    if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
        return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU pixel completion failed"));
    retireCompletedReadback();
    return pending.get();
}

std::future<PixelReadbackOutcome> Execution::readPixelsAsync(DocumentHandle handle, const Snapshot &pinned,
                                         QRect region, const QList<LayerId> &layers, bool includeProjection, PixelReadArea area)
{ return readPixelsAsyncImpl(handle, pinned, region, layers, includeProjection, area, false); }

std::future<PixelReadbackOutcome> Execution::readTransparencyAsync(DocumentHandle handle, const Snapshot &pinned, QRect region)
{ return readPixelsAsyncImpl(handle, pinned, region, {}, true, PixelReadArea::Canvas, true); }

PixelReadbackOutcome Execution::readLayerHitTest(DocumentHandle handle, const Snapshot &pinned, LayerId layer, QPoint point)
{
    if (!layer) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Layer picking requires a layer"));
    auto checked = readPixels(handle, pinned, {}, {layer}, false, PixelReadArea::LayerCoordinates);
    if (checked.code != ExecutionCode::Success) return checked;
    auto pending = readPixelsAsyncImpl(handle, pinned, QRect(point, QSize(1, 1)), {layer}, false,
        PixelReadArea::LayerCoordinates, false, true, false, false, {}, false, nullptr, true);
    if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
        return failure(ExecutionCode::DeviceFailure, QStringLiteral("Layer picking completion failed"));
    retireCompletedReadback();
    return pending.get();
}

PixelReadbackOutcome Execution::readSelectedBounds(DocumentHandle handle, const Snapshot &pinned,
                                                    const TranslationOperation &selection, bool includeLowAlpha)
{
    auto outcome = readPixels(handle, pinned, {}, {selection.layer}, false, PixelReadArea::LayerCoordinates);
    if (outcome.code != ExecutionCode::Success) return outcome;
    if (!selection.selectedPixels || !validPixelRegion(selection.copyRect) || selection.copyRect.isEmpty() ||
        !validPixelRegion(selection.selectionRect) || selection.selectionRect.isEmpty() ||
        (selection.selectionKind != SelectionKind::Rectangle && selection.selectionKind != SelectionKind::Ellipse &&
         selection.selectionKind != SelectionKind::Freehand) ||
        (selection.selectionKind == SelectionKind::Freehand && !selection.selectionMask))
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Selected bounds require frozen paint-layer coverage"));
    const auto layer = std::find_if(pinned->layers.cbegin(), pinned->layers.cend(),
        [&](const auto &value) { return value.id == selection.layer; });
    if (layer == pinned->layers.cend() || layer->kind != LayerKind::Paint)
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Selected bounds require a paint layer"));
    constexpr qsizetype capacity = 256 * 1024 * 1024;
    const auto maximum = capacity / RgbaTile::byteCount(pinned->format);
    const auto sparse = layerCoordinates(pinned, *layer, selection.copyRect, maximum);
    if (!sparse) return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Selected bounds coordinates exceed capacity"));
    QSet<quint64> coordinates(sparse->cbegin(), sparse->cend());
    const auto implicit = pinned->projectionDefaultPixels().value(selection.layer);
    constexpr int size = TileRenderer::TileSize;
    if (implicit[3]) {
        const auto area = selection.copyRect;
        const int left = Detail::pixelTileIndex(area.left(), size), right = Detail::pixelTileIndex(area.right(), size);
        const int top = Detail::pixelTileIndex(area.top(), size), bottom = Detail::pixelTileIndex(area.bottom(), size);
        if (qint64(right - left + 1) * (bottom - top + 1) > maximum)
            return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Selected implicit bounds coordinates exceed capacity"));
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) coordinates.insert(Detail::key(x, y));
    }
    outcome.bounds = QRect();
    outcome.region = selection.copyRect;
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const auto region = QRect(Detail::coordinate(coordinate) * size, QSize(size, size)).intersected(selection.copyRect);
        const auto readPart = [&] {
            auto pending = readPixelsAsyncImpl(handle, pinned, region, {selection.layer}, false,
                PixelReadArea::LayerCoordinates, false, true, false, false, {}, false, &selection, false, includeLowAlpha);
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
                return failure(ExecutionCode::DeviceFailure, QStringLiteral("Selected bounds completion failed"));
            retireCompletedReadback();
            return pending.get();
        };
        auto part = readPart();
        if (part.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches();
            if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
            part = readPart();
        }
        if (part.code != ExecutionCode::Success) return part;
        *outcome.bounds |= *part.bounds;
    }
    return outcome;
}

PixelReadbackOutcome Execution::readBounds(DocumentHandle handle, const Snapshot &pinned, LayerId layer,
                                         bool includeTransparentColors, bool opacityU8)
{
    return readBoundsImpl(handle, pinned, layer, includeTransparentColors, false, opacityU8);
}

PixelReadbackOutcome Execution::readBoundsImpl(DocumentHandle handle, const Snapshot &pinned, LayerId layer,
                                             bool includeTransparentColors, bool sparseProjection, bool opacityU8)
{
    if (!m_d->onThread()) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Bounds read requires the GPU thread"));
    constexpr int limit = RgbaTile::CoordinateLimit;
    constexpr qsizetype capacity = 256 * 1024 * 1024;
    const auto area = layer ? PixelReadArea::LayerCoordinates : PixelReadArea::Canvas;
    const QList<LayerId> selected = layer ? QList<LayerId>{layer} : QList<LayerId>{};
    auto outcome = readPixels(handle, pinned, {}, selected, !layer, area);
    if (outcome.code != ExecutionCode::Success) return outcome;
    outcome.bounds = QRect();
    outcome.region = layer ? QRect(-limit, -limit, 2 * limit, 2 * limit) : QRect(QPoint(), pinned->size);
    QSet<quint64> coordinates;
    const qsizetype columns = (pinned->size.width() + TileRenderer::TileSize - 1) / TileRenderer::TileSize;
    const qsizetype rows = (pinned->size.height() + TileRenderer::TileSize - 1) / TileRenderer::TileSize;
    if (!layer && !sparseProjection && qint64(pinned->size.width()) * pinned->size.height() * pixelByteCount(pinned->format) > capacity)
        return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Projection bounds pixel capacity exceeded"));
    const qsizetype maximumCoordinates = layer || sparseProjection ? capacity / RgbaTile::byteCount(pinned->format) : columns * rows;
    const auto defaults = pinned->projectionDefaultPixels();
    bool uniform = !layer && (sparseProjection && !includeTransparentColors ? pinned->projectionBackground[3] != 0 : pinned->projectionBackground != RgbaPixel{});
    QSet<LayerId> projectionLayers;
    if (!layer) for (const auto &input : pinned->projectionInputs()) projectionLayers.insert(input.layer);
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !projectionLayers.contains(value.id)) continue;
        const auto pixel = defaults.value(value.id);
        uniform |= !layer && (sparseProjection && !includeTransparentColors ? pixel[3] != 0 : pixel != RgbaPixel{});
        const auto tiles = layerCoordinates(pinned, value, outcome.region, maximumCoordinates);
        if (!tiles) return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Bounds tile coordinate capacity exceeded"));
        for (const auto coordinate : *tiles) coordinates.insert(coordinate);
        if (sparseProjection && coordinates.size() > maximumCoordinates)
            return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Preview bounds tile capacity exceeded"));
    }
    if (uniform) {
        if (sparseProjection && columns * rows > maximumCoordinates)
            return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Preview background tile capacity exceeded"));
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x) coordinates.insert(Detail::key(x, y));
    }
    if (coordinates.isEmpty()) return outcome;
    if (m_d->maximumReadbackBytes < qsizetype(4 * sizeof(qint32)))
        return failure(ExecutionCode::CapacityExceeded, QStringLiteral("GPU bounds readback byte capacity exceeded"));
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const QRect region(Detail::coordinate(coordinate) * TileRenderer::TileSize,
                           QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        const auto readPart = [&] {
            auto pending = readPixelsAsyncImpl(handle, pinned, region, selected, !layer, area, false, true, includeTransparentColors, false, {}, false, nullptr, false, false, opacityU8);
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
                return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU bounds completion failed"));
            retireCompletedReadback();
            return pending.get();
        };
        auto part = readPart();
        if (part.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches();
            if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
            part = readPart();
        }
        if (part.code != ExecutionCode::Success) return failure(part.code, part.diagnostic);
        *outcome.bounds |= *part.bounds;
    }
    return outcome;
}

PixelReadbackOutcome Execution::readAffineSelectionBounds(DocumentHandle handle, const Snapshot &pinned,
                                                          const AffineTransformOperation &operation)
{
    auto area = operation.selection->copyRect;
    for (const auto &pass : operation.plan->passes) area = affineOutputRegion(pass, area);
    constexpr int limit = RgbaTile::CoordinateLimit;
    area &= QRect(-limit, -limit, 2 * limit, 2 * limit);
    auto outcome = readPixels(handle, pinned, {}, {operation.layer}, false, PixelReadArea::LayerCoordinates);
    if (outcome.code != ExecutionCode::Success) return outcome;
    outcome.bounds = QRect();
    if (area.isEmpty()) return outcome;
    const int left = Detail::pixelTileIndex(area.left(), TileRenderer::TileSize), right = Detail::pixelTileIndex(area.right(), TileRenderer::TileSize);
    const int top = Detail::pixelTileIndex(area.top(), TileRenderer::TileSize), bottom = Detail::pixelTileIndex(area.bottom(), TileRenderer::TileSize);
    if (qint64(right - left + 1) * (bottom - top + 1) > 256 * 1024 * 1024 / RgbaTile::byteCount(pinned->format))
        return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Selected transform source extent exceeds capacity"));
    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
        const QRect region(QPoint(x, y) * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        const auto readPart = [&] {
            auto pending = readPixelsAsyncImpl(handle, pinned, region, {operation.layer}, false, PixelReadArea::LayerCoordinates,
                false, true, true, false, {}, false, nullptr, false, false, false, &operation);
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
                return failure(ExecutionCode::DeviceFailure, QStringLiteral("Selected transform extent completion failed"));
            retireCompletedReadback(); return pending.get();
        };
        auto part = readPart();
        if (part.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches(); if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
            part = readPart();
        }
        if (part.code != ExecutionCode::Success) return part;
        *outcome.bounds |= *part.bounds;
    }
    return outcome;
}

ExecutionOutcome Execution::readNonDefaultBlocks(DocumentHandle handle, const Snapshot &pinned, LayerId layer, QList<QRect> &blocks)
{
    if (!m_d->onThread()) return {ExecutionCode::InvalidRequest, QStringLiteral("Allocation read requires the GPU thread")};
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    const auto valid = readPixels(handle, pinned, {}, {layer}, false, PixelReadArea::LayerCoordinates);
    if (valid.code != ExecutionCode::Success) return {valid.code, valid.diagnostic};
    const auto found = std::find_if(pinned->layers.cbegin(), pinned->layers.cend(), [&](const auto &v) { return v.id == layer; });
    const auto source = layerCoordinates(pinned, *found, domain, 256 * 1024 * 1024 / RgbaTile::byteCount(pinned->format));
    if (!source) return {ExecutionCode::CapacityExceeded, QStringLiteral("Transform allocation coordinates exceed capacity")};
    constexpr int blockSize = Detail::PixelHistoryBlockSize;
    QSet<quint64> coordinates;
    const auto origin = found->position;
    for (const auto tile : *source) {
        const QRect area(Detail::coordinate(tile) * TileRenderer::TileSize - origin, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        for (int y = Detail::pixelTileIndex(area.top(), blockSize); y <= Detail::pixelTileIndex(area.bottom(), blockSize); ++y)
            for (int x = Detail::pixelTileIndex(area.left(), blockSize); x <= Detail::pixelTileIndex(area.right(), blockSize); ++x)
                coordinates.insert(Detail::key(x, y));
    }
    QList<QRect> result;
    for (const auto coordinate : coordinates) {
        const QRect area = QRect(Detail::coordinate(coordinate) * blockSize + origin, QSize(blockSize, blockSize)).intersected(domain);
        const auto readPart = [&] {
            auto pending = readPixelsAsyncImpl(handle, pinned, area, {layer}, false, PixelReadArea::LayerCoordinates, false, true, true);
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
                return failure(ExecutionCode::DeviceFailure, QStringLiteral("Transform allocation completion failed"));
            retireCompletedReadback(); return pending.get();
        };
        auto part = readPart();
        if (part.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches(); if (retired.code != ExecutionCode::Success) return retired;
            part = readPart();
        }
        if (part.code != ExecutionCode::Success || !part.bounds) return {part.code, part.diagnostic};
        if (!part.bounds->isEmpty()) result.append(area);
    }
    blocks = std::move(result); return {ExecutionCode::Success, {}};
}

PixelReadbackOutcome Execution::readLayerTileExtent(DocumentHandle handle, const Snapshot &pinned, LayerId layer)
{
    auto outcome = readPixels(handle, pinned, {}, layer ? QList<LayerId>{layer} : QList<LayerId>{}, !layer);
    if (outcome.code != ExecutionCode::Success) return outcome;
    outcome.bounds = QRect();
    if (!layer) {
        bool nonempty = pinned->projectionBackground[3] != 0;
        for (const auto &value : pinned->layers)
            if (value.visible && (!value.operations.isEmpty() || value.defaultPixel[3])) nonempty = true;
        // The image-returning projection boundary populates complete 64-pixel storage blocks.
        if (nonempty) outcome.bounds = QRect(0, 0, ((pinned->size.width() + 63) / 64) * 64,
                                             ((pinned->size.height() + 63) / 64) * 64);
        return outcome;
    }
    constexpr int limit = RgbaTile::CoordinateLimit;
    const auto selected = std::find_if(pinned->layers.cbegin(), pinned->layers.cend(),
        [layer](const auto &value) { return value.id == layer; });
    const auto coordinates = layerCoordinates(pinned, *selected, {-limit, -limit, 2 * limit, 2 * limit},
                                              256 * 1024 * 1024 / RgbaTile::byteCount(pinned->format));
    if (!coordinates) return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Thumbnail source tile capacity exceeded"));
    for (const auto tile : *coordinates) {
        const QRect region(Detail::coordinate(tile) * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        auto part = readPixelsAsyncImpl(handle, pinned, region, {layer}, false, PixelReadArea::LayerCoordinates,
                                       false, false, false, true).get();
        if (part.code == ExecutionCode::CapacityExceeded) {
            const auto retired = retireDisplayCaches();
            if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
            part = readPixelsAsyncImpl(handle, pinned, region, {layer}, false, PixelReadArea::LayerCoordinates,
                                       false, false, false, true).get();
        }
        if (part.code != ExecutionCode::Success) return part;
        if (!m_d->device->finish()) return failure(ExecutionCode::DeviceFailure, QStringLiteral("Thumbnail source completion failed"));
        *outcome.bounds |= *part.bounds;
    }
    return outcome;
}

std::future<PixelReadbackOutcome> Execution::readPixelsAsyncImpl(DocumentHandle handle, const Snapshot &pinned,
                                         QRect region, const QList<LayerId> &layers, bool includeProjection,
                                         PixelReadArea area, bool transparencyOnly, bool boundsOnly, bool includeTransparentColors,
                                         bool tileExtentOnly, const SelectionMaskSP &copyMask, bool invertMask,
                                         const TranslationOperation *selectionSource, bool layerHitTest, bool includeLowAlpha, bool nativeBounds, const AffineTransformOperation *affineSource)
{
    const auto rejected = [](ExecutionCode code, const QString &diagnostic) {
        return readyReadback(failure(code, diagnostic));
    };
    auto &d = *m_d;
    if (!d.onThread() || !pinned || (area != PixelReadArea::Canvas && area != PixelReadArea::LayerCoordinates)) {
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read requires a document, valid area and the GPU thread"));
    }
    if (d.active || d.device->isRecordingFrame()) return rejected(ExecutionCode::Busy, QStringLiteral("Pixel read requires submitted frames"));
    if (d.device->readbackPending()) return rejected(ExecutionCode::Busy, QStringLiteral("The native GPU still retains a previous readback"));
    retireCompletedReadback();
    const auto document = d.documents.constFind(handle);
    if (document == d.documents.cend() || document->requested->documentId() != pinned->documentId()) {
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read document is no longer attached or belongs to another handle"));
    }
    if (!validPixelRegion(region)) {
        return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read region is outside the supported range"));
    }
    QSet<LayerId> available, selected;
    for (const auto &layer : pinned->layers) available.insert(layer.id);
    for (const auto layer : layers) {
        if (!available.contains(layer) || selected.contains(layer)) return rejected(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read layer is unavailable or repeated"));
        selected.insert(layer);
    }
    QList<LayerId> generatedLayers = layers;
    if (includeProjection) {
        for (const auto &layer : pinned->projectionInputs()) {
            if (!selected.contains(layer.layer)) {
                generatedLayers.append(layer.layer);
                selected.insert(layer.layer);
            }
        }
    }
    PixelReadbackOutcome outcome;
    outcome.document = pinned;
    outcome.generation = d.generation;
    outcome.region = area == PixelReadArea::Canvas ? region.intersected(QRect(QPoint(0, 0), pinned->size)) : region;
    if (transparencyOnly) outcome.containsTransparency = false;
    if (boundsOnly || tileExtentOnly) outcome.bounds = QRect();
    if (outcome.region.isEmpty() || (layers.isEmpty() && !includeProjection)) {
        outcome.code = ExecutionCode::Success;
        return readyReadback(std::move(outcome));
    }
    const auto base = document->rendered;
    const auto projectedBase = document->projected;
    RenderedSnapshot rendered;
    ProjectedSnapshot projected;
    std::shared_ptr<Detail::ReadbackBatch> reads;
    bool begun = false;
    try {
        const auto frame = beginReadbackFrame();
        if (frame.code != ExecutionCode::Success) return rejected(frame.code, frame.diagnostic);
        begun = true;
        auto sourceRegion = outcome.region;
        if (affineSource) {
            for (qsizetype i = affineSource->plan->passes.size(); i-- > 0;)
                sourceRegion = affineNeededRegion(affineSource->plan->passes[i], sourceRegion);
            sourceRegion &= affineSource->selection->copyRect;
        }
        const auto paint = d.renderer->recordRegion(frame.commands, pinned, sourceRegion, generatedLayers, base);
        if (paint.code != RenderOutcome::Recorded) {
            cancelFrame();
            begun = false;
            return rejected(executionCode(paint.code), paint.diagnostic);
        }
        rendered = paint.version;
        if (includeProjection) {
            const auto compose = d.renderer->compose(frame.commands, rendered, outcome.region, projectedBase);
            if (compose.code != RenderOutcome::Recorded) {
                cancelFrame();
                begun = false;
                return rejected(executionCode(compose.code), compose.diagnostic);
            }
            projected = compose.version;
        }
        QList<WGPUBuffer> buffers;
        QHash<WGPUBuffer, qsizetype> indices;
        QList<qsizetype> outputIndices;
        QList<std::shared_ptr<Detail::Tile>> selectionPins;
        QList<std::shared_ptr<Detail::CoverageTile>> selectionCoveragePins;
        RenderOutcome::Code copyCode = RenderOutcome::Recorded;
        QString copyDiagnostic;
        const auto append = [&](LayerId layer, QList<TileView> tiles) {
            if (copyMask) {
                const auto implicit = layer ? pinned->projectionDefaultPixels().value(layer) : RgbaPixel{};
                if (implicit != RgbaPixel{}) {
                    QSet<QPoint> present;
                    for (const auto &tile : tiles) present.insert(tile.coordinate);
                    const auto &area = outcome.region;
                    for (int y = Detail::pixelTileIndex(area.top(), TileRenderer::TileSize);
                         y <= Detail::pixelTileIndex(area.bottom(), TileRenderer::TileSize); ++y)
                        for (int x = Detail::pixelTileIndex(area.left(), TileRenderer::TileSize);
                             x <= Detail::pixelTileIndex(area.right(), TileRenderer::TileSize); ++x)
                            if (!present.contains(QPoint(x, y))) tiles.append({{x, y}, nullptr});
                }
                for (auto &tile : tiles) {
                    if (!outcome.region.intersects(QRect(tile.coordinate * TileRenderer::TileSize,
                                                        QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                    const auto copied = d.renderer->m_d->copySelection(frame.commands, tile, implicit,
                                                                       pinned->format, copyMask, invertMask);
                    if (copied.code != RenderOutcome::Recorded) {
                        copyCode = copied.code;
                        copyDiagnostic = copied.diagnostic;
                        return false;
                    }
                    selectionPins.append(copied.tile);
                    tile.pixels = copied.tile->pixels.get();
                }
            }
            for (const auto &tile : tiles) {
                if (!outcome.region.intersects(QRect(tile.coordinate * TileRenderer::TileSize,
                                                    QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                if (!indices.contains(tile.pixels)) {
                    if (buffers.size() >= d.maximumReadbackBytes / RgbaTile::byteCount(pinned->format)) return false;
                    indices.insert(tile.pixels, buffers.size());
                    buffers.append(tile.pixels);
                }
                outcome.tiles.append({layer, tile.coordinate, {}});
                outputIndices.append(indices.value(tile.pixels));
            }
            return true;
        };
        bool withinLimit = true;
        Detail::NativeBuffer summary;
        const quint64 transferBytes = boundsOnly ? 4 * sizeof(qint32) :
                                      transparencyOnly ? sizeof(quint32) : RgbaTile::byteCount(pinned->format);
        if (tileExtentOnly) {
            for (const auto layer : layers)
                for (const auto &tile : rendered->tiles(layer)) {
                    const QRect bounds(tile.coordinate * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                    if (outcome.region.intersects(bounds)) *outcome.bounds |= bounds;
                }
        } else if (boundsOnly || transparencyOnly) {
            withinLimit = d.maximumReadbackBytes >= qsizetype(transferBytes);
            if (withinLimit) {
                if (boundsOnly && affineSource) {
                    const auto layer = affineSource->layer;
                    const auto definition = std::find_if(pinned->layers.cbegin(), pinned->layers.cend(), [&](const auto &v) { return v.id == layer; });
                    const auto transformed = d.renderer->m_d->affineSelection(frame.commands, rendered->m_data->layers.value(layer),
                        pinned->projectionDefaultPixels().value(layer), definition->defaultPixel, pinned->format, *affineSource, outcome.region, true);
                    if (transformed.code != RenderOutcome::Recorded) {
                        cancelFrame(); begun = false; return rejected(executionCode(transformed.code), transformed.diagnostic);
                    }
                    QList<TileView> views;
                    for (auto tile = transformed.tiles.cbegin(); tile != transformed.tiles.cend(); ++tile)
                        views.append({Detail::coordinate(tile.key()), tile.value()->pixels.get()});
                    summary = d.renderer->m_d->recordBounds(frame.commands, views, outcome.region, {}, pinned->format, true);
                } else if (boundsOnly && layerHitTest) {
                    const auto layer = layers.first();
                    const auto definition = std::find_if(pinned->layers.cbegin(), pinned->layers.cend(),
                        [&](const auto &value) { return value.id == layer; });
                    const QPoint point(Detail::pixelTileIndex(outcome.region.x(), TileRenderer::TileSize),
                                       Detail::pixelTileIndex(outcome.region.y(), TileRenderer::TileSize));
                    TileView input{point, nullptr};
                    for (const auto &tile : rendered->tiles(layer)) if (tile.coordinate == point) input = tile;
                    const bool masked = definition->transparencyMask && definition->transparencyMaskVisible;
                    const auto copied = d.renderer->m_d->copySelection(frame.commands, input,
                        pinned->projectionDefaultPixels().value(layer), pinned->format,
                        masked ? definition->transparencyMask : SelectionMask::empty(), !masked);
                    if (copied.code != RenderOutcome::Recorded) {
                        cancelFrame(); begun = false;
                        return rejected(executionCode(copied.code), copied.diagnostic);
                    }
                    selectionPins.append(copied.tile);
                    summary = d.renderer->m_d->recordBounds(frame.commands, {{point, copied.tile->pixels.get()}},
                        outcome.region, {}, pinned->format, false, true);
                } else if (boundsOnly && selectionSource) {
                    const auto layer = layers.first();
                    const auto implicit = pinned->projectionDefaultPixels().value(layer);
                    const auto definition = std::find_if(pinned->layers.cbegin(), pinned->layers.cend(),
                        [&](const auto &value) { return value.id == layer; });
                    const QPoint point(Detail::pixelTileIndex(outcome.region.x(), TileRenderer::TileSize),
                                       Detail::pixelTileIndex(outcome.region.y(), TileRenderer::TileSize));
                    TileView input{point, nullptr};
                    for (const auto &tile : rendered->tiles(layer)) if (tile.coordinate == point) input = tile;
                    const auto split = d.renderer->m_d->splitSelectionTile(frame.commands, input, implicit,
                        definition->defaultPixel, pinned->format, *selectionSource);
                    if (split.code != RenderOutcome::Recorded) {
                        cancelFrame(); begun = false;
                        return rejected(executionCode(split.code), split.diagnostic);
                    }
                    selectionPins.append(split.copied); selectionPins.append(split.cleared);
                    selectionCoveragePins.append(split.coverage);
                    summary = d.renderer->m_d->recordBounds(frame.commands, {{point, split.copied->pixels.get()}},
                        outcome.region, {}, pinned->format, false, !includeLowAlpha);
                } else if (boundsOnly) {
                    auto implicit = pinned->projectionBackground;
                    if (!layers.isEmpty()) {
                        implicit = pinned->projectionDefaultPixels().value(layers.first());
                    }
                    summary = d.renderer->m_d->recordBounds(frame.commands,
                        projected ? projected->tiles() : rendered->tiles(layers.first()), outcome.region, implicit, pinned->format, includeTransparentColors, nativeBounds);
                } else summary = d.renderer->m_d->recordTransparency(frame.commands, projected, outcome.region);
                if (!summary) throw std::runtime_error("The GPU pixel summary could not be recorded");
                buffers.append(summary.get());
            }
        } else {
            for (const auto layer : layers) {
                if (!append(layer, rendered->tiles(layer))) { withinLimit = false; break; }
            }
            if (withinLimit && projected) withinLimit = append(0, projected->tiles());
        }
        if (!withinLimit) {
            cancelFrame();
            begun = false;
            return rejected(copyCode == RenderOutcome::Recorded ? ExecutionCode::CapacityExceeded : executionCode(copyCode),
                            copyDiagnostic.isEmpty() ? QStringLiteral("GPU pixel readback byte capacity exceeded") : copyDiagnostic);
        }
        if (!buffers.isEmpty()) {
            reads = Detail::ReadbackBatch::create(*d.device, buffers.size());
            if (!reads) {
                cancelFrame();
                begun = false;
                return rejected(ExecutionCode::Busy, QStringLiteral("The native GPU still retains a previous readback"));
            }
            d.readbackRendered = rendered;
            d.readbackProjected = projected;
            for (qsizetype i = 0; i < buffers.size(); ++i) {
                if (!reads->recordBuffer(*d.device, frame.commands, i, buffers[i], transferBytes))
                    throw std::runtime_error("The native GPU readback buffer could not be recorded");
            }
        }
        std::shared_ptr<PixelReadCompletion> completion;
        std::future<PixelReadbackOutcome> future;
        if (reads) {
            completion = std::make_shared<PixelReadCompletion>();
            outcome.code = ExecutionCode::Success;
            completion->outcome = std::move(outcome);
            completion->remaining = qsizetype(reads->results.size());
            completion->transparencyOnly = transparencyOnly;
            completion->boundsOnly = boundsOnly;
            completion->selectionPins = std::move(selectionPins);
            completion->selectionCoveragePins = std::move(selectionCoveragePins);
            future = completion->result.get_future();
            for (qsizetype index = 0; size_t(index) < reads->results.size(); ++index) {
                QList<qsizetype> outputs;
                for (qsizetype output = 0; output < outputIndices.size(); ++output)
                    if (outputIndices[output] == index) outputs.append(output);
                auto *read = reads->results[size_t(index)].get();
                read->completed = [completion, read, outputs] { completion->complete(*read, outputs); };
            }
        }
        const auto submitted = endFrame(true, false);
        begun = false;
        if (submitted.code != ExecutionCode::Success) return rejected(submitted.code, submitted.diagnostic);
        if (reads) return future;
        outcome.code = ExecutionCode::Success;
        return readyReadback(std::move(outcome));
    } catch (const std::exception &exception) {
        if (begun) cancelFrame();
        retireCompletedReadback();
        return rejected(ExecutionCode::DeviceFailure, QString::fromUtf8(exception.what()));
    }
}
}
