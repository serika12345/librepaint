/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStorage_p.h"
#include "KisGpuProjectionParameters_p.h"
#include "KisGpuReadbackBatch_p.h"
#include <QScopeGuard>
#include <QThread>
#include <array>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;

RenderOutcome TileRenderer::prepareDefaultPixels(const Snapshot &document)
{
    auto &d = *m_d;
    if (!document || d.device->thread() != QThread::currentThread())
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Uniform pixels require the owning GPU thread")};
    if (document->projectionDefaultsReady()) return {RenderOutcome::Recorded, {}, {}};
    const auto key = document->projectionDefaultsKey();
    if (key == d.defaultPixelsKey) {
        document->setProjectionDefaultPixels(key, d.rawDefaultPixels, d.maskedDefaultPixels);
        return {RenderOutcome::Recorded, {}, {}};
    }
    if (d.device->isRecordingFrame())
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Prepare uniform pixels before recording a frame")};
    if (d.device->readbackPending() && !d.device->finish())
        return {RenderOutcome::DeviceFailure, {}, d.device->diagnostic()};
    const auto commands = d.device->beginCommands();
    if (!commands) return {RenderOutcome::DeviceFailure, {}, d.device->diagnostic()};
    const auto rollback = qScopeGuard([&] {
        if (d.device->isRecordingFrame()) d.device->submitCommands(true);
    });
    // The ordinary 8x8 projection workgroup addresses pixels with a 256-pixel
    // row stride. Only its first pixel is read; every composition step uses it.
    constexpr int groupSide = 8;
    constexpr int sampleCount = (groupSide - 1) * TileSize + groupSide;
    const quint64 workingPixelBytes = quint64(sampleCount) * pixelByteCount(document->format);
    bool capacityExceeded = false;
    const auto allocate = [&](quint64 bytes, WGPUBufferUsage usage) -> std::shared_ptr<BufferStorage> {
        if (bytes > quint64(qMax<qsizetype>(0, d.budget->limit - d.budget->used))) {
            capacityExceeded = true; return {};
        }
        auto native = createBuffer(d.device->nativeDevice(), bytes, usage);
        if (!native) return {};
        auto buffer = std::make_shared<BufferStorage>(std::move(native), d.budget);
        d.device->retainForFrame(buffer);
        return buffer;
    };
    const auto failed = [&]() -> RenderOutcome {
        return {capacityExceeded ? RenderOutcome::CapacityExceeded : RenderOutcome::DeviceFailure, {},
            d.error.isEmpty() ? QStringLiteral("GPU uniform-pixel working storage is unavailable") : d.error};
    };
    const auto constant = [&](const RgbaPixel &pixel) -> std::shared_ptr<BufferStorage> {
        auto buffer = allocate(workingPixelBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        if (!buffer) return {};
        const auto bytes = rgbaBytes(pixel, document->format).repeated(sampleCount);
        wgpuQueueWriteBuffer(d.device->queue(), buffer->pixels.get(), 0, bytes.constData(), bytes.size());
        return buffer;
    };
    const auto zero = constant({});
    if (!zero) return failed();
    const auto blend = [&](const std::shared_ptr<BufferStorage> &base, const std::shared_ptr<BufferStorage> &source,
                           LayerBlendMode mode, quint8 opacity, quint8 channels, quint8 coverage) -> std::shared_ptr<BufferStorage> {
        const auto program = d.blendPipeline(false, false, document->format, quint32(mode));
        if (!program) return {};
        const auto output = allocate(workingPixelBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
        const auto uniform = allocate(sizeof(CompositeParameters), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        const auto mask = allocate(sampleCount, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        if (!output || !uniform || !mask) return {};
        CompositeParameters values;
        const float alpha = float(opacity) / 255.0f;
        values.opacity = alpha;
        values.flags = document->format == PixelFormat::Rgba16 ? 2u : 0u;
        values.channelMask = channels; values.blendModeOpacity = quint32(mode) | (quint32(opacity) << 8);
        values.sourceRight = values.sourceBottom = 1;
        const QByteArray maskAlpha(sampleCount, char(coverage));
        wgpuQueueWriteBuffer(d.device->queue(), uniform->pixels.get(), 0, &values, sizeof(values));
        wgpuQueueWriteBuffer(d.device->queue(), mask->pixels.get(), 0, maskAlpha.constData(), maskAlpha.size());
        std::array<WGPUBindGroupEntry, 5> entries{};
        const quint32 bindings[]{0, 1, 2, 3, 6};
        const WGPUBuffer buffers[]{base->pixels.get(), source->pixels.get(), output->pixels.get(), uniform->pixels.get(), mask->pixels.get()};
        for (size_t i = 0; i < entries.size(); ++i) {
            entries[i].binding = bindings[i]; entries[i].buffer = buffers[i]; entries[i].size = wgpuBufferGetSize(buffers[i]);
        }
        WGPUBindGroupDescriptor descriptor{}; descriptor.layout = d.projectionLayout.get();
        descriptor.entryCount = entries.size(); descriptor.entries = entries.data();
        NativeBindings group(wgpuDeviceCreateBindGroup(d.device->nativeDevice(), &descriptor));
        if (!group) return {};
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        if (!pass) return {};
        wgpuComputePassEncoderSetPipeline(pass, program);
        wgpuComputePassEncoderSetBindGroup(pass, 0, group.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, 1, 1, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
        return output;
    };
    QHash<LayerId, std::shared_ptr<BufferStorage>> pixels, masked;
    QHash<LayerId, RgbaPixel> rawValues, maskedValues;
    QHash<LayerId, bool> implicit;
    QHash<LayerId, const Layer *> nodes;
    QHash<QUuid, const Layer *> identities;
    QHash<QUuid, QList<const Layer *>> children;
    QHash<LayerId, int> remaining;
    QList<const Layer *> ready;
    for (const auto &layer : document->layers) {
        nodes.insert(layer.id, &layer); identities.insert(layer.uuid, &layer);
        children[layer.parentUuid].append(&layer);
        if (layer.kind == LayerKind::Group) remaining.insert(layer.id, 0);
        else {
            const auto value = layer.kind == LayerKind::Paint ? layer.defaultPixel : RgbaPixel{};
            rawValues.insert(layer.id, value);
            pixels.insert(layer.id, value == RgbaPixel{} ? zero : constant(value));
            if (!pixels.value(layer.id)) return failed();
            implicit.insert(layer.id, value[3] != 0);
        }
    }
    for (const auto &layer : document->layers) if (layer.kind == LayerKind::Group && !layer.parentUuid.isNull()) {
        const auto parent = identities.value(layer.parentUuid);
        if (!parent || parent->kind != LayerKind::Group)
            return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Invalid group parent for uniform pixels")};
        ++remaining[parent->id];
    }
    for (const auto &layer : document->layers)
        if (layer.kind == LayerKind::Group && !remaining.value(layer.id)) ready.append(&layer);
    for (qsizetype index = 0; index < ready.size(); ++index) {
        const auto node = ready[index];
        const auto ownChildren = children.value(node->uuid);
        const auto inputs = document->projectionInputs(node->id);
        auto output = zero;
        bool allocated = false;
        if (ownChildren.size() == 1 && !ownChildren.first()->passThrough && inputs.size() == 1 &&
            inputs.first().opacity == 255 && inputs.first().channelMask == 15 &&
            inputs.first().blendMode == LayerBlendMode::Normal && !hasEnabledTransparencyMask(*ownChildren.first())) {
            output = pixels.value(inputs.first().layer);
            allocated = implicit.value(inputs.first().layer);
        } else for (const auto &input : inputs) {
            const auto layer = nodes.value(input.layer);
            if (layer->kind == LayerKind::Adjustment) continue;
            const bool rectangle = input.blendMode == LayerBlendMode::Copy || input.blendMode == LayerBlendMode::DestinationIn ||
                input.blendMode == LayerBlendMode::DestinationAtop;
            if ((!allocated && !(input.channelMask & 8u) && input.blendMode != LayerBlendMode::Copy) ||
                (!implicit.value(input.layer) && !rectangle)) continue;
            allocated = true;
            output = blend(output, pixels.value(input.layer), input.blendMode, input.opacity, input.channelMask,
                hasEnabledTransparencyMask(*layer) ? layer->transparencyMask->defaultCoverage() : 255);
            if (!output) return failed();
        }
        pixels.insert(node->id, output); implicit.insert(node->id, allocated);
        if (!node->parentUuid.isNull() && --remaining[identities.value(node->parentUuid)->id] == 0)
            ready.append(identities.value(node->parentUuid));
    }
    if (pixels.size() != document->layers.size())
        return {RenderOutcome::InvalidVersion, {}, QStringLiteral("Invalid group hierarchy for uniform pixels")};
    QList<QPair<LayerId, bool>> reads;
    for (const auto &layer : document->layers) {
        if (layer.kind == LayerKind::Group) reads.append({layer.id, false});
        const auto coverage = hasEnabledTransparencyMask(layer) ? layer.transparencyMask->defaultCoverage() : 255;
        if (coverage != 255) {
            const auto value = blend(zero, pixels.value(layer.id), LayerBlendMode::Copy, 255, 15, coverage);
            if (!value) return failed();
            masked.insert(layer.id, value); reads.append({layer.id, true});
        }
    }
    const auto batch = ReadbackBatch::create(*d.device, reads.size());
    if (!batch) return failed();
    for (qsizetype index = 0; index < reads.size(); ++index) {
        const auto [id, mask] = reads[index];
        const auto value = mask ? masked.value(id) : pixels.value(id);
        if (!batch->recordBuffer(*d.device, commands, index, value->pixels.get(), 8)) return failed();
    }
    if (!d.device->submitCommands(true)) return {RenderOutcome::DeviceFailure, {}, d.device->diagnostic()};
    for (qsizetype index = 0; index < reads.size(); ++index) {
        const auto [id, mask] = reads[index];
        if (batch->results[index]->data.size() != 8)
            return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU uniform pixel read is incomplete")};
        const auto value = rgbaPixel(QByteArrayView(batch->results[index]->data).first(pixelByteCount(document->format)), document->format);
        if (!value) return {RenderOutcome::DeviceFailure, {}, QStringLiteral("GPU uniform pixel read is incomplete")};
        (mask ? maskedValues : rawValues).insert(id, *value);
    }
    for (const auto &layer : document->layers)
        if (!maskedValues.contains(layer.id)) maskedValues.insert(layer.id, rawValues.value(layer.id));
    d.defaultPixelsKey = key; d.rawDefaultPixels = rawValues; d.maskedDefaultPixels = maskedValues;
    document->setProjectionDefaultPixels(key, rawValues, maskedValues);
    return {RenderOutcome::Recorded, {}, {}};
}
}
