/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuTileStorage_p.h"

namespace LibrePaint::Image::Gpu {
using namespace Detail;

TileRenderer::Private::TranslationResult TileRenderer::Private::translateTiles(
    WGPUCommandEncoder commands, const Tiles &source, const std::shared_ptr<Tile> &fallback,
    QPoint offset, PixelFormat format, QRect region, QList<std::shared_ptr<Tile>> &pins)
{
    const auto tileBytes = RgbaTile::byteCount(format);
    Tiles result;
    QSet<quint64> coordinates;
    const bool aligned = offset.x() % TileSize == 0 && offset.y() % TileSize == 0;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile) {
        const QRect shifted(coordinate(tile.key()) * TileSize + offset, QSize(TileSize, TileSize));
        const auto needed = shifted.intersected(region);
        if (needed.isEmpty()) continue;
        if (aligned) {
            const auto point = shifted.topLeft() / TileSize;
            result[key(point.x(), point.y())] = tile.value();
            continue;
        }
        for (int y = pixelTileIndex(needed.top(), TileSize); y <= pixelTileIndex(needed.bottom(), TileSize); ++y)
            for (int x = pixelTileIndex(needed.left(), TileSize); x <= pixelTileIndex(needed.right(), TileSize); ++x) {
                coordinates.insert(key(x, y));
                if (coordinates.size() > budget->limit / tileBytes)
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU translation coordinate capacity exceeded")};
            }
    }
    if (coordinates.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    if (!ensureTileCapacity(coordinates.size(), format))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU translation tile budget exhausted")};
    if (!translationPipeline) {
        translationPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/translate.wgsl"));
        if (translationPipeline)
            translationLayout.reset(wgpuComputePipelineGetBindGroupLayout(translationPipeline.get(), 0));
    }
    if (!translationPipeline || !translationLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU translation pipeline creation failed")};
    for (const auto destination : coordinates) {
        const auto origin = coordinate(destination) * TileSize - offset;
        const QPoint base(pixelTileIndex(origin.x(), TileSize), pixelTileIndex(origin.y(), TileSize));
        const auto residue = origin - base * TileSize;
        const quint32 values[] = {quint32(residue.x()), quint32(residue.y()), quint32(format), 0};
        auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        const auto output = allocateTile(format);
        if (!parameters || !output)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU translation allocation failed")};
        output->mayContainHiddenRgb = false;
        WGPUBindGroupEntry entries[6]{};
        for (int i = 0; i < 4; ++i) {
            const auto point = base + QPoint(i % 2, i / 2);
            const auto tile = source.value(key(point.x(), point.y()), fallback);
            entries[i].binding = quint32(i);
            entries[i].buffer = tile ? tile->pixels.get() : zeroBuffer(commands, format);
            if (!entries[i].buffer)
                return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU empty translation tile allocation failed")};
            entries[i].size = tileBytes;
            output->mayContainHiddenRgb = output->mayContainHiddenRgb || (tile && tile->mayContainHiddenRgb);
        }
        entries[4].binding = 4;
        entries[4].buffer = output->pixels.get();
        entries[4].size = tileBytes;
        entries[5].binding = 5;
        entries[5].buffer = parameters.get();
        entries[5].size = sizeof(values);
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = translationLayout.get();
        descriptor.entryCount = 6;
        descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty())
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU translation binding failed")};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, translationPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        result[destination] = output;
        pins.append(output);
    }
    return {RenderOutcome::Recorded, std::move(result), {}};
}
}
