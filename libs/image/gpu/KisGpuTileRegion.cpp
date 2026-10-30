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
    quint32 boundary[4], documentSize[2], reserved[2];
};
static_assert(sizeof(RegionParameters) == 96);
quint64 labBufferBytes(const LibrePaint::Pigment::Color::NativeLabConversionSP &value)
{
    if (!value) return sizeof(quint32);
    using Value = LibrePaint::Pigment::Color::NativeLabConversion;
    return std::holds_alternative<Value::IntegerClut>(value->definition)
        ? 24 + quint64((value->tableByteCount() + 3) / 4) * 4 : 24 + 12 * sizeof(double);
}
QByteArray packNativeLab(const LibrePaint::Pigment::Color::NativeLabConversionSP &value)
{
    QByteArray bytes(qsizetype(labBufferBytes(value)), '\0');
    if (!value) return bytes;
    using Value = LibrePaint::Pigment::Color::NativeLabConversion;
    quint32 header[6]{};
    if (const auto *clut = std::get_if<Value::IntegerClut>(&value->definition)) {
        header[0] = 1;
        for (int c = 0; c < 3; ++c) header[2 + c] = clut->grid[c];
        std::memcpy(bytes.data() + 24, clut->table.constData(), size_t(value->tableByteCount()));
    } else {
        header[0] = 2;
        const auto &matrix = std::get<Value::LinearRgb>(value->definition).matrix;
        std::memcpy(bytes.data() + 24, matrix.coefficients.data(), 9 * sizeof(double));
        std::memcpy(bytes.data() + 24 + 9 * sizeof(double), matrix.offset.data(), 3 * sizeof(double));
    }
    std::memcpy(bytes.data(), header, sizeof(header));
    return bytes;
}
}

TileRenderer::Private::RegionResult TileRenderer::Private::regionCoverage(
    WGPUCommandEncoder commands, const RegionFillSP &region, const RenderedSnapshot &source, const ProjectedSnapshot &projection)
{
    const auto cached = regionCache.constFind(region.data());
    if (cached != regionCache.cend() && cached->source.toStrongRef() == region)
        return {RenderOutcome::Recorded, cached->coverage, {}};
    if (!region || !region->source || !source || source->document() != region->source ||
        (region->settings.reference == RegionFillReference::CurrentLayer ? region->source->layers.size() != 1 : !projection || projection->document() != region->source))
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU contiguous fill has no frozen reference layer")};
    const auto &document = *region->source;
    const auto &settings = region->settings;
    if ((settings.mode != RegionFillMode::Contiguous && settings.mode != RegionFillMode::Similar) ||
        (settings.reference != RegionFillReference::CurrentLayer && settings.reference != RegionFillReference::AllLayers) || settings.threshold < 0 || settings.threshold > 255 || settings.opacitySpread < 0 || settings.opacitySpread > 100 ||
        !QRect(QPoint(), document.size).contains(settings.seed) ||
        (settings.boundaryColor && (settings.mode != RegionFillMode::Contiguous || !validPixel(*settings.boundaryColor, document.format))))
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("GPU contiguous fill settings are invalid")};
    const QSize expectedSize = settings.antiAlias
        ? QSize((document.size.width() + 63) / 64 * 64, (document.size.height() + 63) / 64 * 64) : document.size;
    if (region->coverageSize != expectedSize)
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Region coverage has an invalid filtering extent")};
    const quint64 width = region->coverageSize.width(), height = region->coverageSize.height();
    const quint64 count = width * height, columns = (width + 255) / 256, rows = (height + 255) / 256;
    const auto sourceTiles = projection ? projection->tiles() : source->tiles(region->layer);
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
    const auto definition = settings.threshold == 1 ? LibrePaint::Pigment::Color::NativeLabConversionSP{} : settings.labConversion;
    if (definition && !definition->isValid())
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Invalid native Lab colour definition")};
    if (nativeLabHasDefinition != bool(definition) || nativeLabDefinition.toStrongRef() != definition) {
        nativeLabValues.reset(); nativeLabDefinition.clear();
    }
    const quint32 kind = !definition ? 0 : std::holds_alternative<LibrePaint::Pigment::Color::NativeLabConversion::IntegerClut>(definition->definition) ? 1 : 2;
    const quint64 labelBytes = ((count + bool(settings.boundaryColor)) * (kind == 2 ? 6 : kind == 1 ? 3 : 1) +
        (settings.antiAlias ? 4 : 0)) * sizeof(quint32);
    const quint64 labBytes = labBufferBytes(definition);
    const quint64 required = coverageBytes + labelBytes + atlasBytes + directoryBytes + selectionBytes + 20 + sizeof(RegionParameters) +
        (nativeLabValues ? 0 : labBytes);
    WGPULimits limits{};
    if (wgpuDeviceGetLimits(device->nativeDevice(), &limits) != WGPUStatus_Success ||
        !count || labelBytes / sizeof(quint32) >= 0xffffffffu || !directoryBytes ||
        (width + 7) / 8 > limits.maxComputeWorkgroupsPerDimension || (height + 7) / 8 > limits.maxComputeWorkgroupsPerDimension ||
        qMax(qMax(qMax(atlasBytes, directoryBytes), qMax(selectionBytes, qMax(coverageBytes, labelBytes))), labBytes) > qMin(limits.maxStorageBufferBindingSize, limits.maxBufferSize) ||
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
    if (!nativeLabValues) {
        const auto bytes = packNativeLab(definition);
        nativeLabValues = allocate(labBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        if (!nativeLabValues) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("Native Lab resource allocation failed")};
        wgpuQueueWriteBuffer(device->queue(), nativeLabValues->pixels.get(), 0, bytes.constData(), size_t(bytes.size()));
        nativeLabDefinition = definition;
        nativeLabHasDefinition = bool(definition);
    }
    const auto lab = nativeLabValues;
    device->retainForFrame(lab);
    const auto atlas = allocate(atlasBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    const auto directory = allocate(directoryBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    const auto selection = allocate(selectionBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    const auto labels = allocate(labelBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
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
    if (projection) {
        for (const auto &tile : projection->m_data->tiles) retainTile(tile);
    } else {
        for (const auto &tile : source->m_data->layers.value(region->layer)) retainTile(tile);
    }
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
    parameters.documentSize[0] = document.size.width(); parameters.documentSize[1] = document.size.height();
    parameters.seed[0] = quint32(settings.seed.x()); parameters.seed[1] = quint32(settings.seed.y());
    parameters.format = quint32(document.format); parameters.threshold = settings.threshold; parameters.spread = settings.opacitySpread;
    if (masked || !document.selection.isEmpty()) parameters.flags |= 1;
    if (document.selectionKind == SelectionKind::Ellipse) parameters.flags |= 2;
    if (masked) parameters.flags |= 4;
    if (masked && document.selectionMask && document.selectionMask->isAlpha()) parameters.flags |= 8;
    if (document.selectionInverted) parameters.flags |= 16;
    if (settings.selectionAsBoundary || settings.mode == RegionFillMode::Similar) parameters.flags |= 32;
    if (settings.mode == RegionFillMode::Similar) parameters.flags |= 64;
    if (settings.antiAlias) parameters.flags |= 256;
    if (settings.boundaryColor) {
        parameters.flags |= 128;
        for (int i = 0; i < 4; ++i) parameters.boundary[i] = (*settings.boundaryColor)[i];
    }
    const auto implicit = projection ? RgbaPixel{} : document.layers[0].defaultPixel;
    for (int i = 0; i < 4; ++i) parameters.implicit[i] = implicit[i];
    parameters.selection[0] = document.selection.x(); parameters.selection[1] = document.selection.y();
    parameters.selection[2] = document.selection.width(); parameters.selection[3] = document.selection.height();
    const quint32 initialStatus[5]{0, quint32(width), quint32(height), 0, 0};
    wgpuQueueWriteBuffer(device->queue(), status->pixels.get(), 0, initialStatus, sizeof(initialStatus));
    if (settings.antiAlias) wgpuQueueWriteBuffer(device->queue(), labels->pixels.get(), labelBytes - 16, initialStatus + 1, 16);
    wgpuQueueWriteBuffer(device->queue(), directory->pixels.get(), 0, entries.constData(), directoryBytes);
    wgpuQueueWriteBuffer(device->queue(), uniform->pixels.get(), 0, &parameters, sizeof(parameters));
    if (!regionLayout) {
        // One explicit layout serves alpha/exact, integer-table and linear
        // classifiers, whose unused arithmetic is removed during compilation.
        std::array<WGPUBindGroupLayoutEntry, 8> entries{};
        for (quint32 i = 0; i < entries.size(); ++i) {
            entries[i].binding = i;
            entries[i].visibility = WGPUShaderStage_Compute;
            entries[i].buffer.type = i == 0 ? WGPUBufferBindingType_Uniform :
                (i >= 4 && i <= 6) ? WGPUBufferBindingType_Storage : WGPUBufferBindingType_ReadOnlyStorage;
        }
        WGPUBindGroupLayoutDescriptor layout{};
        layout.entryCount = entries.size(); layout.entries = entries.data();
        regionLayout.reset(wgpuDeviceCreateBindGroupLayout(device->nativeDevice(), &layout));
    }
    if (regionLayout) {
        if (kind && !regionConversions[kind - 1]) regionConversions[kind - 1] = createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "convert", 0, 0, regionLayout.get(), false, 0, kind);
        if (kind == 2 && !regionEncoding) regionEncoding = createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "encode", 0, 0, regionLayout.get(), false, 0, kind);
        if (!regionClassifiers[kind]) regionClassifiers[kind] = createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "classify", 0, 0, regionLayout.get(), false, 0, kind);
        if (!regionConnect[kind]) regionConnect[kind] = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "connect", 0, 0, regionLayout.get(), false, 0, kind);
        if (settings.antiAlias && !regionAntiAlias[kind]) regionAntiAlias[kind] = createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "antiAlias", 0, 0, regionLayout.get(), false, 0, kind);
        if (settings.antiAlias && !regionFiltered[kind]) regionFiltered[kind] = createPipeline(
            QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "filtered", 0, 0, regionLayout.get(), false, 0, kind);
        if (!regionResolve[kind]) regionResolve[kind] = createPipeline(QStringLiteral(":/librepaint/gpu/shaders/region.wgsl"), "resolve", 0, 0, regionLayout.get(), false, 0, kind);
    }
    if ((settings.antiAlias && (!regionAntiAlias[kind] || !regionFiltered[kind])) ||
        (kind && !regionConversions[kind - 1]) || (kind == 2 && !regionEncoding) ||
        !regionClassifiers[kind] || !regionConnect[kind] || !regionResolve[kind] || !regionLayout)
        return {RenderOutcome::DeviceFailure, {}, error};
    const std::array<std::shared_ptr<BufferStorage>, 8> buffers{uniform, atlas, directory, selection, labels, coverage, status, lab};
    std::array<WGPUBindGroupEntry, 8> bindings{};
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
    QList<QPair<WGPUComputePipeline, quint32>> stages;
    if (kind) stages.append({regionConversions[kind - 1].get(), (kind == 2 ? 3u : 1u) * (settings.boundaryColor ? 2u : 1u)});
    if (kind == 2) stages.append({regionEncoding.get(), settings.boundaryColor ? 2u : 1u});
    stages.append({regionClassifiers[kind].get(), 1u});
    if (settings.mode == RegionFillMode::Contiguous) stages.append({regionConnect[kind].get(), 1u});
    stages.append({regionResolve[kind].get(), 1u});
    if (settings.antiAlias) {
        stages.append({regionAntiAlias[kind].get(), 1u});
        stages.append({regionFiltered[kind].get(), 1u});
    }
    for (const auto &stage : stages) {
        const auto pipeline = stage.first;
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU contiguous fill commands failed")};
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, group.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, quint32((width + 7) / 8), quint32((height + 7) / 8), stage.second);
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
    const QRect area(QPoint(), region->source->size);
    QList<LayerId> layers{region->layer};
    const bool merged = region->settings.reference == RegionFillReference::AllLayers;
    if (merged) {
        layers.clear();
        for (const auto &layer : region->source->layers) layers.append(layer.id);
    }
    const auto source = recordRegion(commands, region->source, area, layers, compatible);
    if (source.code != RenderOutcome::Recorded) return source;
    ProjectedSnapshot projection;
    if (merged) {
        const auto result = compose(commands, source.version, area);
        if (result.code != RenderOutcome::Recorded) return {result.code, {}, result.diagnostic};
        projection = result.version;
    }
    const auto coverage = m_d->regionCoverage(commands, region, source.version, projection);
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
