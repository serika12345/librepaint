/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <tuple>

using namespace KisGpuTileStorage;

KisGpuTileStore::Edit KisGpuTileStore::composite(const Version &base, const Version &source, QRect rectangle,
                                               CompositeOp operation, quint8 opacity, quint8 coverage)
{
    return compositePixels(base, source, nullptr, rectangle, operation, opacity, coverage);
}

KisGpuTileStore::Edit KisGpuTileStore::compositeMasked(const Version &base, const Version &source,
    const Version &mask, QRect rectangle, CompositeOp operation, quint8 opacity)
{
    return compositePixels(base, source, &mask, rectangle, operation, opacity, 255);
}

KisGpuTileStore::Edit KisGpuTileStore::compositePixels(const Version &base, const Version &source,
    const Version *mask, QRect rectangle, CompositeOp operation, quint8 opacity, quint8 coverage)
{
    Edit result;
    if (!base.d || !source.d || base.d->owner != d->state || source.d->owner != d->state
        || (mask && (!mask->d || mask->d->owner != d->state))) {
        result.error = Error::InvalidVersion;
        return result;
    }
    if (!deviceAvailable()) {
        result.error = Error::DeviceLost;
        return result;
    }
    auto unchanged = [&] {
        result.version = base;
        result.completion.d = base.d->completion;
        return result;
    };
    if (rectangle.isEmpty() || opacity == 0 || coverage == 0) return unchanged();
    const quint64 resident = d->state->residentBytes.load();
    const quint64 available = resident <= d->budget ? d->budget - resident : 0;
    const quint64 capacity = d->tilesPerAllocation;
    const qint64 left = rectangle.x(), top = rectangle.y();
    const qint64 right = left + rectangle.width(), bottom = top + rectangle.height();
    struct Group {
        std::shared_ptr<Allocation> source, mask;
        quint64 destinationAllocation = 0;
        quint64 parameterOffset = 0;
        std::vector<CompositeParameters> parameters;
    };
    std::map<std::tuple<quintptr, quintptr, quint64>, Group> groups;
    std::vector<Coordinate> coordinates;
    for (const auto &entry : source.d->tiles) {
        const qint64 tileLeft = qint64(entry.first.first) * 64, tileTop = qint64(entry.first.second) * 64;
        const qint64 clippedLeft = std::max(left, tileLeft), clippedTop = std::max(top, tileTop);
        const qint64 clippedRight = std::min(right, tileLeft + 64), clippedBottom = std::min(bottom, tileTop + 64);
        if (clippedLeft >= clippedRight || clippedTop >= clippedBottom) continue;
        VersionData::Tile maskTile;
        if (mask) {
            const auto found = mask->d->tiles.find(entry.first);
            if (found == mask->d->tiles.end()) continue;
            maskTile = found->second;
        }
        if (coordinates.size() >= available / TileBytes) {
            result.error = Error::BudgetExceeded;
            return result;
        }
        const quint64 index = coordinates.size();
        auto &group = groups[{quintptr(entry.second.allocation->buffer),
            mask ? quintptr(maskTile.allocation->buffer) : 0, index / capacity}];
        group.source = entry.second.allocation;
        group.mask = mask ? maskTile.allocation : group.source;
        group.destinationAllocation = index / capacity;
        group.parameters.push_back({
            quint32(entry.second.offset / TileBytes), quint32(index % capacity),
            quint32(clippedLeft - tileLeft), quint32(clippedTop - tileTop),
            quint32(clippedRight - tileLeft), quint32(clippedBottom - tileTop),
            quint32(operation == CompositeOp::Erase ? UpdateKind::Erase : UpdateKind::Over), opacity, coverage,
            mask ? quint32(maskTile.offset / TileBytes) : std::numeric_limits<quint32>::max()
        });
        coordinates.push_back(entry.first);
    }
    if (coordinates.empty()) return unchanged();
    if (d->pending.size() >= d->maximumPending) {
        result.error = Error::QueueFull;
        return result;
    }
    quint64 parameterBytes = 0;
    const quint64 alignment = d->limits.minStorageBufferOffsetAlignment;
    for (auto &entry : groups) {
        auto &group = entry.second;
        group.parameterOffset = (parameterBytes + alignment - 1) / alignment * alignment;
        parameterBytes = group.parameterOffset + group.parameters.size() * sizeof(CompositeParameters);
    }
    if (parameterBytes > d->limits.maxBufferSize || parameterBytes > available - coordinates.size() * TileBytes) {
        result.error = Error::BudgetExceeded;
        return result;
    }
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto data = std::make_shared<VersionData>(*base.d);
    data->completion = result.completion.d;
    auto parameters = std::make_shared<Allocation>(d->state, parameterBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    std::vector<char> packedParameters(size_t(parameterBytes), 0);
    std::vector<std::shared_ptr<Allocation>> allocations;
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(
        wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
    quint64 copiedBytes = 0;
    for (quint64 index = 0; index < coordinates.size(); ++index) {
        if (index % capacity == 0) {
            const quint64 count = std::min(capacity, quint64(coordinates.size()) - index);
            allocations.push_back(std::make_shared<Allocation>(d->state, count * TileBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst));
        }
        const auto &allocation = allocations.back();
        const quint64 offset = index % capacity * TileBytes;
        const auto oldTile = base.d->tiles.find(coordinates[index]);
        if (oldTile != base.d->tiles.end()) {
            wgpuCommandEncoderCopyBufferToBuffer(encoder.value, oldTile->second.allocation->buffer, oldTile->second.offset,
                                                 allocation->buffer, offset, TileBytes);
            copiedBytes += TileBytes;
        }
        data->tiles[coordinates[index]] = {allocation, offset};
    }
    {
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(
            wgpuCommandEncoderBeginComputePass(encoder.value, nullptr));
        wgpuComputePassEncoderSetPipeline(pass.value, d->compositePipeline);
        for (const auto &entry : groups) {
            const auto &group = entry.second;
            const auto &allocation = allocations[group.destinationAllocation];
            const quint64 bytes = group.parameters.size() * sizeof(CompositeParameters);
            std::memcpy(packedParameters.data() + group.parameterOffset, group.parameters.data(), bytes);
            WGPUBindGroupEntry entries[4]{};
            entries[0].binding = 0;
            entries[0].buffer = allocation->buffer;
            entries[0].size = allocation->bytes;
            entries[1].binding = 3;
            entries[1].buffer = group.source->buffer;
            entries[1].size = group.source->bytes;
            entries[2].binding = 4;
            entries[2].buffer = parameters->buffer;
            entries[2].offset = group.parameterOffset;
            entries[2].size = bytes;
            entries[3].binding = 5;
            entries[3].buffer = group.mask->buffer;
            entries[3].size = group.mask->bytes;
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->compositeLayout;
            descriptor.entryCount = 4;
            descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> bindGroup(
                wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, bindGroup.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, quint32(group.parameters.size()));
        }
        wgpuComputePassEncoderEnd(pass.value);
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commandBuffer(
        wgpuCommandEncoderFinish(encoder.value, nullptr));
    result.version.d = std::move(data);
    d->submit({result.completion.d, base, result.version, parameters, {}, source, mask ? *mask : Version{}}, commandBuffer.value,
              packedParameters, {}, copiedBytes, groups.size());
    errors.submitted = true;
    return result;
}

