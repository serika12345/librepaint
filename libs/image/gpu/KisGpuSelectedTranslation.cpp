/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
std::array<quint32, 20> parameters(QPoint origin, PixelFormat format, quint32 flags,
                                  QRect copy, QRect clear, const RgbaPixel &implicit, const RgbaPixel &background)
{
    return {quint32(origin.x()), quint32(origin.y()), quint32(format), flags,
        quint32(copy.left()), quint32(copy.top()), quint32(copy.x() + copy.width()), quint32(copy.y() + copy.height()),
        quint32(clear.left()), quint32(clear.top()), quint32(clear.x() + clear.width()), quint32(clear.y() + clear.height()),
        implicit[0], implicit[1], implicit[2], implicit[3], background[0], background[1], background[2], background[3]};
}
}

TileRenderer::Private::SelectionSplitResult TileRenderer::Private::splitSelectionTile(
    WGPUCommandEncoder commands, const TileView &source, const RgbaPixel &implicit, const RgbaPixel &defaultPixel,
    PixelFormat format, const TranslationOperation &operation)
{
    const QPoint origin = source.coordinate * TileSize;
    const QRect tileBounds(origin, QSize(TileSize, TileSize));
    auto selection = operation.selectionMask;
    if (operation.selectionKind != SelectionKind::Freehand) {
        selection = operation.selectionKind == SelectionKind::Ellipse ?
            SelectionMask::fromEllipse(operation.selectionRect, tileBounds) :
            SelectionMask::fromRectangle(operation.selectionRect, tileBounds);
        if (!selection) selection = SelectionMask::empty();
    }
    if (!selection) return {RenderOutcome::InvalidVersion, {}, {}, {}, QStringLiteral("The selected move has no frozen coverage")};
    auto coverage = coverageTile(selection, key(source.coordinate.x(), source.coordinate.y()));
    if (!coverage || !ensureTileCapacity(2, format))
        return {RenderOutcome::CapacityExceeded, {}, {}, {}, QStringLiteral("GPU selection split budget exhausted")};
    if (!selectionSplitPipeline) {
        selectionSplitPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/selection-move.wgsl"), "split");
        if (selectionSplitPipeline)
            selectionSplitLayout.reset(wgpuComputePipelineGetBindGroupLayout(selectionSplitPipeline.get(), 0));
    }
    if (!selectionSplitPipeline || !selectionSplitLayout)
        return {RenderOutcome::DeviceFailure, {}, {}, {}, QStringLiteral("GPU selection split pipeline creation failed")};
    const auto copied = allocateTile(format), cleared = allocateTile(format);
    const auto values = parameters(origin, format, quint32(operation.selectionInverted) | (source.pixels ? 0u : 2u),
                                   operation.copyRect, operation.clearRect, implicit, defaultPixel);
    auto uniform = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!copied || !cleared || !uniform)
        return {RenderOutcome::DeviceFailure, {}, {}, {}, QStringLiteral("GPU selection split allocation failed")};
    WGPUBindGroupEntry entries[5]{};
    const WGPUBuffer buffers[]{source.pixels ? source.pixels : unusedPattern.get(), copied->pixels.get(),
                               cleared->pixels.get(), uniform.get(), coverage->pixels.get()};
    const quint32 bindings[]{0, 2, 3, 4, 5};
    for (quint32 i = 0; i < 5; ++i) {
        entries[i].binding = bindings[i]; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = selectionSplitLayout.get(); descriptor.entryCount = 5; descriptor.entries = entries;
    NativeBindings group(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!group || !device->diagnostic().isEmpty())
        return {RenderOutcome::DeviceFailure, {}, {}, {}, QStringLiteral("GPU selection split binding failed")};
    wgpuQueueWriteBuffer(device->queue(), uniform.get(), 0, values.data(), sizeof(values));
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, selectionSplitPipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, group.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return {RenderOutcome::Recorded, copied, cleared, coverage, {}};
}

TileRenderer::Private::SelectionCopyResult TileRenderer::Private::mergeSelectionTile(
    WGPUCommandEncoder commands, const TileView &source, const TileView &base, const RgbaPixel &implicit, PixelFormat format)
{
    if (!ensureTileCapacity(1, format))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU selected move merge budget exhausted")};
    if (!selectionMergePipeline) {
        selectionMergePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/selection-move.wgsl"), "merge");
        if (selectionMergePipeline)
            selectionMergeLayout.reset(wgpuComputePipelineGetBindGroupLayout(selectionMergePipeline.get(), 0));
    }
    if (!selectionMergePipeline || !selectionMergeLayout)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selected move merge pipeline creation failed")};
    const auto output = allocateTile(format);
    const auto values = parameters(source.coordinate * TileSize, format,
        (source.pixels ? 0u : 2u) | (base.pixels ? 0u : 4u), {}, {}, implicit, {});
    auto uniform = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!output || !uniform)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selected move merge allocation failed")};
    WGPUBindGroupEntry entries[4]{};
    const WGPUBuffer buffers[]{source.pixels ? source.pixels : unusedPattern.get(), base.pixels ? base.pixels : unusedPattern.get(),
                               output->pixels.get(), uniform.get()};
    const quint32 bindings[]{0, 1, 2, 4};
    for (quint32 i = 0; i < 4; ++i) {
        entries[i].binding = bindings[i]; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = selectionMergeLayout.get(); descriptor.entryCount = 4; descriptor.entries = entries;
    NativeBindings group(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!group || !device->diagnostic().isEmpty())
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU selected move merge binding failed")};
    wgpuQueueWriteBuffer(device->queue(), uniform.get(), 0, values.data(), sizeof(values));
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, selectionMergePipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, group.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return {RenderOutcome::Recorded, output, {}};
}

TileRenderer::Private::TranslationResult TileRenderer::Private::translateSelection(
    WGPUCommandEncoder commands, const Tiles &source, const RgbaPixel &implicit, const RgbaPixel &defaultPixel,
    PixelFormat format, const TranslationOperation &operation, QRect region, QList<std::shared_ptr<Tile>> &pins,
    QList<std::shared_ptr<CoverageTile>> &coveragePins)
{
    retainTiles(source);
    const QRect needed = region.united(region.translated(-operation.offset).intersected(operation.copyRect));
    QSet<quint64> coordinates;
    for (auto tile = source.cbegin(); tile != source.cend(); ++tile)
        if (needed.intersects(QRect(coordinate(tile.key()) * TileSize, QSize(TileSize, TileSize)))) coordinates.insert(tile.key());
    const auto appendImplicit = [&](QRect area) {
        if (area.isEmpty() || !implicit[3]) return true;
        const int left = pixelTileIndex(area.left(), TileSize), right = pixelTileIndex(area.right(), TileSize);
        const int top = pixelTileIndex(area.top(), TileSize), bottom = pixelTileIndex(area.bottom(), TileSize);
        const auto maximum = budget->limit / RgbaTile::byteCount(format);
        if (qint64(right - left + 1) * (bottom - top + 1) > maximum) return false;
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
            coordinates.insert(key(x, y));
            if (coordinates.size() > maximum) return false;
        }
        return true;
    };
    if (!appendImplicit(operation.copyRect.intersected(region.translated(-operation.offset))) ||
        !appendImplicit(operation.clearRect.intersected(region)))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("Selected move implicit coordinates exceed capacity")};
    Tiles captured, remainder;
    for (const auto point : coordinates) {
        const auto input = source.value(point);
        if (input) pins.append(input);
        const auto split = splitSelectionTile(commands, {coordinate(point), input ? input->pixels.get() : nullptr},
                                              implicit, defaultPixel, format, operation);
        if (split.code != RenderOutcome::Recorded) return {split.code, {}, split.diagnostic};
        captured.insert(point, split.copied);
        if (region.intersects(QRect(coordinate(point) * TileSize, QSize(TileSize, TileSize)))) remainder.insert(point, split.cleared);
        pins.append(split.copied); pins.append(split.cleared); coveragePins.append(split.coverage);
    }
    auto moved = translateTiles(commands, captured, {}, operation.offset, format, region, pins);
    if (moved.code != RenderOutcome::Recorded) return moved;
    for (auto tile = moved.tiles.cbegin(); tile != moved.tiles.cend(); ++tile) {
        const auto background = remainder.value(tile.key());
        const auto merged = mergeSelectionTile(commands, {coordinate(tile.key()), tile.value()->pixels.get()},
            {coordinate(tile.key()), background ? background->pixels.get() : nullptr}, implicit, format);
        if (merged.code != RenderOutcome::Recorded) return {merged.code, {}, merged.diagnostic};
        remainder.insert(tile.key(), merged.tile);
        pins.append(merged.tile);
    }
    return {RenderOutcome::Recorded, std::move(remainder), {}};
}
}
