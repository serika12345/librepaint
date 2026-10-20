/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileStorage_p.h"
#include "KisGpuPaintJob_p.h"
#include <QFile>
#include <QThread>
#include <QSet>
#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
struct CompositeParameters { quint32 opacityBits = 0x3f800000; quint32 normalizeBase = 0; quint32 channelMask = 15; quint32 padding = 0; };
struct CompositeJob
{
    std::shared_ptr<Tile> base, output;
    std::shared_ptr<Tile> layers[2];
    CompositeParameters values[2]{};
    NativeBuffer parameters;
    NativeBindings bindings;
};
}

void TileRenderer::Private::initializeProjection()
{
    projectionPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"));
    if (!projectionPipeline) return;
    projectionLayout.reset(wgpuComputePipelineGetBindGroupLayout(projectionPipeline.get(), 0));
    if (!projectionLayout) error = QStringLiteral("GPU projection bindings could not be created");
}

ProjectedVersion::ProjectedVersion(std::shared_ptr<Data> data) : m_data(std::move(data)) {}
DevicePtr ProjectedVersion::device() const { return m_data->source->device(); }
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

Detail::NativeBuffer TileRenderer::Private::recordTransparency(WGPUCommandEncoder commands,
                                                              const ProjectedSnapshot &projection, QRect region)
{
    const auto tiles = projection->tiles();
    const qint64 coordinates = qint64(region.right() / TileSize - region.left() / TileSize + 1) *
                               (region.bottom() / TileSize - region.top() / TileSize + 1);
    quint32 initial = tiles.size() < coordinates ? 1 : 0;
    auto output = createBuffer(device->nativeDevice(), sizeof(initial),
                               WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    if (!output) return {};
    wgpuQueueWriteBuffer(device->queue(), output.get(), 0, &initial, sizeof(initial));
    if (initial) return output;
    if (!transparencyPipeline) {
        transparencyPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/transparency.wgsl"));
        if (!transparencyPipeline) return {};
        transparencyLayout.reset(wgpuComputePipelineGetBindGroupLayout(transparencyPipeline.get(), 0));
    }
    if (!transparencyLayout) return {};
    for (const auto &tile : tiles) {
        const auto local = region.intersected(QRect(tile.coordinate * TileSize, QSize(TileSize, TileSize)))
            .translated(-tile.coordinate * TileSize);
        const quint32 values[] = {quint32(local.left()), quint32(local.top()),
                                  quint32(local.right() + 1), quint32(local.bottom() + 1)};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!parameters) return {};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        const WGPUBuffer buffers[] = {tile.pixels, output.get(), parameters.get()};
        WGPUBindGroupEntry entries[3]{};
        for (quint32 i = 0; i < 3; ++i) {
            entries[i].binding = i;
            entries[i].buffer = buffers[i];
            entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = transparencyLayout.get();
        descriptor.entryCount = 3;
        descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty()) return {};
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, transparencyPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
    }
    return output;
}

Detail::NativeBuffer TileRenderer::Private::recordBounds(WGPUCommandEncoder commands,
    const QList<TileView> &tiles, QRect region, const std::array<quint8, 4> &defaultPixel, bool includeTransparentColors)
{
    const qint32 initial[] = {std::numeric_limits<qint32>::max(), std::numeric_limits<qint32>::max(),
                              std::numeric_limits<qint32>::min(), std::numeric_limits<qint32>::min()};
    auto output = createBuffer(device->nativeDevice(), sizeof(initial),
                               WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    if (!output) return {};
    wgpuQueueWriteBuffer(device->queue(), output.get(), 0, initial, sizeof(initial));
    if (tiles.isEmpty()) return output;
    if (!boundsPipeline) {
        boundsPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/bounds.wgsl"));
        if (!boundsPipeline) return {};
        boundsLayout.reset(wgpuComputePipelineGetBindGroupLayout(boundsPipeline.get(), 0));
    }
    if (!boundsLayout) return {};
    for (const auto &tile : tiles) {
        const auto origin = tile.coordinate * TileSize;
        const auto local = region.intersected(QRect(origin, QSize(TileSize, TileSize))).translated(-origin);
        if (local.isEmpty()) continue;
        const quint32 values[] = {quint32(origin.x()), quint32(origin.y()), packedPixel(defaultPixel),
                                  defaultPixel[3] || includeTransparentColors ? 1u : 0u, quint32(local.left()), quint32(local.top()),
                                  quint32(local.right() + 1), quint32(local.bottom() + 1)};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!parameters) return {};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        const WGPUBuffer buffers[] = {tile.pixels, output.get(), parameters.get()};
        WGPUBindGroupEntry entries[3]{};
        for (quint32 i = 0; i < 3; ++i) {
            entries[i].binding = i;
            entries[i].buffer = buffers[i];
            entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = boundsLayout.get();
        descriptor.entryCount = 3;
        descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty()) return {};
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, boundsPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 16, TileSize / 16, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
    }
    return output;
}

ProjectionOutcome TileRenderer::compose(WGPUCommandEncoder commands, const RenderedSnapshot &document,
                                       QRect region, const ProjectedSnapshot &base)
{ return composeImpl(commands, document, region, base, 0); }

ProjectionOutcome TileRenderer::composeImpl(WGPUCommandEncoder commands, const RenderedSnapshot &document,
                                           QRect region, const ProjectedSnapshot &base, LayerId group)
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || !d.device->ownsCommands(commands) || !document ||
        document->m_data->owner != d.budget || region.width() < 0 || region.height() < 0 ||
        (base && (base->m_data->source->m_data->owner != d.budget ||
                  base->document()->documentId() != document->document()->documentId()))) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Projection requires the owning GPU and document")};
    }
    if (!d.error.isEmpty() || d.device->isLost()) return {RenderOutcome::DeviceFailure, {}, d.error};
    auto data = std::make_shared<ProjectedVersion::Data>();
    data->source = document;
    QUuid parent;
    if (group) {
        for (const auto &layer : document->document()->layers) if (layer.id == group) parent = layer.uuid;
        if (parent.isNull()) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU group is unavailable")};
    }
    QList<const Layer *> children;
    for (const auto &layer : document->document()->layers) if (layer.parentUuid == parent) children.append(&layer);
    data->directLayerProjection = group ? children.size() == 1 && children.first()->visible && children.first()->opacity == 255 && children.first()->projectionChannelMask == 15 && !children.first()->passThrough :
        document->document()->directProjectionLayer() != 0;
    data->region = group ? region : region.intersected(QRect(QPoint(0, 0), document->document()->size));
    if (document->m_data->region && !data->region.isEmpty() && !document->m_data->region->contains(data->region)) {
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Projection extends outside the generated GPU region")};
    }
    const auto inputs = document->document()->projectionInputs(group);
    QSet<quint64> coordinates;
    // A single fully opaque paint layer supplies the legacy transparent-root projection,
    // including the RGB components of pixels whose alpha is zero.
    const auto background = group || data->directLayerProjection ? std::shared_ptr<Tile>{} : document->m_data->background;
    bool implicit = bool(background);
    for (const auto &layer : inputs) {
        if (!document->m_data->selected.contains(layer.layer)) {
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Projection requires every visible GPU layer")};
        }
        implicit = implicit || bool(document->m_data->defaults.value(layer.layer));
        const auto &tiles = document->m_data->layers[layer.layer];
        for (auto tile = tiles.cbegin(); tile != tiles.cend(); ++tile) {
            if (data->region.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) {
                coordinates.insert(tile.key());
            }
        }
    }
    if (!group && implicit && !data->region.isEmpty()) {
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
    for (const auto &layer : inputs) {
        const auto fallback = document->m_data->defaults.value(layer.layer);
        const auto &tiles = document->m_data->layers[layer.layer];
        for (const auto coordinate : coordinates) {
            const auto pixels = tiles.value(coordinate, fallback);
            if (pixels) data->inputs[coordinate].append({pixels, layer.opacity, layer.channelMask});
        }
    }
    const auto firstComposition = [&](const auto &inputs) {
        return inputs.first().opacity == 255 && inputs.first().channelMask == 15 && (inputs.size() > 1 || background || data->directLayerProjection ||
                                                !inputs.first().pixels->mayContainHiddenRgb) ? 1 : 0;
    };
    const bool cachedBase = !group && base && d.backdropVersion.toStrongRef() == base &&
        base->m_data->directLayerProjection == data->directLayerProjection;
    const Layer *topLayer = nullptr;
    if (!group) for (const auto child : children)
        if (child->visible && child->opacity) topLayer = child;
    const bool keepBackdrop = !group && !document->document()->isCommitted() && topLayer &&
        topLayer->id == document->document()->activeLayer && topLayer->kind == LayerKind::Paint;
    Tiles reusedBackdrops, nextBackdrops;
    qsizetype required = 0;
    bool needsPair = false;
    for (auto tile = data->inputs.cbegin(); tile != data->inputs.cend(); ++tile) {
        if (base && base->m_data->directLayerProjection == data->directLayerProjection &&
            base->m_data->inputs.value(tile.key()) == tile.value()) {
            data->tiles[tile.key()] = base->m_data->tiles.value(tile.key());
        } else if (tile.value().size() == 1 && firstComposition(tile.value()) == 1) {
            data->tiles[tile.key()] = tile.value().first().pixels;
        } else {
            const auto backdrop = cachedBase ? d.projectionBackdrops.value(tile.key()) : std::shared_ptr<Tile>{};
            const auto previous = base ? base->m_data->inputs.value(tile.key()) : QList<ProjectionInput>{};
            if (backdrop && tile.value().size() > 1 && previous.size() == tile.value().size() &&
                std::equal(tile.value().cbegin(), tile.value().cend() - 1, previous.cbegin())) {
                reusedBackdrops[tile.key()] = backdrop;
                ++required;
            } else {
                const auto jobs = tile.value().size() - firstComposition(tile.value());
                const auto passes = keepBackdrop ? jobs / 2 + 1 : (jobs + 1) / 2;
                needsPair |= jobs >= (keepBackdrop ? 3 : 2);
                required += std::min<qsizetype>(2, passes);
            }
        }
    }
    if (needsPair && !d.pairedProjectionPipeline) {
        d.pairedProjectionPipeline = d.createPipeline(QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"), "mainPair");
        if (!d.pairedProjectionPipeline) return {RenderOutcome::DeviceFailure, {}, d.error};
        d.pairedProjectionLayout.reset(wgpuComputePipelineGetBindGroupLayout(d.pairedProjectionPipeline.get(), 0));
        if (!d.pairedProjectionLayout) d.error = QStringLiteral("GPU paired projection bindings could not be created");
    }
    if (needsPair && !d.pairedProjectionLayout) return {RenderOutcome::DeviceFailure, {}, d.error};
    if (!d.ensureTileCapacity(required)) {
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU projection tile budget exhausted")};
    }
    std::vector<CompositeJob> jobs;
    for (auto tile = data->inputs.cbegin(); tile != data->inputs.cend(); ++tile) {
        if (data->tiles.contains(tile.key())) continue;
        const auto firstJob = firstComposition(tile.value());
        const auto backdrop = reusedBackdrops.value(tile.key());
        const auto start = backdrop ? tile.value().size() - 1 : firstJob;
        const auto remaining = tile.value().size() - start;
        const auto jobCount = keepBackdrop ? remaining / 2 + 1 : (remaining + 1) / 2;
        const auto a = d.allocateTile();
        const auto b = jobCount > 1 ? d.allocateTile() : std::shared_ptr<Tile>{};
        if (!a || (jobCount > 1 && !b)) {
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection allocation failed")};
        }
        auto input = backdrop ? backdrop : firstJob ? tile.value().first().pixels : std::shared_ptr<Tile>{};
        const auto pairEnd = tile.value().size() - (keepBackdrop ? 1 : 0);
        for (qsizetype i = start, passIndex = 0; i < tile.value().size(); ++passIndex) {
            const int count = i + 2 <= pairEnd ? 2 : 1;
            const bool paired = count == 2;
            if (keepBackdrop && tile.value().size() > 1 && i == tile.value().size() - 1)
                nextBackdrops[tile.key()] = input;
            CompositeJob job;
            job.base = input;
            job.output = passIndex % 2 ? b : a;
            bool hiddenRgb = job.base && job.base->mayContainHiddenRgb;
            for (int layer = 0; layer < count; ++layer) {
                const auto &source = tile.value()[i + layer];
                job.layers[layer] = source.pixels;
                const float opacity = float(source.opacity) / 255.0f;
                std::memcpy(&job.values[layer].opacityBits, &opacity, sizeof(opacity));
                job.values[layer].channelMask = source.channelMask;
                job.values[layer].normalizeBase = !background && !data->directLayerProjection && firstJob == 1 && i + layer == firstJob;
                hiddenRgb = !(source.channelMask & 8u) || source.opacity < 128 ||
                    (hiddenRgb && !job.values[layer].normalizeBase);
            }
            job.output->mayContainHiddenRgb = hiddenRgb;
            job.parameters = createBuffer(d.device->nativeDevice(), sizeof(job.values),
                                           WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
            if (!job.parameters) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection parameters failed")};
            WGPUBuffer buffers[] = {job.base ? job.base->pixels.get() : d.zero.get(), job.layers[0]->pixels.get(),
                                    job.output->pixels.get(), paired ? job.layers[1]->pixels.get() : job.parameters.get(), job.parameters.get()};
            WGPUBindGroupEntry entries[5]{};
            const quint32 bindingCount = paired ? 5 : 4;
            for (quint32 index = 0; index < bindingCount; ++index) {
                entries[index].binding = paired && index >= 3 ? index + 1 : index;
                entries[index].buffer = buffers[index];
                entries[index].size = wgpuBufferGetSize(buffers[index]);
            }
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = paired ? d.pairedProjectionLayout.get() : d.projectionLayout.get();
            descriptor.entryCount = bindingCount;
            descriptor.entries = entries;
            job.bindings.reset(wgpuDeviceCreateBindGroup(d.device->nativeDevice(), &descriptor));
            if (!job.bindings || !d.device->diagnostic().isEmpty())
                return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU projection binding failed")};
            input = job.output;
            jobs.push_back(std::move(job));
            i += count;
        }
        data->tiles[tile.key()] = input;
    }
    for (const auto &job : jobs) {
        wgpuQueueWriteBuffer(d.device->queue(), job.parameters.get(), 0, &job.values, sizeof(job.values));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, job.layers[1] ? d.pairedProjectionPipeline.get() : d.projectionPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, job.bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
    }
    const ProjectedSnapshot result(new ProjectedVersion(data));
    if (!group) {
        if (keepBackdrop && cachedBase && d.backdropVersion.toStrongRef() == base) {
            for (auto tile = d.projectionBackdrops.cbegin(); tile != d.projectionBackdrops.cend(); ++tile)
                if (data->tiles.contains(tile.key()) && data->inputs.value(tile.key()) == base->m_data->inputs.value(tile.key()))
                    nextBackdrops[tile.key()] = tile.value();
        }
        d.projectionBackdrops = std::move(nextBackdrops);
        d.backdropVersion = keepBackdrop ? result : ProjectedSnapshot{};
    }
    return {RenderOutcome::Recorded, result, {}};
}
}
