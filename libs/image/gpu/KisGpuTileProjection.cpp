/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileStorage_p.h"
#include <QFile>
#include <QThread>
#include <QSet>
#include <vector>

namespace Krita::Image::Gpu {
using namespace Detail;
namespace {
struct CompositeJob
{
    std::shared_ptr<Tile> base, layer, output;
    Resource<QRhiShaderResourceBindings> bindings;
};
}

void TileRenderer::Private::initializeProjection()
{
    projectionLayout = own(rhi, rhi.newShaderResourceBindings());
    projectionPipeline = own(rhi, rhi.newComputePipeline());
    projectionLayout->setBindings({
        QRhiShaderResourceBinding::bufferLoad(0, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, zero.get()),
        QRhiShaderResourceBinding::bufferStore(2, QRhiShaderResourceBinding::ComputeStage, zero.get())
    });
    QFile shader(QStringLiteral(":/librepaint/gpu/shaders/over.comp.qsb"));
    if (!projectionLayout->create() || !shader.open(QIODevice::ReadOnly)) {
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
    data->region = region.intersected(QRect(QPoint(0, 0), document->document()->size));
    QSet<quint64> coordinates;
    bool implicit = false;
    for (const auto &layer : document->document()->layers) {
        if (!layer.visible) continue;
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
    for (const auto &layer : document->document()->layers) {
        if (!layer.visible) continue;
        const auto fallback = document->m_data->defaults.value(layer.id);
        const auto &tiles = document->m_data->layers[layer.id];
        for (const auto coordinate : coordinates) {
            const auto pixels = tiles.value(coordinate, fallback);
            if (pixels) data->inputs[coordinate].append(pixels);
        }
    }
    qsizetype required = 0;
    for (auto tile = data->inputs.cbegin(); tile != data->inputs.cend(); ++tile) {
        if (base && base->m_data->inputs.value(tile.key()) == tile.value()) {
            data->tiles[tile.key()] = base->m_data->tiles.value(tile.key());
        } else if (tile.value().size() == 1) {
            data->tiles[tile.key()] = tile.value().first();
        } else {
            required += tile.value().size() > 2 ? 2 : 1;
        }
    }
    if (required > (d.budget->limit - d.budget->used) / TileBytes) {
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU projection tile budget exhausted")};
    }
    std::vector<CompositeJob> jobs;
    for (auto tile = data->inputs.cbegin(); tile != data->inputs.cend(); ++tile) {
        if (data->tiles.contains(tile.key())) continue;
        const auto a = d.allocateTile();
        const auto b = tile.value().size() > 2 ? d.allocateTile() : std::shared_ptr<Tile>{};
        if (!a || (tile.value().size() > 2 && !b)) {
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection allocation failed")};
        }
        auto input = tile.value().first();
        for (qsizetype i = 1; i < tile.value().size(); ++i) {
            CompositeJob job;
            job.base = input;
            job.layer = tile.value()[i];
            job.output = i % 2 ? a : b;
            job.bindings = own(d.rhi, d.rhi.newShaderResourceBindings());
            job.bindings->setBindings({
                QRhiShaderResourceBinding::bufferLoad(0, QRhiShaderResourceBinding::ComputeStage, job.base->pixels.get()),
                QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, job.layer->pixels.get()),
                QRhiShaderResourceBinding::bufferStore(2, QRhiShaderResourceBinding::ComputeStage, job.output->pixels.get())
            });
            if (!job.bindings->create()) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection binding failed")};
            input = job.output;
            jobs.push_back(std::move(job));
        }
        data->tiles[tile.key()] = input;
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
