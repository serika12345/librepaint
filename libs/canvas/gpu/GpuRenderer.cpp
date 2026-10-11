/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GpuRenderer.h"
#include <QFile>
#include <QResource>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <stdexcept>
#include <vector>

static void initializeGpuCanvasResources() { Q_INIT_RESOURCE(gpu_canvas); }

namespace Krita::Canvas {
namespace {
template<typename T, void (*Release)(T)>
struct Resource {
    T value = nullptr;
    Resource() = default;
    explicit Resource(T handle) { reset(handle); }
    void reset(T handle) {
        if (!handle) throw std::runtime_error("Cannot create GPU canvas resource");
        if (value) Release(value);
        value = handle;
    }
    ~Resource() { if (value) Release(value); }
    Resource(const Resource &) = delete;
    Resource &operator=(const Resource &) = delete;
};
struct Parameters {
    float rowX[4], rowY[4], background[4];
};
static_assert(sizeof(Parameters) == 48);
}

struct GpuRenderer::CompletionData {
    std::atomic<Status> status{Status::Pending};
    std::atomic<unsigned> remaining{3};
    std::atomic<bool> failed{false};
    quint64 sequence = 0;
    void complete(bool success) {
        if (!success) failed.store(true);
        remaining.fetch_sub(1);
    }
};

struct GpuRenderer::Private {
    struct DisplayLookup {
        std::weak_ptr<const QByteArray> values;
        KisGpuDevice::MemoryReservation memory;
        Resource<WGPUBuffer, wgpuBufferRelease> buffer;
    };
    struct Pending {
        std::shared_ptr<CompletionData> completion;
        KisGpuTileStore::TextureSnapshot image;
        Resource<WGPUTexture, wgpuTextureRelease> target;
        Resource<WGPUTextureView, wgpuTextureViewRelease> inputView, targetView;
        KisGpuDevice::MemoryReservation memory;
        std::shared_ptr<DisplayLookup> displayLookup;
        KisGpuDevice::MemoryReservation lookupUploadMemory;
        Resource<WGPUBuffer, wgpuBufferRelease> parameters;
        Resource<WGPUBindGroup, wgpuBindGroupRelease> bindings;
        Pending(std::shared_ptr<CompletionData> state, const KisGpuTileStore::TextureSnapshot &input, WGPUTexture output)
            : completion(std::move(state)), image(input) {
            wgpuTextureAddRef(output);
            target.reset(output);
        }
    };
    struct ErrorScopes {
        WGPUDevice device;
        std::shared_ptr<CompletionData> completion;
        std::array<std::unique_ptr<std::shared_ptr<CompletionData>>, 2> callbacks;
        bool submitted = false;
        ErrorScopes(WGPUDevice handle, std::shared_ptr<CompletionData> state)
            : device(handle), completion(std::move(state)) {
            for (auto &data : callbacks) data = std::make_unique<std::shared_ptr<CompletionData>>(completion);
            wgpuDevicePushErrorScope(device, WGPUErrorFilter_Validation);
            wgpuDevicePushErrorScope(device, WGPUErrorFilter_OutOfMemory);
        }
        ~ErrorScopes() {
            for (auto &data : callbacks) {
                WGPUPopErrorScopeCallbackInfo callback{};
                callback.mode = WGPUCallbackMode_AllowSpontaneous;
                callback.userdata1 = data.release();
                callback.callback = [](WGPUPopErrorScopeStatus status, WGPUErrorType error, WGPUStringView, void *data, void *) {
                    std::unique_ptr<std::shared_ptr<CompletionData>> state(static_cast<std::shared_ptr<CompletionData> *>(data));
                    (*state)->complete(status == WGPUPopErrorScopeStatus_Success && error == WGPUErrorType_NoError);
                };
                wgpuDevicePopErrorScope(device, callback);
            }
            if (!submitted) completion->complete(false);
        }
    };
    KisGpuDevice &owner;
    WGPUTextureFormat format;
    quint32 maximumPending;
    Resource<WGPUQueue, wgpuQueueRelease> queue;
    Resource<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease> layout;
    Resource<WGPURenderPipeline, wgpuRenderPipelineRelease> pipeline;
    Resource<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease> displayLayout;
    Resource<WGPURenderPipeline, wgpuRenderPipelineRelease> displayPipeline;
    std::shared_ptr<DisplayLookup> displayLookup;
    std::vector<std::unique_ptr<Pending>> pending;
    WGPUSubmissionIndex lastSubmission = 0;
    Statistics statistics;
    Private(KisGpuDevice &device, WGPUTextureFormat targetFormat, quint32 maximum)
        : owner(device), format(targetFormat), maximumPending(maximum) {
        if (!owner.available() || !maximum
            || (format != WGPUTextureFormat_RGBA8Unorm && format != WGPUTextureFormat_BGRA8Unorm)) {
            throw std::runtime_error("Invalid GPU canvas configuration");
        }
        pending.reserve(maximum);
        queue.reset(wgpuDeviceGetQueue(owner.device()));
        initializeGpuCanvasResources();
        createPipeline(false);
    }
    void createPipeline(bool display) {
        auto &selectedLayout = display ? displayLayout : layout;
        auto &selectedPipeline = display ? displayPipeline : pipeline;
        WGPUBindGroupLayoutEntry entries[3]{};
        entries[0].binding = 0;
        entries[0].visibility = WGPUShaderStage_Fragment;
        entries[0].buffer.type = WGPUBufferBindingType_Uniform;
        entries[0].buffer.minBindingSize = sizeof(Parameters);
        entries[1].binding = 1;
        entries[1].visibility = WGPUShaderStage_Fragment;
        entries[1].texture.sampleType = WGPUTextureSampleType_Float;
        entries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
        WGPUBindGroupLayoutDescriptor bindings{};
        entries[2].binding = 2;
        entries[2].visibility = WGPUShaderStage_Fragment;
        entries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
        entries[2].buffer.minBindingSize = DisplayLookupBytes;
        bindings.entryCount = display ? 3 : 2;
        bindings.entries = entries;
        selectedLayout.reset(wgpuDeviceCreateBindGroupLayout(owner.device(), &bindings));
        WGPUPipelineLayoutDescriptor pipelineBindings{};
        pipelineBindings.bindGroupLayoutCount = 1;
        pipelineBindings.bindGroupLayouts = &selectedLayout.value;
        Resource<WGPUPipelineLayout, wgpuPipelineLayoutRelease> pipelineLayout(
            wgpuDeviceCreatePipelineLayout(owner.device(), &pipelineBindings));
        QFile file(QStringLiteral(":/librepaint/canvas/GpuRenderer.wgsl"));
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot load GPU canvas shader");
        const auto code = file.readAll();
        WGPUShaderSourceWGSL shaderSource{};
        shaderSource.chain.sType = WGPUSType_ShaderSourceWGSL;
        shaderSource.code = {code.constData(), size_t(code.size())};
        WGPUShaderModuleDescriptor shaderDescriptor{};
        shaderDescriptor.nextInChain = &shaderSource.chain;
        Resource<WGPUShaderModule, wgpuShaderModuleRelease> shader(
            wgpuDeviceCreateShaderModule(owner.device(), &shaderDescriptor));
        WGPUColorTargetState target{};
        target.format = format;
        target.writeMask = WGPUColorWriteMask_All;
        WGPUFragmentState fragment{};
        fragment.module = shader.value;
        fragment.entryPoint = {display ? "fragmentDisplay" : "fragment", WGPU_STRLEN};
        fragment.targetCount = 1;
        fragment.targets = &target;
        WGPURenderPipelineDescriptor descriptor{};
        descriptor.layout = pipelineLayout.value;
        descriptor.vertex.module = shader.value;
        descriptor.vertex.entryPoint = {"vertex", WGPU_STRLEN};
        descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        descriptor.primitive.frontFace = WGPUFrontFace_CCW;
        descriptor.primitive.cullMode = WGPUCullMode_None;
        descriptor.multisample.count = 1;
        descriptor.multisample.mask = ~quint32(0);
        descriptor.fragment = &fragment;
        selectedPipeline.reset(wgpuDeviceCreateRenderPipeline(owner.device(), &descriptor));
        if (!queue.value || !selectedLayout.value || !selectedPipeline.value || !owner.available()) {
            throw std::runtime_error("Cannot create GPU canvas pipeline");
        }
    }
    void collect() {
        for (auto &operation : pending) {
            auto &completion = *operation->completion;
            if (!owner.available()) {
                completion.failed.store(true);
                completion.status.store(Status::Failed);
            }
            if (completion.remaining.load() == 0) {
                completion.status.store(completion.failed.load() ? Status::Failed : Status::Succeeded);
            }
        }
        pending.erase(std::remove_if(pending.begin(), pending.end(), [](const auto &operation) {
            return operation->completion->remaining.load() == 0;
        }), pending.end());
    }
    void drain() {
        if (!pending.empty()) wgpuDevicePoll(owner.device(), true, lastSubmission ? &lastSubmission : nullptr);
        collect();
    }
    ~Private() { drain(); }
};

GpuRenderer::GpuRenderer(KisGpuDevice &device, WGPUTextureFormat format, quint32 maximum)
    : d(new Private(device, format, maximum)) {}
GpuRenderer::~GpuRenderer() = default;
void GpuRenderer::drain() { d->drain(); }
quint64 GpuRenderer::frameMemoryBytes() { return sizeof(Parameters); }
GpuRenderer::Status GpuRenderer::Frame::status() const { return d ? d->status.load() : Status::Failed; }
quint64 GpuRenderer::Frame::sequence() const { return d ? d->sequence : 0; }

GpuRenderer::Frame GpuRenderer::render(const KisGpuTileStore::TextureSnapshot &image, WGPUTexture target, const View &view)
{
    Frame result;
    if (!d->owner.available()) {
        result.error = Error::DeviceLost;
        return result;
    }
    if (image.error != KisGpuTileStore::Error::None || !image.texture() || !image.usesDevice(d->owner)
        || image.completion.status() == Status::Failed) {
        result.error = Error::InvalidImage;
        return result;
    }
    if (image.completion.status() == Status::Pending) {
        result.error = Error::ImagePending;
        return result;
    }
    if (!target || !wgpuLibrePaintTextureUsesDevice(target, d->owner.device())
        || target == image.texture() || wgpuTextureGetFormat(target) != d->format
        || wgpuTextureGetDimension(target) != WGPUTextureDimension_2D || wgpuTextureGetDepthOrArrayLayers(target) != 1
        || wgpuTextureGetSampleCount(target) != 1 || !(wgpuTextureGetUsage(target) & WGPUTextureUsage_RenderAttachment)) {
        result.error = Error::InvalidTarget;
        return result;
    }
    bool invertible = false;
    const auto inverse = view.canvasToTarget.inverted(&invertible);
    Parameters parameters{
        {float(inverse.m11()), float(inverse.m21()), float(inverse.dx() - image.bounds().x()),
         view.sampling == Sampling::Nearest ? 0.0f : 1.0f},
        {float(inverse.m12()), float(inverse.m22()), float(inverse.dy() - image.bounds().y()), 0},
        {0, 0, 0, 0}};
    const auto finite = [](const auto &values) {
        return std::all_of(std::begin(values), std::end(values), [](float value) { return std::isfinite(value); });
    };
    if (!view.canvasToTarget.isAffine() || !invertible || !finite(parameters.rowX) || !finite(parameters.rowY)) {
        result.error = Error::InvalidTransform;
        return result;
    }
    if (view.displayLookup && view.displayLookup->size() != DisplayLookupBytes) {
        result.error = Error::InvalidDisplayLookup;
        return result;
    }
    if (d->pending.size() >= d->maximumPending) {
        result.error = Error::QueueFull;
        return result;
    }
    auto memory = d->owner.reserveMemory(sizeof(Parameters));
    if (!memory) { result.error = Error::BudgetExceeded; return result; }
    const bool uploadLookup = view.displayLookup
        && (!d->displayLookup || d->displayLookup->values.lock() != view.displayLookup);
    KisGpuDevice::MemoryReservation lookupMemory, lookupUploadMemory;
    if (uploadLookup) {
        WGPULimits limits{};
        if (wgpuDeviceGetLimits(d->owner.device(), &limits) != WGPUStatus_Success
            || limits.maxStorageBufferBindingSize < quint64(DisplayLookupBytes)
            || limits.maxBufferSize < quint64(DisplayLookupBytes)) {
            result.error = Error::InvalidDisplayLookup;
            return result;
        }
        lookupMemory = d->owner.reserveMemory(DisplayLookupBytes);
        lookupUploadMemory = d->owner.reserveMemory(DisplayLookupBytes);
        if (!lookupMemory || !lookupUploadMemory) { result.error = Error::BudgetExceeded; return result; }
    }
    for (int c = 0; c < 4; ++c) parameters.background[c] = float((view.backgroundRgba >> (c * 8)) & 255) / 255;
    result.d = std::make_shared<CompletionData>();
    Private::ErrorScopes errors(d->owner.device(), result.d);
    auto lookup = view.displayLookup ? d->displayLookup : nullptr;
    if (uploadLookup) {
        if (!d->displayPipeline.value) d->createPipeline(true);
        lookup = std::make_shared<Private::DisplayLookup>();
        lookup->values = view.displayLookup;
        lookup->memory = std::move(lookupMemory);
        WGPUBufferDescriptor descriptor{};
        descriptor.size = DisplayLookupBytes;
        descriptor.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
        lookup->buffer.reset(wgpuDeviceCreateBuffer(d->owner.device(), &descriptor));
    }
    auto operation = std::make_unique<Private::Pending>(result.d, image, target);
    operation->memory = std::move(memory);
    operation->displayLookup = lookup;
    operation->lookupUploadMemory = std::move(lookupUploadMemory);
    operation->inputView.reset(wgpuTextureCreateView(image.texture(), nullptr));
    WGPUTextureViewDescriptor targetView{};
    targetView.format = d->format;
    targetView.dimension = WGPUTextureViewDimension_2D;
    targetView.mipLevelCount = 1;
    targetView.arrayLayerCount = 1;
    targetView.aspect = WGPUTextureAspect_All;
    operation->targetView.reset(wgpuTextureCreateView(target, &targetView));
    WGPUBufferDescriptor parameterDescriptor{};
    parameterDescriptor.size = sizeof(Parameters);
    parameterDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    operation->parameters.reset(wgpuDeviceCreateBuffer(d->owner.device(), &parameterDescriptor));
    WGPUBindGroupEntry bindings[3]{};
    bindings[0].binding = 0;
    bindings[0].buffer = operation->parameters.value;
    bindings[0].size = sizeof(Parameters);
    bindings[1].binding = 1;
    bindings[1].textureView = operation->inputView.value;
    if (lookup) {
        bindings[2].binding = 2;
        bindings[2].buffer = lookup->buffer.value;
        bindings[2].size = DisplayLookupBytes;
    }
    WGPUBindGroupDescriptor bindingDescriptor{};
    bindingDescriptor.layout = lookup ? d->displayLayout.value : d->layout.value;
    bindingDescriptor.entryCount = lookup ? 3 : 2;
    bindingDescriptor.entries = bindings;
    operation->bindings.reset(wgpuDeviceCreateBindGroup(d->owner.device(), &bindingDescriptor));
    Resource<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(
        wgpuDeviceCreateCommandEncoder(d->owner.device(), nullptr));
    WGPURenderPassColorAttachment attachment{};
    attachment.view = operation->targetView.value;
    attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    attachment.loadOp = WGPULoadOp_Clear;
    attachment.storeOp = WGPUStoreOp_Store;
    WGPURenderPassDescriptor passDescriptor{};
    passDescriptor.colorAttachmentCount = 1;
    passDescriptor.colorAttachments = &attachment;
    {
        Resource<WGPURenderPassEncoder, wgpuRenderPassEncoderRelease> pass(
            wgpuCommandEncoderBeginRenderPass(encoder.value, &passDescriptor));
        wgpuRenderPassEncoderSetPipeline(pass.value, lookup ? d->displayPipeline.value : d->pipeline.value);
        wgpuRenderPassEncoderSetBindGroup(pass.value, 0, operation->bindings.value, 0, nullptr);
        wgpuRenderPassEncoderDraw(pass.value, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(pass.value);
    }
    Resource<WGPUCommandBuffer, wgpuCommandBufferRelease> commands(wgpuCommandEncoderFinish(encoder.value, nullptr));
    auto callbackData = std::make_unique<std::shared_ptr<CompletionData>>(result.d);
    result.d->sequence = d->statistics.submissions + 1;
    d->pending.push_back(std::move(operation));
    if (uploadLookup) {
        wgpuQueueWriteBuffer(d->queue.value, lookup->buffer.value, 0, view.displayLookup->constData(), DisplayLookupBytes);
        d->statistics.displayLookupUploadBytes += DisplayLookupBytes;
    }
    d->displayLookup = std::move(lookup);
    wgpuQueueWriteBuffer(d->queue.value, d->pending.back()->parameters.value, 0, &parameters, sizeof(parameters));
    d->lastSubmission = wgpuQueueSubmitForIndex(d->queue.value, 1, &commands.value);
    WGPUQueueWorkDoneCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = callbackData.release();
    callback.callback = [](WGPUQueueWorkDoneStatus status, void *data, void *) {
        std::unique_ptr<std::shared_ptr<CompletionData>> completion(static_cast<std::shared_ptr<CompletionData> *>(data));
        (*completion)->complete(status == WGPUQueueWorkDoneStatus_Success);
    };
    wgpuQueueOnSubmittedWorkDone(d->queue.value, callback);
    ++d->statistics.submissions;
    d->statistics.parameterUploadBytes += sizeof(parameters);
    errors.submitted = true;
    return result;
}

void GpuRenderer::poll()
{
    wgpuDevicePoll(d->owner.device(), false, nullptr);
    d->collect();
}
GpuRenderer::Statistics GpuRenderer::statistics() const
{
    auto result = d->statistics;
    result.pendingFrames = quint32(d->pending.size());
    result.residentParameterBytes = d->pending.size() * sizeof(Parameters);
    std::vector<const Private::DisplayLookup *> lookups;
    if (d->displayLookup) lookups.push_back(d->displayLookup.get());
    for (const auto &operation : d->pending) {
        const auto *lookup = operation->displayLookup.get();
        if (lookup && std::find(lookups.begin(), lookups.end(), lookup) == lookups.end()) lookups.push_back(lookup);
    }
    result.residentDisplayLookupBytes = lookups.size() * DisplayLookupBytes;
    return result;
}
}
