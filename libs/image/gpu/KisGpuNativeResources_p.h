/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_NATIVE_RESOURCES_P_H
#define KIS_GPU_NATIVE_RESOURCES_P_H

#include <webgpu/wgpu.h>
#include <QtGlobal>
#include <memory>
#include <type_traits>

namespace LibrePaint::Image::Gpu::Detail {
template<class Handle, void (*Release)(Handle)> struct NativeDeleter
{
    void operator()(std::remove_pointer_t<Handle> *value) const noexcept
    { if (value) Release(value); }
};
template<class Handle, void (*Release)(Handle)>
using NativeResource = std::unique_ptr<std::remove_pointer_t<Handle>, NativeDeleter<Handle, Release>>;

using NativeInstance = NativeResource<WGPUInstance, wgpuInstanceRelease>;
using NativeAdapter = NativeResource<WGPUAdapter, wgpuAdapterRelease>;
using NativeDevice = NativeResource<WGPUDevice, wgpuDeviceRelease>;
using NativeQueue = NativeResource<WGPUQueue, wgpuQueueRelease>;
using NativeCommands = NativeResource<WGPUCommandEncoder, wgpuCommandEncoderRelease>;
using NativeSubmission = NativeResource<WGPUCommandBuffer, wgpuCommandBufferRelease>;
using NativeSurface = NativeResource<WGPUSurface, wgpuSurfaceRelease>;
using NativeTexture = NativeResource<WGPUTexture, wgpuTextureRelease>;
using NativeTextureView = NativeResource<WGPUTextureView, wgpuTextureViewRelease>;
using NativeBuffer = NativeResource<WGPUBuffer, wgpuBufferRelease>;
using NativeShader = NativeResource<WGPUShaderModule, wgpuShaderModuleRelease>;
using NativeBindings = NativeResource<WGPUBindGroup, wgpuBindGroupRelease>;
using NativeBindingLayout = NativeResource<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease>;
using NativeComputePipeline = NativeResource<WGPUComputePipeline, wgpuComputePipelineRelease>;

inline NativeBuffer createBuffer(WGPUDevice device, quint64 size, WGPUBufferUsage usage)
{
    WGPUBufferDescriptor descriptor{};
    descriptor.size = size;
    descriptor.usage = usage;
    return NativeBuffer(wgpuDeviceCreateBuffer(device, &descriptor));
}
}
#endif
