/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

using namespace KisGpuTileStorage;

KisGpuTileStore::Edit KisGpuTileStore::mixDabs(const Version &base, const QVector<UpdateCommand> &commands,
                                            const Version *selection)
{
    struct Dispatch { Coordinate coordinate; TileCommand command, shape; };
    std::vector<std::vector<Dispatch>> ordered(size_t(commands.size()));
    std::map<Coordinate, VersionData::Tile> changed;
    const quint64 alignment = d->limits.minStorageBufferOffsetAlignment;
    const quint64 stride = (sizeof(TileParameters) + alignment - 1) / alignment * alignment;
    const quint64 available = d->availableForOperation(0);
    quint64 dispatches = 0, passes = 0;
    auto parametersSize = [&](quint64 count) { return count ? (count - 1) * stride + sizeof(TileParameters) : 0; };
    auto requiredBytes = [&] { return changed.size() * TileBytes + parametersSize(dispatches)
        + dispatches * 2 * sizeof(TileCommand) + 4; };
    for (qsizetype ordinal = 0; ordinal < commands.size(); ++ordinal) {
        const auto &command = commands[ordinal];
        if (command.coverage == 0 || (command.kind != UpdateKind::DabMix
            && (command.opacity == 0 || (command.rgba >> 24) == 0))) continue;
        const qint64 left = command.rectangle.x(), top = command.rectangle.y();
        const qint64 right = left + command.rectangle.width(), bottom = top + command.rectangle.height();
        const int firstX = tileCoordinate(left), lastX = tileCoordinate(right - 1);
        const int firstY = tileCoordinate(top), lastY = tileCoordinate(bottom - 1);
        const quint64 count = quint64(lastX - firstX + 1) * quint64(lastY - firstY + 1);
        if (!selection && count > available / TileBytes) return {Error::BudgetExceeded, {}, {}};
        const auto add = [&](int x, int y) {
            changed.try_emplace({x, y});
            ++dispatches;
            if (requiredBytes() > available || parametersSize(dispatches) > d->limits.maxBufferSize
                || dispatches * 2 * sizeof(TileCommand) > std::min<quint64>({d->limits.maxBufferSize, d->limits.maxStorageBufferBindingSize, quint64(std::numeric_limits<quint32>::max())})) return false;
            const qint64 tileLeft = qint64(x) * 64, tileTop = qint64(y) * 64;
            TileCommand raster{quint32(std::max(left, tileLeft) - tileLeft), quint32(std::max(top, tileTop) - tileTop),
                quint32(std::min(right, tileLeft + 64) - tileLeft), quint32(std::min(bottom, tileTop + 64) - tileTop),
                command.rgba, quint32(command.kind), command.opacity, command.coverage};
            const DabParameters shape{float(command.center.x() - tileLeft), float(command.center.y() - tileTop),
                float(2 / command.diameter.width()), float(2 / command.diameter.height()),
                float(2 / command.diameter.width() / command.fade.width()),
                float(2 / command.diameter.height() / command.fade.height()), command.textureStrength, 0};
            TileCommand record;
            std::memcpy(&record, &shape, sizeof(record));
            ordered[size_t(ordinal)].push_back({{x, y}, raster, record});
            return true;
        };
        if (selection) {
            auto tile = selection->d->tiles.lower_bound({firstX, std::numeric_limits<int>::min()});
            for (; tile != selection->d->tiles.end() && tile->first.first <= lastX; ++tile) {
                const auto [x, y] = tile->first;
                if (y >= firstY && y <= lastY && !add(x, y)) return {Error::BudgetExceeded, {}, {}};
            }
        } else {
            for (int y = firstY; y <= lastY; ++y) for (int x = firstX; x <= lastX; ++x)
                if (!add(x, y)) return {Error::BudgetExceeded, {}, {}};
        }
        if (!ordered[size_t(ordinal)].empty()) ++passes;
    }
    if (!dispatches) { Edit result; result.version = base; result.completion.d = base.d->completion; return result; }
    if (requiredBytes() > d->availableForOperation(quint32(passes))) return {Error::BudgetExceeded, {}, {}};
    if (d->pending.size() >= d->maximumPending
        || (d->timestamps && !d->state->nativeOwner->queriesAvailable(quint32(passes * 2)))) return {Error::QueueFull, {}, {}};
    Edit result;
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto data = std::make_shared<VersionData>(*base.d);
    data->completion = result.completion.d;
    auto parameters = std::make_shared<Allocation>(d->state, parametersSize(dispatches), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto storage = std::make_shared<Allocation>(d->state, dispatches * 2 * sizeof(TileCommand), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto pickup = std::make_shared<Allocation>(d->state, 4, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    Private::Recording encoder(d->state, d->timestamps ? quint32(passes) : 0);
    quint64 copiedBytes = 0, pickupBytes = 0;
    for (auto &[coordinate, tile] : changed) {
        tile = {std::make_shared<Allocation>(d->state, TileBytes,
            WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst), 0};
        const auto old = base.d->tiles.find(coordinate);
        if (old != base.d->tiles.end()) {
            wgpuCommandEncoderCopyBufferToBuffer(encoder.value, old->second.allocation->buffer, old->second.offset, tile.allocation->buffer, 0, TileBytes);
            copiedBytes += TileBytes;
        }
        data->tiles[coordinate] = tile;
    }
    std::vector<char> packedParameters(size_t(parameters->bytes), 0);
    std::vector<TileCommand> packedCommands;
    packedCommands.reserve(size_t(dispatches * 2));
    quint64 index = 0;
    for (qsizetype ordinal = 0; ordinal < commands.size(); ++ordinal) {
        const auto &list = ordered[size_t(ordinal)];
        if (list.empty()) continue;
        const auto &command = commands[ordinal];
        float opacity = 0;
        if (command.pickup) {
            const auto point = command.pickup->pixel;
            const Coordinate coordinate{tileCoordinate(point.x()), tileCoordinate(point.y())};
            const auto source = data->tiles.find(coordinate);
            if (source == data->tiles.end()) {
                wgpuCommandEncoderClearBuffer(encoder.value, pickup->buffer, 0, 4);
            } else {
                const quint64 pixel = quint64(qint64(point.y()) - qint64(coordinate.second) * 64) * 64
                    + quint64(qint64(point.x()) - qint64(coordinate.first) * 64);
                wgpuCommandEncoderCopyBufferToBuffer(encoder.value, source->second.allocation->buffer,
                    source->second.offset + pixel * 4, pickup->buffer, 0, 4);
                pickupBytes += 4;
            }
            opacity = float(.8 * (double(command.pickup->rate) / 255) * (double(command.opacity) / 255));
        }
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(encoder.beginComputePass());
        wgpuComputePassEncoderSetPipeline(pass.value, d->mixingPipelines->pipelines[bool(selection)]);
        for (const auto &dispatch : list) {
            const auto &tile = changed.at(dispatch.coordinate);
            const auto mask = selection ? selection->d->tiles.at(dispatch.coordinate) : VersionData::Tile{};
            TileParameters parameter{quint32(packedCommands.size()), 2, quint32(mask.offset / 4), 0, 0, 0, 0, 0};
            std::memcpy(&parameter.reserved, &opacity, sizeof(opacity));
            std::memcpy(packedParameters.data() + index * stride, &parameter, sizeof(parameter));
            packedCommands.push_back(dispatch.command);
            packedCommands.push_back(dispatch.shape);
            WGPUBindGroupEntry entries[5]{};
            const quint32 bindings[] = {0, 1, 2, 7, 5};
            const WGPUBuffer buffers[] = {tile.allocation->buffer, parameters->buffer, storage->buffer, pickup->buffer,
                                         selection ? mask.allocation->buffer : nullptr};
            const quint64 sizes[] = {TileBytes, sizeof(TileParameters), storage->bytes, 4, selection ? mask.allocation->bytes : 0};
            for (int i = 0; i < 4 + bool(selection); ++i) {
                entries[i].binding = bindings[i]; entries[i].buffer = buffers[i]; entries[i].size = sizes[i];
            }
            entries[1].offset = index * stride;
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->mixingPipelines->layouts[bool(selection)];
            descriptor.entryCount = 4 + bool(selection); descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> group(wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, group.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, 1);
            ++index;
        }
        wgpuComputePassEncoderEnd(pass.value);
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> buffer(encoder.finish());
    result.version.d = std::move(data);
    Private::Pending pending{result.completion.d, base, result.version, parameters, storage,
        selection ? QVector<Version>{*selection} : QVector<Version>{}};
    pending.pickup = std::move(pickup);
    d->submit(std::move(pending), buffer.value, packedParameters, packedCommands, copiedBytes, dispatches, {}, encoder.timing);
    d->statistics.pickupCopyBytes += pickupBytes;
    errors.submitted = true;
    return result;
}
