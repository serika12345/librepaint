/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"

namespace LibrePaint::Image::Gpu {
using namespace Detail;

TileRenderer::Private::CropResult TileRenderer::Private::cropTiles(
    WGPUCommandEncoder commands, const Tiles &source, QRect keep, quint32 defaultPixel,
    QRect region, QList<std::shared_ptr<Tile>> &pins)
{
    Tiles result, boundaries;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile) {
        const QRect bounds(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize));
        if (!region.intersects(bounds) || !keep.intersects(bounds)) continue;
        if (keep.contains(bounds)) result.insert(tile.key(), tile.value());
        else boundaries.insert(tile.key(), tile.value());
    }
    if (boundaries.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    if (!ensureTileCapacity(boundaries.size()))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU crop boundary tile budget exhausted")};
    if (!cropPipeline) {
        cropPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/crop.wgsl"));
        if (cropPipeline) cropLayout.reset(wgpuComputePipelineGetBindGroupLayout(cropPipeline.get(), 0));
    }
    if (!cropPipeline || !cropLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU crop pipeline creation failed")};
    for (auto tile = boundaries.cbegin(); tile != boundaries.cend(); ++tile) {
        const auto origin = coordinate(tile.key()) * TileSize;
        const quint32 values[]{quint32(origin.x()), quint32(origin.y()), quint32(keep.left()), quint32(keep.top()),
                               quint32(keep.x() + keep.width()), quint32(keep.y() + keep.height()), defaultPixel, 0};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        const auto output = allocateTile();
        if (!parameters || !output)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU crop allocation failed")};
        output->mayContainHiddenRgb = tile.value()->mayContainHiddenRgb ||
            ((defaultPixel >> 24) == 0 && (defaultPixel & 0x00ffffffu));
        WGPUBindGroupEntry entries[3]{};
        const WGPUBuffer buffers[]{tile.value()->pixels.get(), output->pixels.get(), parameters.get()};
        for (quint32 i = 0; i < 3; ++i) {
            entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = cropLayout.get(); descriptor.entryCount = 3; descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty())
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU crop binding failed")};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, cropPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        result.insert(tile.key(), output);
        pins.append(tile.value()); pins.append(output);
    }
    return {RenderOutcome::Recorded, std::move(result), {}};
}
}
