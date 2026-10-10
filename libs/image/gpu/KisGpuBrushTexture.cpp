/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <limits>
#include <utility>

using namespace KisGpuTileStorage;

KisGpuTileStore::BrushTextureResult KisGpuTileStore::uploadBrushTexture(QSize size, const QByteArray &alpha)
{
    if (!deviceAvailable()) return {Error::DeviceLost, {}, {}};
    const qint64 count = qint64(size.width()) * size.height();
    if (size.width() <= 0 || size.height() <= 0 || count != alpha.size()) return {Error::InvalidCommand, {}, {}};
    const quint64 bytes = (quint64(count) + 3) / 4 * 4;
    if (bytes > std::min<quint64>({d->availableForOperation(0), d->limits.maxBufferSize,
        d->limits.maxStorageBufferBindingSize, quint64(std::numeric_limits<quint32>::max())})) {
        return {Error::BudgetExceeded, {}, {}};
    }
    if (d->pending.size() >= d->maximumPending) return {Error::QueueFull, {}, {}};
    BrushTextureResult result;
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto allocation = std::make_shared<Allocation>(d->state, bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto data = std::make_shared<BrushTextureData>(BrushTextureData{allocation, result.completion.d, size});
    QByteArray packed = alpha;
    packed.resize(qsizetype(bytes), '\0');
    Private::Recording encoder(d->state, 0);
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> buffer(encoder.finish());
    Private::Pending pending{result.completion.d, {}, {}, allocation, {}, {}};
    pending.brushTexture = data;
    d->submit(std::move(pending), buffer.value, {}, {}, 0, 0, packed);
    result.texture.d = std::move(data);
    errors.submitted = true;
    return result;
}

void KisGpuTileStore::Private::initializeBrushTexturePipelines(WGPUShaderModule shader)
{
    // Both current consumers use the same prepared pattern; selection adds one concrete input.
    std::array<std::unique_ptr<Handle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease>>, 2> layouts;
    std::array<std::unique_ptr<Handle<WGPUComputePipeline, wgpuComputePipelineRelease>>, 2> pipelines;
    for (int selected = 0; selected < 2; ++selected) {
        WGPUBindGroupLayoutEntry entries[5]{};
        const quint32 bindings[] = {0, 1, 2, 6, 5};
        const quint64 sizes[] = {TileBytes, sizeof(TileParameters), sizeof(TileCommand), 4, TileBytes};
        for (int i = 0; i < 4 + selected; ++i) {
            entries[i].binding = bindings[i];
            entries[i].visibility = WGPUShaderStage_Compute;
            entries[i].buffer.type = i ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;
            entries[i].buffer.minBindingSize = sizes[i];
        }
        WGPUBindGroupLayoutDescriptor groupDescriptor{};
        groupDescriptor.entryCount = 4 + selected;
        groupDescriptor.entries = entries;
        layouts[selected] = std::make_unique<Handle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease>>(
            wgpuDeviceCreateBindGroupLayout(state->device, &groupDescriptor));
        WGPUPipelineLayoutDescriptor layoutDescriptor{};
        layoutDescriptor.bindGroupLayoutCount = 1;
        layoutDescriptor.bindGroupLayouts = &layouts[selected]->value;
        Handle<WGPUPipelineLayout, wgpuPipelineLayoutRelease> pipelineLayout(wgpuDeviceCreatePipelineLayout(state->device, &layoutDescriptor));
        WGPUComputePipelineDescriptor descriptor{};
        descriptor.layout = pipelineLayout.value;
        descriptor.compute.module = shader;
        descriptor.compute.entryPoint = {selected ? "paintSelectedTextured" : "paintTextured", WGPU_STRLEN};
        pipelines[selected] = std::make_unique<Handle<WGPUComputePipeline, wgpuComputePipelineRelease>>(
            wgpuDeviceCreateComputePipeline(state->device, &descriptor));
    }
    dabTextureLayout = std::exchange(layouts[0]->value, nullptr);
    dabSelectedTextureLayout = std::exchange(layouts[1]->value, nullptr);
    dabTexturePipeline = std::exchange(pipelines[0]->value, nullptr);
    dabSelectedTexturePipeline = std::exchange(pipelines[1]->value, nullptr);
}
