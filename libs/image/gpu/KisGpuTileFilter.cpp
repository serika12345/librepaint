/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"

namespace LibrePaint::Image::Gpu {
using namespace Detail;

TileRenderer::Private::FilterResult TileRenderer::Private::filterTiles(
    WGPUCommandEncoder commands, const Tiles &source, const FilterOperation &filter,
    const std::shared_ptr<Tile> &fallback, QSize documentSize, QRect region,
    QList<std::shared_ptr<Tile>> &pins)
{
    Tiles result = source;
    QRect bounds = region;
    if (!filter.selectionRect.isEmpty() && !filter.selectionInverted)
        bounds = bounds.intersected(filter.selectionRect);
    if (bounds.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    const bool masked = filter.selectionKind == SelectionKind::Freehand;
    if (masked && (!filter.selectionMask || filter.selectionMask->isEmpty()))
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU filter freehand selection has no coverage")};
    // Existing content is the layer's painted extent. A non-transparent default
    // pixel extends that extent over the canvas, so those tiles are synthesized.
    QList<quint64> planned;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile) {
        const QPoint coordinate = Detail::coordinate(tile.key());
        const QRect tileRect(coordinate * TileSize, QSize(TileSize, TileSize));
        if (!tileRect.intersects(bounds)) continue;
        if (masked && !filter.selectionInverted &&
            !filter.selectionMask->tiles().contains(SelectionTileKey(coordinate.x(), coordinate.y())))
            continue;
        planned.append(tile.key());
    }
    if (fallback && !documentSize.isEmpty()) {
        const QRect content(QPoint(), documentSize);
        const QRect area = bounds.intersected(content);
        if (!area.isEmpty()) {
            const int left = pixelTileIndex(area.left(), TileSize);
            const int top = pixelTileIndex(area.top(), TileSize);
            const int right = pixelTileIndex(area.right(), TileSize);
            const int bottom = pixelTileIndex(area.bottom(), TileSize);
            for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                const auto coordinate = Detail::key(x, y);
                if (source.contains(coordinate)) continue;
                if (masked && !filter.selectionInverted &&
                    !filter.selectionMask->tiles().contains(SelectionTileKey(x, y)))
                    continue;
                planned.append(coordinate);
            }
        }
    }
    if (planned.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    if (!filterPipeline) {
        filterPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/filter.wgsl"));
        if (filterPipeline) filterLayout.reset(wgpuComputePipelineGetBindGroupLayout(filterPipeline.get(), 0));
    }
    if (!filterPipeline || !filterLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter pipeline creation failed")};
    quint32 flags = 1u;
    if (masked) flags |= 2u;
    else if (filter.selectionKind == SelectionKind::Ellipse) flags |= 4u;
    if (filter.selectionInverted) flags |= 8u;
    if (filter.selectionRect.isEmpty() && !masked) flags = 0u;
    for (const auto coordinate : planned) {
        const auto input = source.value(coordinate, fallback);
        if (!input) continue;
        if (!ensureTileCapacity(1))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU filter tile budget exhausted")};
        const auto output = allocateTile();
        if (!output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter allocation failed")};
        std::shared_ptr<CoverageTile> coverage;
        if (masked) {
            coverage = coverageTile(filter.selectionMask, coordinate);
            if (!coverage)
                return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU filter coverage budget exhausted")};
        }
        const QPoint origin = Detail::coordinate(coordinate) * TileSize;
        const qint32 values[]{origin.x(), origin.y(), qint32(filter.kind), qint32(flags),
                              filter.selectionRect.x(), filter.selectionRect.y(),
                              filter.selectionRect.width(), filter.selectionRect.height()};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values),
                                       WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!parameters) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter parameter allocation failed")};
        WGPUBindGroupEntry entries[4]{};
        const WGPUBuffer buffers[]{input->pixels.get(), output->pixels.get(), parameters.get(),
                                   coverage ? coverage->pixels.get() : unusedSelection.get()};
        for (quint32 i = 0; i < 4; ++i) {
            entries[i].binding = i;
            entries[i].buffer = buffers[i];
            entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = filterLayout.get();
        descriptor.entryCount = 4;
        descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty())
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter binding failed")};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter commands failed")};
        wgpuComputePassEncoderSetPipeline(pass, filterPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        // Complementing zero RGB produces hidden color under alpha zero; the
        // projection already normalizes such tiles when the flag is set.
        output->mayContainHiddenRgb = true;
        result.insert(coordinate, output);
        pins.append(input);
        pins.append(output);
    }
    return {RenderOutcome::Recorded, std::move(result), {}};
}
}
