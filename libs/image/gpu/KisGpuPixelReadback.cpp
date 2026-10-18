/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuExecution_p.h"
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

std::optional<QList<quint64>> layerCoordinates(const Layer &layer, QRect region, qsizetype maximum)
{
    QSet<quint64> coordinates;
    for (const auto &operation : layer.operations) {
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            for (const auto &tile : (*write)->tiles) {
                if (!region.intersects(QRect(tile->coordinate() * TileRenderer::TileSize,
                                             QSize(TileRenderer::TileSize, TileRenderer::TileSize)))) continue;
                coordinates.insert(Detail::key(tile->coordinate().x(), tile->coordinate().y()));
                if (coordinates.size() > maximum) return std::nullopt;
            }
        } else {
            const auto &paint = **std::get_if<PaintSnapshot>(&operation);
            const auto diameter = quint32(std::max<qint64>(1, std::llround(paint.brush.diameter * 16)));
            const auto step = quint32(std::max<qint64>(1, std::llround(paint.brush.diameter * paint.brush.spacing * 16)));
            const auto geometry = Detail::paintGeometry(paint, step);
            if (!geometry) return std::nullopt;
            for (const auto &segment : paint.figureStyle.outline ? geometry->outline : QList<Detail::PaintSegment>{}) {
                const auto tiles = Detail::paintSegmentTiles(segment, diameter, paint.brush, region, maximum);
                if (!tiles) return std::nullopt;
                for (const auto tile : *tiles) {
                    coordinates.insert(tile);
                    if (coordinates.size() > maximum) return std::nullopt;
                }
            }
            if (paint.figureStyle.fill) {
                const auto tiles = Detail::figureFillTiles(geometry->fill, paint.brush, region, maximum);
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
    const auto coordinates = layerCoordinates(*selected, outcome.region, maximumBytes / TileRenderer::TileBytes);
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
    bool uniform = Detail::packedPixel(pinned->projectionBackground) != 0;
    for (const auto &layer : pinned->layers) {
        if (!layer.visible) continue;
        uniform |= Detail::packedPixel(layer.defaultPixel) != 0;
        const auto tiles = layerCoordinates(layer, canvas, columns * rows);
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
    bool uniformProjection = !layer && Detail::packedPixel(pinned->projectionBackground) != 0;
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !value.visible) continue;
        if (layer) defaultPixel = value.defaultPixel;
        else uniformProjection |= Detail::packedPixel(value.defaultPixel) != 0;
        const auto tiles = layerCoordinates(value, clipped, std::numeric_limits<qsizetype>::max());
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

PixelReadbackOutcome Execution::readBounds(DocumentHandle handle, const Snapshot &pinned, LayerId layer)
{
    if (!m_d->onThread()) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Bounds read requires the GPU thread"));
    constexpr int limit = RgbaTile::CoordinateLimit;
    constexpr qsizetype capacity = 256 * 1024 * 1024;
    const auto area = layer ? PixelReadArea::LayerCoordinates : PixelReadArea::Canvas;
    const QList<LayerId> selected = layer ? QList<LayerId>{layer} : QList<LayerId>{};
    auto outcome = readPixelsAsyncImpl(handle, pinned, {}, selected, !layer, area, false, true).get();
    if (outcome.code != ExecutionCode::Success) return outcome;
    outcome.region = layer ? QRect(-limit, -limit, 2 * limit, 2 * limit) : QRect(QPoint(), pinned->size);
    QSet<quint64> coordinates;
    const qsizetype columns = (pinned->size.width() + TileRenderer::TileSize - 1) / TileRenderer::TileSize;
    const qsizetype rows = (pinned->size.height() + TileRenderer::TileSize - 1) / TileRenderer::TileSize;
    if (!layer && qint64(pinned->size.width()) * pinned->size.height() * 4 > capacity)
        return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Projection bounds pixel capacity exceeded"));
    bool uniform = !layer && Detail::packedPixel(pinned->projectionBackground) != 0;
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !value.visible) continue;
        uniform |= !layer && Detail::packedPixel(value.defaultPixel) != 0;
        const auto tiles = layerCoordinates(value, outcome.region, layer ? capacity / TileRenderer::TileBytes : columns * rows);
        if (!tiles) return failure(ExecutionCode::CapacityExceeded, QStringLiteral("Bounds tile coordinate capacity exceeded"));
        for (const auto coordinate : *tiles) coordinates.insert(coordinate);
    }
    if (uniform) {
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x) coordinates.insert(Detail::key(x, y));
    }
    if (coordinates.isEmpty()) return outcome;
    const auto retired = retireDisplayCaches();
    if (retired.code != ExecutionCode::Success) return failure(retired.code, retired.diagnostic);
    auto ordered = coordinates.values();
    std::sort(ordered.begin(), ordered.end());
    for (const auto coordinate : ordered) {
        const QRect region(Detail::coordinate(coordinate) * TileRenderer::TileSize,
                           QSize(TileRenderer::TileSize, TileRenderer::TileSize));
        auto pending = readPixelsAsyncImpl(handle, pinned, region, selected, !layer, area, false, true);
        if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready && !m_d->device->finish())
            return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU bounds completion failed"));
        retireCompletedReadback();
        const auto part = pending.get();
        if (part.code != ExecutionCode::Success) return failure(part.code, part.diagnostic);
        *outcome.bounds |= *part.bounds;
    }
    return outcome;
}

std::future<PixelReadbackOutcome> Execution::readPixelsAsyncImpl(DocumentHandle handle, const Snapshot &pinned,
                                         QRect region, const QList<LayerId> &layers, bool includeProjection,
                                         PixelReadArea area, bool transparencyOnly, bool boundsOnly)
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
        for (const auto &layer : pinned->layers) {
            if (layer.visible && (!transparencyOnly || layer.opacity) && !selected.contains(layer.id)) {
                generatedLayers.append(layer.id);
                selected.insert(layer.id);
            }
        }
    }
    PixelReadbackOutcome outcome;
    outcome.document = pinned;
    outcome.generation = d.generation;
    outcome.region = area == PixelReadArea::Canvas ? region.intersected(QRect(QPoint(0, 0), pinned->size)) : region;
    if (transparencyOnly) outcome.containsTransparency = false;
    if (boundsOnly) outcome.bounds = QRect();
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
        if (boundsOnly || transparencyOnly) {
            withinLimit = d.maximumReadbackBytes >= qsizetype(transferBytes);
            if (withinLimit) {
                if (boundsOnly) {
                    auto implicit = pinned->projectionBackground;
                    if (!layers.isEmpty()) {
                        for (const auto &layer : pinned->layers)
                            if (layer.id == layers.first()) implicit = layer.defaultPixel;
                    }
                    summary = d.renderer->m_d->recordBounds(frame.commands,
                        projected ? projected->tiles() : rendered->tiles(layers.first()), outcome.region, implicit);
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
