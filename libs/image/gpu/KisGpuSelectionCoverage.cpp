/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include <QScopeGuard>
#include <array>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;

std::shared_ptr<BufferStorage> TileRenderer::Private::linearSelectionCoverage(WGPUCommandEncoder commands,
    const SelectionMaskSP &mask)
{
    if (!mask || !mask->isGpuGenerated() || !device->ownsCommands(commands)) return {};
    const auto cached = maskTiles.constFind(mask.data());
    if (cached != maskTiles.cend() && cached->generated) return cached->generated;
    const auto native = mask->nativePixels();
    if (!native) return {};
    {
        const std::lock_guard<std::mutex> lock(native->mutex);
        if (native->device.lock() == device && native->region) return native->region;
    }
    std::shared_ptr<BufferStorage> pixels;
    if (const auto &region = mask->regionSource()) {
        const auto prepared = renderer.prepareRegion(commands, region);
        if (prepared.code != RenderOutcome::Recorded) { error = prepared.diagnostic; return {}; }
        pixels = regionCache.value(region.data()).coverage.pixels;
    } else if (const auto &operation = mask->combinationSource()) {
        const auto extent = operation->extent;
        const quint64 bytes = quint64(extent.width()) * extent.height() * 4;
        WGPULimits limits{};
        if (extent.isEmpty() || wgpuDeviceGetLimits(device->nativeDevice(), &limits) != WGPUStatus_Success ||
            bytes > qMin(limits.maxStorageBufferBindingSize, limits.maxBufferSize) ||
            bytes > quint64(qMax<qsizetype>(0, budget->limit - budget->used))) return {};
        auto output = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage);
        if (!output) return {};
        pixels = std::make_shared<BufferStorage>(std::move(output), budget);
        device->retainForFrame(pixels);
        if (!selectionCombinePipeline) {
            selectionCombinePipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/selection-combine.wgsl"));
            if (selectionCombinePipeline)
                selectionCombineLayout.reset(wgpuComputePipelineGetBindGroupLayout(selectionCombinePipeline.get(), 0));
        }
        if (!selectionCombinePipeline || !selectionCombineLayout) return {};
        const int left = pixelTileIndex(extent.left(), TileSize), right = pixelTileIndex(extent.right(), TileSize);
        const int top = pixelTileIndex(extent.top(), TileSize), bottom = pixelTileIndex(extent.bottom(), TileSize);
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
            const auto first = coverageTile(commands, operation->first, key(x, y));
            const auto second = coverageTile(commands, operation->second, key(x, y));
            if (!first || !second || qsizetype(32) > budget->limit - budget->used) return {};
            device->retainForFrame(first->storage); device->retainForFrame(second->storage);
            const qint32 parameters[8]{extent.x(), extent.y(), extent.width(), extent.height(), x * TileSize, y * TileSize,
                                      qint32(operation->action), qint32(operation->inverted)};
            auto buffer = createBuffer(device->nativeDevice(), sizeof(parameters), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
            if (!buffer) return {};
            const auto uniform = std::make_shared<BufferStorage>(std::move(buffer), budget);
            device->retainForFrame(uniform);
            wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, parameters, sizeof(parameters));
            const WGPUBuffer buffers[]{uniform->pixels.get(), first->pixels.get(), second->pixels.get(), pixels->pixels.get()};
            std::array<WGPUBindGroupEntry, 4> entries{};
            for (quint32 i = 0; i < entries.size(); ++i) {
                entries[i].binding = i; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
            }
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = selectionCombineLayout.get(); descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
            NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
            if (!bindings) return {};
            const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
            if (!pass) return {};
            wgpuComputePassEncoderSetPipeline(pass, selectionCombinePipeline.get());
            wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass, TileSize / 8, TileSize / 8, 1);
            wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        }
    }
    if (pixels) {
        const std::lock_guard<std::mutex> lock(native->mutex);
        native->device = device; native->region = pixels;
    }
    return pixels;
}

std::shared_ptr<CoverageTile> TileRenderer::Private::generatedCoverageTile(WGPUCommandEncoder commands,
    const SelectionMaskSP &mask, quint64 key)
{
    if (!mask || !mask->isGpuGenerated() || !device->ownsCommands(commands)) return {};
    const QPoint coordinate(int(quint32(key)), int(quint32(key >> 32)));
    const QRect area(coordinate * TileSize, QSize(TileSize, TileSize));
    std::shared_ptr<BufferStorage> generated;
    WGPUBuffer source = unusedCoverage.get();
    qint32 values[12]{0, 0, 0, 0, area.x(), area.y(), 2, 0,
                      mask->bounds().left(), mask->bounds().top(), mask->bounds().right() + 1, mask->bounds().bottom() + 1};
    if (area.intersects(mask->bounds())) {
        generated = linearSelectionCoverage(commands, mask);
        if (!generated) return {};
        const auto extent = mask->regionSource() ? QRect(mask->regionSource()->coverageOrigin, mask->regionSource()->coverageSize)
                                                : mask->combinationSource()->extent;
        source = generated->pixels.get();
        values[0] = extent.x(); values[1] = extent.y(); values[2] = extent.width(); values[3] = extent.height(); values[6] = 0;
        device->retainForFrame(generated);
    }
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
    device->retainForFrame(uniform);
    wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, values, sizeof(values));
    if (!coveragePackPipeline) {
        coveragePackPipeline = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/coverage-pack.wgsl"));
        if (coveragePackPipeline) coveragePackLayout.reset(wgpuComputePipelineGetBindGroupLayout(coveragePackPipeline.get(), 0));
    }
    if (!coveragePackPipeline || !coveragePackLayout) return {};
    const WGPUBuffer buffers[]{uniform->pixels.get(), source, output->pixels.get()};
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
    auto &entry = maskTiles[mask.data()];
    entry.source = mask; entry.generated = generated;
    entry.tiles.insert(key, output);
    for (auto old = maskTiles.begin(); maskTiles.size() > 4 && old != maskTiles.end();) {
        if (old.key() != mask.data()) old = maskTiles.erase(old);
        else ++old;
    }
    return output;
}

ExecutionOutcome Execution::combineSelectionCoverage(const SelectionMaskSP &first, const SelectionMaskSP &second,
    SelectionCombination action, QRect clip, bool inverted, SelectionMaskSP *result)
{
    auto &d = *m_d;
    if (!d.onThread() || !first || !second || !result || action < SelectionCombination::Replace ||
        action > SelectionCombination::SymmetricDifference || clip.isEmpty()) return {ExecutionCode::InvalidRequest, {}};
    if (d.active || d.device->isRecordingFrame()) return {ExecutionCode::Busy, QStringLiteral("Selection arithmetic requires submitted work")};
    if (d.device->readbackPending() && !d.device->finish()) return {ExecutionCode::DeviceFailure, d.device->diagnostic()};
    const auto a = inverted ? clip : first->bounds().intersected(clip);
    const auto b = second->bounds().intersected(clip);
    const auto extent = action == SelectionCombination::Replace ? b : action == SelectionCombination::Intersect ? a.intersected(b) :
        action == SelectionCombination::Subtract ? a : a.united(b);
    if (extent.isEmpty()) { *result = SelectionMask::empty(); return {ExecutionCode::Success, {}}; }
    if (quint64(extent.width()) * extent.height() > quint64(SelectionMask::MaximumBytes))
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Selection arithmetic exceeds its pixel capacity")};
    auto operation = QSharedPointer<SelectionCombinationSource>::create();
    operation->first = first; operation->second = second; operation->action = action;
    operation->extent = extent; operation->inverted = inverted;
    const auto native = std::make_shared<SelectionPixels>();
    const auto pending = SelectionMask::fromCombination(operation, extent, {}, native);
    if (!pending) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection arithmetic inputs are invalid")};
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); });
    const auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    const auto pixels = d.renderer->m_d->linearSelectionCoverage(frame.commands, pending);
    if (!pixels) return {d.device->isLost() || !d.device->diagnostic().isEmpty() ? ExecutionCode::DeviceFailure : ExecutionCode::CapacityExceeded,
        QStringLiteral("GPU selection arithmetic could not retain its working coverage: %1").arg(d.renderer->m_d->error)};
    const auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    QRect bounds;
    QList<QRect> runs;
    const auto read = readSelectionOutline(pixels, extent, {}, &bounds, &runs);
    if (read.code != ExecutionCode::Success) return read;
    const auto mask = SelectionMask::fromCombination(operation, bounds, runs, native);
    if (!mask) return {ExecutionCode::InvalidRequest, QStringLiteral("Selection arithmetic geometry is invalid")};
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
        outcome.bytes = QByteArray(qsizetype(region.width()) * region.height(), '\0');
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
