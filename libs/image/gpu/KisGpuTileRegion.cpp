/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include "KisGpuExecution_p.h"
#include "KisGpuReadbackBatch_p.h"
#include <QScopeGuard>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
struct RegionParameters {
    quint32 size[2], seed[2];
    quint32 format, threshold, spread, flags;
    quint32 implicit[4];
    qint32 selection[4];
};
static_assert(sizeof(RegionParameters) == 64);
}

TileRenderer::Private::RegionResult TileRenderer::Private::regionCoverage(
    WGPUCommandEncoder commands, const RegionFillSP &region, const RenderedSnapshot &source)
{
    const auto cached = regionCache.constFind(region.data());
    if (cached != regionCache.cend() && cached->source.toStrongRef() == region)
        return {RenderOutcome::Recorded, cached->coverage, {}};
    if (!region || !region->source || !source || source->document() != region->source || region->source->layers.size() != 1)
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU contiguous fill has no frozen reference layer")};
    const auto &document = *region->source;
    const auto &settings = region->settings;
    if (settings.threshold < 0 || settings.threshold > 255 || settings.opacitySpread < 0 || settings.opacitySpread > 100 ||
        settings.antiAlias || !QRect(QPoint(), document.size).contains(settings.seed))
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU contiguous fill settings are invalid")};
    const quint64 width = document.size.width(), height = document.size.height();
    const quint64 count = width * height, columns = (width + 255) / 256, rows = (height + 255) / 256;
    const auto sourceTiles = source->tiles(region->layer);
    QList<TileView> present;
    for (const auto &tile : sourceTiles)
        if (tile.coordinate.x() >= 0 && tile.coordinate.y() >= 0 && quint64(tile.coordinate.x()) < columns && quint64(tile.coordinate.y()) < rows)
            present.append(tile);
    const bool masked = document.selectionKind == SelectionKind::Freehand;
    quint64 maskBytes = 0;
    if (masked && document.selectionMask)
        for (auto tile = document.selectionMask->tiles().cbegin(); tile != document.selectionMask->tiles().cend(); ++tile)
            if (tile.key().first >= 0 && tile.key().second >= 0 &&
                quint64(tile.key().first) < columns && quint64(tile.key().second) < rows)
                maskBytes += document.selectionMask->tileBytes();
    const quint64 atlasBytes = qMax<quint64>(4, quint64(present.size()) * RgbaTile::byteCount(document.format));
    const quint64 directoryBytes = columns * rows * 8;
    const quint64 selectionBytes = qMax<quint64>(4, maskBytes);
    const quint64 coverageBytes = count * 4;
    const quint64 required = coverageBytes * 2 + atlasBytes + directoryBytes + selectionBytes + 20 + sizeof(RegionParameters);
    WGPULimits limits{};
    if (wgpuDeviceGetLimits(device->nativeDevice(), &limits) != WGPUStatus_Success ||
        !count || count >= 0xffffffffu || !directoryBytes ||
        (width + 7) / 8 > limits.maxComputeWorkgroupsPerDimension || (height + 7) / 8 > limits.maxComputeWorkgroupsPerDimension ||
        qMax(qMax(atlasBytes, directoryBytes), qMax(selectionBytes, coverageBytes)) > qMin(limits.maxStorageBufferBindingSize, limits.maxBufferSize) ||
        required > quint64(budget->limit) || !ensureTileCapacity(0, document.format) ||
        required > quint64(qMax<qsizetype>(0, budget->limit - budget->used)))
        return {RenderOutcome::CapacityExceeded, {}, QStringLiteral("GPU contiguous fill exceeds the available working capacity")};
    auto allocate = [&](quint64 bytes, WGPUBufferUsage usage) -> std::shared_ptr<BufferStorage> {
        auto buffer = createBuffer(device->nativeDevice(), bytes, usage);
        if (!buffer) return {};
        auto result = std::make_shared<BufferStorage>(std::move(buffer), budget);
        device->retainForFrame(result);
        return result;
    };
    const auto atlas = allocate(atlasBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    const auto directory = allocate(directoryBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    const auto selection = allocate(selectionBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    const auto labels = allocate(coverageBytes, WGPUBufferUsage_Storage);
    const auto coverage = allocate(coverageBytes, WGPUBufferUsage_Storage);
    const auto status = allocate(20, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
    const auto uniform = allocate(sizeof(RegionParameters), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    if (!atlas || !directory || !selection || !labels || !coverage || !status || !uniform)
        return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU contiguous fill allocation failed")};
    QList<std::array<qint32, 2>> entries(qsizetype(columns * rows), {-1, -1});
    quint64 offset = 0;
    for (const auto &tile : present) {
        entries[tile.coordinate.y() * qsizetype(columns) + tile.coordinate.x()][0] = qint32(offset / 4);
        wgpuCommandEncoderCopyBufferToBuffer(commands, tile.pixels, 0, atlas->pixels.get(), offset, RgbaTile::byteCount(document.format));
        offset += RgbaTile::byteCount(document.format);
    }
    // The source version pins every copied buffer through this submission.
    for (const auto &tile : source->m_data->layers.value(region->layer)) retainTile(tile);
    if (masked && document.selectionMask) {
        offset = 0;
        for (auto tile = document.selectionMask->tiles().cbegin(); tile != document.selectionMask->tiles().cend(); ++tile) {
            const int x = tile.key().first, y = tile.key().second;
            if (x >= 0 && y >= 0 && quint64(x) < columns && quint64(y) < rows) {
                entries[y * qsizetype(columns) + x][1] = qint32(offset / 4);
                wgpuQueueWriteBuffer(device->queue(), selection->pixels.get(), offset, tile.value().constData(), size_t(tile.value().size()));
                offset += document.selectionMask->tileBytes();
            }
        }
    }
    RegionParameters parameters{};
    parameters.size[0] = quint32(width); parameters.size[1] = quint32(height);
    parameters.seed[0] = quint32(settings.seed.x()); parameters.seed[1] = quint32(settings.seed.y());
    parameters.format = quint32(document.format); parameters.threshold = settings.threshold; parameters.spread = settings.opacitySpread;
    if (masked || !document.selection.isEmpty()) parameters.flags |= 1;
    if (document.selectionKind == SelectionKind::Ellipse) parameters.flags |= 2;
    if (masked) parameters.flags |= 4;
    if (masked && document.selectionMask && document.selectionMask->isAlpha()) parameters.flags |= 8;
    if (document.selectionInverted) parameters.flags |= 16;
    if (settings.selectionAsBoundary) parameters.flags |= 32;
    const auto implicit = document.layers[0].defaultPixel;
    for (int i = 0; i < 4; ++i) parameters.implicit[i] = implicit[i];
    parameters.selection[0] = document.selection.x(); parameters.selection[1] = document.selection.y();
    parameters.selection[2] = document.selection.width(); parameters.selection[3] = document.selection.height();
    const quint32 initialStatus[5]{0, quint32(width), quint32(height), 0, 0};
    wgpuQueueWriteBuffer(device->queue(), status->pixels.get(), 0, initialStatus, sizeof(initialStatus));
    wgpuQueueWriteBuffer(device->queue(), directory->pixels.get(), 0, entries.constData(), directoryBytes);
    wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, &parameters, sizeof(parameters));
    if (!regionPipelines[0]) {
        regionPipelines[0] = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "classify");
        if (regionPipelines[0]) regionLayout.reset(wgpuComputePipelineGetBindGroupLayout(regionPipelines[0].get(), 0));
        if (regionLayout) {
            regionPipelines[1] = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "connect", 0, 0, regionLayout.get());
            regionPipelines[2] = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "resolve", 0, 0, regionLayout.get());
        }
    }
    if (!regionPipelines[0] || !regionPipelines[1] || !regionPipelines[2] || !regionLayout)
        return {RenderOutcome::DeviceFailure, {}, error};
    const std::array<std::shared_ptr<BufferStorage>, 7> buffers{uniform, atlas, directory, selection, labels, coverage, status};
    std::array<WGPUBindGroupEntry, 7> bindings{};
    for (quint32 i = 0; i < bindings.size(); ++i) {
        bindings[i].binding = i; bindings[i].buffer = buffers[i]->pixels.get();
        bindings[i].size = wgpuBufferGetSize(bindings[i].buffer);
    }
    WGPUBindGroupDescriptor descriptor{};
    descriptor.layout = regionLayout.get(); descriptor.entryCount = bindings.size(); descriptor.entries = bindings.data();
    NativeBindings group(wgpuDeviceCreateBindGroup(device->nativeDevice(), &descriptor));
    if (!group) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU contiguous fill bindings failed")};
    // Dispatch boundaries make classification, atomic union and resolution
    // separate globally ordered stages. No host iteration or pixel read is used.
    for (const auto &pipeline : regionPipelines) {
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU contiguous fill commands failed")};
        wgpuComputePassEncoderSetPipeline(pass, pipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, group.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, quint32((width + 7) / 8), quint32((height + 7) / 8), 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    }
    RegionCoverage result{coverage, status};
    regionCache.insert(region.data(), {region, result});
    return {RenderOutcome::Recorded, result, {}};
}

RenderOutcome TileRenderer::prepareRegion(WGPUCommandEncoder commands, const RegionFillSP &region, const RenderedSnapshot &base)
{
    if (!region || !region->source) return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Missing contiguous fill reference")};
    const auto compatible = base && base->document()->documentId() == region->source->documentId() ? base : RenderedSnapshot{};
    const auto source = recordRegion(commands, region->source, QRect(QPoint(), region->source->size), {region->layer}, compatible);
    if (source.code != RenderOutcome::Recorded) return source;
    const auto coverage = m_d->regionCoverage(commands, region, source.version);
    return {coverage.code, source.version, coverage.diagnostic};
}

ExecutionOutcome Execution::checkRegionFill(DocumentHandle handle, const RegionFillSP &region, QRect *bounds)
{
    auto &d = *m_d;
    if (!d.onThread() || !bounds || !region || !region->source || !d.documents.contains(handle) ||
        d.documents[handle].requested != region->preparedFor.toStrongRef())
        return {ExecutionCode::InvalidRequest, QStringLiteral("Contiguous fill requires the attached current version")};
    if (d.active || d.device->isRecordingFrame())
        return {ExecutionCode::Busy, QStringLiteral("Contiguous fill requires submitted earlier work")};
    if (d.device->readbackPending() && !d.device->finish())
        return {ExecutionCode::DeviceFailure, QStringLiteral("Earlier GPU reads could not complete before contiguous filling")};
    if (d.maximumReadbackBytes < 20)
        return {ExecutionCode::CapacityExceeded, QStringLiteral("Contiguous fill admission metadata exceeds the readback capacity")};
    retireCompletedReadback();
    const auto frame = beginReadbackFrame();
    if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
    const auto rollback = qScopeGuard([&] { if (d.active) cancelFrame(); d.renderer->m_d->regionCache.clear(); });
    const auto source = d.renderer->prepareRegion(frame.commands, region, d.documents[handle].rendered);
    if (source.code != RenderOutcome::Recorded)
        return {source.code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded :
            source.code == RenderOutcome::InvalidVersion ? ExecutionCode::InvalidRequest : ExecutionCode::DeviceFailure, source.diagnostic};
    const auto coverage = d.renderer->m_d->regionCache.value(region.data()).coverage;
    auto reads = ReadbackBatch::create(*d.device, 1);
    if (!reads || !coverage.status || !reads->recordBuffer(*d.device, frame.commands, 0, coverage.status->pixels.get(), 20))
        return {ExecutionCode::DeviceFailure, QStringLiteral("GPU contiguous fill admission read failed")};
    const auto submitted = endFrame(false);
    if (submitted.code != ExecutionCode::Success) return submitted;
    if (reads->results[0]->data.size() != 20)
        return {ExecutionCode::DeviceFailure, QStringLiteral("GPU contiguous fill admission status is incomplete")};
    quint32 values[5]; std::memcpy(values, reads->results[0]->data.constData(), sizeof(values));
    if (values[0]) return {ExecutionCode::InvalidRequest, QStringLiteral("This color reference requires native Lab difference support")};
    *bounds = values[1] <= values[3] && values[2] <= values[4]
        ? QRect(QPoint(values[1], values[2]), QPoint(values[3], values[4])) : QRect();
    return {ExecutionCode::Success, {}};
}
}
