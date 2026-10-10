/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <set>
#include <tuple>

using namespace KisGpuTileStorage;

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
    const quint64 resident = d->state->residentBytes.load();
    const quint64 available = resident <= d->budget ? d->budget - resident : 0;
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
    const quint64 clearStride = align(capacity * sizeof(TileParameters));
    quint64 parameterBytes = allocationCount * clearStride;
    const quint64 commandBytes = count * sizeof(TileCommand);
    struct Group {
        qsizetype layer;
        quint64 destinationAllocation, parameterOffset = 0;
        std::shared_ptr<Allocation> source, mask;
        std::vector<CompositeParameters> parameters;
    };
    using GroupKey = std::tuple<qsizetype, quint64, quintptr, quintptr>;
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
        const TileParameters clear{quint32(index), 1, {0, 0}};
        std::memcpy(packedParameters.data() + index / capacity * clearStride + index % capacity * sizeof(clear),
                    &clear, sizeof(clear));
        for (qsizetype i = 0; i < layers.size(); ++i) {
            const auto &layer = layers[i];
            if (!layer.opacity) continue;
            const auto source = layer.pixels.d->tiles.find(coordinate);
            if (source == layer.pixels.d->tiles.end()) continue;
            const VersionData::Tile *mask = nullptr;
            if (layer.mask.d) {
                const auto found = layer.mask.d->tiles.find(coordinate);
                if (found == layer.mask.d->tiles.end()) continue;
                mask = &found->second;
            }
            auto &group = groups[{i, index / capacity, quintptr(source->second.allocation->buffer),
                mask ? quintptr(mask->allocation->buffer) : 0}];
            group.layer = i;
            group.destinationAllocation = index / capacity;
            group.source = source->second.allocation;
            group.mask = mask ? mask->allocation : group.source;
            group.parameters.push_back({quint32(source->second.offset / TileBytes), quint32(index % capacity),
                x0, y0, x1, y1,
                quint32(layer.operation == CompositeOp::Erase ? UpdateKind::Erase : UpdateKind::Over),
                layer.opacity, 255, mask ? quint32(mask->offset / TileBytes) : std::numeric_limits<quint32>::max()});
        }
        ++index;
    }
    for (auto &entry : groups) {
        entry.second.parameterOffset = align(parameterBytes);
        parameterBytes = entry.second.parameterOffset + entry.second.parameters.size() * sizeof(CompositeParameters);
    }
    const quint64 remaining = available - count * TileBytes;
    if (commandBytes > d->limits.maxStorageBufferBindingSize || commandBytes > d->limits.maxBufferSize
        || parameterBytes > d->limits.maxBufferSize || parameterBytes > remaining || commandBytes > remaining - parameterBytes) {
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
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
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
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(wgpuCommandEncoderBeginComputePass(encoder.value, nullptr));
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
        const qsizetype layer = entry->second.layer;
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(wgpuCommandEncoderBeginComputePass(encoder.value, nullptr));
        wgpuComputePassEncoderSetPipeline(pass.value, d->compositePipeline);
        do {
            const auto &group = entry->second;
            const quint64 bytes = group.parameters.size() * sizeof(CompositeParameters);
            std::memcpy(packedParameters.data() + group.parameterOffset, group.parameters.data(), bytes);
            const auto &allocation = allocations[group.destinationAllocation];
            WGPUBindGroupEntry entries[4]{};
            entries[0].binding = 0; entries[0].buffer = allocation->buffer; entries[0].size = allocation->bytes;
            entries[1].binding = 3; entries[1].buffer = group.source->buffer; entries[1].size = group.source->bytes;
            entries[2].binding = 4; entries[2].buffer = parameters->buffer; entries[2].offset = group.parameterOffset; entries[2].size = bytes;
            entries[3].binding = 5; entries[3].buffer = group.mask->buffer; entries[3].size = group.mask->bytes;
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->compositeLayout; descriptor.entryCount = 4; descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> binding(wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, binding.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, quint32(group.parameters.size()));
            ++entry;
        } while (entry != groups.end() && entry->second.layer == layer);
        wgpuComputePassEncoderEnd(pass.value);
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commandBuffer(wgpuCommandEncoderFinish(encoder.value, nullptr));
    QVector<Version> inputs;
    for (const auto &layer : layers) {
        inputs.push_back(layer.pixels);
        if (layer.mask.d) inputs.push_back(layer.mask);
    }
    result.version.d = std::move(data);
    d->submit({result.completion.d, previous, result.version, parameters, commands, inputs}, commandBuffer.value,
              packedParameters, clearCommands, copiedBytes, allocationCount + groups.size());
    errors.submitted = true;
    return result;
}
