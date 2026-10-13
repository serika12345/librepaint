/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileRenderer.h"
#include "KisGpuPaintJob_p.h"

#include <QFile>
#include <QMap>
#include <QThread>
#include <rhi/qrhi.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace Krita::Image::Gpu {
using namespace Detail;

TileRenderer::Private::Private(DevicePtr device, qsizetype byteBudget)
    : device(std::move(device)), rhi(this->device->rhi()),
      budget(std::make_shared<Budget>(Budget{this->device, byteBudget})),
      zero(own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer, TileBytes))),
      layoutParameters(own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(PaintParameters)))),
      layout(own(rhi, rhi.newShaderResourceBindings())), pipeline(own(rhi, rhi.newComputePipeline()))
{
    if (!rhi.isFeatureSupported(QRhi::Compute) || !zero->create() || !layoutParameters->create()) {
        error = QStringLiteral("GPU storage buffers and compute are required");
        return;
    }
    // The pipeline's layout is independent of each tile's actual bindings.
    layout->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, layoutParameters.get()),
        QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::bufferLoad(2, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::bufferStore(3, QRhiShaderResourceBinding::ComputeStage, zero.get())
    });
    QFile shader(QStringLiteral(":/librepaint/gpu/shaders/paint.comp.qsb"));
    if (!layout->create() || !shader.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("GPU paint resources could not be initialized");
        return;
    }
    pipeline->setShaderStage({QRhiShaderStage::Compute, QShader::fromSerialized(shader.readAll())});
    pipeline->setShaderResourceBindings(layout.get());
    if (!pipeline->create()) error = QStringLiteral("GPU paint pipeline could not be created");
    if (error.isEmpty()) initializeProjection();
}

std::shared_ptr<Tile> TileRenderer::Private::allocateTile()
{
    auto pixels = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer, TileBytes));
    if (!pixels->create()) return {};
    return std::make_shared<Tile>(std::move(pixels), budget);
}

RenderedVersion::RenderedVersion(std::shared_ptr<Data> data) : m_data(std::move(data)) {}
Snapshot RenderedVersion::document() const { return m_data->document; }
QList<TileView> RenderedVersion::tiles(LayerId layer) const
{
    QList<TileView> result;
    const auto found = m_data->layers.constFind(layer);
    if (found == m_data->layers.cend()) return result;
    result.reserve(found->size());
    for (auto i = found->cbegin(); i != found->cend(); ++i) result.append({coordinate(i.key()), i.value()->pixels.get()});
    return result;
}

TileRenderer::TileRenderer(DevicePtr device, qsizetype byteBudget)
{
    if (!device || device->rhi().thread() != QThread::currentThread() || byteBudget < TileBytes) {
        throw std::invalid_argument("invalid GPU owner thread or tile budget");
    }
    m_d = std::make_unique<Private>(std::move(device), byteBudget);
}
TileRenderer::~TileRenderer() = default;
qsizetype TileRenderer::residentBytes() const { return m_d->budget->used; }

RenderOutcome TileRenderer::record(QRhiCommandBuffer &commands, const Snapshot &document, const RenderedSnapshot &base)
{
    QList<LayerId> layers;
    if (document) for (const auto &layer : document->layers) layers.append(layer.id);
    return recordImpl(commands, document, base, std::nullopt, layers);
}

RenderOutcome TileRenderer::recordRegion(QRhiCommandBuffer &commands, const Snapshot &document,
                                        QRect region, const QList<LayerId> &layers, const RenderedSnapshot &base)
{
    constexpr qint64 limit = qint64(OperationDocument::CoordinateLimit);
    if (region.width() < 0 || region.height() < 0 || region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU tile region is outside the supported range")};
    }
    if (region.isEmpty()) return recordImpl(commands, document, base, QRect{}, layers);
    const int left = int(std::floor(qreal(region.left()) / TileSize));
    const int top = int(std::floor(qreal(region.top()) / TileSize));
    const int right = int(std::floor(qreal(region.right()) / TileSize));
    const int bottom = int(std::floor(qreal(region.bottom()) / TileSize));
    return recordImpl(commands, document, base,
                      QRect(left * TileSize, top * TileSize, (right - left + 1) * TileSize, (bottom - top + 1) * TileSize), layers);
}

RenderOutcome TileRenderer::recordImpl(QRhiCommandBuffer &commands, const Snapshot &document,
                                      const RenderedSnapshot &base, std::optional<QRect> region,
                                      const QList<LayerId> &layers)
{
    auto &d = *m_d;
    if (d.rhi.thread() != QThread::currentThread() || !d.rhi.isRecordingFrame() || !document ||
        (base && (base->m_data->owner != d.budget || base->document()->documentId() != document->documentId()))) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU versions require their owning device and document")};
    }
    if (!d.error.isEmpty() || d.rhi.isDeviceLost()) return {RenderOutcome::DeviceFailure, {}, d.error};
    QSet<LayerId> available, selected;
    for (const auto &layer : document->layers) available.insert(layer.id);
    for (const auto layer : layers) {
        if (!available.contains(layer) || selected.contains(layer)) {
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU tile layer is unavailable or repeated")};
        }
        selected.insert(layer);
    }
    if (base && base->document() == document && base->m_data->region == region && base->m_data->selected == selected)
        return {RenderOutcome::Recorded, base, {}};
    auto data = std::make_shared<RenderedVersion::Data>();
    data->document = document;
    data->owner = d.budget;
    data->region = region;
    data->selected = selected;
    const QRect canvas(QPoint(0, 0), document->size);
    const QRect paintRegion = region ? region->intersected(canvas) : canvas;
    std::vector<PaintJob> jobs;
    struct Checkpoint { qsizetype operation = 0; QSharedPointer<const RasterTile> pixels; };
    struct Upload { std::shared_ptr<Tile> tile; QByteArray rgba; };
    std::vector<Upload> uploads;
    for (const auto &layer : document->layers) {
        if (!selected.contains(layer.id) || (region && region->isEmpty())) continue;
        qsizetype prefix = 0;
        qsizetype reusedPrefix = 0;
        QSet<quint64> reusedCoordinates;
        std::optional<qsizetype> appendedFrom;
        Tiles strokeInputs;
        if (base) {
            for (const auto &old : base->document()->layers) {
                if (old.id != layer.id) continue;
                if (old.defaultPixel != layer.defaultPixel) break;
                data->defaults[layer.id] = base->m_data->defaults.value(layer.id);
                const bool droppingStroke = old.operations.size() == layer.operations.size() + 1 &&
                    base->m_data->strokeInputs.contains(layer.id);
                if ((old.operations.size() > layer.operations.size() && !droppingStroke) || !base->m_data->selected.contains(layer.id)) break;
                qsizetype shared = droppingStroke ? layer.operations.size() : old.operations.size();
                bool changingStroke = false;
                qsizetype previousSamples = -1;
                if (shared > 0 && shared == layer.operations.size() && base->m_data->strokeInputs.contains(layer.id)) {
                    const auto previous = std::get_if<PaintSnapshot>(&old.operations.back());
                    const auto current = std::get_if<PaintSnapshot>(&layer.operations.back());
                    changingStroke = previous && current && *previous != *current && (*previous)->id == (*current)->id &&
                        (*previous)->brush == (*current)->brush;
                    if (changingStroke && (*previous)->shape.kind == ShapeKind::Freehand &&
                        (*current)->shape.kind == ShapeKind::Freehand && (*current)->sampleCount >= (*previous)->sampleCount)
                        previousSamples = (*previous)->sampleCount;
                    if (changingStroke) --shared;
                }
                bool matches = true;
                for (qsizetype i = 0; i < shared; ++i) {
                    if (old.operations[i] != layer.operations[i]) { matches = false; break; }
                }
                if (matches) {
                    const auto &oldTiles = base->m_data->layers[layer.id];
                    for (auto tile = oldTiles.cbegin(); tile != oldTiles.cend(); ++tile) {
                        if (region && !region->intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) continue;
                        data->layers[layer.id][tile.key()] = tile.value();
                        reusedCoordinates.insert(tile.key());
                    }
                    if (changingStroke || droppingStroke) {
                        if (previousSamples >= 0) appendedFrom = std::max<qsizetype>(0, previousSamples - 1);
                        const auto &inputs = base->m_data->strokeInputs[layer.id];
                        for (auto input = inputs.cbegin(); input != inputs.cend(); ++input) {
                            if (!reusedCoordinates.contains(input.key())) continue;
                            if (appendedFrom) strokeInputs[input.key()] = input.value();
                            else if (input.value() && input.value() != data->defaults.value(layer.id))
                                data->layers[layer.id][input.key()] = input.value();
                            else data->layers[layer.id].remove(input.key());
                        }
                    } else if (base->m_data->strokeInputs.contains(layer.id) && !old.operations.isEmpty()) {
                        const auto paint = std::get_if<PaintSnapshot>(&old.operations.back());
                        if (paint && document->pendingOperation() == (*paint)->id) {
                            auto &inputs = data->strokeInputs[layer.id];
                            const auto &oldInputs = base->m_data->strokeInputs[layer.id];
                            for (auto input = oldInputs.cbegin(); input != oldInputs.cend(); ++input)
                                if (reusedCoordinates.contains(input.key())) inputs[input.key()] = input.value();
                        }
                    }
                    reusedPrefix = shared;
                    if (!base->m_data->region || (region && base->m_data->region->contains(*region))) prefix = reusedPrefix;
                }
                break;
            }
        }
        if (packedPixel(layer.defaultPixel) && !data->defaults.value(layer.id)) {
            if (d.budget->used > d.budget->limit - TileBytes) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU default pixel tile budget exhausted")};
            }
            PaintJob fill;
            fill.output = d.allocateTile();
            fill.values.color = packedPixel(layer.defaultPixel);
            fill.values.uniformFill = 1;
            if (!preparePaintJob(d.rhi, fill, d.zero.get())) {
                return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU default pixel allocation failed")};
            }
            data->defaults[layer.id] = fill.output;
            jobs.push_back(std::move(fill));
        }
        auto &tiles = data->layers[layer.id];
        QMap<quint64, std::array<std::shared_ptr<Tile>, 2>> writable;
        QMap<quint64, Checkpoint> checkpoints;
        for (qsizetype op = prefix; op < layer.operations.size(); ++op) {
            const auto write = std::get_if<TileWriteSnapshot>(&layer.operations[op]);
            if (!write) continue;
            for (const auto &tile : (*write)->tiles) {
                const auto point = tile->coordinate();
                if (region && !region->intersects(QRect(point * TileSize, QSize(TileSize, TileSize)))) continue;
                const auto coordinate = key(point.x(), point.y());
                if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                checkpoints[coordinate] = {op, tile};
                if (checkpoints.size() > d.budget->limit / TileBytes) {
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU checkpoint tile budget exhausted")};
                }
            }
        }
        if (checkpoints.size() > (d.budget->limit - d.budget->used) / TileBytes) {
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU checkpoint tile budget exhausted")};
        }
        // Each coordinate starts at its last whole-tile write. Later pixel-local
        // paint operations alternate the two writable buffers in operation order.
        for (auto checkpoint = checkpoints.cbegin(); checkpoint != checkpoints.cend(); ++checkpoint) {
            const auto pixels = checkpoint->pixels->rgba();
            if (pixels.size() != TileBytes) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("CPU raster checkpoint is incomplete")};
            const auto output = d.allocateTile();
            if (!output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU checkpoint allocation failed")};
            tiles[checkpoint.key()] = output;
            writable[checkpoint.key()][0] = output;
            uploads.push_back({output, pixels});
        }
        for (qsizetype op = prefix; op < layer.operations.size(); ++op) {
            const auto paint = std::get_if<PaintSnapshot>(&layer.operations[op]);
            if (!paint || paintRegion.isEmpty()) continue;
            const auto &operation = **paint;
            const bool pending = document->pendingOperation() == operation.id;
            const bool extending = appendedFrom && op == layer.operations.size() - 1;
            if (pending) data->strokeInputs[layer.id];
            if (pending && extending) data->strokeInputs[layer.id] = strokeInputs;
            const auto diameter = quint32(std::max<qint64>(1, std::llround(operation.brush.diameter * 16)));
            const auto step = quint32(std::max<qint64>(1, std::llround(operation.brush.diameter * operation.brush.spacing * 16)));
            const auto segments = paintGeometry(operation, step);
            QMap<quint64, QList<PaintSegment>> affected;
            QSet<quint64> changedCoordinates;
            for (qsizetype index = 0; index < segments.size(); ++index) {
                const auto &segment = segments[index];
                const auto coordinates = paintSegmentTiles(segment, diameter, paintRegion, document->size, d.budget->limit / TileBytes);
                if (!coordinates) return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                for (const auto coordinate : *coordinates) {
                    if (op < reusedPrefix && reusedCoordinates.contains(coordinate)) continue;
                    const auto checkpoint = checkpoints.constFind(coordinate);
                    if (checkpoint != checkpoints.cend() && op < checkpoint->operation) continue;
                    affected[coordinate].append(segment);
                    if (extending && index >= *appendedFrom) changedCoordinates.insert(coordinate);
                    if (affected.size() > d.budget->limit / TileBytes)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
                }
            }
            if (extending) {
                for (auto tile = affected.begin(); tile != affected.end();) {
                    if (strokeInputs.contains(tile.key()) && !changedCoordinates.contains(tile.key())) tile = affected.erase(tile);
                    else ++tile;
                }
            }
            qsizetype required = 0;
            for (auto tile = affected.cbegin(); tile != affected.cend(); ++tile) {
                const auto pool = writable.value(tile.key());
                const int next = pool[0] && tiles.value(tile.key()) == pool[0] ? 1 : 0;
                if (!pool[next]) ++required;
            }
            if (required > (d.budget->limit - d.budget->used) / TileBytes) {
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU tile budget exhausted")};
            }
            for (auto tile = affected.cbegin(); tile != affected.cend(); ++tile) {
                const auto bytes = quint64(tile.value().size()) * sizeof(PaintSegment);
                if (bytes > std::numeric_limits<quint32>::max()) {
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU operation buffer limit exceeded")};
                }
                PaintJob job;
                job.input = extending && strokeInputs.contains(tile.key()) ? strokeInputs.value(tile.key()) :
                    tiles.value(tile.key(), data->defaults.value(layer.id));
                if (pending) data->strokeInputs[layer.id][tile.key()] = job.input;
                auto &pool = writable[tile.key()];
                auto &output = pool[pool[0] && job.input == pool[0] ? 1 : 0];
                if (!output) output = d.allocateTile();
                job.output = output;
                job.geometry = tile.value();
                const auto point = coordinate(tile.key()) * TileSize;
                job.values = {point.x(), point.y(), document->size.width(), document->size.height(),
                              packedPixel(operation.brush.rgba),
                              operation.brush.opacity, quint32(operation.brush.erase), step, quint32(tile.value().size()), diameter};
                if (!preparePaintJob(d.rhi, job, d.zero.get())) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU paint allocation failed")};
                tiles[tile.key()] = job.output;
                jobs.push_back(std::move(job));
            }
        }
    }
    if (!d.zeroUploaded) {
        auto updates = d.rhi.nextResourceUpdateBatch();
        updates->uploadStaticBuffer(d.zero.get(), QByteArray(TileBytes, '\0'));
        commands.resourceUpdate(updates);
        d.zeroUploaded = true;
    }
    if (!uploads.empty()) {
        auto updates = d.rhi.nextResourceUpdateBatch();
        for (const auto &upload : uploads) {
            updates->uploadStaticBuffer(upload.tile->pixels.get(), upload.rgba);
            if (!updates->hasOptimalCapacity()) {
                commands.resourceUpdate(updates);
                updates = d.rhi.nextResourceUpdateBatch();
            }
        }
        commands.resourceUpdate(updates);
    }
    for (const auto &job : jobs) {
        auto updates = d.rhi.nextResourceUpdateBatch();
        if (job.samples) updates->uploadStaticBuffer(job.samples.get(), job.geometry.constData());
        updates->uploadStaticBuffer(job.parameters.get(), &job.values);
        commands.beginComputePass(updates);
        commands.setComputePipeline(d.pipeline.get());
        commands.setShaderResources(job.bindings.get());
        commands.dispatch(TileSize / 8, TileSize / 8, 1);
        commands.endComputePass();
    }
    return {RenderOutcome::Recorded, RenderedSnapshot(new RenderedVersion(data)), {}};
}
}
