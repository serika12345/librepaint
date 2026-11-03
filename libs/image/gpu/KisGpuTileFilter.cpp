/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include "KisGpuLayerAllocation_p.h"
#include <algorithm>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
// Must match the native f32 FilterParameters uniform layout in filter.wgsl.
struct FilterParameters
{
    qint32 tile[4];
    qint32 selection[4];
    qint32 adjustment[4];
    float luma[4];
    float yuv[2][4];
    quint32 overlayColor[4];
    quint32 control[4];
    qint32 applyRect[4];
};
static_assert(sizeof(FilterParameters) == 144,
              "filter parameter layout must match filter.wgsl");
}

TileRenderer::Private::FilterResult TileRenderer::Private::filterTiles(
    WGPUCommandEncoder commands, const Tiles &source, const FilterOperation &filter,
    const std::shared_ptr<Tile> &fallback, QRect implicitBounds, PixelFormat format, QRect region,
    QList<std::shared_ptr<Tile>> &pins, bool transparency)
{
    retainTiles(source);
    retainTile(fallback);
    Tiles result = source;
    const bool convolution = isConvolutionFilter(filter.kind) && !transparency;
    auto implicit = fallback;
    QRect bounds = region;
    const bool masked = filter.selectionKind == SelectionKind::Freehand;
    if (filter.bounded || filter.kind >= FilterKind::MaximizeChannel) bounds &= filter.applyRect;
    if (!transparency && !filter.selectionRect.isEmpty() && !filter.selectionInverted &&
        (!masked || (filter.selectionMask && !filter.selectionMask->defaultCoverage())))
        bounds = bounds.intersected(filter.selectionRect);
    if (bounds.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    if (masked && !filter.selectionMask)
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU filter freehand selection has no coverage")};
    if (masked && filter.selectionMask->isEmpty() && !transparency)
        return {RenderOutcome::Recorded, std::move(result), {}};
    // Existing content is the layer's painted extent. A non-transparent default
    // pixel extends that extent over the canvas, so those tiles are synthesized.
    QList<quint64> planned;
    const qsizetype maximumPlanned = budget->limit / RgbaTile::byteCount(format) / (convolution ? 2 : 1);
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile) {
        const QPoint coordinate = Detail::coordinate(tile.key());
        const QRect tileRect(coordinate * TileSize, QSize(TileSize, TileSize));
        if (!tileRect.intersects(bounds)) continue;
        if (!transparency && masked && !filter.selectionInverted && !filter.selectionMask->defaultCoverage() &&
            !filter.selectionMask->isGpuGenerated() && !filter.selectionMask->tiles().contains(SelectionTileKey(coordinate.x(), coordinate.y())))
            continue;
        if (planned.size() >= maximumPlanned)
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU filter tile plan exceeds working capacity")};
        planned.append(tile.key());
    }
    if (convolution && !implicit) {
        if (!ensureTileCapacity(1, format))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU convolution default exceeds working capacity")};
        implicit = allocateTile(format);
        if (!implicit) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU convolution default allocation failed")};
        retainTile(implicit);
        wgpuCommandEncoderClearBuffer(commands, implicit->pixels.get(), 0, RgbaTile::byteCount(format));
        pins.append(implicit);
    }
    if ((fallback && !implicitBounds.isEmpty()) || convolution) {
        const QRect area = convolution ? bounds : bounds.intersected(implicitBounds);
        if (!area.isEmpty()) {
            if (!transparency && masked && !filter.selectionInverted && !filter.selectionMask->defaultCoverage() &&
                !filter.selectionMask->isGpuGenerated()) {
                for (auto tile = filter.selectionMask->tiles().cbegin(); tile != filter.selectionMask->tiles().cend(); ++tile) {
                    const QPoint point(tile.key().first, tile.key().second);
                    const auto coordinate = Detail::key(point.x(), point.y());
                    if (source.contains(coordinate) || !area.intersects(QRect(point * TileSize, QSize(TileSize, TileSize)))) continue;
                    if (planned.size() >= maximumPlanned)
                        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU filter tile plan exceeds working capacity")};
                    planned.append(coordinate);
                }
            } else {
                const int left = pixelTileIndex(area.left(), TileSize);
                const int top = pixelTileIndex(area.top(), TileSize);
                const int right = pixelTileIndex(area.right(), TileSize);
                const int bottom = pixelTileIndex(area.bottom(), TileSize);
                if (qint64(right - left + 1) * (bottom - top + 1) > maximumPlanned)
                    return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU filter tile plan exceeds working capacity")};
                for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
                    const auto coordinate = Detail::key(x, y);
                    if (source.contains(coordinate)) continue;
                    planned.append(coordinate);
                }
            }
        }
    }
    if (planned.isEmpty()) return {RenderOutcome::Recorded, std::move(result), {}};
    Tiles convolved;
    if (convolution) {
        const auto repeat = filter.samplingBounds.isEmpty() ? implicitBounds.united(filter.applyRect) : filter.samplingBounds;
        const auto processed = convolutionTiles(commands, source, filter, implicit, repeat, format, bounds, planned, pins);
        if (processed.code != RenderOutcome::Recorded) return processed;
        convolved = processed.tiles;
    }
    const auto mode = filter.kind == FilterKind::ColorOverlay ? filter.overlay.blendMode : LayerBlendMode::Normal;
    const bool wide = format == PixelFormat::Rgba16;
    if (!filterLayout) {
        WGPUBindGroupLayoutEntry entries[5]{};
        for (quint32 i = 0; i < 5; ++i) {
            entries[i].binding = i;
            entries[i].visibility = WGPUShaderStage_Compute;
            entries[i].buffer.type = i == 2 ? WGPUBufferBindingType_Uniform :
                i == 1 ? WGPUBufferBindingType_Storage : WGPUBufferBindingType_ReadOnlyStorage;
        }
        WGPUBindGroupLayoutDescriptor descriptor{};
        descriptor.entryCount = 5;
        descriptor.entries = entries;
        filterLayout.reset(wgpuDeviceCreateBindGroupLayout(device->nativeDevice(), &descriptor));
    }
    auto &pipeline = filterPipelines[quint32(mode) | (wide ? 256u : 0u) | (transparency ? 512u : 0u)];
    if (!pipeline && filterLayout)
        pipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/filter.wgsl"), transparency ? "maskMain" : "main",
                                  filterLayout.get(), quint32(mode), 0, wide ? 2u : 1u);
    if (!pipeline || !filterLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter pipeline creation failed: ") + error};
    quint32 flags = 1u;
    if (masked) flags |= 2u;
    else if (filter.selectionKind == SelectionKind::Ellipse) flags |= 4u;
    if (filter.selectionInverted) flags |= 8u;
    if (filter.selectionRect.isEmpty() && !masked) flags = 0u;
    if (filter.bounded) flags |= 16u;
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
        const auto input = source.value(coordinate, implicit);
        if (!input) continue;
        if (!ensureTileCapacity(1, format))
            return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU filter tile budget exhausted")};
        const auto output = allocateTile(format);
        if (!output) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter allocation failed")};
        retainTile(output);
        std::shared_ptr<CoverageTile> coverage;
        if (masked && !filter.selectionMask->isEmpty()) {
            coverage = coverageTile(commands, filter.selectionMask, coordinate);
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
        parameterValues.applyRect[0] = filter.applyRect.x();
        parameterValues.applyRect[1] = filter.applyRect.y();
        parameterValues.applyRect[2] = filter.applyRect.width();
        parameterValues.applyRect[3] = filter.applyRect.height();
        for (qsizetype i = 0; i < 4; ++i) parameterValues.overlayColor[i] = filter.overlay.color[i];
        const float opacity = float(filter.overlay.opacity) / 100.0f;
        parameterValues.control[0] = wide ? 1u : 0u;
        parameterValues.control[3] = quint32(mode);
        std::memcpy(&parameterValues.control[1], &opacity, sizeof(opacity));
        parameterValues.control[2] = quint32(opacity * maximumComponent(format) + 0.5f);
        if (filter.kind == FilterKind::HsvAdjustment) {
            adjustment = qint32(filter.hsvType) |
                (filter.compatibilityMode ? 0x100 : 0) | (filter.colorize ? 0x200 : 0);
            for (qsizetype i = 0; i < 3; ++i)
                parameterValues.luma[i] = float(filter.luma[i]);
            for (qsizetype i = 0; i < qsizetype(yuv.size()); ++i)
                parameterValues.yuv[i / 4][i % 4] = float(yuv[i]);
        }
        const qint32 values[]{origin.x(), origin.y(), qint32(filter.kind), qint32(flags),
                              filter.selectionRect.x(), filter.selectionRect.y(),
                              filter.selectionRect.width(), filter.selectionRect.height(),
                              filter.hue, filter.saturation, filter.value, adjustment};
        std::memcpy(parameterValues.tile, values, sizeof(values));
        auto parameters = createBuffer(device->nativeDevice(), sizeof(parameterValues),
                                       WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!parameters) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter parameter allocation failed")};
        WGPUBindGroupEntry entries[5]{};
        const auto processed = convolved.value(coordinate);
        const WGPUBuffer buffers[]{input->pixels.get(), output->pixels.get(), parameters.get(),
                                   coverage ? coverage->pixels.get() : unusedSelection.get(),
                                   processed ? processed->pixels.get() : unusedPattern.get()};
        for (quint32 i = 0; i < 5; ++i) {
            entries[i].binding = i;
            entries[i].buffer = buffers[i];
            entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = filterLayout.get();
        descriptor.entryCount = 5;
        descriptor.entries = entries;
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings || !device->diagnostic().isEmpty())
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter binding failed")};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0,
                             &parameterValues, sizeof(parameterValues));
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU filter commands failed")};
        wgpuComputePassEncoderSetPipeline(pass, pipeline.get());
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

TileRenderer::Private::FilterResult TileRenderer::Private::applyLayerMasks(
    WGPUCommandEncoder commands, const Tiles &source, const std::shared_ptr<Tile> &fallback,
    const Layer &layer, const Snapshot &document, QRect region, QRect nativeOutput, QList<std::shared_ptr<Tile>> &pins)
{
    QList<const Layer *> filters;
    for (const auto &child : document->layers)
        if (child.kind == LayerKind::FilterMask && child.parentUuid == layer.uuid) filters.append(&child);
    const auto regions = filterMaskRegions(document, layer, region);
    const auto nativeRegions = filterMaskRegions(document, layer, nativeOutput, 3);
    QRect defaultBounds = region;
    if (std::any_of(filters.cbegin(), filters.cend(), [](const auto *node) {
        return node->visible && node->filter && isConvolutionFilter(node->filter->settings.kind);
    })) {
        defaultBounds = QRect(QPoint(), document->size);
    }
    Tiles tiles = source;
    const auto apply = [&](const FilterSettings &settings, const SelectionMaskSP &mask,
                           bool transparency, QRect output, QRect nativeArea) -> FilterResult {
        FilterOperation operation;
        static_cast<FilterSettings &>(operation) = settings;
        operation.layer = layer.id;
        operation.applyRect = output;
        operation.samplingBounds = defaultBounds.united(nativeArea);
        operation.bounded = true;
        if (mask) {
            operation.selectionKind = SelectionKind::Freehand;
            operation.selectionMask = mask;
            operation.selectionRect = mask->bounds();
        }
        return filterTiles(commands, tiles, operation, fallback, defaultBounds, document->format, output, pins, transparency);
    };
    for (qsizetype i = 0; i <= filters.size(); ++i) {
        if (i == layer.transparencyMaskIndex && hasEnabledTransparencyMask(layer)) {
            const auto masked = apply(FilterSettings{}, layer.transparencyMask, true, regions[i], nativeRegions[i]);
            if (masked.code != RenderOutcome::Recorded) return masked;
            tiles = masked.tiles;
        }
        if (i == filters.size()) break;
        const auto &node = *filters[i];
        if (!node.visible) continue;
        if (!node.filter || !node.filter->isValid(document->format))
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU filter mask configuration is unavailable")};
        const auto filtered = apply(node.filter->settings, node.filterSelection, false, regions[i + 1], nativeRegions[i + 1]);
        if (filtered.code != RenderOutcome::Recorded) return filtered;
        tiles = filtered.tiles;
    }
    return {RenderOutcome::Recorded, std::move(tiles), {}};
}

}
