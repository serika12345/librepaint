/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include "KisGpuLayerAllocation_p.h"
#include <QScopeGuard>
#include <array>
#include <algorithm>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;

namespace {
int tileIndex(int value)
{ return value / TileRenderer::TileSize - (value % TileRenderer::TileSize < 0); }

bool appendTiles(QSet<QPoint> &tiles, QRect area)
{
    if (area.isEmpty()) return true;
    const int left = tileIndex(area.left()), right = tileIndex(area.right());
    const int top = tileIndex(area.top()), bottom = tileIndex(area.bottom());
    constexpr int maximum = SelectionMask::MaximumBytes / SelectionMask::AlphaTileBytes;
    if (qint64(right - left + 1) * (bottom - top + 1) > maximum) return false;
    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
        tiles.insert(QPoint(x, y));
        if (tiles.size() > maximum) return false;
    }
    return true;
}

QList<QPoint> sortedTiles(const QSet<QPoint> &tiles)
{
    auto result = tiles.values();
    std::sort(result.begin(), result.end(), [](QPoint a, QPoint b) {
        return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
    });
    return result;
}

void mergeRuns(QList<QRect> &runs)
{
    std::sort(runs.begin(), runs.end(), [](const QRect &a, const QRect &b) {
        return a.y() == b.y() ? a.x() < b.x() : a.y() < b.y();
    });
    QList<QRect> merged;
    for (const auto &run : runs) {
        if (!merged.isEmpty() && merged.last().y() == run.y() && merged.last().right() + 1 >= run.left())
            merged.last() = merged.last().united(run);
        else merged.append(run);
    }
    runs = std::move(merged);
}
}

std::shared_ptr<CoverageTile> TileRenderer::Private::packSelectionTile(WGPUCommandEncoder commands,
    WGPUBuffer source, QRect sourceExtent, QPoint point, QRect bounds, quint32 format, quint8 constant)
{
    if (!device->ownsCommands(commands)) return {};
    const qint32 values[12]{sourceExtent.x(), sourceExtent.y(), sourceExtent.width(), sourceExtent.height(),
        point.x() * TileSize, point.y() * TileSize, qint32(format), constant,
        bounds.left(), bounds.top(), bounds.right() + 1, bounds.bottom() + 1};
    constexpr qsizetype required = SelectionMask::AlphaTileBytes + sizeof(values);
    if (required > budget->limit - budget->used) {
        clearMaskTiles();
        if (required > budget->limit - budget->used) return {};
    }
    auto pixels = createBuffer(device->nativeDevice(), SelectionMask::AlphaTileBytes,
                               WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!pixels || !parameters || !device->diagnostic().isEmpty()) return {};
    auto uniform = std::make_shared<BufferStorage>(std::move(parameters), budget);
    auto output = std::make_shared<CoverageTile>(std::move(pixels), budget);
    device->retainForFrame(uniform); device->retainForFrame(output->storage);
    wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, values, sizeof(values));
    if (!coveragePackPipeline) {
        coveragePackPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/coverage-pack.wgsl"));
        if (coveragePackPipeline) coveragePackLayout.reset(wgpuComputePipelineGetBindGroupLayout(coveragePackPipeline.get(), 0));
    }
    if (!coveragePackPipeline || !coveragePackLayout) return {};
    const WGPUBuffer buffers[]{uniform->pixels.get(), source ? source : unusedCoverage.get(), output->pixels.get()};
    std::array<WGPUBindGroupEntry, 3> entries{};
    for (quint32 index = 0; index < entries.size(); ++index) {
        entries[index].binding = index; entries[index].buffer = buffers[index];
        entries[index].size = wgpuBufferGetSize(buffers[index]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = coveragePackLayout.get(); descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings) return {};
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    if (!pass) return {};
    wgpuComputePassEncoderSetPipeline(pass, coveragePackPipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, SelectionMask::AlphaTileBytes / 4 / 64, 1, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return output;
}

std::shared_ptr<CoverageTile> TileRenderer::Private::translatedCoverageTile(WGPUCommandEncoder commands,
    const SelectionTranslationSource &source, QPoint point)
{
    const QPoint origin = point * TileSize - source.offset;
    const QPoint base(tileIndex(origin.x()), tileIndex(origin.y()));
    std::array<std::shared_ptr<CoverageTile>, 4> inputs;
    for (int index = 0; index < 4; ++index) {
        const auto inputPoint = base + QPoint(index % 2, index / 2);
        inputs[index] = coverageTile(commands, source.source, key(inputPoint.x(), inputPoint.y()));
        if (!inputs[index]) return {};
        device->retainForFrame(inputs[index]->storage);
    }
    constexpr qsizetype required = SelectionMask::AlphaTileBytes + 64;
    if (required > budget->limit - budget->used) return {};
    auto pixels = createBuffer(device->nativeDevice(), SelectionMask::AlphaTileBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    auto parameters = createBuffer(device->nativeDevice(), 64, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!pixels || !parameters) return {};
    const auto output = std::make_shared<CoverageTile>(std::move(pixels), budget);
    const auto uniform = std::make_shared<BufferStorage>(std::move(parameters), budget);
    device->retainForFrame(output->storage); device->retainForFrame(uniform);
    const auto bounds = source.source->bounds();
    const qint32 values[16]{origin.x(), origin.y(), base.x() * TileSize, base.y() * TileSize,
        source.extent.left(), source.extent.top(), source.extent.right() + 1, source.extent.bottom() + 1,
        bounds.left(), bounds.top(), bounds.right() + 1, bounds.bottom() + 1,
        point.x() * TileSize, point.y() * TileSize, source.source->defaultCoverage(), 0};
    wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, values, sizeof(values));
    if (!selectionTranslatePipeline) {
        selectionTranslatePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/selection-translate.wgsl"));
        if (selectionTranslatePipeline)
            selectionTranslateLayout.reset(wgpuComputePipelineGetBindGroupLayout(selectionTranslatePipeline.get(), 0));
    }
    if (!selectionTranslatePipeline || !selectionTranslateLayout) return {};
    const WGPUBuffer buffers[]{uniform->pixels.get(), inputs[0]->pixels.get(), inputs[1]->pixels.get(),
        inputs[2]->pixels.get(), inputs[3]->pixels.get(), output->pixels.get()};
    std::array<WGPUBindGroupEntry, 6> entries{};
    for (quint32 index = 0; index < entries.size(); ++index) {
        entries[index].binding = index; entries[index].buffer = buffers[index];
        entries[index].size = wgpuBufferGetSize(buffers[index]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = selectionTranslateLayout.get(); descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings) return {};
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    if (!pass) return {};
    wgpuComputePassEncoderSetPipeline(pass, selectionTranslatePipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, SelectionMask::AlphaTileBytes / 4 / 64, 1, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return output;
}

std::shared_ptr<CoverageTile> TileRenderer::Private::combinedCoverageTile(WGPUCommandEncoder commands,
    const SelectionCombinationSource &source, QPoint point)
{
    const auto first = coverageTile(commands, source.first, key(point.x(), point.y()));
    const auto second = coverageTile(commands, source.second, key(point.x(), point.y()));
    if (!first || !second || SelectionMask::AlphaTileBytes + 32 > budget->limit - budget->used) return {};
    device->retainForFrame(first->storage); device->retainForFrame(second->storage);
    auto pixels = createBuffer(device->nativeDevice(), SelectionMask::AlphaTileBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    auto parameters = createBuffer(device->nativeDevice(), 32, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!pixels || !parameters) return {};
    const auto output = std::make_shared<CoverageTile>(std::move(pixels), budget);
    const auto uniform = std::make_shared<BufferStorage>(std::move(parameters), budget);
    device->retainForFrame(output->storage); device->retainForFrame(uniform);
    const auto action = source.pixels == SelectionCombinationSource::Pixels::Selection ? qint32(source.action) : 7;
    const qint32 values[8]{source.extent.x(), source.extent.y(), source.extent.width(), source.extent.height(),
        point.x() * TileSize, point.y() * TileSize, action, qint32(source.inverted) | (qint32(source.defaultCoverage()) << 8)};
    wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, values, sizeof(values));
    if (!selectionCombinePipeline) {
        selectionCombinePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/selection-combine.wgsl"));
        if (selectionCombinePipeline)
            selectionCombineLayout.reset(wgpuComputePipelineGetBindGroupLayout(selectionCombinePipeline.get(), 0));
    }
    if (!selectionCombinePipeline || !selectionCombineLayout) return {};
    const WGPUBuffer buffers[]{uniform->pixels.get(), first->pixels.get(), second->pixels.get(), output->pixels.get()};
    std::array<WGPUBindGroupEntry, 4> entries{};
    for (quint32 index = 0; index < entries.size(); ++index) {
        entries[index].binding = index; entries[index].buffer = buffers[index];
        entries[index].size = wgpuBufferGetSize(buffers[index]);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = selectionCombineLayout.get(); descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!bindings) return {};
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    if (!pass) return {};
    wgpuComputePassEncoderSetPipeline(pass, selectionCombinePipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, SelectionMask::AlphaTileBytes / 4 / 64, 1, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    return output;
}

bool TileRenderer::Private::ensureSelectionTiles(WGPUCommandEncoder commands, const SelectionMaskSP &mask)
{
    if (!mask || !mask->isGpuGenerated() || mask->regionSource() || !device->ownsCommands(commands)) return false;
    const auto native = mask->nativePixels();
    if (!native) return false;
    { const std::lock_guard<std::mutex> lock(native->mutex);
      if (native->device.lock() == device && native->tilesReady) return true; }
    QMap<quint64, std::shared_ptr<BufferStorage>> tiles;
    if (const auto &transform = mask->transformSource()) {
        if (!transformMaskTiles(commands, *transform, &tiles)) return false;
    } else if (const auto &paint = mask->paintSource()) {
        const auto source = renderer.record(commands, paint->source);
        if (source.code != RenderOutcome::Recorded) { error = source.diagnostic; return false; }
        const auto layers = source.version->m_data->layers.value(paint->layer);
        for (const auto point : paint->tiles) {
            const auto raw = layers.value(key(point.x(), point.y()));
            if (!raw) { error = QStringLiteral("Painted mask replay lost an occupied source tile"); return false; }
            retainTile(raw);
            const auto packed = packSelectionTile(commands, raw->pixels.get(), QRect(point * TileSize, QSize(TileSize, TileSize)),
                                                   point, mask->bounds(), 1, mask->defaultCoverage());
            if (!packed) return false;
            tiles.insert(key(point.x(), point.y()), packed->storage);
        }
    } else {
        for (const auto point : mask->tileCoordinates()) {
            const auto output = mask->translationSource() ? translatedCoverageTile(commands, *mask->translationSource(), point) :
                mask->combinationSource() ? combinedCoverageTile(commands, *mask->combinationSource(), point) : nullptr;
            if (!output) return false;
            tiles.insert(key(point.x(), point.y()), output->storage);
        }
    }
    { const std::lock_guard<std::mutex> lock(native->mutex);
      native->device = device; native->tiles = std::move(tiles); native->tilesReady = true; }
    return true;
}

bool TileRenderer::Private::transformMaskTiles(WGPUCommandEncoder commands, const SelectionMaskTransformSource &source,
    QMap<quint64, std::shared_ptr<BufferStorage>> *tiles, Tiles *rgba)
{
    if (!maskCapturePipeline) {
        maskCapturePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/mask-affine.wgsl"), "capture");
        if (maskCapturePipeline) maskCaptureLayout.reset(wgpuComputePipelineGetBindGroupLayout(maskCapturePipeline.get(), 0));
    }
    if (!maskMergePipeline) {
        maskMergePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/mask-affine.wgsl"), "merge");
        if (maskMergePipeline) maskMergeLayout.reset(wgpuComputePipelineGetBindGroupLayout(maskMergePipeline.get(), 0));
    }
    if (!maskCapturePipeline || !maskMergePipeline || !maskCaptureLayout || !maskMergeLayout) return false;
    const auto empty = SelectionMask::empty();
    const auto record = [&](QPoint point, bool capture, WGPUBuffer rgba, WGPUBuffer output, bool hasTransformed) {
        const auto gray = coverageTile(commands, source.source, key(point.x(), point.y()));
        const auto selected = coverageTile(commands, source.selection ? source.selection : empty, key(point.x(), point.y()));
        if (!gray || !selected || 64 > budget->limit - budget->used) return false;
        auto parameters = createBuffer(device->nativeDevice(), 64, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        if (!parameters) return false;
        auto uniform = std::make_shared<BufferStorage>(std::move(parameters), budget);
        device->retainForFrame(uniform); device->retainForFrame(gray->storage); device->retainForFrame(selected->storage);
        const qint32 values[16]{point.x() * TileSize, point.y() * TileSize, qint32(source.inverted), bool(source.selection),
            source.copyRect.left(), source.copyRect.top(), source.copyRect.x() + source.copyRect.width(), source.copyRect.y() + source.copyRect.height(),
            source.output.left(), source.output.top(), source.output.x() + source.output.width(), source.output.y() + source.output.height(),
            source.source->defaultCoverage(), hasTransformed, 0, 0};
        wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, values, sizeof(values));
        const WGPUBuffer buffers[]{uniform->pixels.get(), gray->pixels.get(), selected->pixels.get(), rgba, output};
        std::array<WGPUBindGroupEntry, 5> entries{};
        const quint32 count = capture ? 4 : 5;
        for (quint32 i = 0; i < count; ++i) {
            entries[i].binding = i < 3 || capture ? i : i + 1;
            entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{};
        descriptor.layout = capture ? maskCaptureLayout.get() : maskMergeLayout.get();
        descriptor.entryCount = count; descriptor.entries = entries.data();
        NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
        if (!bindings) return false;
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return false;
        wgpuComputePassEncoderSetPipeline(pass, capture ? maskCapturePipeline.get() : maskMergePipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (capture ? 65536 : 16384) / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        return true;
    };
    QRect input = source.output;
    for (qsizetype i = source.plan->passes.size(); i-- > 0;) input = affineNeededRegion(source.plan->passes[i], input);
    if (source.selection) input &= source.copyRect;
    QSet<QPoint> coordinates;
    if (!appendTiles(coordinates, input)) { error = QStringLiteral("Mask transform input exceeds tile capacity"); return false; }
    if (!ensureTileCapacity(coordinates.size(), PixelFormat::Rgba8)) return false;
    Tiles captured;
    for (const auto point : sortedTiles(coordinates)) {
        auto tile = allocateTile(PixelFormat::Rgba8);
        if (!tile || !record(point, true, tile->pixels.get(), nullptr, false)) return false;
        captured.insert(key(point.x(), point.y()), tile); retainTile(tile);
    }
    const quint16 gray = source.source->defaultCoverage();
    const RgbaPixel implicit = source.selection ? RgbaPixel{} : RgbaPixel{gray, gray, gray, 255};
    const auto transformed = affineTiles(commands, captured, implicit, PixelFormat::Rgba8, *source.plan, source.output);
    if (transformed.code != RenderOutcome::Recorded) { error = transformed.diagnostic; return false; }
    retainTiles(transformed.tiles);
    if (rgba) { *rgba = transformed.tiles; return true; }
    for (const auto point : source.tiles) {
        if (SelectionMask::AlphaTileBytes > budget->limit - budget->used) return false;
        auto pixels = createBuffer(device->nativeDevice(), SelectionMask::AlphaTileBytes,
            WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
        if (!pixels) return false;
        auto coverage = std::make_shared<CoverageTile>(std::move(pixels), budget);
        device->retainForFrame(coverage->storage);
        const auto rgba = transformed.tiles.value(key(point.x(), point.y()));
        if (!record(point, false, rgba ? rgba->pixels.get() : unusedCoverage.get(), coverage->pixels.get(), bool(rgba))) return false;
        tiles->insert(key(point.x(), point.y()), coverage->storage);
    }
    return true;
}

std::shared_ptr<CoverageTile> TileRenderer::Private::generatedCoverageTile(WGPUCommandEncoder commands,
    const SelectionMaskSP &mask, quint64 tileKey)
{
    if (!mask || !mask->isGpuGenerated() || !device->ownsCommands(commands)) return {};
    const auto point = coordinate(tileKey);
    std::shared_ptr<CoverageTile> output;
    if (const auto &region = mask->regionSource()) {
        std::shared_ptr<BufferStorage> pixels;
        if (QRect(point * TileSize, QSize(TileSize, TileSize)).intersects(mask->bounds())) {
            { const auto native = mask->nativePixels(); const std::lock_guard<std::mutex> lock(native->mutex);
              if (native->device.lock() == device) pixels = native->region; }
            if (!pixels) {
                const auto prepared = renderer.prepareRegion(commands, region);
                if (prepared.code != RenderOutcome::Recorded) { error = prepared.diagnostic; return {}; }
                pixels = regionCache.value(region.data()).coverage.pixels;
            }
            if (!pixels) return {};
            { const auto native = mask->nativePixels(); const std::lock_guard<std::mutex> lock(native->mutex);
              native->device = device; native->region = pixels; }
            device->retainForFrame(pixels);
        }
        output = packSelectionTile(commands, pixels ? pixels->pixels.get() : nullptr,
            QRect(region->coverageOrigin, region->coverageSize), point, mask->bounds(), pixels ? 0 : 2);
    } else {
        if (!ensureSelectionTiles(commands, mask)) return {};
        std::shared_ptr<BufferStorage> pixels;
        { const auto native = mask->nativePixels(); const std::lock_guard<std::mutex> lock(native->mutex); pixels = native->tiles.value(tileKey); }
        const QRect defaultExtent = mask->defaultCoverage() ? QRect(point * TileSize, QSize(TileSize, TileSize)) : QRect();
        output = pixels ? std::make_shared<CoverageTile>(pixels, budget) :
            packSelectionTile(commands, nullptr, {}, point, defaultExtent, 2, mask->defaultCoverage());
    }
    if (!output) return {};
    auto &entry = maskTiles[mask.data()]; entry.source = mask; entry.tiles.insert(tileKey, output);
    for (auto old = maskTiles.begin(); maskTiles.size() > 4 && old != maskTiles.end();) {
        if (old.key() != mask.data()) old = maskTiles.erase(old);
        else ++old;
    }
    return output;
}

ExecutionOutcome Execution::readSelectionTileOutlines(const SelectionMaskSP &mask, QRect *bounds,
    QList<QRect> *runs, QList<QPoint> *tiles)
{
    const auto native = mask->nativePixels();
    QMap<quint64, std::shared_ptr<BufferStorage>> pixels;
    { const std::lock_guard<std::mutex> lock(native->mutex); pixels = native->tiles; }
    QMap<quint64, std::shared_ptr<BufferStorage>> occupiedTiles;
    for (auto tile = pixels.cbegin(); tile != pixels.cend(); ++tile) {
        const auto point = coordinate(tile.key());
        QRect occupied;
        QList<QRect> rows;
        const auto geometry = readSelectionOutline(tile.value(),
            QRect(point * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize)),
            {}, &occupied, &rows, 2, mask->defaultCoverage());
        if (geometry.code != ExecutionCode::Success) return geometry;
        if (occupied.isEmpty()) continue;
        if (rows.size() > SelectionMask::MaximumBytes / qsizetype(sizeof(QRect)) - runs->size())
            return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection outline exceeds its metadata capacity")};
        *bounds = bounds->isEmpty() ? occupied : bounds->united(occupied);
        runs->append(rows); tiles->append(point); occupiedTiles.insert(tile.key(), tile.value());
    }
    mergeRuns(*runs);
    { const std::lock_guard<std::mutex> lock(native->mutex); native->tiles = std::move(occupiedTiles); }
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::paintedSelectionCoverage(DocumentHandle handle, const Snapshot &source, LayerId layer,
    SelectionMaskSP *result)
{
    auto &d = *m_d;
    const auto document = d.documents.constFind(handle);
    if (!d.onThread() || document == d.documents.cend() || !source || !result || source->format != PixelFormat::Rgba8 ||
        !document->requested || document->requested->documentId() != source->documentId())
        return {ExecutionCode::InvalidRequest, QStringLiteral("Painted coverage requires its retained raw layer")};
    if (d.active || d.device->isRecordingFrame()) return {ExecutionCode::Busy, QStringLiteral("Painted coverage requires submitted work")};
    if (d.device->readbackPending() && !d.device->finish()) return {ExecutionCode::DeviceFailure, d.device->diagnostic()};
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
    auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    const auto rendered = d.renderer->record(frame.commands, source, document->rendered);
    if (rendered.code != RenderOutcome::Recorded)
        return {rendered.code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded : ExecutionCode::DeviceFailure, rendered.diagnostic};
    auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    const auto raw = rendered.version->m_data->layers.value(layer);
    const auto sourceLayer = std::find_if(source->layers.cbegin(), source->layers.cend(),
        [&](const auto &value) { return value.id == layer; });
    if (sourceLayer == source->layers.cend()) return {ExecutionCode::InvalidRequest, {}};
    const quint8 defaultCoverage = quint8(sourceLayer->defaultPixel[3]);
    auto paint = QSharedPointer<SelectionPaintSource>::create();
    paint->source = source; paint->layer = layer;
    const auto allocation = paintLayerAllocation(source, *sourceLayer, SelectionMask::MaximumBytes / qsizetype(sizeof(QRect)));
    if (!allocation) return {ExecutionCode::CapacityExceeded, QStringLiteral("Painted mask storage exceeds metadata capacity")};
    for (const auto &block : allocation->blocks) paint->allocatedBlocks.append(block);
    QRect bounds;
    QList<QRect> runs;
    for (auto tile = raw.cbegin(); tile != raw.cend(); ++tile) {
        const auto point = coordinate(tile.key());
        QRect occupied;
        QList<QRect> rows;
        const auto geometry = readSelectionOutline(tile.value()->storage,
            QRect(point * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize)),
            {}, &occupied, &rows, 1, defaultCoverage);
        if (geometry.code != ExecutionCode::Success) return geometry;
        if (occupied.isEmpty()) continue;
        if (paint->tiles.size() == SelectionMask::MaximumBytes / SelectionMask::AlphaTileBytes ||
            rows.size() > SelectionMask::MaximumBytes / qsizetype(sizeof(QRect)) - runs.size())
            return {ExecutionCode::CapacityExceeded, QStringLiteral("Painted coverage exceeds its tile or outline capacity")};
        paint->tiles.append(point);
        bounds = bounds.isEmpty() ? occupied : bounds.united(occupied);
        runs.append(rows);
    }
    mergeRuns(runs);
    const auto native = std::make_shared<SelectionPixels>();
    frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    for (const auto point : paint->tiles) {
        const auto tile = raw.value(key(point.x(), point.y()));
        d.device->retainForFrame(tile->storage);
        const auto packed = d.renderer->m_d->packSelectionTile(frame.commands, tile->pixels.get(),
            QRect(point * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize)),
            point, bounds, 1, defaultCoverage);
        if (!packed) return {ExecutionCode::CapacityExceeded, QStringLiteral("Painted coverage packing capacity is unavailable")};
        native->tiles.insert(key(point.x(), point.y()), packed->storage);
    }
    submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    native->device = d.device; native->tilesReady = true;
    const auto mask = SelectionMask::fromPaint(paint, bounds, runs, native);
    if (!mask) return {ExecutionCode::InvalidRequest, QStringLiteral("Painted coverage geometry or source is invalid")};
    *result = mask;
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::combineSelectionCoverage(const SelectionMaskSP &first, const SelectionMaskSP &second,
    SelectionCombination action, QRect clip, bool inverted, SelectionMaskSP *result)
{
    auto &d = *m_d;
    if (!d.onThread() || !first || !second || !result || action < SelectionCombination::Replace ||
        action > SelectionCombination::SymmetricDifference || clip.isEmpty()) return {ExecutionCode::InvalidRequest, {}};
    if (d.active || d.device->isRecordingFrame()) return {ExecutionCode::Busy, QStringLiteral("Selection arithmetic requires submitted work")};
    if (d.device->readbackPending() && !d.device->finish()) return {ExecutionCode::DeviceFailure, d.device->diagnostic()};
    const auto a = inverted || first->defaultCoverage() ? clip : first->bounds().intersected(clip);
    const auto b = second->defaultCoverage() ? clip : second->bounds().intersected(clip);
    const auto extent = action == SelectionCombination::Replace ? b : action == SelectionCombination::Intersect ? a.intersected(b) :
        action == SelectionCombination::Subtract ? a : a.united(b);
    if (extent.isEmpty()) { *result = SelectionMask::empty(); return {ExecutionCode::Success, {}}; }
    QSet<QPoint> candidates;
    const auto append = [&](const SelectionMaskSP &mask) {
        if (mask->defaultCoverage()) return appendTiles(candidates, extent);
        for (const auto point : mask->tileCoordinates())
            if (!appendTiles(candidates, QRect(point * TileRenderer::TileSize,
                    QSize(TileRenderer::TileSize, TileRenderer::TileSize)).intersected(extent))) return false;
        return true;
    };
    const bool valid = action == SelectionCombination::Replace || action == SelectionCombination::Intersect ? append(second) :
        inverted ? appendTiles(candidates, extent) :
        action == SelectionCombination::Subtract ? append(first) : append(first) && append(second);
    if (!valid) return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection arithmetic exceeds its tile capacity")};
    // Sparse bounds can overlap while none of their retained tiles reach the
    // intersection. The result then has no coverage to evaluate or allocate.
    if (candidates.isEmpty()) { *result = SelectionMask::empty(); return {ExecutionCode::Success, {}}; }
    auto operation = QSharedPointer<SelectionCombinationSource>::create();
    operation->first = first; operation->second = second; operation->action = action;
    operation->extent = extent; operation->inverted = inverted; operation->tiles = sortedTiles(candidates);
    return evaluateSelectionCombination(operation, result);
}

ExecutionOutcome Execution::evaluateSelectionCombination(const QSharedPointer<const SelectionCombinationSource> &operation,
    SelectionMaskSP *result)
{
    auto &d = *m_d;
    const auto extent = operation->extent;
    const auto native = std::make_shared<SelectionPixels>();
    const auto pending = SelectionMask::fromCombination(operation, extent, {}, native);
    if (!pending) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection arithmetic inputs are invalid")};
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
    const auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    if (!d.renderer->m_d->ensureSelectionTiles(frame.commands, pending)) return {d.device->isLost() || !d.device->diagnostic().isEmpty() ? ExecutionCode::DeviceFailure : ExecutionCode::CapacityExceeded,
        QStringLiteral("GPU selection arithmetic could not retain its working coverage: %1").arg(d.renderer->m_d->error)};
    const auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    QRect bounds;
    QList<QRect> runs;
    QList<QPoint> occupied;
    const auto read = readSelectionTileOutlines(pending, &bounds, &runs, &occupied);
    if (read.code != ExecutionCode::Success) return read;
    auto frozen = QSharedPointer<SelectionCombinationSource>::create(*operation);
    frozen->tiles = std::move(occupied);
    const auto mask = SelectionMask::fromCombination(frozen, bounds, runs, native);
    if (!mask) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection arithmetic geometry is invalid")};
    *result = mask;
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::transformedMaskCoverage(const SelectionMaskSP &mask, const SelectionMaskSP &selection,
    bool inverted, QRect copyRect, const AffinePlan &plan, QRect defaultBounds, SelectionMaskSP *result)
{
    auto &d = *m_d;
    constexpr int limit = RgbaTile::CoordinateLimit;
    if (!d.onThread() || !mask || !result || !validAffinePlan(plan) ||
        (selection ? copyRect.isEmpty() : inverted || !copyRect.isEmpty()))
        return {ExecutionCode::InvalidRequest, QStringLiteral("Mask transform requires frozen gray, selection and affine inputs")};
    if (d.active || d.device->isRecordingFrame()) return {ExecutionCode::Busy, QStringLiteral("Mask transform requires submitted work")};
    if (d.device->readbackPending() && !d.device->finish()) return {ExecutionCode::DeviceFailure, d.device->diagnostic()};
    QRect output = selection ? copyRect : mask->bounds();
    if (!selection && !plan.passes.isEmpty() && !plan.passes.first().source.isEmpty()) output = plan.passes.first().source;
    for (const auto &pass : plan.passes) output = affineOutputRegion(pass, output);
    const auto valid = [&](QRect area) {
        return area.isEmpty() || (area.left() >= -limit && area.top() >= -limit && area.right() < limit && area.bottom() < limit);
    };
    if (!valid(copyRect) || !valid(output) || !valid(defaultBounds)) return {ExecutionCode::InvalidRequest, QStringLiteral("Mask transform exceeds coordinate bounds")};
    auto operation = QSharedPointer<SelectionMaskTransformSource>::create();
    operation->source = mask; operation->selection = selection; operation->inverted = inverted;
    operation->copyRect = copyRect; operation->output = output;
    operation->plan = QSharedPointer<AffinePlan>::create(plan);
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
    if (!selection) {
        // The native pixel selection clears its default before GrayA is merged
        // back. Its opaque composition source retains the parent's bounds and
        // occupied 64-pixel blocks, even when the mask default was nonzero.
        auto frame = beginReadbackFrame();
        if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
        Tiles rgba;
        if (!d.renderer->m_d->transformMaskTiles(frame.commands, *operation, nullptr, &rgba))
            return {d.device->isLost() || !d.device->diagnostic().isEmpty() ? ExecutionCode::DeviceFailure : ExecutionCode::CapacityExceeded,
                d.renderer->m_d->error};
        auto submitted = endFrame(false);
        if (submitted.code != ExecutionCode::Success) return submitted;
        QRect occupied;
        for (auto tile = rgba.cbegin(); tile != rgba.cend(); ++tile) {
            const auto point = coordinate(tile.key());
            QRect part; QList<QRect> runs;
            const auto geometry = readSelectionOutline(tile.value()->storage, QRect(point * 256, QSize(256, 256)),
                {}, &part, &runs, 3, mask->defaultCoverage());
            if (geometry.code != ExecutionCode::Success) return geometry;
            occupied |= part;
        }
        if (!occupied.isEmpty()) {
            occupied.translate(-plan.positionDelta);
            const auto floor = [](int v) { return v / 64 - (v % 64 < 0); };
            occupied = QRect(QPoint(floor(occupied.left()) * 64, floor(occupied.top()) * 64),
                QPoint((floor(occupied.right()) + 1) * 64 - 1, (floor(occupied.bottom()) + 1) * 64 - 1));
            occupied.translate(plan.positionDelta);
        }
        output = occupied.united(defaultBounds);
        if (!valid(output)) return {ExecutionCode::InvalidRequest, QStringLiteral("Transformed mask extent exceeds coordinate bounds")};
        operation->output = output;
    }
    QSet<QPoint> candidates;
    if (!appendTiles(candidates, output) || (selection && !appendTiles(candidates, copyRect)))
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Mask transform exceeds output tile capacity")};
    if (selection) for (const auto point : mask->tileCoordinates()) {
        candidates.insert(point);
        if (candidates.size() > SelectionMask::MaximumBytes / SelectionMask::AlphaTileBytes)
            return {ExecutionCode::CapacityExceeded, QStringLiteral("Mask remainder exceeds tile capacity")};
    }
    operation->tiles = sortedTiles(candidates);
    const auto native = std::make_shared<SelectionPixels>();
    const auto pending = SelectionMask::fromTransform(operation, {}, {}, native);
    if (!pending) return {ExecutionCode::InvalidRequest, QStringLiteral("Mask transform inputs are invalid")};
    auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    if (!d.renderer->m_d->ensureSelectionTiles(frame.commands, pending))
        return {d.device->isLost() || !d.device->diagnostic().isEmpty() ? ExecutionCode::DeviceFailure : ExecutionCode::CapacityExceeded,
            QStringLiteral("GPU mask transform failed: %1").arg(d.renderer->m_d->error)};
    const auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    QRect bounds;
    QList<QRect> runs;
    QList<QPoint> occupied;
    const auto read = readSelectionTileOutlines(pending, &bounds, &runs, &occupied);
    if (read.code != ExecutionCode::Success) return read;
    operation->tiles = std::move(occupied);
    const auto transformed = SelectionMask::fromTransform(operation, bounds, runs, native);
    if (!transformed) return {ExecutionCode::InvalidRequest, QStringLiteral("Mask transform geometry is invalid")};
    *result = transformed;
    return {ExecutionCode::Success, {}};
}

ExecutionOutcome Execution::translateSelectionCoverage(const SelectionMaskSP &source, QPoint offset, QRect clip,
    SelectionMaskSP *result)
{
    auto &d = *m_d;
    constexpr qint64 limit = OperationDocument::CoordinateLimit;
    if (!d.onThread() || !source || !result || clip.isEmpty() ||
        qAbs(qint64(offset.x())) > 2 * limit || qAbs(qint64(offset.y())) > 2 * limit ||
        clip.x() < -limit || clip.y() < -limit || qint64(clip.x()) + clip.width() > limit ||
        qint64(clip.y()) + clip.height() > limit)
        return {ExecutionCode::InvalidRequest, QStringLiteral("Selection movement requires valid frozen coverage and coordinates")};
    if (d.active || d.device->isRecordingFrame()) return {ExecutionCode::Busy, QStringLiteral("Selection movement requires submitted work")};
    if (d.device->readbackPending() && !d.device->finish()) return {ExecutionCode::DeviceFailure, d.device->diagnostic()};
    const auto extent = source->bounds().translated(offset).intersected(clip);
    auto operation = QSharedPointer<SelectionTranslationSource>::create();
    operation->source = source; operation->offset = offset; operation->extent = extent; operation->clip = clip;
    if (extent.isEmpty()) {
        *result = SelectionMask::fromTranslation(operation, {}, {}, std::make_shared<SelectionPixels>());
        return *result ? ExecutionOutcome{ExecutionCode::Success, {}} :
            ExecutionOutcome{ExecutionCode::CapacityExceeded, QStringLiteral("Translated mask storage exceeds capacity")};
    }
    QSet<QPoint> candidates;
    for (const auto point : source->tileCoordinates()) {
        const auto area = QRect(point * TileRenderer::TileSize, QSize(TileRenderer::TileSize, TileRenderer::TileSize))
            .intersected(source->bounds()).translated(offset).intersected(extent);
        if (!appendTiles(candidates, area))
            return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection movement exceeds its tile capacity")};
    }
    operation->tiles = sortedTiles(candidates);
    const auto native = std::make_shared<SelectionPixels>();
    const auto pending = SelectionMask::fromTranslation(operation, extent, {}, native);
    if (!pending) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection movement inputs are invalid")};
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
    const auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    if (!d.renderer->m_d->ensureSelectionTiles(frame.commands, pending)) return {d.device->isLost() || !d.device->diagnostic().isEmpty() ? ExecutionCode::DeviceFailure : ExecutionCode::CapacityExceeded,
        QStringLiteral("GPU selection movement could not retain its coverage")};
    const auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    QRect bounds;
    QList<QRect> runs;
    QList<QPoint> occupied;
    const auto read = readSelectionTileOutlines(pending, &bounds, &runs, &occupied);
    if (read.code != ExecutionCode::Success) return read;
    operation->tiles = std::move(occupied);
    const auto mask = SelectionMask::fromTranslation(operation, bounds, runs, native);
    if (!mask) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection movement geometry is invalid")};
    *result = mask;
    return {ExecutionCode::Success, {}};
}

PixelBytesOutcome Execution::readSelectionPixels(const SelectionMaskSP &mask, QRect region, qsizetype maximumBytes)
{
    auto &d = *m_d;
    const auto failed = [](ExecutionCode code, const QString &diagnostic) {
        PixelBytesOutcome value; value.code = code; value.diagnostic = diagnostic; return value;
    };
    constexpr qint64 limit = OperationDocument::CoordinateLimit;
    if (!d.onThread() || !mask || region.width() < 0 || region.height() < 0 ||
        region.x() < -limit || region.y() < -limit ||
        qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit || maximumBytes < 0)
        return failed(ExecutionCode::InvalidRequest, QStringLiteral("Selection reads require a retained mask, valid region and GPU thread"));
    if (d.active || d.device->isRecordingFrame())
        return failed(ExecutionCode::Busy, QStringLiteral("Selection reads require submitted frames"));
    if (d.device->readbackPending() && !d.device->finish())
        return failed(ExecutionCode::DeviceFailure, QStringLiteral("Earlier GPU read completion failed"));
    retireCompletedReadback();
    if (qint64(region.width()) * region.height() > maximumBytes ||
        (mask->isGpuGenerated() && SelectionMask::AlphaTileBytes > d.maximumReadbackBytes))
        return failed(ExecutionCode::CapacityExceeded, QStringLiteral("Selection read exceeds the pixel capacity"));
    try {
        PixelBytesOutcome outcome;
        outcome.region = region; outcome.generation = d.generation;
        outcome.document = mask->regionSource() ? mask->regionSource()->source : Snapshot{};
        outcome.bytes = QByteArray(qsizetype(region.width()) * region.height(), char(mask->defaultCoverage()));
        const auto area = region.intersected(mask->bounds());
        if (!mask->isGpuGenerated()) {
            for (int y = area.top(); !area.isEmpty() && y <= area.bottom(); ++y)
                for (int x = area.left(); x <= area.right(); ++x)
                    outcome.bytes[(qsizetype(y) - region.y()) * region.width() + x - region.x()] = char(mask->coverage(x, y));
        } else if (!area.isEmpty()) {
            const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
            for (int y = pixelTileIndex(area.top(), TileRenderer::TileSize); y <= pixelTileIndex(area.bottom(), TileRenderer::TileSize); ++y)
                for (int x = pixelTileIndex(area.left(), TileRenderer::TileSize); x <= pixelTileIndex(area.right(), TileRenderer::TileSize); ++x) {
                    const auto frame = beginReadbackFrame();
                    if (frame.code != ExecutionCode::Success) return failed(frame.code, frame.diagnostic);
                    const auto tile = d.renderer->m_d->coverageTile(frame.commands, mask, key(x, y));
                    if (!tile) return failed(ExecutionCode::CapacityExceeded, QStringLiteral("Selection read tile is unavailable"));
                    d.device->retainForFrame(tile->storage);
                    const auto read = ReadbackBatch::create(*d.device, 1);
                    if (!read || !read->recordBuffer(*d.device, frame.commands, 0, tile->pixels.get(), SelectionMask::AlphaTileBytes))
                        return failed(ExecutionCode::DeviceFailure, QStringLiteral("Selection read recording failed"));
                    const auto submitted = endFrame(false);
                    if (submitted.code != ExecutionCode::Success) return failed(submitted.code, submitted.diagnostic);
                    if (read->results[0]->data.size() != SelectionMask::AlphaTileBytes)
                        return failed(ExecutionCode::DeviceFailure, QStringLiteral("Selection read is incomplete"));
                    const QPoint origin(x * TileRenderer::TileSize, y * TileRenderer::TileSize);
                    const auto portion = area.intersected(QRect(origin, QSize(TileRenderer::TileSize, TileRenderer::TileSize)));
                    for (int row = portion.top(); row <= portion.bottom(); ++row)
                        std::memcpy(outcome.bytes.data() + (qsizetype(row) - region.y()) * region.width() + portion.x() - region.x(),
                            read->results[0]->data.constData() + (row - origin.y()) * TileRenderer::TileSize + portion.x() - origin.x(),
                            size_t(portion.width()));
                }
        }
        outcome.code = ExecutionCode::Success;
        return outcome;
    } catch (const std::bad_alloc &) {
        return failed(ExecutionCode::CapacityExceeded, QStringLiteral("Selection read allocation failed"));
    }
}
}
