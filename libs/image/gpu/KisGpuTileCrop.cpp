/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"

namespace LibrePaint::Image::Gpu {
using namespace Detail;

TileRenderer::Private::SelectionCopyResult TileRenderer::Private::copySelection(
    WGPUCommandEncoder commands, const TileView &source, const RgbaPixel &implicit, PixelFormat format,
    const SelectionMaskSP &mask, bool inverted, bool projection)
{
    if (!ensureTileCapacity(1, format))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU selection copy tile budget exhausted")};
    const auto coverage = coverageTile(commands, mask, key(source.coordinate.x(), source.coordinate.y()));
    if (!coverage)
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU selection copy coverage budget exhausted")};
    if (!selectionCopyPipeline) {
        selectionCopyPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/selection-copy.wgsl"));
        if (selectionCopyPipeline)
            selectionCopyLayout.reset(wgpuComputePipelineGetBindGroupLayout(selectionCopyPipeline.get(), 0));
    }
    if (!selectionCopyPipeline || !selectionCopyLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selection copy pipeline creation failed")};
    const quint32 values[]{quint32(format), quint32(inverted), quint32(!source.pixels), quint32(projection),
                           implicit[0], implicit[1], implicit[2], implicit[3]};
    auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    const auto output = allocateTile(format);
    if (!parameters || !output)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selection copy allocation failed")};
    WGPUBindGroupEntry entries[4]{};
    const WGPUBuffer buffers[]{source.pixels ? source.pixels : unusedPattern.get(), output->pixels.get(),
                               parameters.get(), coverage->pixels.get()};
    for (quint32 i = 0; i < 4; ++i) {
        entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = selectionCopyLayout.get(); descriptor.entryCount = 4; descriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings || !device->diagnostic().isEmpty())
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selection copy binding failed")};
    wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, selectionCopyPipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return {RenderOutcome::Recorded, output, {}};
}

TileRenderer::Private::CropResult TileRenderer::Private::cropTiles(
    WGPUCommandEncoder commands, const Tiles &source, QRect keep, const RgbaPixel &defaultPixel, PixelFormat format,
    QRect region, QList<std::shared_ptr<Tile>> &pins)
{
    retainTiles(source);
    Tiles result, boundaries;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile) {
        const QRect bounds(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize));
        if (!region.intersects(bounds) || !keep.intersects(bounds)) continue;
        if (keep.contains(bounds)) result.insert(tile.key(), tile.value());
        else boundaries.insert(tile.key(), tile.value());
    }
    if (boundaries.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    if (!ensureTileCapacity(boundaries.size(), format))
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
                               quint32(keep.x() + keep.width()), quint32(keep.y() + keep.height()), quint32(format), 0,
                               defaultPixel[0], defaultPixel[1], defaultPixel[2], defaultPixel[3]};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        const auto output = allocateTile(format);
        if (!parameters || !output)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU crop allocation failed")};
        output->mayContainHiddenRgb = tile.value()->mayContainHiddenRgb ||
            (!defaultPixel[3] && (defaultPixel[0] || defaultPixel[1] || defaultPixel[2]));
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
TileRenderer::Private::CropResult TileRenderer::Private::restoreImplicitPixels(
    WGPUCommandEncoder commands, const Tiles &source, const QList<QRect> &allocatedBlocks,
    const RgbaPixel &implicit, PixelFormat format)
{
    QRegion allocation;
    for (const auto &block : allocatedBlocks) allocation += block;
    Tiles result;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile) {
        const auto origin = coordinate(tile.key()) * TileSize;
        const QRect bounds(origin, QSize(TileSize, TileSize));
        const auto allocated = allocation.intersected(bounds);
        if (allocated.isEmpty()) continue;
        if (QRegion(bounds).subtracted(allocated).isEmpty()) { result.insert(tile.key(), tile.value()); continue; }
        if (!ensureTileCapacity(1, format))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU merged layer boundary tile budget exhausted")};
        if (!implicitPixelPipeline) {
            implicitPixelPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/implicit-pixels.wgsl"));
            if (implicitPixelPipeline) implicitPixelLayout.reset(wgpuComputePipelineGetBindGroupLayout(implicitPixelPipeline.get(), 0));
        }
        if (!implicitPixelPipeline || !implicitPixelLayout)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU implicit pixel pipeline creation failed")};
        QList<std::array<qint32, 4>> rectangles;
        for (const auto &area : allocated) {
            const auto local = area.translated(-origin);
            rectangles.append({local.left(), local.top(), local.right() + 1, local.bottom() + 1});
        }
        const quint32 values[]{quint32(format), quint32(rectangles.size()), 0, 0,
            implicit[0], implicit[1], implicit[2], implicit[3]};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        auto regions = createBuffer(device->nativeDevice(), rectangles.size() * sizeof(rectangles[0]),
            WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        const auto output = allocateTile(format);
        if (!parameters || !regions || !output)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU merged layer boundary allocation failed")};
        output->mayContainHiddenRgb = tile.value()->mayContainHiddenRgb || (!implicit[3] && (implicit[0] || implicit[1] || implicit[2]));
        WGPUBindGroupEntry entries[4]{};
        const WGPUBuffer buffers[]{tile.value()->pixels.get(), output->pixels.get(), parameters.get(), regions.get()};
        for (quint32 i = 0; i < 4; ++i) { entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]); }
        WGPUBindGroupDescriptor descriptor{}; descriptor.layout = implicitPixelLayout.get(); descriptor.entryCount = 4; descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty())
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU implicit pixel binding failed")};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        wgpuQueueWriteBuffer(device->queue(), regions.get(), 0, rectangles.constData(), rectangles.size() * sizeof(rectangles[0]));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, implicitPixelPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        retainTile(tile.value()); retainTile(output); result.insert(tile.key(), output);
    }
    return {RenderOutcome::Recorded, std::move(result), {}};
}

}
