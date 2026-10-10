/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
using namespace KisGpuTileStorage;

KisGpuTileStore::Private::DabPipelines::~DabPipelines()
{
    for (const auto pipeline : pipelines) if (pipeline) wgpuComputePipelineRelease(pipeline);
    for (const auto layout : layouts) if (layout) wgpuBindGroupLayoutRelease(layout);
}

std::unique_ptr<KisGpuTileStore::Private::DabPipelines>
KisGpuTileStore::Private::createDabPipelines(WGPUShaderModule shader, quint32 inputBinding,
                                            const char *entryPoint, const char *selectedEntryPoint)
{
    auto result = std::make_unique<DabPipelines>();
    for (int selected = 0; selected < 2; ++selected) {
        WGPUBindGroupLayoutEntry entries[5]{};
        const quint32 bindings[] = {0, 1, 2, inputBinding, 5};
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
        result->layouts[selected] = wgpuDeviceCreateBindGroupLayout(state->device, &groupDescriptor);
        if (!result->layouts[selected]) throw std::runtime_error("Cannot create GPU dab layout");
        WGPUPipelineLayoutDescriptor layoutDescriptor{};
        layoutDescriptor.bindGroupLayoutCount = 1;
        layoutDescriptor.bindGroupLayouts = &result->layouts[selected];
        Handle<WGPUPipelineLayout, wgpuPipelineLayoutRelease> pipelineLayout(wgpuDeviceCreatePipelineLayout(state->device, &layoutDescriptor));
        WGPUComputePipelineDescriptor descriptor{};
        descriptor.layout = pipelineLayout.value;
        descriptor.compute.module = shader;
        descriptor.compute.entryPoint = {selected ? selectedEntryPoint : entryPoint, WGPU_STRLEN};
        result->pipelines[selected] = wgpuDeviceCreateComputePipeline(state->device, &descriptor);
        if (!result->pipelines[selected]) throw std::runtime_error("Cannot create GPU dab pipeline");
    }
    return result;
}
