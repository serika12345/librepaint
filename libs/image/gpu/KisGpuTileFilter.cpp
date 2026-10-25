/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
// Must match the FilterParameters uniform layout in filter.wgsl: three
// vec4<i32> words, binary32 luma, original binary64 coefficients, and the
// profile/control-only YUV constants as low/high binary64 words.
struct FilterParameters
{
    qint32 tile[4];
    qint32 selection[4];
    qint32 adjustment[4];
    float luma[4];
    quint32 lumaLow[4];
    quint32 lumaHigh[4];
    quint32 yuvLow[2][4];
    quint32 yuvHigh[2][4];
};
static_assert(sizeof(FilterParameters) == 160,
              "filter parameter layout must match filter.wgsl");
}

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
    // These constants depend only on the frozen profile and sliders. Pixel
    // conversion and its ordered qreal rounding stay entirely on the GPU.
    std::array<double, 7> yuv{};
    if (filter.kind == FilterKind::HsvAdjustment &&
        filter.hsvType == HsvAdjustmentType::Yuv && !filter.colorize) {
        const double negR = 1.0 - filter.luma[0];
        const double negB = 1.0 - filter.luma[2];
        yuv = {negR, negB,
               (filter.luma[2] * negB) / (0.5 * filter.luma[1]),
               (filter.luma[0] * negR) / (0.5 * filter.luma[1]),
               1.0 + double(filter.hue) / 100.0,
               1.0 + double(filter.saturation) / 100.0,
               double(filter.value) / 100.0};
    }
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
        // The adjustment word carries the frozen product values for the HSV
        // kind and zero for the other kinds. Colorize and the legacy flag
        // share the high bits of the mode word. Colorize and YUV consume
        // original coefficients; the other rules use binary32 conversions.
        qint32 adjustment = 0;
        FilterParameters parameterValues{};
        if (filter.kind == FilterKind::HsvAdjustment) {
            adjustment = qint32(filter.hsvType) |
                (filter.compatibilityMode ? 0x100 : 0) | (filter.colorize ? 0x200 : 0);
            for (qsizetype i = 0; i < 3; ++i) {
                parameterValues.luma[i] = float(filter.luma[i]);
                quint64 bits = 0;
                std::memcpy(&bits, &filter.luma[i], sizeof(bits));
                parameterValues.lumaLow[i] = quint32(bits);
                parameterValues.lumaHigh[i] = quint32(bits >> 32);
            }
            for (qsizetype i = 0; i < qsizetype(yuv.size()); ++i) {
                quint64 bits = 0;
                std::memcpy(&bits, &yuv[i], sizeof(bits));
                parameterValues.yuvLow[i / 4][i % 4] = quint32(bits);
                parameterValues.yuvHigh[i / 4][i % 4] = quint32(bits >> 32);
            }
        }
        const qint32 values[]{origin.x(), origin.y(), qint32(filter.kind), qint32(flags),
                              filter.selectionRect.x(), filter.selectionRect.y(),
                              filter.selectionRect.width(), filter.selectionRect.height(),
                              filter.hue, filter.saturation, filter.value, adjustment};
        std::memcpy(parameterValues.tile, values, sizeof(values));
        auto parameters = createBuffer(device->nativeDevice(), sizeof(parameterValues),
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
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0,
                             &parameterValues, sizeof(parameterValues));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter commands failed")};
        wgpuComputePassEncoderSetPipeline(pass, filterPipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        // Filtering RGB independently of alpha can produce hidden color under
        // alpha zero (for example complemented black); the projection already
        // normalizes such tiles when the flag is set.
        output->mayContainHiddenRgb = true;
        result.insert(coordinate, output);
        pins.append(input);
        pins.append(output);
    }
    return {RenderOutcome::Recorded, std::move(result), {}};
}
}
