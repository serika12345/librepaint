/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include <algorithm>

namespace LibrePaint::Image::Gpu {
using namespace Detail;

TileRenderer::Private::FilterResult TileRenderer::Private::convolutionTiles(
    WGPUCommandEncoder commands, const Tiles &source, const FilterOperation &filter,
    const std::shared_ptr<Tile> &fallback, QRect repeatBounds, PixelFormat format, QRect area,
    const QList<quint64> &planned, QList<std::shared_ptr<Tile>> &pins)
{
    if (repeatBounds.isEmpty())
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Convolution requires a finite repeat border")};
    if (!convolutionPipeline) {
        convolutionPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/convolution.wgsl"));
        if (convolutionPipeline) convolutionLayout.reset(wgpuComputePipelineGetBindGroupLayout(convolutionPipeline.get(), 0));
    }
    if (!convolutionPipeline || !convolutionLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU convolution pipeline creation failed: ") + error};
    constexpr int side = TileSize + 2;
    const auto pixelBytes = pixelByteCount(format);
    const qsizetype paddedBytes = qsizetype(side) * side * pixelBytes;
    if (!ensureTileCapacity(0, format) || paddedBytes > budget->limit - budget->used)
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU convolution border exceeds working capacity")};
    auto buffer = createBuffer(device->nativeDevice(), paddedBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    if (!buffer) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU convolution border allocation failed")};
    auto padded = std::make_shared<BufferStorage>(std::move(buffer), budget);
    device->retainForFrame(padded);
    Tiles result;
    for (const auto coordinate : planned) {
        const auto origin = Detail::coordinate(coordinate) * TileSize;
        const auto outputArea = QRect(origin, QSize(TileSize, TileSize)).intersected(area);
        if (outputArea.isEmpty()) continue;
        const int width = outputArea.width() + 2, height = outputArea.height() + 2;
        wgpuCommandEncoderClearBuffer(commands, padded->pixels.get(), 0, paddedBytes);
        // Capture immutable neighbours with buffer copies. Border repetition
        // changes only source coordinates; editing pixels stay on the GPU.
        for (int y = 0; y < height; ++y) {
            const int worldY = std::clamp(outputArea.y() + y - 1, repeatBounds.top(), repeatBounds.bottom());
            for (int x = 0; x < width;) {
                const int requestedX = outputArea.x() + x - 1;
                const int worldX = std::clamp(requestedX, repeatBounds.left(), repeatBounds.right());
                const QPoint point(pixelTileIndex(worldX, TileSize), pixelTileIndex(worldY, TileSize));
                const auto tile = source.value(key(point.x(), point.y()), fallback);
                int count = 1;
                if (worldX == requestedX)
                    count = qMin(width - x, qMin((point.x() + 1) * TileSize - worldX, repeatBounds.right() - worldX + 1));
                if (tile) {
                    const auto local = QPoint(worldX, worldY) - point * TileSize;
                    const quint64 inputOffset = quint64(local.y() * TileSize + local.x()) * pixelBytes;
                    const quint64 outputOffset = quint64(y * width + x) * pixelBytes;
                    wgpuCommandEncoderCopyBufferToBuffer(commands, tile->pixels.get(), inputOffset,
                        padded->pixels.get(), outputOffset, quint64(count) * pixelBytes);
                }
                x += count;
            }
        }
        if (!ensureTileCapacity(1, format))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU convolution output exceeds working capacity")};
        const auto output = allocateTile(format);
        if (!output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU convolution output allocation failed")};
        retainTile(output);
        const qint32 words[]{outputArea.x(), outputArea.y(), outputArea.width(), outputArea.height(),
            qint32(filter.kind), format == PixelFormat::Rgba16, filter.channelWriteMask, width,
            outputArea.x() - origin.x(), outputArea.y() - origin.y(), 0, 0};
        auto uniform = createBuffer(device->nativeDevice(), sizeof(words), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!uniform) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU convolution parameters unavailable")};
        wgpuQueueWriteBuffer(device->queue(), uniform.get(), 0, words, sizeof(words));
        WGPUBindGroupEntry entries[3]{};
        const WGPUBuffer buffers[]{padded->pixels.get(), output->pixels.get(), uniform.get()};
        for (quint32 i = 0; i < 3; ++i) {
            entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = convolutionLayout.get(); descriptor.entryCount = 3; descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty())
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU convolution bindings unavailable")};
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        wgpuComputePassEncoderSetPipeline(pass, convolutionPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (outputArea.width() + 7) / 8, (outputArea.height() + 7) / 8, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        output->mayContainHiddenRgb = true;
        result.insert(coordinate, output); pins.append(output);
    }
    return {RenderOutcome::Recorded, std::move(result), {}};
}
}
