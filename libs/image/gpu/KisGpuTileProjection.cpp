/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileStorage_p.h"
#include <QFile>
#include <QThread>
#include <QSet>
#include <algorithm>
#include <vector>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
struct CompositeParameters { quint32 opacity = 255; quint32 normalizeBase = 0; quint32 padding[2]{}; };
struct CompositeJob
{
    std::shared_ptr<Tile> base, layer, output;
    CompositeParameters values;
    Resource<QRhiBuffer> parameters;
    Resource<QRhiShaderResourceBindings> bindings;
};
}

void TileRenderer::Private::initializeProjection()
{
    projectionLayout = own(rhi, rhi.newShaderResourceBindings());
    projectionPipeline = own(rhi, rhi.newComputePipeline());
    projectionParameters = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(CompositeParameters)));
    projectionLayout->setBindings({
        QRhiShaderResourceBinding::bufferLoad(0, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::bufferStore(2, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::uniformBuffer(3, QRhiShaderResourceBinding::ComputeStage, projectionParameters.get())
    });
    QFile shader(QStringLiteral(":/librepaint/gpu/shaders/over.comp.qsb"));
    if (!projectionParameters->create() || !projectionLayout->create() || !shader.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("GPU projection resources could not be initialized");
        return;
    }
    projectionPipeline->setShaderStage({QRhiShaderStage::Compute, QShader::fromSerialized(shader.readAll())});
    projectionPipeline->setShaderResourceBindings(projectionLayout.get());
    if (!projectionPipeline->create()) error = QStringLiteral("GPU projection pipeline could not be created");
}

ProjectedVersion::ProjectedVersion(std::shared_ptr<Data> data) : m_data(std::move(data)) {}
Snapshot ProjectedVersion::document() const { return m_data->source->document(); }
QRect ProjectedVersion::region() const { return m_data->region; }
QList<TileView> ProjectedVersion::tiles() const
{
    QList<TileView> result;
    result.reserve(m_data->tiles.size());
    for (auto i = m_data->tiles.cbegin(); i != m_data->tiles.cend(); ++i) {
        result.append({coordinate(i.key()), i.value()->pixels.get()});
    }
    return result;
}

ProjectionOutcome TileRenderer::compose(QRhiCommandBuffer &commands, const RenderedSnapshot &document,
                                       QRect region, const ProjectedSnapshot &base)
{
    auto &d = *m_d;
    if (d.rhi.thread() != QThread::currentThread() || !d.rhi.isRecordingFrame() || !document ||
        document->m_data->owner != d.budget || region.width() < 0 || region.height() < 0 ||
        (base && (base->m_data->source->m_data->owner != d.budget ||
                  base->document()->documentId() != document->document()->documentId()))) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Projection requires the owning GPU and document")};
    }
    if (!d.error.isEmpty() || d.rhi.isDeviceLost()) return {RenderOutcome::DeviceFailure, {}, d.error};
    auto data = std::make_shared<ProjectedVersion::Data>();
    data->source = document;
    data->directLayerProjection = document->document()->directProjectionLayer() != 0;
    data->region = region.intersected(QRect(QPoint(0, 0), document->document()->size));
    if (document->m_data->region && !data->region.isEmpty() && !document->m_data->region->contains(data->region)) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Projection extends outside the generated GPU region")};
    }
    QSet<quint64> coordinates;
    // A single fully opaque paint layer supplies the legacy transparent-root projection,
    // including the RGB components of pixels whose alpha is zero.
    const auto background = data->directLayerProjection ? std::shared_ptr<Tile>{} : document->m_data->background;
    bool implicit = bool(background);
    for (const auto &layer : document->document()->layers) {
        if (!layer.visible || !layer.opacity) continue;
        if (!document->m_data->selected.contains(layer.id)) {
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Projection requires every visible GPU layer")};
        }
        implicit = implicit || bool(document->m_data->defaults.value(layer.id));
        const auto &tiles = document->m_data->layers[layer.id];
        for (auto tile = tiles.cbegin(); tile != tiles.cend(); ++tile) {
            if (data->region.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) {
                coordinates.insert(tile.key());
            }
        }
    }
    if (implicit && !data->region.isEmpty()) {
        const int left = data->region.left() / TileSize, right = data->region.right() / TileSize;
        const int top = data->region.top() / TileSize, bottom = data->region.bottom() / TileSize;
        // Bound derived coordinate bookkeeping before expanding a uniform region.
        if (quint64(right - left + 1) * quint64(bottom - top + 1) > quint64(d.budget->limit / TileBytes)) {
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU projection coordinate capacity exceeded")};
        }
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) coordinates.insert(key(x, y));
        }
    }
    if (background) {
        for (const auto coordinate : coordinates) data->inputs[coordinate].append({background, 255});
    }
    for (const auto &layer : document->document()->layers) {
        if (!layer.visible || !layer.opacity) continue;
        const auto fallback = document->m_data->defaults.value(layer.id);
        const auto &tiles = document->m_data->layers[layer.id];
        for (const auto coordinate : coordinates) {
            const auto pixels = tiles.value(coordinate, fallback);
            if (pixels) data->inputs[coordinate].append({pixels, layer.opacity});
        }
    }
    const auto firstComposition = [&](const auto &inputs) {
        return inputs.first().opacity == 255 && (inputs.size() > 1 || background || data->directLayerProjection ||
                                                !inputs.first().pixels->mayContainHiddenRgb) ? 1 : 0;
    };
    qsizetype required = 0;
    for (auto tile = data->inputs.cbegin(); tile != data->inputs.cend(); ++tile) {
        if (base && base->m_data->directLayerProjection == data->directLayerProjection &&
            base->m_data->inputs.value(tile.key()) == tile.value()) {
            data->tiles[tile.key()] = base->m_data->tiles.value(tile.key());
        } else if (tile.value().size() == 1 && firstComposition(tile.value()) == 1) {
            data->tiles[tile.key()] = tile.value().first().pixels;
        } else {
            const auto jobs = tile.value().size() - firstComposition(tile.value());
            required += std::min<qsizetype>(2, jobs);
        }
    }
    if (required > (d.budget->limit - d.budget->used) / TileBytes) {
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU projection tile budget exhausted")};
    }
    std::vector<CompositeJob> jobs;
    for (auto tile = data->inputs.cbegin(); tile != data->inputs.cend(); ++tile) {
        if (data->tiles.contains(tile.key())) continue;
        const auto firstJob = firstComposition(tile.value());
        const auto jobCount = tile.value().size() - firstJob;
        const auto a = d.allocateTile();
        const auto b = jobCount > 1 ? d.allocateTile() : std::shared_ptr<Tile>{};
        if (!a || (jobCount > 1 && !b)) {
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection allocation failed")};
        }
        auto input = firstJob ? tile.value().first().pixels : std::shared_ptr<Tile>{};
        for (qsizetype i = firstJob; i < tile.value().size(); ++i) {
            CompositeJob job;
            job.base = input;
            job.layer = tile.value()[i].pixels;
            job.output = (i - firstJob) % 2 ? b : a;
            job.values.opacity = tile.value()[i].opacity;
            job.values.normalizeBase = !background && !data->directLayerProjection && firstJob == 1 && i == firstJob;
            job.output->mayContainHiddenRgb = job.base && job.base->mayContainHiddenRgb && !job.values.normalizeBase;
            job.parameters = own(d.rhi, d.rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(CompositeParameters)));
            if (!job.parameters->create()) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection parameters failed")};
            job.bindings = own(d.rhi, d.rhi.newShaderResourceBindings());
            job.bindings->setBindings({
                QRhiShaderResourceBinding::bufferLoad(0, QRhiShaderResourceBinding::ComputeStage, job.base ? job.base->pixels.get() : d.zero.get()),
                QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, job.layer->pixels.get()),
                QRhiShaderResourceBinding::bufferStore(2, QRhiShaderResourceBinding::ComputeStage, job.output->pixels.get()),
                QRhiShaderResourceBinding::uniformBuffer(3, QRhiShaderResourceBinding::ComputeStage, job.parameters.get())
            });
            if (!job.bindings->create()) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection binding failed")};
            input = job.output;
            jobs.push_back(std::move(job));
        }
        data->tiles[tile.key()] = input;
    }
    if (!jobs.empty()) {
        auto *updates = d.rhi.nextResourceUpdateBatch();
        for (const auto &job : jobs) updates->uploadStaticBuffer(job.parameters.get(), &job.values);
        commands.resourceUpdate(updates);
    }
    for (const auto &job : jobs) {
        commands.beginComputePass();
        commands.setComputePipeline(d.projectionPipeline.get());
        commands.setShaderResources(job.bindings.get());
        commands.dispatch(TileSize / 8, TileSize / 8, 1);
        commands.endComputePass();
    }
    return {RenderOutcome::Recorded, ProjectedSnapshot(new ProjectedVersion(data)), {}};
}
}
