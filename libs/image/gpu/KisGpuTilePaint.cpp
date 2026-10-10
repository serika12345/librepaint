/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <vector>

using namespace KisGpuTileStorage;

KisGpuTileStore::Edit KisGpuTileStore::fill(const Version &base, QRect rectangle, quint32 rgba)
{
    return update(base, {{rectangle, rgba, UpdateKind::Fill, 255, 255, {}, {}, {}}});
}

KisGpuTileStore::Edit KisGpuTileStore::paint(const Version &base, QRect rectangle, quint32 rgba,
                                          CompositeOp operation, quint8 opacity, quint8 coverage)
{
    return paint(base, {{rectangle, rgba, operation, opacity, coverage}});
}

KisGpuTileStore::Edit KisGpuTileStore::paint(const Version &base, const QVector<PaintCommand> &commands)
{
    QVector<UpdateCommand> updates;
    updates.reserve(commands.size());
    for (const auto &command : commands) {
        updates.push_back({command.rectangle, command.rgba,
                           command.operation == CompositeOp::Erase ? UpdateKind::Erase : UpdateKind::Over,
                           command.opacity, command.coverage, {}, {}, {}});
    }
    return update(base, updates);
}

KisGpuTileStore::Edit KisGpuTileStore::paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip)
{
    return paintDabCommands(base, commands, clip, nullptr);
}

KisGpuTileStore::Edit KisGpuTileStore::paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip,
                                              const Version &selection)
{
    return paintDabCommands(base, commands, clip, &selection);
}

KisGpuTileStore::Edit KisGpuTileStore::paintDabCommands(const Version &base, const QVector<DabCommand> &commands, QRect clip,
                                                     const Version *selection, const BrushTexture *texture, QPoint origin)
{
    if (texture && (!texture->d || texture->d->allocation->owner != d->state)) return {Error::InvalidVersion, {}, {}};
    if (selection && (!selection->d || selection->d->owner != d->state)) return {Error::InvalidVersion, {}, {}};
    if (!base.d || base.d->owner != d->state) return {Error::InvalidVersion, {}, {}};
    if (!deviceAvailable()) return {Error::DeviceLost, {}, {}};
    QVector<UpdateCommand> updates;
    updates.reserve(commands.size());
    for (const auto &command : commands) {
        const double cx = command.center.x(), cy = command.center.y();
        const double dx = command.diameter.width(), dy = command.diameter.height();
        const double fx = command.fade.width(), fy = command.fade.height();
        if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(dx) || !std::isfinite(dy)
            || !std::isfinite(fx) || !std::isfinite(fy) || dx <= 0 || dy <= 0 || fx <= 0 || fy <= 0
            || fx > 1 || fy > 1 || std::abs(cx) > std::numeric_limits<int>::max()
            || std::abs(cy) > std::numeric_limits<int>::max()
            || dx > std::numeric_limits<int>::max() || dy > std::numeric_limits<int>::max()
            || !std::isfinite(float(2 / dx / fx))
            || !std::isfinite(float(2 / dy / fy))) {
            return {Error::InvalidCommand, {}, {}};
        }
        if (clip.isEmpty()) continue;
        const qint64 left = std::max(qint64(clip.x()), qint64(std::floor(cx - dx / 2)));
        const qint64 top = std::max(qint64(clip.y()), qint64(std::floor(cy - dy / 2)));
        const qint64 right = std::min(qint64(clip.x()) + clip.width(), qint64(std::ceil(cx + dx / 2)) + 1);
        const qint64 bottom = std::min(qint64(clip.y()) + clip.height(), qint64(std::ceil(cy + dy / 2)) + 1);
        if (left >= right || top >= bottom) continue;
        updates.push_back({QRect(int(left), int(top), int(right - left), int(bottom - top)), command.rgba,
            command.operation == CompositeOp::Erase ? UpdateKind::DabErase : UpdateKind::DabOver,
            command.opacity, command.coverage, command.center, command.diameter, command.fade});
    }
    return update(base, updates, selection, texture, origin);
}

KisGpuTileStore::Edit KisGpuTileStore::paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip,
                                              const BrushTexture &texture, QPoint origin, const Version *selection)
{
    return paintDabCommands(base, commands, clip, selection, &texture, origin);
}

KisGpuTileStore::Edit KisGpuTileStore::update(const Version &base, const QVector<UpdateCommand> &commands,
                                           const Version *selection, const BrushTexture *texture, QPoint origin)
{
    Edit result;
    if (!base.d || base.d->owner != d->state) {
        result.error = Error::InvalidVersion;
        return result;
    }
    if (!deviceAvailable()) {
        result.error = Error::DeviceLost;
        return result;
    }
    const quint64 available = d->availableForOperation();
    const quint64 alignment = d->limits.minStorageBufferOffsetAlignment;
    const quint64 capacity = selection ? 1 : d->tilesPerAllocation;
    const quint64 parameterStride = (capacity * sizeof(TileParameters) + alignment - 1) / alignment * alignment;
    auto parameterSize = [&](quint64 tiles) {
        return (tiles - 1) / capacity * parameterStride + ((tiles - 1) % capacity + 1) * sizeof(TileParameters);
    };
    const quint64 maximumCommands = std::min<quint64>({d->limits.maxBufferSize,
        d->limits.maxStorageBufferBindingSize, quint64(std::numeric_limits<quint32>::max())}) / sizeof(TileCommand);
    quint64 commandCount = 0;
    std::map<Coordinate, std::vector<TileCommand>> tileCommands;
    auto fits = [&](quint64 tiles, quint64 count) {
        if (tiles > available / TileBytes || count > maximumCommands) return false;
        const quint64 parameterBytes = parameterSize(tiles);
        const quint64 storageBytes = count * sizeof(TileCommand);
        const quint64 remaining = available - tiles * TileBytes;
        return parameterBytes <= d->limits.maxBufferSize
            && parameterBytes <= remaining && storageBytes <= remaining - parameterBytes;
    };
    for (const auto &command : commands) {
        const QRect rectangle = command.rectangle;
        if (rectangle.isEmpty() || (command.kind != UpdateKind::Fill
            && (command.opacity == 0 || command.coverage == 0 || (command.rgba >> 24) == 0))) continue;
        // Widen before adding: QRect's inclusive right/bottom can overflow int.
        const qint64 left = rectangle.x(), top = rectangle.y();
        const qint64 right = left + rectangle.width(), bottom = top + rectangle.height();
        const int firstX = tileCoordinate(left), lastX = tileCoordinate(right - 1);
        const int firstY = tileCoordinate(top), lastY = tileCoordinate(bottom - 1);
        const quint64 count = quint64(lastX - firstX + 1) * quint64(lastY - firstY + 1);
        const bool dab = command.kind == UpdateKind::DabOver || command.kind == UpdateKind::DabErase;
        const quint64 records = dab ? 2 : 1;
        if (!selection && (count > available / TileBytes || count > (maximumCommands - commandCount) / records)) {
            result.error = Error::BudgetExceeded;
            return result;
        }
        const auto addTile = [&](int x, int y) {
            auto &list = tileCommands[{x, y}];
            commandCount += records;
            if (!fits(tileCommands.size(), commandCount)) return false;
            const qint64 tileLeft = qint64(x) * 64, tileTop = qint64(y) * 64;
            list.push_back({
                quint32(std::max(left, tileLeft) - tileLeft),
                quint32(std::max(top, tileTop) - tileTop),
                quint32(std::min(right, tileLeft + 64) - tileLeft),
                quint32(std::min(bottom, tileTop + 64) - tileTop),
                command.rgba, quint32(command.kind), command.opacity, command.coverage
            });
            if (dab) {
                const DabParameters shape {float(command.center.x() - tileLeft), float(command.center.y() - tileTop),
                    float(2 / command.diameter.width()), float(2 / command.diameter.height()),
                    float(2 / command.diameter.width() / command.fade.width()),
                    float(2 / command.diameter.height() / command.fade.height()), {0, 0}};
                TileCommand record;
                std::memcpy(&record, &shape, sizeof(record));
                list.push_back(record);
            }
            return true;
        };
        if (selection) {
            auto tile = selection->d->tiles.lower_bound({firstX, std::numeric_limits<int>::min()});
            for (; tile != selection->d->tiles.end() && tile->first.first <= lastX; ++tile) {
                const auto [x, y] = tile->first;
                if (y >= firstY && y <= lastY && !addTile(x, y)) return {Error::BudgetExceeded, {}, {}};
            }
        } else {
            for (int y = firstY; y <= lastY; ++y) {
                for (int x = firstX; x <= lastX; ++x) {
                    if (!addTile(x, y)) return {Error::BudgetExceeded, {}, {}};
                }
            }
        }
    }
    if (tileCommands.empty()) {
        result.version = base;
        result.completion.d = base.d->completion;
        return result;
    }
    if (d->pending.size() >= d->maximumPending
        || (d->timestamps && !d->state->nativeOwner->queriesAvailable(2))) {
        result.error = Error::QueueFull;
        return result;
    }
    const quint64 parameterBytes = parameterSize(tileCommands.size());
    const quint64 storageBytes = commandCount * sizeof(TileCommand);
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto data = std::make_shared<VersionData>(*base.d);
    data->completion = result.completion.d;
    auto parameters = std::make_shared<Allocation>(d->state, parameterBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto commandStorage = std::make_shared<Allocation>(d->state, storageBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    std::vector<char> packedParameters(size_t(parameterBytes), 0);
    std::vector<std::shared_ptr<Allocation>> allocations;
    std::vector<TileCommand> packedCommands;
    std::vector<VersionData::Tile> selectionTiles;
    packedCommands.reserve(size_t(commandCount));
    Private::Recording encoder(d->state, d->timestamps);
    quint64 copiedBytes = 0;
    quint32 index = 0;
    for (const auto &entry : tileCommands) {
        const auto maskTile = selection ? selection->d->tiles.at(entry.first) : VersionData::Tile{};
        TileParameters tileParameters {quint32(packedCommands.size()), quint32(entry.second.size()),
                                       quint32(maskTile.offset / sizeof(quint32)), 0, 0, 0, 0, 0};
        if (texture) {
            const auto size = texture->d->size;
            const auto wrap = [](qint64 value, int period) { return quint32((value % period + period) % period); };
            tileParameters.textureX = wrap(qint64(entry.first.first) * 64 - origin.x(), size.width());
            tileParameters.textureY = wrap(qint64(entry.first.second) * 64 - origin.y(), size.height());
            tileParameters.textureWidth = size.width();
            tileParameters.textureHeight = size.height();
        }
        if (selection) selectionTiles.push_back(maskTile);
        const quint64 parameterOffset = index / capacity * parameterStride + index % capacity * sizeof(TileParameters);
        std::memcpy(packedParameters.data() + parameterOffset, &tileParameters, sizeof(tileParameters));
        packedCommands.insert(packedCommands.end(), entry.second.begin(), entry.second.end());
        if (index % capacity == 0) {
            const quint64 count = std::min(capacity, quint64(tileCommands.size()) - index);
            allocations.push_back(std::make_shared<Allocation>(d->state, count * TileBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst));
        }
        const auto &allocation = allocations.back();
        const quint64 tileOffset = index % capacity * TileBytes;
        const auto oldTile = base.d->tiles.find(entry.first);
        const auto &first = entry.second.front();
        if (oldTile != base.d->tiles.end()
            && (first.operation != quint32(UpdateKind::Fill)
                || first.left != 0 || first.top != 0 || first.right != 64 || first.bottom != 64)) {
            wgpuCommandEncoderCopyBufferToBuffer(encoder.value, oldTile->second.allocation->buffer, oldTile->second.offset,
                                                 allocation->buffer, tileOffset, TileBytes);
            copiedBytes += TileBytes;
        }
        data->tiles[entry.first] = {allocation, tileOffset};
        ++index;
    }
    {
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(
            encoder.beginComputePass());
        const auto pipeline = texture ? (selection ? d->dabSelectedTexturePipeline : d->dabTexturePipeline)
                                      : (selection ? d->dabSelectionPipeline : d->pipeline);
        const auto layout = texture ? (selection ? d->dabSelectedTextureLayout : d->dabTextureLayout)
                                    : (selection ? d->dabSelectionLayout : d->layout);
        wgpuComputePassEncoderSetPipeline(pass.value, pipeline);
        for (size_t groupIndex = 0; groupIndex < allocations.size(); ++groupIndex) {
            const auto &allocation = allocations[groupIndex];
            const quint64 count = allocation->bytes / TileBytes;
            WGPUBindGroupEntry entries[5]{};
            entries[0].binding = 0;
            entries[0].buffer = allocation->buffer;
            entries[0].size = allocation->bytes;
            entries[1].binding = 1;
            entries[1].buffer = parameters->buffer;
            entries[1].offset = groupIndex * parameterStride;
            entries[1].size = count * sizeof(TileParameters);
            entries[2].binding = 2;
            entries[2].buffer = commandStorage->buffer;
            entries[2].size = storageBytes;
            if (selection) {
                entries[3].binding = 5;
                entries[3].buffer = selectionTiles[groupIndex].allocation->buffer;
                entries[3].size = selectionTiles[groupIndex].allocation->bytes;
            }
            if (texture) {
                auto &entry = entries[selection ? 4 : 3];
                entry.binding = 6;
                entry.buffer = texture->d->allocation->buffer;
                entry.size = texture->d->allocation->bytes;
            }
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = layout;
            descriptor.entryCount = 3 + bool(selection) + bool(texture);
            descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> group(
                wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, group.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, quint32(count));
        }
        wgpuComputePassEncoderEnd(pass.value);
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commandBuffer(
        encoder.finish());
    result.version.d = std::move(data);
    Private::Pending pending{result.completion.d, base, result.version, parameters, commandStorage,
                             selection ? QVector<Version>{*selection} : QVector<Version>{}};
    pending.brushTexture = texture ? texture->d : nullptr;
    d->submit(std::move(pending), commandBuffer.value,
              packedParameters, packedCommands, copiedBytes, allocations.size(), {}, encoder.timing);
    errors.submitted = true;
    return result;
}
