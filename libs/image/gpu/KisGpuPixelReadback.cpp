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

namespace LibrePaint::Image::Gpu {
namespace {
PixelReadbackOutcome failure(ExecutionCode code, const QString &diagnostic)
{
    PixelReadbackOutcome outcome;
    outcome.code = code;
    outcome.diagnostic = diagnostic;
    return outcome;
}
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

std::optional<QList<quint64>> layerCoordinates(const Layer &layer, QSize size, QRect region, qsizetype maximum)
{
    QSet<quint64> coordinates;
    const QRect canvas(QPoint(0, 0), size);
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
            for (const auto &segment : Detail::paintGeometry(paint, step)) {
                const auto tiles = Detail::paintSegmentTiles(segment, diameter, region.intersected(canvas), size, maximum);
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
    if (d.device->rhi().finish() != QRhi::FrameOpSuccess)
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
    const auto coordinates = layerCoordinates(*selected, pinned->size, outcome.region, maximumBytes / TileRenderer::TileBytes);
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
    bool uniform = false;
    for (const auto &layer : pinned->layers) {
        if (!layer.visible) continue;
        uniform |= Detail::packedPixel(layer.defaultPixel) != 0;
        const auto tiles = layerCoordinates(layer, pinned->size, canvas, columns * rows);
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
    bool uniformProjection = false;
    for (const auto &value : pinned->layers) {
        if (layer ? value.id != layer : !value.visible) continue;
        if (layer) defaultPixel = value.defaultPixel;
        else uniformProjection |= Detail::packedPixel(value.defaultPixel) != 0;
        const auto tiles = layerCoordinates(value, pinned->size, clipped, std::numeric_limits<qsizetype>::max());
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
    auto &d = *m_d;
    if (!d.onThread() || !pinned || (area != PixelReadArea::Canvas && area != PixelReadArea::LayerCoordinates)) {
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read requires a document, valid area and the GPU thread"));
    }
    if (d.active || d.device->rhi().isRecordingFrame()) return failure(ExecutionCode::Busy, QStringLiteral("Pixel read requires submitted frames"));
    if (d.device->readbackPending()) return failure(ExecutionCode::Busy, QStringLiteral("The native GPU still retains a previous readback"));
    retireCompletedReadback();
    const auto document = d.documents.constFind(handle);
    if (document == d.documents.cend() || document->requested->documentId() != pinned->documentId()) {
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read document is no longer attached or belongs to another handle"));
    }
    if (!validPixelRegion(region)) {
        return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read region is outside the supported range"));
    }
    QSet<LayerId> available, selected;
    for (const auto &layer : pinned->layers) available.insert(layer.id);
    for (const auto layer : layers) {
        if (!available.contains(layer) || selected.contains(layer)) return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read layer is unavailable or repeated"));
        selected.insert(layer);
    }
    QList<LayerId> generatedLayers = layers;
    if (includeProjection) {
        for (const auto &layer : pinned->layers) {
            if (layer.visible && !selected.contains(layer.id)) {
                generatedLayers.append(layer.id);
                selected.insert(layer.id);
            }
        }
    }
    PixelReadbackOutcome outcome;
    outcome.document = pinned;
    outcome.generation = d.generation;
    outcome.region = area == PixelReadArea::Canvas ? region.intersected(QRect(QPoint(0, 0), pinned->size)) : region;
    if (outcome.region.isEmpty() || (layers.isEmpty() && !includeProjection)) {
        outcome.code = ExecutionCode::Success;
        return outcome;
    }
    if (!d.device->rhi().isFeatureSupported(QRhi::ReadBackNonUniformBuffer)) {
        return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU storage buffer readback is unavailable"));
    }
    const auto base = document->rendered;
    const auto projectedBase = document->projected;
    RenderedSnapshot rendered;
    ProjectedSnapshot projected;
    std::shared_ptr<Detail::ReadbackBatch> reads;
    QRhiResourceUpdateBatch *batch = nullptr;
    bool begun = false;
    try {
        const auto frame = beginReadbackFrame();
        if (frame.code != ExecutionCode::Success) return failure(frame.code, frame.diagnostic);
        begun = true;
        const auto paint = d.renderer->recordRegion(*frame.commands, pinned, outcome.region, generatedLayers, base);
        if (paint.code != RenderOutcome::Recorded) {
            cancelFrame();
            begun = false;
            return failure(executionCode(paint.code), paint.diagnostic);
        }
        rendered = paint.version;
        if (includeProjection) {
            const auto compose = d.renderer->compose(*frame.commands, rendered, outcome.region, projectedBase);
            if (compose.code != RenderOutcome::Recorded) {
                cancelFrame();
                begun = false;
                return failure(executionCode(compose.code), compose.diagnostic);
            }
            projected = compose.version;
        }
        QList<QRhiBuffer *> buffers;
        QHash<QRhiBuffer *, qsizetype> indices;
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
        for (const auto layer : layers) {
            if (!append(layer, rendered->tiles(layer))) { withinLimit = false; break; }
        }
        if (withinLimit && projected) withinLimit = append(0, projected->tiles());
        if (!withinLimit) {
            cancelFrame();
            begun = false;
            return failure(ExecutionCode::CapacityExceeded, QStringLiteral("GPU pixel readback byte capacity exceeded"));
        }
        if (!buffers.isEmpty()) {
            reads = Detail::ReadbackBatch::create(*d.device, buffers.size());
            if (!reads) {
                cancelFrame();
                begun = false;
                return failure(ExecutionCode::Busy, QStringLiteral("The native GPU still retains a previous readback"));
            }
            d.readbackRendered = rendered;
            d.readbackProjected = projected;
            batch = d.device->rhi().nextResourceUpdateBatch();
            for (qsizetype i = 0; i < buffers.size(); ++i) {
                batch->readBackBuffer(buffers[i], 0, TileRenderer::TileBytes, reads->results[i].get());
                if (!batch->hasOptimalCapacity()) {
                    frame.commands->resourceUpdate(batch);
                    batch = d.device->rhi().nextResourceUpdateBatch();
                }
            }
            frame.commands->resourceUpdate(batch);
            batch = nullptr;
        }
        const auto submitted = submitFrame();
        begun = false;
        if (submitted.code != ExecutionCode::Success) return failure(submitted.code, submitted.diagnostic);
        if (!d.documents.contains(handle) || d.generation != outcome.generation) {
            return failure(ExecutionCode::InvalidRequest, QStringLiteral("Pixel read document closed or GPU generation changed"));
        }
        if (reads) {
            for (const auto &read : reads->results) {
                if (read->data.size() != TileRenderer::TileBytes) return failure(ExecutionCode::DeviceFailure, QStringLiteral("GPU pixel readback did not return a complete tile"));
            }
        }
        for (qsizetype i = 0; i < outcome.tiles.size(); ++i) outcome.tiles[i].rgba = reads->results[outputIndices[i]]->data;
        outcome.code = ExecutionCode::Success;
        return outcome;
    } catch (const std::exception &exception) {
        if (batch) batch->release();
        if (begun) cancelFrame();
        retireCompletedReadback();
        return failure(ExecutionCode::DeviceFailure, QString::fromUtf8(exception.what()));
    }
}
}
