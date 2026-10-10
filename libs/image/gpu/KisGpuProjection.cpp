/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <QFile>
#include <algorithm>
#include <cstring>
#include <limits>
#include <set>
#include <tuple>

using namespace KisGpuTileStorage;

namespace {
constexpr quint32 LayersPerPass = 3;
constexpr quint32 MissingTile = std::numeric_limits<quint32>::max();
struct LayerParameters {
    quint32 sourceTile = MissingTile, maskTile = MissingTile, operation = 0, opacity = 0;
};
struct ProjectionParameters {
    quint32 left, top, right, bottom, destinationTile, reserved;
    std::array<LayerParameters, LayersPerPass> layers;
};
static_assert(sizeof(ProjectionParameters) == 72);
}

KisGpuTileStore::Private::ProjectionPipelines::~ProjectionPipelines()
{
    if (pipeline) wgpuComputePipelineRelease(pipeline);
    if (layout) wgpuBindGroupLayoutRelease(layout);
}

std::unique_ptr<KisGpuTileStore::Private::ProjectionPipelines>
KisGpuTileStore::Private::createProjectionPipelines(const QByteArray &pixelOperators)
{
    auto result = std::make_unique<ProjectionPipelines>();
    WGPUBindGroupLayoutEntry entries[8]{};
    for (quint32 i = 0; i < 8; ++i) {
        entries[i].binding = i;
        entries[i].visibility = WGPUShaderStage_Compute;
        entries[i].buffer.type = i ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;
        entries[i].buffer.minBindingSize = i == 1 ? sizeof(ProjectionParameters) : TileBytes;
    }
    WGPUBindGroupLayoutDescriptor groupDescriptor{};
    groupDescriptor.entryCount = 8;
    groupDescriptor.entries = entries;
    result->layout = wgpuDeviceCreateBindGroupLayout(state->device, &groupDescriptor);
    if (!result->layout) throw std::runtime_error("Cannot create GPU projection layout");
    WGPUPipelineLayoutDescriptor layoutDescriptor{};
    layoutDescriptor.bindGroupLayoutCount = 1;
    layoutDescriptor.bindGroupLayouts = &result->layout;
    Handle<WGPUPipelineLayout, wgpuPipelineLayoutRelease> layout(wgpuDeviceCreatePipelineLayout(state->device, &layoutDescriptor));
    QFile shaderFile(QStringLiteral(":/librepaint/gpu/KisGpuProjection.wgsl"));
    if (!shaderFile.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot load GPU projection shader resource");
    const QByteArray code = pixelOperators + shaderFile.readAll();
    WGPUShaderSourceWGSL source{};
    source.chain.sType = WGPUSType_ShaderSourceWGSL;
    source.code = {code.constData(), size_t(code.size())};
    WGPUShaderModuleDescriptor shaderDescriptor{};
    shaderDescriptor.nextInChain = &source.chain;
    Handle<WGPUShaderModule, wgpuShaderModuleRelease> shader(wgpuDeviceCreateShaderModule(state->device, &shaderDescriptor));
    WGPUComputePipelineDescriptor descriptor{};
    descriptor.layout = layout.value;
    descriptor.compute.module = shader.value;
    descriptor.compute.entryPoint = {"project", WGPU_STRLEN};
    result->pipeline = wgpuDeviceCreateComputePipeline(state->device, &descriptor);
    if (!result->pipeline) throw std::runtime_error("Cannot create GPU projection pipeline");
    return result;
}

KisGpuTileStore::Edit KisGpuTileStore::project(const Version &previous, const QVector<Layer> &layers, QRect damage)
{
    Edit result;
    if (!previous.d || previous.d->owner != d->state) return {Error::InvalidVersion, {}, {}};
    for (const auto &layer : layers) {
        if (!layer.pixels.d || layer.pixels.d->owner != d->state
            || (layer.mask.d && layer.mask.d->owner != d->state)) return {Error::InvalidVersion, {}, {}};
    }
    if (!deviceAvailable()) return {Error::DeviceLost, {}, {}};
    const auto unchanged = [&] {
        result.version = previous;
        result.completion.d = previous.d->completion;
        return result;
    };
    if (damage.isEmpty()) return unchanged();
    const qint64 left = damage.x(), top = damage.y();
    const qint64 right = left + damage.width(), bottom = top + damage.height();
    const auto intersects = [&](Coordinate coordinate) {
        const qint64 x = qint64(coordinate.first) * 64, y = qint64(coordinate.second) * 64;
        return x < right && x + 64 > left && y < bottom && y + 64 > top;
    };
    const quint64 available = d->availableForOperation();
    std::set<Coordinate> affected;
    const auto include = [&](Coordinate coordinate) {
        affected.insert(coordinate);
        return affected.size() <= available / TileBytes;
    };
    for (const auto &tile : previous.d->tiles) {
        if (intersects(tile.first) && !include(tile.first)) return {Error::BudgetExceeded, {}, {}};
    }
    for (const auto &layer : layers) {
        if (!layer.opacity) continue;
        for (const auto &tile : layer.pixels.d->tiles) {
            if (!intersects(tile.first) || (layer.mask.d && !layer.mask.d->tiles.count(tile.first))) continue;
            if (!include(tile.first)) return {Error::BudgetExceeded, {}, {}};
        }
    }
    if (affected.empty()) return unchanged();
    if (d->pending.size() >= d->maximumPending) return {Error::QueueFull, {}, {}};
    const quint64 capacity = d->tilesPerAllocation;
    const quint64 count = affected.size(), allocationCount = (count + capacity - 1) / capacity;
    const quint64 alignment = d->limits.minStorageBufferOffsetAlignment;
    const auto align = [alignment](quint64 offset) { return (offset + alignment - 1) / alignment * alignment; };
    const quint64 clearStride = align(std::min(capacity, count) * sizeof(TileParameters));
    quint64 parameterBytes = allocationCount * clearStride;
    const quint64 commandBytes = count * sizeof(TileCommand);
    struct Group {
        qsizetype batch;
        quint64 destinationAllocation, parameterOffset = 0;
        std::array<std::shared_ptr<Allocation>, LayersPerPass> sources, masks;
        std::vector<ProjectionParameters> parameters;
    };
    using GroupKey = std::tuple<qsizetype, quint64, std::array<quintptr, LayersPerPass * 2>>;
    std::map<GroupKey, Group> groups;
    std::vector<TileCommand> clearCommands;
    std::vector<char> packedParameters(size_t(parameterBytes), 0);
    clearCommands.reserve(count);
    quint64 index = 0;
    for (const auto &coordinate : affected) {
        const qint64 x = qint64(coordinate.first) * 64, y = qint64(coordinate.second) * 64;
        const quint32 x0 = quint32(std::max(left, x) - x), y0 = quint32(std::max(top, y) - y);
        const quint32 x1 = quint32(std::min(right, x + 64) - x), y1 = quint32(std::min(bottom, y + 64) - y);
        clearCommands.push_back({x0, y0, x1, y1, 0, 0, 255, 255});
        const TileParameters clear{quint32(index), 1, 0, 0, 0, 0, 0, 0};
        std::memcpy(packedParameters.data() + index / capacity * clearStride + index % capacity * sizeof(clear),
                    &clear, sizeof(clear));
        const qsizetype batches = layers.size() / LayersPerPass + (layers.size() % LayersPerPass != 0);
        for (qsizetype batch = 0; batch < batches; ++batch) {
            const quint32 layerCount = quint32(std::min<qsizetype>(LayersPerPass, layers.size() - batch * LayersPerPass));
            ProjectionParameters tile{x0, y0, x1, y1, quint32(index % capacity), 0, {}};
            std::array<std::shared_ptr<Allocation>, LayersPerPass> sources{}, masks{};
            std::array<quintptr, LayersPerPass * 2> buffers{};
            std::shared_ptr<Allocation> fallback;
            for (quint32 slot = 0; slot < layerCount; ++slot) {
                const auto &layer = layers[batch * LayersPerPass + slot];
                if (!layer.opacity) continue;
                const auto source = layer.pixels.d->tiles.find(coordinate);
                if (source == layer.pixels.d->tiles.end()) continue;
                const VersionData::Tile *mask = nullptr;
                if (layer.mask.d) {
                    const auto found = layer.mask.d->tiles.find(coordinate);
                    if (found == layer.mask.d->tiles.end()) continue;
                    mask = &found->second;
                }
                sources[slot] = source->second.allocation;
                masks[slot] = mask ? mask->allocation : sources[slot];
                buffers[slot] = quintptr(sources[slot]->buffer);
                buffers[slot + LayersPerPass] = quintptr(masks[slot]->buffer);
                fallback = sources[slot];
                tile.layers[slot] = {quint32(source->second.offset / TileBytes),
                    mask ? quint32(mask->offset / TileBytes) : MissingTile,
                    quint32(layer.operation == CompositeOp::Erase ? UpdateKind::Erase : UpdateKind::Over), layer.opacity};
            }
            if (!fallback) continue;
            auto &group = groups[{batch, index / capacity, buffers}];
            group.batch = batch;
            group.destinationAllocation = index / capacity;
            for (quint32 slot = 0; slot < LayersPerPass; ++slot) {
                group.sources[slot] = sources[slot] ? sources[slot] : fallback;
                group.masks[slot] = masks[slot] ? masks[slot] : fallback;
            }
            group.parameters.push_back(tile);
        }
        ++index;
    }
    for (auto &entry : groups) {
        entry.second.parameterOffset = align(parameterBytes);
        parameterBytes = entry.second.parameterOffset + entry.second.parameters.size() * sizeof(ProjectionParameters);
    }
    quint32 passes = 1;
    qsizetype lastBatch = -1;
    for (const auto &entry : groups) {
        if (entry.second.batch != lastBatch) { ++passes; lastBatch = entry.second.batch; }
    }
    // Bound timestamped recording before native command-buffer allocation.
    if (d->timestamps && passes > NativeDevice::MaximumTimedPasses) return {Error::BudgetExceeded, {}, {}};
    if (d->timestamps && !d->state->nativeOwner->queriesAvailable(passes * 2)) return {Error::QueueFull, {}, {}};
    const quint64 timingExtra = d->timestamps ? quint64(passes - 1) * 32 : 0;
    const quint64 remaining = available - count * TileBytes;
    if (commandBytes > d->limits.maxStorageBufferBindingSize || commandBytes > d->limits.maxBufferSize
        || parameterBytes > d->limits.maxBufferSize || parameterBytes > remaining || commandBytes > remaining - parameterBytes
        || timingExtra > remaining - parameterBytes - commandBytes) {
        return {Error::BudgetExceeded, {}, {}};
    }
    packedParameters.resize(size_t(parameterBytes), 0);
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto data = std::make_shared<VersionData>(*previous.d);
    data->completion = result.completion.d;
    auto parameters = std::make_shared<Allocation>(d->state, parameterBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto commands = std::make_shared<Allocation>(d->state, commandBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    std::vector<std::shared_ptr<Allocation>> allocations;
    Private::Recording encoder(d->state, d->timestamps ? passes : 0);
    quint64 copiedBytes = 0;
    index = 0;
    for (const auto &coordinate : affected) {
        if (index % capacity == 0) {
            allocations.push_back(std::make_shared<Allocation>(d->state, std::min(capacity, count - index) * TileBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst));
        }
        const auto &allocation = allocations.back();
        const auto &clear = clearCommands[index];
        const quint64 offset = index % capacity * TileBytes;
        const auto old = previous.d->tiles.find(coordinate);
        if (old != previous.d->tiles.end() && !(clear.left == 0 && clear.top == 0 && clear.right == 64 && clear.bottom == 64)) {
            wgpuCommandEncoderCopyBufferToBuffer(encoder.value, old->second.allocation->buffer, old->second.offset,
                                                 allocation->buffer, offset, TileBytes);
            copiedBytes += TileBytes;
        }
        data->tiles[coordinate] = {allocation, offset};
        ++index;
    }
    {
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(encoder.beginComputePass());
        wgpuComputePassEncoderSetPipeline(pass.value, d->pipeline);
        for (quint64 i = 0; i < allocationCount; ++i) {
            WGPUBindGroupEntry entries[3]{};
            entries[0].binding = 0; entries[0].buffer = allocations[i]->buffer; entries[0].size = allocations[i]->bytes;
            entries[1].binding = 1; entries[1].buffer = parameters->buffer; entries[1].offset = i * clearStride;
            entries[1].size = std::min(capacity, count - i * capacity) * sizeof(TileParameters);
            entries[2].binding = 2; entries[2].buffer = commands->buffer; entries[2].size = commandBytes;
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->layout; descriptor.entryCount = 3; descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> group(wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, group.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, quint32(allocations[i]->bytes / TileBytes));
        }
        wgpuComputePassEncoderEnd(pass.value);
    }
    auto entry = groups.begin();
    while (entry != groups.end()) {
        const qsizetype batch = entry->second.batch;
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(encoder.beginComputePass());
        wgpuComputePassEncoderSetPipeline(pass.value, d->projectionPipelines->pipeline);
        do {
            const auto &group = entry->second;
            const quint64 bytes = group.parameters.size() * sizeof(ProjectionParameters);
            std::memcpy(packedParameters.data() + group.parameterOffset, group.parameters.data(), bytes);
            const auto &allocation = allocations[group.destinationAllocation];
            WGPUBindGroupEntry entries[8]{};
            entries[0].binding = 0; entries[0].buffer = allocation->buffer; entries[0].size = allocation->bytes;
            entries[1].binding = 1; entries[1].buffer = parameters->buffer; entries[1].offset = group.parameterOffset; entries[1].size = bytes;
            for (quint32 slot = 0; slot < LayersPerPass; ++slot) {
                entries[2 + slot].binding = 2 + slot;
                entries[2 + slot].buffer = group.sources[slot]->buffer; entries[2 + slot].size = group.sources[slot]->bytes;
                entries[5 + slot].binding = 5 + slot;
                entries[5 + slot].buffer = group.masks[slot]->buffer; entries[5 + slot].size = group.masks[slot]->bytes;
            }
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->projectionPipelines->layout; descriptor.entryCount = 8; descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> binding(wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, binding.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, quint32(group.parameters.size()));
            ++entry;
        } while (entry != groups.end() && entry->second.batch == batch);
        wgpuComputePassEncoderEnd(pass.value);
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commandBuffer(encoder.finish());
    QVector<Version> inputs;
    for (const auto &layer : layers) {
        inputs.push_back(layer.pixels);
        if (layer.mask.d) inputs.push_back(layer.mask);
    }
    result.version.d = std::move(data);
    d->submit({result.completion.d, previous, result.version, parameters, commands, inputs}, commandBuffer.value,
              packedParameters, clearCommands, copiedBytes, allocationCount + groups.size(), {}, encoder.timing);
    errors.submitted = true;
    return result;
}
