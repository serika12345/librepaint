/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuImageRenderer.h"

#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>
#include <webgpu/wgpu.h>

namespace Krita::Canvas
{
namespace
{
template<typename T, void (*Release)(T)>
class GpuHandle
{
public:
    explicit GpuHandle(T value = nullptr) : m_value(value) {}
    ~GpuHandle() { reset(); }
    GpuHandle(const GpuHandle &) = delete;
    GpuHandle &operator=(const GpuHandle &) = delete;
    operator T() const { return m_value; }
    void reset(T value = nullptr)
    {
        if (m_value) Release(m_value);
        m_value = value;
    }
private:
    T m_value;
};

QString messageText(WGPUStringView message)
{
    if (!message.data) return {};
    return message.length == WGPU_STRLEN ? QString::fromUtf8(message.data)
                                        : QString::fromUtf8(message.data, int(message.length));
}

constexpr char imageShader[] = R"WGSL(
@group(0) @binding(0) var image: texture_2d<f32>;
@group(0) @binding(1) var imageSampler: sampler;
struct Vertex { @builtin(position) position: vec4<f32>, @location(0) uv: vec2<f32> };
@vertex fn vs_main(@builtin(vertex_index) index: u32) -> Vertex {
    let positions = array<vec2<f32>, 3>(vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    let p = positions[index];
    return Vertex(vec4(p, 0.0, 1.0), vec2((p.x + 1.0) * 0.5, (1.0 - p.y) * 0.5));
}
@fragment fn fs_main(v: Vertex) -> @location(0) vec4<f32> {
    return textureSample(image, imageSampler, v.uv);
}
)WGSL";
constexpr char projectionShader[] = R"WGSL(
@group(0) @binding(0) var image: texture_2d<f32>;
@group(0) @binding(1) var imageSampler: sampler;
@group(0) @binding(2) var<uniform> geometry: array<vec4<f32>,4>;
struct Vertex { @builtin(position) position: vec4<f32>, @location(0) uv: vec2<f32> };
@vertex fn vs_main(@builtin(vertex_index) index: u32) -> Vertex {
    let positions = array<vec2<f32>, 3>(vec2(-1.0,-1.0),vec2(3.0,-1.0),vec2(-1.0,3.0));
    let p=positions[index]; return Vertex(vec4(p,0.0,1.0),vec2(0.0));
}
@fragment fn fs_main(v: Vertex) -> @location(0) vec4<f32> {
    let p=v.position.xy/geometry[1].z;
    let t=geometry[0];
    let viewport=vec2(p.x*t.x+p.y*t.z,p.x*t.y+p.y*t.w)+geometry[1].xy;
    let dim=vec2<f32>(textureDimensions(image));
    let color=textureSample(image,imageSampler,viewport/dim);
    let bounds=geometry[3];
    if(p.x<bounds.x || p.y<bounds.y || p.x>=bounds.z || p.y>=bounds.w
       || viewport.x<0.0 || viewport.y<0.0 || viewport.x>=dim.x || viewport.y>=dim.y) {return geometry[2];}
    return color;
}
)WGSL";
}

struct WgpuImageRenderer::Private
{
    QThread *const owner = QThread::currentThread();
    mutable QMutex errorMutex;
    QString lastError;
    QString gpuError;
    WGPUBackendType backend = WGPUBackendType_Undefined;
    QSize imageSize;
    quint64 uploadedBytes = 0;
    WGPUSurfaceConfiguration surfaceConfig = {};
    WGPUSurfaceConfigurationExtras surfaceExtras = {};
    bool surfaceConfigured = false;
    bool surfaceCopySupported = false;
    uint32_t maxDimension = 0;
    uint64_t maxBufferSize = 0;
    bool directBgra = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_DIRECT_BGRA");
    bool gpuProjection = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_GPU_PROJECTION");
    bool stagingPool = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_STAGING_POOL");
    bool stagingBatch = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_STAGING_BATCH");
    int stagingMinBytes = qMax(0, qEnvironmentVariableIntValue("LIBREPAINT_WGPU_STAGING_MIN_BYTES"));

    // Reverse destruction releases image/pipeline resources before their device.
    GpuHandle<WGPUInstance, wgpuInstanceRelease> instance;
    GpuHandle<WGPUAdapter, wgpuAdapterRelease> adapter;
    GpuHandle<WGPUDevice, wgpuDeviceRelease> device;
    GpuHandle<WGPUQueue, wgpuQueueRelease> queue;
    GpuHandle<WGPUSurface, wgpuSurfaceRelease> surface;
    GpuHandle<WGPUShaderModule, wgpuShaderModuleRelease> shader;
    GpuHandle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease> bindingLayout;
    GpuHandle<WGPUPipelineLayout, wgpuPipelineLayoutRelease> pipelineLayout;
    GpuHandle<WGPUSampler, wgpuSamplerRelease> sampler;
    GpuHandle<WGPURenderPipeline, wgpuRenderPipelineRelease> readbackPipeline;
    GpuHandle<WGPURenderPipeline, wgpuRenderPipelineRelease> surfacePipeline;
    GpuHandle<WGPUTexture, wgpuTextureRelease> imageTexture;
    GpuHandle<WGPUTextureView, wgpuTextureViewRelease> imageView;
    GpuHandle<WGPUBindGroup, wgpuBindGroupRelease> bindings;
    GpuHandle<WGPUBuffer, wgpuBufferRelease> geometryBuffer;
    GpuHandle<WGPUTexture, wgpuTextureRelease> layerTexture;
    GpuHandle<WGPUComputePipeline, wgpuComputePipelineRelease> layerPipeline;
    GpuHandle<WGPUBindGroup, wgpuBindGroupRelease> layerBindings;
    GpuHandle<WGPUBuffer, wgpuBufferRelease> layerRegion;
    QRect layerDirty;
    struct UploadBuffer {
        GpuHandle<WGPUBuffer, wgpuBufferRelease> buffer;
        size_t capacity = 0;
        std::atomic<bool> ready{true};
    };
    std::array<UploadBuffer, 3> uploadBuffers;
    size_t nextUpload = 0;
    struct PendingCopy { size_t offset; uint32_t rowBytes; QRect rect; WGPUTexture texture; uint32_t layer; };
    std::vector<PendingCopy> pendingCopies;
    size_t pendingBytes = 0;

    bool waitUploadBuffer(UploadBuffer &slot)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!slot.ready.load()) {
            wgpuDevicePoll(device, false, nullptr);
            if (!healthy()) return false;
            if (std::chrono::steady_clock::now() > deadline) return fail(QStringLiteral("Upload buffer map timeout"));
            if (!slot.ready.load()) std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        return healthy();
    }

    void remapUploadBuffer(UploadBuffer &slot)
    {
        WGPUBufferMapCallbackInfo callback = {};
        callback.mode = WGPUCallbackMode_AllowSpontaneous;
        callback.userdata1 = &slot;
        callback.userdata2 = this;
        callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *owner) {
            auto *d = static_cast<Private *>(owner);
            if (status != WGPUMapAsyncStatus_Success) {
                QMutexLocker lock(&d->errorMutex);
                d->gpuError = QStringLiteral("Upload buffer remapping failed");
            }
            static_cast<UploadBuffer *>(data)->ready.store(true);
        };
        wgpuBufferMapAsync(slot.buffer, WGPUMapMode_Write, 0, slot.capacity, callback);
        wgpuDevicePoll(device, false, nullptr);
    }

    bool flushUploads()
    {
        if (pendingCopies.empty()) return true;
        auto &slot = uploadBuffers[nextUpload++ % uploadBuffers.size()];
        slot.ready.store(false);
        wgpuBufferUnmap(slot.buffer);
        GpuHandle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(wgpuDeviceCreateCommandEncoder(device, nullptr));
        for (const auto &copy : pendingCopies) {
            WGPUTexelCopyBufferInfo source = {};
            source.buffer = slot.buffer;
            source.layout.offset = copy.offset;
            source.layout.bytesPerRow = copy.rowBytes;
            source.layout.rowsPerImage = uint32_t(copy.rect.height());
            WGPUTexelCopyTextureInfo destination = {};
            destination.texture = copy.texture;
            destination.origin = {uint32_t(copy.rect.x()), uint32_t(copy.rect.y()), copy.layer};
            const WGPUExtent3D extent = {uint32_t(copy.rect.width()), uint32_t(copy.rect.height()), 1};
            wgpuCommandEncoderCopyBufferToTexture(encoder, &source, &destination, &extent);
        }
        GpuHandle<WGPUCommandBuffer, wgpuCommandBufferRelease> command(wgpuCommandEncoderFinish(encoder, nullptr));
        WGPUCommandBuffer submitted = command;
        wgpuQueueSubmit(queue, 1, &submitted);
        pendingCopies.clear();
        pendingBytes = 0;
        remapUploadBuffer(slot);
        return healthy();
    }

    ~Private()
    {
        Q_ASSERT(owner == QThread::currentThread());
        if (device) wgpuDevicePoll(device, true, nullptr);
        if (surfaceConfigured) wgpuSurfaceUnconfigure(surface);
    }

    bool fail(const QString &message)
    {
        QMutexLocker lock(&errorMutex);
        lastError = message;
        return false;
    }

    bool ready()
    {
        Q_ASSERT(owner == QThread::currentThread());
        QMutexLocker lock(&errorMutex);
        lastError.clear();
        if (!gpuError.isEmpty()) {
            lastError = gpuError;
            return false;
        }
        if (!device) {
            lastError = QStringLiteral("GPU device is not initialized");
            return false;
        }
        return true;
    }

    bool healthy() const
    {
        QMutexLocker lock(&errorMutex);
        return gpuError.isEmpty();
    }

    bool validSize(QSize size)
    {
        if (size.isEmpty() || uint64_t(size.width()) > maxDimension || uint64_t(size.height()) > maxDimension) {
            return fail(QStringLiteral("Image dimensions exceed GPU limits or are empty"));
        }
        return true;
    }

    QImage readPixels(WGPUTexture texture, WGPURenderPipeline pipeline, QSize size);

    bool writeImage(const uchar *pixels, uint32_t sourceRowBytes, const QRect &rect, WGPUTexture texture = nullptr, uint32_t layer = 0)
    {
        WGPUTexelCopyTextureInfo destination = {};
        destination.texture = texture ? texture : WGPUTexture(imageTexture);
        destination.origin = {uint32_t(rect.x()), uint32_t(rect.y()), layer};
        destination.aspect = WGPUTextureAspect_All;
        const WGPUExtent3D extent = {uint32_t(rect.width()), uint32_t(rect.height()), 1};
        const size_t pixelBytes = size_t(rect.width()) * rect.height() * 4;
        if (!stagingPool || pixelBytes < size_t(stagingMinBytes)) {
            if (!flushUploads()) return false;
            WGPUTexelCopyBufferLayout layout = {};
            layout.bytesPerRow = sourceRowBytes;
            layout.rowsPerImage = uint32_t(rect.height());
            const size_t bytes = size_t(rect.height()-1) * sourceRowBytes + size_t(rect.width()) * 4;
            wgpuQueueWriteTexture(queue, &destination, pixels, bytes, &layout, &extent);
        } else {
            const uint32_t rowBytes = (uint32_t(rect.width()) * 4 + 255) & ~uint32_t(255);
            const size_t bytes = size_t(rowBytes) * rect.height();
            if (bytes > maxBufferSize) return fail(QStringLiteral("Upload buffer exceeds GPU limits"));
            if (stagingBatch) {
                if (!pendingCopies.empty() && pendingBytes + bytes > uploadBuffers[nextUpload % uploadBuffers.size()].capacity) {
                    if (!flushUploads()) return false;
                }
                auto &slot = uploadBuffers[nextUpload % uploadBuffers.size()];
                if (pendingCopies.empty()) {
                    if (!waitUploadBuffer(slot)) return false;
                    if (slot.capacity < bytes) {
                        WGPUBufferDescriptor desc = {};
                        desc.size = qMin<uint64_t>(maxBufferSize, qMax<size_t>(bytes, 16 * 1024 * 1024));
                        desc.usage = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
                        desc.mappedAtCreation = true;
                        slot.buffer.reset(wgpuDeviceCreateBuffer(device, &desc));
                        slot.capacity = desc.size;
                    }
                }
                auto *mapped = static_cast<uchar *>(wgpuBufferGetMappedRange(slot.buffer, 0, slot.capacity));
                if (!mapped) return fail(QStringLiteral("Batched upload buffer mapping failed"));
                for (int y = 0; y < rect.height(); ++y) {
                    memcpy(mapped + pendingBytes + size_t(y) * rowBytes, pixels + size_t(y) * sourceRowBytes, size_t(rect.width()) * 4);
                }
                pendingCopies.push_back({pendingBytes, rowBytes, rect, destination.texture, layer});
                pendingBytes += bytes;
                uploadedBytes += pixelBytes;
                return healthy();
            }
            auto &slot = uploadBuffers[nextUpload++ % uploadBuffers.size()];
            if (!waitUploadBuffer(slot)) return false;
            if (slot.capacity < bytes) {
                WGPUBufferDescriptor desc = {};
                desc.size = bytes;
                desc.usage = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
                desc.mappedAtCreation = true;
                slot.buffer.reset(wgpuDeviceCreateBuffer(device, &desc));
                slot.capacity = bytes;
            }
            auto *mapped = static_cast<uchar *>(wgpuBufferGetMappedRange(slot.buffer, 0, bytes));
            if (!mapped) return fail(QStringLiteral("Upload buffer mapping failed"));
            for (int y = 0; y < rect.height(); ++y) {
                memcpy(mapped + size_t(y) * rowBytes, pixels + size_t(y) * sourceRowBytes, size_t(rect.width()) * 4);
            }
            slot.ready.store(false);
            wgpuBufferUnmap(slot.buffer);
            GpuHandle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(wgpuDeviceCreateCommandEncoder(device, nullptr));
            WGPUTexelCopyBufferInfo source = {};
            source.buffer = slot.buffer;
            source.layout.bytesPerRow = rowBytes;
            source.layout.rowsPerImage = uint32_t(rect.height());
            wgpuCommandEncoderCopyBufferToTexture(encoder, &source, &destination, &extent);
            GpuHandle<WGPUCommandBuffer, wgpuCommandBufferRelease> command(wgpuCommandEncoderFinish(encoder, nullptr));
            WGPUCommandBuffer submitted = command;
            wgpuQueueSubmit(queue, 1, &submitted);
            remapUploadBuffer(slot);
        }
        if (!healthy()) return false;
        uploadedBytes += pixelBytes;
        return true;
    }

    WGPURenderPipeline makePipeline(WGPUTextureFormat format)
    {
        WGPUColorTargetState target = {};
        target.format = format;
        target.writeMask = WGPUColorWriteMask_All;
        WGPUFragmentState fragment = {};
        fragment.module = shader;
        fragment.entryPoint = {"fs_main", WGPU_STRLEN};
        fragment.targetCount = 1;
        fragment.targets = &target;
        WGPURenderPipelineDescriptor desc = {};
        desc.layout = pipelineLayout;
        desc.vertex.module = shader;
        desc.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
        desc.fragment = &fragment;
        desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        desc.multisample.count = 1;
        desc.multisample.mask = 0xffffffff;
        return wgpuDeviceCreateRenderPipeline(device, &desc);
    }

    void draw(WGPUCommandEncoder encoder, WGPUTextureView target, WGPURenderPipeline pipeline)
    {
        if (layerPipeline && !layerDirty.isEmpty()) {
            const std::array<uint32_t,4> region = {uint32_t(layerDirty.x()),uint32_t(layerDirty.y()),uint32_t(layerDirty.width()),uint32_t(layerDirty.height())};
            wgpuQueueWriteBuffer(queue,layerRegion,0,region.data(),sizeof(region));
            GpuHandle<WGPUComputePassEncoder,wgpuComputePassEncoderRelease> compute(wgpuCommandEncoderBeginComputePass(encoder,nullptr));
            wgpuComputePassEncoderSetPipeline(compute,layerPipeline);
            wgpuComputePassEncoderSetBindGroup(compute,0,layerBindings,0,nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(compute,(layerDirty.width()+7)/8,(layerDirty.height()+7)/8,1);
            wgpuComputePassEncoderEnd(compute);
            layerDirty = {};
        }
        WGPURenderPassColorAttachment attachment = {};
        attachment.view = target;
        attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        attachment.loadOp = WGPULoadOp_Clear;
        attachment.storeOp = WGPUStoreOp_Store;
        WGPURenderPassDescriptor desc = {};
        desc.colorAttachmentCount = 1;
        desc.colorAttachments = &attachment;
        GpuHandle<WGPURenderPassEncoder, wgpuRenderPassEncoderRelease> pass(wgpuCommandEncoderBeginRenderPass(encoder, &desc));
        wgpuRenderPassEncoderSetPipeline(pass, pipeline);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindings, 0, nullptr);
        wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(pass);
    }
};

WgpuImageRenderer::WgpuImageRenderer() : m_d(new Private) {}
WgpuImageRenderer::~WgpuImageRenderer() = default;

bool WgpuImageRenderer::initialize(const WGPUChainedStruct *surfaceSource)
{
    Q_ASSERT(m_d->owner == QThread::currentThread());
    if (m_d->instance) return m_d->fail(QStringLiteral("GPU renderer is already initialized"));
    WGPUInstanceExtras extras = {};
    extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
    extras.flags = WGPUInstanceFlag_Validation;
#ifdef Q_OS_DARWIN
    extras.backends = WGPUInstanceBackend_Metal;
    const auto backend = WGPUBackendType_Metal;
#else
    extras.backends = WGPUInstanceBackend_Vulkan;
    const auto backend = WGPUBackendType_Vulkan;
#endif
    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = &extras.chain;
    m_d->instance.reset(wgpuCreateInstance(&instanceDesc));
    if (!m_d->instance) return m_d->fail(QStringLiteral("Cannot create GPU instance"));
    if (surfaceSource) {
        WGPUSurfaceDescriptor desc = {};
        desc.nextInChain = surfaceSource;
        m_d->surface.reset(wgpuInstanceCreateSurface(m_d->instance, &desc));
        if (!m_d->surface) return m_d->fail(QStringLiteral("Cannot create native GPU surface"));
    }

    // The pinned wgpu-native adapter/device requests complete synchronously.
    WGPURequestAdapterOptions options = {};
    options.backendType = backend;
    options.compatibleSurface = m_d->surface;
    WGPURequestAdapterCallbackInfo adapterCallback = {};
    adapterCallback.mode = WGPUCallbackMode_AllowSpontaneous;
    adapterCallback.userdata1 = m_d.get();
    adapterCallback.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *data, void *) {
        auto *d = static_cast<Private *>(data);
        d->adapter.reset(adapter);
        if (status != WGPURequestAdapterStatus_Success) d->fail(messageText(message));
    };
    wgpuInstanceRequestAdapter(m_d->instance, &options, adapterCallback);
    if (!m_d->adapter) return false;

    WGPUDeviceDescriptor desc = {};
    desc.uncapturedErrorCallbackInfo.userdata1 = m_d.get();
    desc.uncapturedErrorCallbackInfo.callback = [](const WGPUDevice *, WGPUErrorType, WGPUStringView message, void *data, void *) {
        auto *d = static_cast<Private *>(data);
        QMutexLocker lock(&d->errorMutex);
        d->gpuError = messageText(message);
    };
    desc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    desc.deviceLostCallbackInfo.userdata1 = m_d.get();
    desc.deviceLostCallbackInfo.callback = [](const WGPUDevice *, WGPUDeviceLostReason reason, WGPUStringView message, void *data, void *) {
        if (reason == WGPUDeviceLostReason_Destroyed) return;
        auto *d = static_cast<Private *>(data);
        QMutexLocker lock(&d->errorMutex);
        d->gpuError = QStringLiteral("GPU device lost: ") + messageText(message);
    };
    WGPURequestDeviceCallbackInfo deviceCallback = {};
    deviceCallback.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceCallback.userdata1 = m_d.get();
    deviceCallback.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *data, void *) {
        auto *d = static_cast<Private *>(data);
        d->device.reset(device);
        if (status != WGPURequestDeviceStatus_Success) d->fail(messageText(message));
    };
    wgpuAdapterRequestDevice(m_d->adapter, &desc, deviceCallback);
    if (!m_d->device) return false;
    m_d->queue.reset(wgpuDeviceGetQueue(m_d->device));
    WGPUAdapterInfo info = {};
    wgpuAdapterGetInfo(m_d->adapter, &info);
    m_d->backend = info.backendType;
    wgpuAdapterInfoFreeMembers(info);
    WGPULimits limits = {};
    wgpuDeviceGetLimits(m_d->device, &limits);
    m_d->maxDimension = limits.maxTextureDimension2D;
    m_d->maxBufferSize = limits.maxBufferSize;

    WGPUShaderSourceWGSL source = {};
    source.chain.sType = WGPUSType_ShaderSourceWGSL;
    source.code = {m_d->gpuProjection ? projectionShader : imageShader, WGPU_STRLEN};
    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = &source.chain;
    m_d->shader.reset(wgpuDeviceCreateShaderModule(m_d->device, &shaderDesc));
    WGPUBindGroupLayoutEntry entries[3] = {};
    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Fragment;
    entries[0].texture.sampleType = WGPUTextureSampleType_Float;
    entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Fragment;
    entries[1].sampler.type = WGPUSamplerBindingType_Filtering;
    entries[2].binding = 2;
    entries[2].visibility = WGPUShaderStage_Fragment;
    entries[2].buffer.type = WGPUBufferBindingType_Uniform;
    entries[2].buffer.minBindingSize = 64;
    WGPUBindGroupLayoutDescriptor bindingDesc = {};
    bindingDesc.entryCount = m_d->gpuProjection ? 3 : 2;
    bindingDesc.entries = entries;
    m_d->bindingLayout.reset(wgpuDeviceCreateBindGroupLayout(m_d->device, &bindingDesc));
    WGPUBindGroupLayout layout = m_d->bindingLayout;
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = &layout;
    m_d->pipelineLayout.reset(wgpuDeviceCreatePipelineLayout(m_d->device, &layoutDesc));
    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = m_d->gpuProjection ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
    samplerDesc.minFilter = m_d->gpuProjection ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    samplerDesc.lodMaxClamp = 1;
    samplerDesc.maxAnisotropy = 1;
    m_d->sampler.reset(wgpuDeviceCreateSampler(m_d->device, &samplerDesc));
    if (m_d->gpuProjection) {
        WGPUBufferDescriptor desc = {};
        desc.size = 64;
        desc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        m_d->geometryBuffer.reset(wgpuDeviceCreateBuffer(m_d->device, &desc));
    }
    m_d->readbackPipeline.reset(m_d->makePipeline(WGPUTextureFormat_RGBA8Unorm));
    return m_d->healthy();
}

bool WgpuImageRenderer::upload(const QImage &image, const QRect &dirty)
{
    if (!m_d->ready()) return false;
    if (image.isNull() || (!dirty.isEmpty() && !image.rect().contains(dirty))) {
        return m_d->fail(QStringLiteral("Image update is empty or outside image bounds"));
    }
    if (!m_d->validSize(image.size())) return false;
    if (dirty.isEmpty()) return true;
    if (image.size() != m_d->imageSize) {
        if (dirty != image.rect()) return m_d->fail(QStringLiteral("A new image requires a complete initial update"));
        if (!m_d->flushUploads()) return false;
        WGPUTextureDescriptor desc = {};
        desc.size = {uint32_t(image.width()), uint32_t(image.height()), 1};
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;
        desc.dimension = WGPUTextureDimension_2D;
        desc.format = m_d->directBgra ? WGPUTextureFormat_BGRA8Unorm : WGPUTextureFormat_RGBA8Unorm;
        desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        m_d->bindings.reset();
        m_d->imageView.reset();
        m_d->imageTexture.reset(wgpuDeviceCreateTexture(m_d->device, &desc));
        m_d->imageView.reset(wgpuTextureCreateView(m_d->imageTexture, nullptr));
        WGPUBindGroupEntry entries[3] = {};
        entries[0].binding = 0;
        entries[0].textureView = m_d->imageView;
        entries[1].binding = 1;
        entries[1].sampler = m_d->sampler;
        entries[2].binding = 2;
        entries[2].buffer = m_d->geometryBuffer;
        entries[2].size = 64;
        WGPUBindGroupDescriptor bindingDesc = {};
        bindingDesc.layout = m_d->bindingLayout;
        bindingDesc.entryCount = m_d->gpuProjection ? 3 : 2;
        bindingDesc.entries = entries;
        m_d->bindings.reset(wgpuDeviceCreateBindGroup(m_d->device, &bindingDesc));
        m_d->imageSize = image.size();
    }
    // Convert only the changed pixels; queueWriteTexture copies them before return.
    const bool direct = m_d->directBgra && (image.format() == QImage::Format_ARGB32_Premultiplied || image.format() == QImage::Format_ARGB32);
    const QImage patch = direct ? image : image.copy(dirty).convertToFormat(m_d->directBgra ? QImage::Format_ARGB32 : QImage::Format_RGBA8888);
    const uchar *data = patch.constBits() + (direct ? dirty.y()*patch.bytesPerLine() + dirty.x()*4 : 0);
    return m_d->writeImage(data, uint32_t(patch.bytesPerLine()), dirty);
}

bool WgpuImageRenderer::initializeLayers(const QVector<quint8> &opacities)
{
    if (!m_d->ready() || m_d->imageSize.isEmpty() || opacities.isEmpty() || !m_d->flushUploads()) return false;
    WGPUTextureDescriptor td = {};
    td.size = {uint32_t(m_d->imageSize.width()),uint32_t(m_d->imageSize.height()),uint32_t(opacities.size())};
    td.dimension=WGPUTextureDimension_2D;td.mipLevelCount=1;td.sampleCount=1;
    td.format=WGPUTextureFormat_RGBA8Unorm;td.usage=WGPUTextureUsage_TextureBinding|WGPUTextureUsage_CopyDst;
    m_d->layerTexture.reset(wgpuDeviceCreateTexture(m_d->device,&td));
    WGPUTextureViewDescriptor vd={};vd.dimension=WGPUTextureViewDimension_2DArray;vd.arrayLayerCount=opacities.size();vd.mipLevelCount=1;
    GpuHandle<WGPUTextureView,wgpuTextureViewRelease> inputView(wgpuTextureCreateView(m_d->layerTexture,&vd));
    td.size.depthOrArrayLayers=1;td.usage=WGPUTextureUsage_TextureBinding|WGPUTextureUsage_StorageBinding;
    m_d->bindings.reset();m_d->imageView.reset();m_d->imageTexture.reset(wgpuDeviceCreateTexture(m_d->device,&td));
    m_d->imageView.reset(wgpuTextureCreateView(m_d->imageTexture,nullptr));
    WGPUBindGroupEntry render[3]={};render[0].textureView=m_d->imageView;render[1].binding=1;render[1].sampler=m_d->sampler;
    render[2].binding=2;render[2].buffer=m_d->geometryBuffer;render[2].size=64;
    WGPUBindGroupDescriptor rd={};rd.layout=m_d->bindingLayout;rd.entryCount=m_d->gpuProjection?3:2;rd.entries=render;
    m_d->bindings.reset(wgpuDeviceCreateBindGroup(m_d->device,&rd));
    QByteArray code=R"WGSL(
@group(0) @binding(0) var layers:texture_2d_array<f32>;
@group(0) @binding(1) var result:texture_storage_2d<rgba8unorm,write>;
@group(0) @binding(2) var<uniform> region:vec4u;
@compute @workgroup_size(8,8) fn compose(@builtin(global_invocation_id) id:vec3u) {
 if(any(id.xy>=region.zw)){return;} let xy=vec2i(id.xy+region.xy);var c=vec4f(0.);
)WGSL";
    for(int i=0;i<opacities.size();++i) {
        code += "{let s=textureLoad(layers,xy,"+QByteArray::number(i)+",0)*"+QByteArray::number(double(opacities[i])/255,'f',9)+";c=round((s+c*(1.-s.a))*255.)/255.;}\n";
    }
    code += "textureStore(result,xy,c);}";
    WGPUShaderSourceWGSL source={};source.chain.sType=WGPUSType_ShaderSourceWGSL;source.code={code.constData(),size_t(code.size())};
    WGPUShaderModuleDescriptor sd={};sd.nextInChain=&source.chain;
    GpuHandle<WGPUShaderModule,wgpuShaderModuleRelease> module(wgpuDeviceCreateShaderModule(m_d->device,&sd));
    WGPUComputePipelineDescriptor pd={};pd.compute.module=module;pd.compute.entryPoint={"compose",WGPU_STRLEN};
    m_d->layerPipeline.reset(wgpuDeviceCreateComputePipeline(m_d->device,&pd));
    WGPUBufferDescriptor bd={};bd.size=16;bd.usage=WGPUBufferUsage_Uniform|WGPUBufferUsage_CopyDst;
    m_d->layerRegion.reset(wgpuDeviceCreateBuffer(m_d->device,&bd));
    GpuHandle<WGPUBindGroupLayout,wgpuBindGroupLayoutRelease> layout(wgpuComputePipelineGetBindGroupLayout(m_d->layerPipeline,0));
    WGPUBindGroupEntry entries[3]={};entries[0].textureView=inputView;entries[1].binding=1;entries[1].textureView=m_d->imageView;
    entries[2].binding=2;entries[2].buffer=m_d->layerRegion;entries[2].size=16;
    WGPUBindGroupDescriptor gd={};gd.layout=layout;gd.entryCount=3;gd.entries=entries;
    m_d->layerBindings.reset(wgpuDeviceCreateBindGroup(m_d->device,&gd));
    m_d->layerDirty=QRect(QPoint(),m_d->imageSize);
    return m_d->healthy();
}

bool WgpuImageRenderer::uploadLayerPatch(int layer,const QImage &image,const QRect &rect)
{
    if (!m_d->ready() || !m_d->layerTexture || image.size()!=rect.size()) return false;
    const QImage patch=image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    m_d->layerDirty |= rect;
    return m_d->writeImage(patch.constBits(),patch.bytesPerLine(),rect,m_d->layerTexture,uint32_t(layer));
}

void WgpuImageRenderer::setProjectionGeometry(const std::array<float,16> &geometry)
{
    if (m_d->geometryBuffer) wgpuQueueWriteBuffer(m_d->queue, m_d->geometryBuffer, 0, geometry.data(), 64);
}

bool WgpuImageRenderer::uploadPatch(const QImage &image, const QRect &rect)
{
    if (!m_d->ready() || !m_d->bindings || image.size()!=rect.size()
        || !QRect(QPoint(),m_d->imageSize).contains(rect)) return false;
    const QImage patch = image.convertToFormat(m_d->directBgra ? QImage::Format_ARGB32 : QImage::Format_RGBA8888);
    return m_d->writeImage(patch.constBits(), uint32_t(patch.bytesPerLine()), rect);
}

QImage WgpuImageRenderer::readback(QSize size)
{
    if (!m_d->ready()) return {};
    if (!m_d->bindings) {
        m_d->fail(QStringLiteral("No image has been uploaded"));
        return {};
    }
    if (size == QSize()) size = m_d->imageSize;
    if (!m_d->validSize(size)) return {};
    WGPUTextureDescriptor desc = {};
    desc.size = {uint32_t(size.width()), uint32_t(size.height()), 1};
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.dimension = WGPUTextureDimension_2D;
    desc.format = WGPUTextureFormat_RGBA8Unorm;
    desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    GpuHandle<WGPUTexture, wgpuTextureRelease> texture(wgpuDeviceCreateTexture(m_d->device, &desc));
    if (!m_d->healthy()) return {};
    return m_d->readPixels(texture, m_d->readbackPipeline, size);
}

QImage WgpuImageRenderer::Private::readPixels(WGPUTexture texture, WGPURenderPipeline pipeline, QSize size)
{
    if (!flushUploads()) return {};
    GpuHandle<WGPUTextureView, wgpuTextureViewRelease> view(wgpuTextureCreateView(texture, nullptr));
    const WGPUExtent3D extent = {uint32_t(size.width()), uint32_t(size.height()), 1};
    const uint32_t rowBytes = (uint32_t(size.width()) * 4 + 255) & ~uint32_t(255);
    const size_t bufferSize = size_t(rowBytes) * size.height();
    if (bufferSize > maxBufferSize) {
        fail(QStringLiteral("Readback buffer exceeds GPU limits"));
        return {};
    }
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = bufferSize;
    bufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    GpuHandle<WGPUBuffer, wgpuBufferRelease> buffer(wgpuDeviceCreateBuffer(device, &bufferDesc));
    GpuHandle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(wgpuDeviceCreateCommandEncoder(device, nullptr));
    draw(encoder, view, pipeline);
    WGPUTexelCopyTextureInfo src = {};
    src.texture = texture;
    src.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo dst = {};
    dst.buffer = buffer;
    dst.layout.bytesPerRow = rowBytes;
    dst.layout.rowsPerImage = uint32_t(size.height());
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &extent);
    GpuHandle<WGPUCommandBuffer, wgpuCommandBufferRelease> command(wgpuCommandEncoderFinish(encoder, nullptr));
    WGPUCommandBuffer submitted = command;
    wgpuQueueSubmit(queue, 1, &submitted);
    std::atomic<WGPUMapAsyncStatus> status{WGPUMapAsyncStatus_Error};
    WGPUBufferMapCallbackInfo callback = {};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = &status;
    callback.callback = [](WGPUMapAsyncStatus result, WGPUStringView, void *data, void *) {
        static_cast<std::atomic<WGPUMapAsyncStatus> *>(data)->store(result);
    };
    wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, bufferSize, callback);
    wgpuDevicePoll(device, true, nullptr);
    QImage image;
    if (status == WGPUMapAsyncStatus_Success && healthy()) {
        auto *pixels = static_cast<const uchar *>(wgpuBufferGetConstMappedRange(buffer, 0, bufferSize));
        if (pixels) image = QImage(pixels, size.width(), size.height(), rowBytes, QImage::Format_RGBA8888).copy();
    }
    wgpuBufferUnmap(buffer);
    if (image.isNull()) fail(QStringLiteral("GPU image readback failed"));
    return image;
}

bool WgpuImageRenderer::configureSurface(QSize size, WGPUPresentMode mode)
{
    if (!m_d->ready()) return false;
    if (!m_d->surface) return m_d->fail(QStringLiteral("A native surface is required"));
    if (!m_d->validSize(size)) return false;
    WGPUSurfaceCapabilities caps = {};
    if (wgpuSurfaceGetCapabilities(m_d->surface, m_d->adapter, &caps) != WGPUStatus_Success) {
        return m_d->fail(QStringLiteral("Cannot query GPU surface capabilities"));
    }
    bool modeSupported = false;
    for (size_t i = 0; i < caps.presentModeCount; ++i) modeSupported |= caps.presentModes[i] == mode;
    if (!modeSupported) {
        wgpuSurfaceCapabilitiesFreeMembers(caps);
        return m_d->fail(QStringLiteral("GPU surface does not support presentation mode %1").arg(int(mode)));
    }
    if (!m_d->surfacePipeline) {
        WGPUTextureFormat format = WGPUTextureFormat_Undefined;
        for (size_t i = 0; i < caps.formatCount; ++i) {
            if (caps.formats[i] == WGPUTextureFormat_BGRA8Unorm || caps.formats[i] == WGPUTextureFormat_RGBA8Unorm) {
                format = caps.formats[i];
                break;
            }
        }
        m_d->surfaceConfig.alphaMode = caps.alphaModeCount ? caps.alphaModes[0] : WGPUCompositeAlphaMode_Auto;
        m_d->surfaceCopySupported = (caps.usages & WGPUTextureUsage_CopySrc) != 0;
        if (format == WGPUTextureFormat_Undefined) {
            wgpuSurfaceCapabilitiesFreeMembers(caps);
            return m_d->fail(QStringLiteral("GPU surface requires an unsupported display color format"));
        }
        m_d->surfaceConfig.device = m_d->device;
        m_d->surfaceConfig.format = format;
        m_d->surfaceConfig.usage = WGPUTextureUsage_RenderAttachment
            | (m_d->surfaceCopySupported ? WGPUTextureUsage_CopySrc : 0);
        m_d->surfacePipeline.reset(m_d->makePipeline(format));
    }
    wgpuSurfaceCapabilitiesFreeMembers(caps);
    m_d->surfaceConfig.presentMode = mode;
    const int frameLatency = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_FRAME_LATENCY");
    if(frameLatency>0) {
        m_d->surfaceExtras.chain.sType=static_cast<WGPUSType>(WGPUSType_SurfaceConfigurationExtras);
        m_d->surfaceExtras.desiredMaximumFrameLatency=uint32_t(frameLatency);
        m_d->surfaceConfig.nextInChain=&m_d->surfaceExtras.chain;
    }
    m_d->surfaceConfig.width = uint32_t(size.width());
    m_d->surfaceConfig.height = uint32_t(size.height());
    wgpuSurfaceConfigure(m_d->surface, &m_d->surfaceConfig);
    m_d->surfaceConfigured = m_d->healthy();
    return m_d->surfaceConfigured;
}

bool WgpuImageRenderer::present(PresentationTiming *timing, const std::function<bool()> &beforeDraw)
{
    if (timing) *timing = {};
    if (!m_d->ready()) return false;
    if (!m_d->surfaceConfigured || !m_d->bindings) return m_d->fail(QStringLiteral("GPU surface or image is not ready"));
    WGPUSurfaceTexture frame = {};
    const auto nowNs = [] {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    };
    if (timing) timing->acquireStartNs = nowNs();
    wgpuSurfaceGetCurrentTexture(m_d->surface, &frame);
    if (timing) timing->acquireEndNs = nowNs();
    GpuHandle<WGPUTexture, wgpuTextureRelease> texture(frame.texture);
    if (frame.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
        && frame.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        return m_d->fail(QStringLiteral("GPU surface acquisition failed: %1").arg(int(frame.status)));
    }
    if (beforeDraw && !beforeDraw()) return false;
    if (timing) timing->beforeDrawEndNs = nowNs();
    if (!m_d->flushUploads()) return false;
    GpuHandle<WGPUTextureView, wgpuTextureViewRelease> view(wgpuTextureCreateView(texture, nullptr));
    GpuHandle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(wgpuDeviceCreateCommandEncoder(m_d->device, nullptr));
    m_d->draw(encoder, view, m_d->surfacePipeline);
    GpuHandle<WGPUCommandBuffer, wgpuCommandBufferRelease> command(wgpuCommandEncoderFinish(encoder, nullptr));
    WGPUCommandBuffer submitted = command;
    wgpuQueueSubmit(m_d->queue, 1, &submitted);
    if (timing) timing->submitEndNs = nowNs();
    const auto result = wgpuSurfacePresent(m_d->surface);
    if (timing) timing->presentEndNs = nowNs();
    if (qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_GPU_COMPLETION")) {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        while (!wgpuDevicePoll(m_d->device,false,nullptr)) {
            if (!m_d->healthy()) return false;
            if (std::chrono::steady_clock::now()>deadline) return m_d->fail(QStringLiteral("GPU completion timeout"));
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    } else wgpuDevicePoll(m_d->device,false,nullptr);
    if (timing) timing->gpuCompletionEndNs = nowNs();
    return result == WGPUStatus_Success && m_d->healthy();
}

QImage WgpuImageRenderer::readbackSurface()
{
    if (!m_d->ready()) return {};
    if (!m_d->surfaceConfigured || !m_d->bindings || !m_d->surfaceCopySupported) {
        m_d->fail(QStringLiteral("Surface readback requires a configured image and CopySrc support"));
        return {};
    }
    WGPUSurfaceTexture frame = {};
    wgpuSurfaceGetCurrentTexture(m_d->surface, &frame);
    GpuHandle<WGPUTexture, wgpuTextureRelease> texture(frame.texture);
    if (frame.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
        && frame.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        m_d->fail(QStringLiteral("GPU surface acquisition failed: %1").arg(int(frame.status)));
        return {};
    }
    QImage result = m_d->readPixels(texture, m_d->surfacePipeline,
                                   QSize(m_d->surfaceConfig.width, m_d->surfaceConfig.height));
    const auto status = wgpuSurfacePresent(m_d->surface);
    if (status != WGPUStatus_Success) return {};
    // Surface storage may be BGRA while the comparison contract is RGBA.
    if (m_d->surfaceConfig.format == WGPUTextureFormat_BGRA8Unorm) {
        result = result.rgbSwapped();
    }
    return result;
}

QString WgpuImageRenderer::error() const
{
    QMutexLocker lock(&m_d->errorMutex);
    return m_d->gpuError.isEmpty() ? m_d->lastError : m_d->gpuError;
}
WGPUBackendType WgpuImageRenderer::backend() const { return m_d->backend; }
quint64 WgpuImageRenderer::uploadedBytes() const { return m_d->uploadedBytes; }
}
