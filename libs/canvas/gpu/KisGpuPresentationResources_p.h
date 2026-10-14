/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PRESENTATION_RESOURCES_P_H
#define KIS_GPU_PRESENTATION_RESOURCES_P_H
#include <webgpu/wgpu.h>
#include <memory>
#include <type_traits>
namespace LibrePaint::Canvas::Gpu::Detail {
template<class Handle, void (*Release)(Handle)> struct ResourceDeleter
{
    void operator()(std::remove_pointer_t<Handle> *value) const noexcept
    { if (value) Release(value); }
};
template<class Handle, void (*Release)(Handle)>
using Resource = std::unique_ptr<std::remove_pointer_t<Handle>, ResourceDeleter<Handle, Release>>;
using Buffer = Resource<WGPUBuffer, wgpuBufferRelease>;
using Bindings = Resource<WGPUBindGroup, wgpuBindGroupRelease>;
using Layout = Resource<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease>;
using Shader = Resource<WGPUShaderModule, wgpuShaderModuleRelease>;
using Pipeline = Resource<WGPURenderPipeline, wgpuRenderPipelineRelease>;
using Texture = Resource<WGPUTexture, wgpuTextureRelease>;
using TextureView = Resource<WGPUTextureView, wgpuTextureViewRelease>;
}
#endif
