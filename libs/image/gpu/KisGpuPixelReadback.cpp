/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuExecution_p.h"
#include "KisGpuGaussianBrush_p.h"
#include "KisGpuReadbackBatch_p.h"
#include "KisGpuPaintJob_p.h"
#include <QColorSpace>
#include <QSet>
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
    std::mutex mutex;

    void complete(const Detail::ReadbackBatch::Result &read, const QList<qsizetype> &outputs)
    {
        std::lock_guard<std::mutex> guard(mutex);
        const qsizetype expectedBytes = boundsOnly ? 4 * sizeof(qint32) :
                                        transparencyOnly ? sizeof(quint32) : TileRenderer::TileBytes;
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
                                                  OperationId pendingOperation, QSize documentSize)
{
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    QRect needed = region.intersected(domain);
    QHash<quint64, QRect> afterSpatialOperations;
    qsizetype remainingTranslations = 0;
    for (auto operation = layer.operations.crbegin(); operation != layer.operations.crend(); ++operation) {
        if (const auto translation = std::get_if<TranslationSnapshot>(&*operation)) {
            afterSpatialOperations.insert((*translation)->id.sequence, needed);
            ++remainingTranslations;
            needed = needed.translated(-(*translation)->offset).intersected(domain);
        } else if (const auto crop = std::get_if<CropSnapshot>(&*operation)) {
            afterSpatialOperations.insert((*crop)->id.sequence, needed);
            needed = needed.intersected((*crop)->rect.normalized());
        }
    }
    QSet<quint64> coordinates;
    // An unaligned translation can read four source tiles for one output tile.
    // Bound coordinate discovery separately from the caller's final tile budget.
    qsizetype stageMaximum = remainingTranslations
        ? (maximum <= std::numeric_limits<qsizetype>::max() / 4 ? maximum * 4 : std::numeric_limits<qsizetype>::max())
        : maximum;
    for (const auto &operation : layer.operations) {
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            for (const auto &tile : (*write)->tiles) {
                if (!needed.intersects(QRect(tile->coordinate() * TileRenderer::TileSize,
                                             QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                coordinates.insert(Detail::key(tile->coordinate().x(), tile->coordinate().y()));
                if (coordinates.size() > stageMaximum) return std::nullopt;
            }
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            if (--remainingTranslations == 0) stageMaximum = maximum;
            const auto target = afterSpatialOperations.value((*translation)->id.sequence);
            QSet<quint64> moved;
            for (const auto tile : coordinates) {
                const auto pixels = QRect(Detail::coordinate(tile) * TileRenderer::TileSize + (*translation)->offset,
                    QSize(TileRenderer::TileSize, TileRenderer::TileSize)).intersected(target);
                if (pixels.isEmpty()) continue;
                for (int y = Detail::pixelTileIndex(pixels.top(), TileRenderer::TileSize);
                     y <= Detail::pixelTileIndex(pixels.bottom(), TileRenderer::TileSize); ++y) {
                    for (int x = Detail::pixelTileIndex(pixels.left(), TileRenderer::TileSize);
                         x <= Detail::pixelTileIndex(pixels.right(), TileRenderer::TileSize); ++x) {
                        moved.insert(Detail::key(x, y));
                        if (moved.size() > stageMaximum) return std::nullopt;
                    }
                }
            }
            coordinates.swap(moved);
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
            if (!Detail::packedPixel(layer.defaultPixel)) continue;
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
            if (paint.brush.gaussianWash) {
                const auto dabs = Detail::gaussianDabs(paint, paint.id == pendingOperation);
                if (!dabs) return std::nullopt;
                if (!paint.figureStyle.outline) continue;
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
                continue;
            }
            const auto diameter = quint32(std::max<qint64>(1, std::llround(paint.brush.diameter * 16)));
            const auto step = quint32(std::max<qint64>(1, std::llround(paint.brush.diameter * paint.brush.spacing * 16)));
            const auto geometry = Detail::paintGeometry(paint, step);
            if (!geometry) return std::nullopt;
            for (const auto &segment : paint.figureStyle.outline ? geometry->outline : QList<Detail::PaintSegment>{}) {
                const auto tiles = Detail::paintSegmentTiles(segment, diameter, paint.brush, needed, stageMaximum);
                if (!tiles) return std::nullopt;
                for (const auto tile : *tiles) {
                    coordinates.insert(tile);
                    if (coordinates.size() > stageMaximum) return std::nullopt;
                }
            }
            if (hasFigureFill(paint.figureStyle)) {
                const auto tiles = Detail::figureFillTiles(geometry->fill, paint.brush, needed, stageMaximum);
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
    if (layer.kind == LayerKind::Paint) return paintLayerCoordinates(layer, region, maximum, document->pendingOperation(), document->size);
    QHash<QUuid, QList<const Layer *>> children;
    for (const auto &value : document->layers) children[value.parentUuid].append(&value);
    QList<const Layer *> pending{&layer};
    QSet<quint64> coordinates;
    for (qsizetype i = 0; i < pending.size(); ++i) {
        for (const auto child : children.value(pending[i]->uuid)) {
            if (!child->visible || !child->opacity) continue;
            if (child->kind == LayerKind::Group) pending.append(child);
            else {
                const auto tiles = paintLayerCoordinates(*child, region, maximum, document->pendingOperation(), document->size);
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

PixelReadbackOutcome Execution::readLayerPixels(DocumentHandle handle, const Snapshot &pinned,
                                               LayerId layer, qsizetype maximumBytes)
{
    if (!layer || maximumBytes < 0) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Layer pixel capacity and identity are invalid"));
    auto outcome = readPixels(handle, pinned, {}, {layer}, false, PixelReadArea::LayerCoordinates);
    if (outcome.code != ExecutionCode::Success) return outcome;
    constexpr int limit = RgbaTile::CoordinateLimit;
    outcome.region = {-limit, -limit, 2 * limit, 2 * limit};
    const Layer *selected = nullptr;
    for (const auto &value : pinned->layers) if (value.id == layer) selected = &value;
    for (const auto &operation : selected->operations) {
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            if ((*write)->writtenAreas.size() > maximumBytes / qsizetype(sizeof(QRect)) - outcome.writtenAreas.size())
                return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Layer written-area capacity exceeded"));
            outcome.writtenAreas.append((*write)->writtenAreas);
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            for (auto &area : outcome.writtenAreas) area = area.translated((*translation)->offset).intersected(outcome.region);
        } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
            for (auto &area : outcome.writtenAreas) area &= (*crop)->rect.normalized();
        } else if (const auto filter = std::get_if<FilterSnapshot>(&operation)) {
            if (!(*filter)->selectionRect.isEmpty())
                outcome.writtenAreas.append((*filter)->selectionRect.intersected(outcome.region));
        } else {
            const auto &paint = *std::get<PaintSnapshot>(operation);
            if (paint.brush.gaussianWash && paint.figureStyle.outline) {
                const auto dabs = Detail::gaussianDabs(paint, paint.id == pinned->pendingOperation());
                if (!dabs) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Layer written-area geometry is unsupported"));
                if (dabs->size() > maximumBytes / qsizetype(sizeof(QRect)) - outcome.writtenAreas.size())
                    return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Layer written-area capacity exceeded"));
                for (const auto &dab : *dabs) outcome.writtenAreas.append(dab.bounds().intersected(outcome.region));
            }
        }
        if (outcome.writtenAreas.size() > maximumBytes / qsizetype(sizeof(QRect)))
            return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Layer written-area capacity exceeded"));
    }
    outcome.writtenAreas.removeIf([](const QRect &area) { return area.isEmpty(); });
    const auto coordinates = layerCoordinates(pinned, *selected, outcome.region, maximumBytes / TileRenderer::TileBytes);
    if (!coordinates) return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Complete layer pixel capacity exceeded"));
    if (coordinates->isEmpty()) return outcome;
    const auto retired = retireDisplayCaches();
    if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
    for (const auto coordinate : *coordinates) {
        const QRect region(Detail::coordinate(coordinate) * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        const auto pixels = readPixels(handle, pinned, region, {layer}, false, PixelReadArea::LayerCoordinates);
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
    if (qint64(pinned->size.width()) * pinned->size.height() * 4 > maximumBytes)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Complete projection pixel capacity exceeded"));
    const QRect canvas(QPoint(0, 0), pinned->size);
    constexpr int size = TileRenderer::TileSize;
    const qsizetype columns = (pinned->size.width() + size - 1) / size;
    const qsizetype rows = (pinned->size.height() + size - 1) / size;
    QSet<quint64> coordinates;
    const auto defaults = pinned->projectionDefaultPixels();
    bool uniform = Detail::packedPixel(pinned->projectionBackground) != 0;
    QSet<LayerId> projectionLayers;
    for (const auto &input : pinned->projectionInputs()) projectionLayers.insert(input.layer);
    for (const auto &layer : pinned->layers) {
        if (!projectionLayers.contains(layer.id)) continue;
        uniform |= Detail::packedPixel(defaults.value(layer.id)) != 0;
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
    outcome.image = QImage(pinned->size, QImage::Format_RGBA8888);
    if (outcome.image.isNull()) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Projection image allocation failed"));
    outcome.image.fill(Qt::transparent);
    outcome.image.setColorSpace(QColorSpace::SRgb);
    if (!coordinates.isEmpty()) {
        const auto retired = retireDisplayCaches();
        if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
    }
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const QRect region(Detail::coordinate(coordinate) * size, QSize(size, size));
        const auto pixels = readPixels(handle, pinned, region);
        if (pixels.code != ExecutionCode::Success) return rejected(pixels.code, pixels.diagnostic);
        for (const auto &tile : pixels.tiles) {
            const auto origin = tile.coordinate * size;
            const auto clipped = QRect(origin, QSize(size, size)).intersected(canvas);
            for (int y = clipped.top(); y <= clipped.bottom(); ++y) {
                const auto source = tile.rgba.constData() + ((y - origin.y()) * size + clipped.x() - origin.x()) * 4;
                std::memcpy(outcome.image.scanLine(y) + clipped.x() * 4, source, size_t(clipped.width()) * 4);
            }
        }
    }
    outcome.code = ExecutionCode::Success;
    return outcome;
}

PixelBytesOutcome Execution::readRgbaPixels(DocumentHandle handle, const Snapshot &pinned,
                                           QRect region, LayerId layer, qsizetype maximumBytes)
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
    const qint64 byteCount = qint64(region.width()) * region.height() * 4;
    if (byteCount > maximumBytes)
        return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Packed pixel output capacity exceeded"));
    const auto clipped = layer ? region : region.intersected(QRect(QPoint(), pinned->size));
    constexpr int size = TileRenderer::TileSize;
    QSet<quint64> coordinates;
    std::array<quint8, 4> defaultPixel{};
    const auto defaults = pinned->projectionDefaultPixels();
    bool uniformProjection = !layer && Detail::packedPixel(pinned->projectionBackground) != 0;
    QSet<LayerId> projectionLayers;
    if (!layer) for (const auto &input : pinned->projectionInputs()) projectionLayers.insert(input.layer);
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !projectionLayers.contains(value.id)) continue;
        if (layer) defaultPixel = defaults.value(value.id);
        else uniformProjection |= Detail::packedPixel(defaults.value(value.id)) != 0;
        const auto tiles = layerCoordinates(pinned, value, clipped, std::numeric_limits<qsizetype>::max());
        if (!tiles) return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("Packed pixel coordinates exceed capacity"));
        for (const auto tile : *tiles) coordinates.insert(tile);
    }
    if (uniformProjection && !clipped.isEmpty()) {
        for (int y = clipped.top() / size; y <= clipped.bottom() / size; ++y)
            for (int x = clipped.left() / size; x <= clipped.right() / size; ++x)
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
    if (Detail::packedPixel(defaultPixel) == 0) outcome.bytes.fill('\0');
    else for (qsizetype offset = 0; offset < outcome.bytes.size(); offset += 4)
        std::memcpy(outcome.bytes.data() + offset, defaultPixel.data(), 4);
    if (!coordinates.isEmpty()) {
        const auto retired = retireDisplayCaches();
        if (retired.code != ExecutionCode::Success) return rejected(retired.code, retired.diagnostic);
    }
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const QRect tileRegion(Detail::coordinate(coordinate) * size, QSize(size, size));
        const auto pixels = readPixels(handle, pinned, tileRegion, layers, !layer, PixelReadArea::LayerCoordinates);
        if (pixels.code != ExecutionCode::Success) return rejected(pixels.code, pixels.diagnostic);
        for (const auto &tile : pixels.tiles) {
            const auto origin = tile.coordinate * size;
            const auto portion = QRect(origin, QSize(size, size)).intersected(clipped);
            for (int y = portion.top(); y <= portion.bottom(); ++y) {
                const auto source = tile.rgba.constData() + ((y - origin.y()) * size + portion.x() - origin.x()) * 4;
                const qsizetype destination = (qsizetype(y - region.y()) * region.width() + portion.x() - region.x()) * 4;
                std::memcpy(outcome.bytes.data() + destination, source, size_t(portion.width()) * 4);
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

PixelReadbackOutcome Execution::readBounds(DocumentHandle handle, const Snapshot &pinned, LayerId layer,
                                         bool includeTransparentColors)
{
    return readBoundsImpl(handle, pinned, layer, includeTransparentColors, false);
}

PixelReadbackOutcome Execution::readBoundsImpl(DocumentHandle handle, const Snapshot &pinned, LayerId layer,
                                             bool includeTransparentColors, bool sparseProjection)
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
    if (!layer && !sparseProjection && qint64(pinned->size.width()) * pinned->size.height() * 4 > capacity)
        return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Projection bounds pixel capacity exceeded"));
    const qsizetype maximumCoordinates = layer || sparseProjection ? capacity / TileRenderer::TileBytes : columns * rows;
    const auto defaults = pinned->projectionDefaultPixels();
    bool uniform = !layer && (sparseProjection && !includeTransparentColors ? pinned->projectionBackground[3] != 0 : Detail::packedPixel(pinned->projectionBackground) != 0);
    QSet<LayerId> projectionLayers;
    if (!layer) for (const auto &input : pinned->projectionInputs()) projectionLayers.insert(input.layer);
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !projectionLayers.contains(value.id)) continue;
        const auto pixel = defaults.value(value.id);
        uniform |= !layer && (sparseProjection && !includeTransparentColors ? pixel[3] != 0 : Detail::packedPixel(pixel) != 0);
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
            auto pending = readPixelsAsyncImpl(handle, pinned, region, selected, !layer, area, false, true, includeTransparentColors);
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
                                              256 * 1024 * 1024 / TileRenderer::TileBytes);
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
                                         bool tileExtentOnly)
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
        const auto paint = d.renderer->recordRegion(frame.commands, pinned, outcome.region, generatedLayers, base);
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
        const auto append = [&](LayerId layer, const QList<TileView> &tiles) {
            for (const auto &tile : tiles) {
                if (!outcome.region.intersects(QRect(tile.coordinate * TileRenderer::TileSize,
                                                    QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                if (!indices.contains(tile.pixels)) {
                    if (buffers.size() >= d.maximumReadbackBytes / TileRenderer::TileBytes) return false;
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
                                      transparencyOnly ? sizeof(quint32) : TileRenderer::TileBytes;
        if (tileExtentOnly) {
            for (const auto layer : layers)
                for (const auto &tile : rendered->tiles(layer)) {
                    const QRect bounds(tile.coordinate * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize));
                    if (outcome.region.intersects(bounds)) *outcome.bounds |= bounds;
                }
        } else if (boundsOnly || transparencyOnly) {
            withinLimit = d.maximumReadbackBytes >= qsizetype(transferBytes);
            if (withinLimit) {
                if (boundsOnly) {
                    auto implicit = pinned->projectionBackground;
                    if (!layers.isEmpty()) {
                        implicit = pinned->projectionDefaultPixels().value(layers.first());
                    }
                    summary = d.renderer->m_d->recordBounds(frame.commands,
                        projected ? projected->tiles() : rendered->tiles(layers.first()), outcome.region, implicit, includeTransparentColors);
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
            return rejected(ExecutionCode::CapacityExceeded, QStringLiteral("GPU pixel readback byte capacity exceeded"));
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
