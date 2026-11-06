// SPDX-License-Identifier: GPL-2.0-or-later
#include "gpu.h"
#include <webgpu/wgpu.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace poc {
static void require(bool value, const char *message)
{
    if (!value) throw std::runtime_error(message);
}
static std::string string(WGPUStringView value)
{
    return value.data ? std::string(value.data, value.length == WGPU_STRLEN ? strlen(value.data) : value.length) : "";
}

struct Gpu::Impl {
    Gpu &owner;
    const Scene &scene;
    bool timestamps;
    bool cpuOnly;
    Mode lastMode = Mode::Gpu;
    WGPUInstance instance{};
    WGPUAdapter adapter{};
    WGPUDevice device{};
    WGPUQueue queue{};
    WGPUBuffer document{}, projection{}, command{}, mapping{}, backup{}, grid{}, resolve{}, times{};
    WGPUTexture target{};
    WGPUTextureView view{};
    WGPUBindGroup computeGroup{}, displayGroup{};
    WGPUComputePipeline paint{}, compose{};
    WGPURenderPipeline display{};
    WGPUQuerySet queries{};
    std::vector<WGPUBuffer> buffers;
    uint64_t documentBytes, projectionBytes, activeBytes;
    static constexpr uint64_t CommandBytes = 4 * 1024 * 1024;

    Impl(Gpu &o, const Scene &s, bool timing, Mode mode)
        : owner(o), scene(s), timestamps(timing), cpuOnly(mode == Mode::Cpu),
          documentBytes(cpuOnly ? 4 : s.initial.size() * 4),
          projectionBytes(uint64_t(s.options.size) * s.options.size * 4),
          activeBytes(cpuOnly ? 4 : uint64_t(s.activeTiles) * TilePixels * 4)
    {
        WGPUInstanceExtras extra{};
        extra.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
#ifdef __APPLE__
        extra.backends = WGPUInstanceBackend_Metal;
#else
        extra.backends = WGPUInstanceBackend_Vulkan;
#endif
        extra.flags = WGPUInstanceFlag_Validation;
        WGPUInstanceDescriptor id{}; id.nextInChain = &extra.chain;
        instance = wgpuCreateInstance(&id);
        require(instance, "create instance");
        WGPURequestAdapterOptions ao{};
        ao.powerPreference = WGPUPowerPreference_HighPerformance;
        WGPURequestAdapterCallbackInfo ac{};
        ac.mode = WGPUCallbackMode_AllowSpontaneous; ac.userdata1 = this;
        ac.callback = [](WGPURequestAdapterStatus status, WGPUAdapter a, WGPUStringView message, void *p, void *) {
            if (status != WGPURequestAdapterStatus_Success) std::cerr << string(message) << '\n';
            static_cast<Impl *>(p)->adapter = a;
        };
        wgpuInstanceRequestAdapter(instance, &ao, ac);
        require(adapter, "hardware adapter unavailable");
        WGPUAdapterInfo info{}; wgpuAdapterGetInfo(adapter, &info);
        owner.adapter = string(info.device);
        require(info.adapterType != WGPUAdapterType_CPU, "software adapter is not a valid GPU comparison");
        wgpuAdapterInfoFreeMembers(info);
        require(!timing || wgpuAdapterHasFeature(adapter, WGPUFeatureName_TimestampQuery), "GPU timestamps unavailable");
        WGPUFeatureName feature = WGPUFeatureName_TimestampQuery;
        WGPUDeviceDescriptor dd{};
        dd.requiredFeatureCount = timing ? 1 : 0; dd.requiredFeatures = timing ? &feature : nullptr;
        dd.uncapturedErrorCallbackInfo.callback = [](const WGPUDevice *, WGPUErrorType, WGPUStringView message, void *, void *) {
            std::cerr << "GPU validation error: " << string(message) << '\n'; std::abort();
        };
        WGPURequestDeviceCallbackInfo dc{};
        dc.mode = WGPUCallbackMode_AllowSpontaneous; dc.userdata1 = this;
        dc.callback = [](WGPURequestDeviceStatus status, WGPUDevice dev, WGPUStringView message, void *p, void *) {
            if (status != WGPURequestDeviceStatus_Success) std::cerr << string(message) << '\n';
            static_cast<Impl *>(p)->device = dev;
        };
        wgpuAdapterRequestDevice(adapter, &dd, dc);
        require(device, "create device");
        queue = wgpuDeviceGetQueue(device);
        WGPULimits limits{}; wgpuDeviceGetLimits(device, &limits);
        require(documentBytes <= limits.maxStorageBufferBindingSize, "sparse document exceeds storage binding budget");
        require(projectionBytes <= limits.maxStorageBufferBindingSize, "projection exceeds storage binding budget");
        const auto storage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
        document = buffer(documentBytes, storage);
        projection = buffer(projectionBytes, storage);
        command = buffer(CommandBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        mapping = buffer(s.mapping.size() * 4, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
        backup = buffer(activeBytes, WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
        grid = buffer(16, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
        wgpuQueueWriteBuffer(queue, document, 0, s.initial.data(), documentBytes);
        wgpuQueueWriteBuffer(queue, mapping, 0, s.mapping.data(), s.mapping.size() * 4);
        uint32_t gridData[4]{s.grid, 0, 0, 0};
        wgpuQueueWriteBuffer(queue, grid, 0, gridData, sizeof(gridData));
        WGPUBindGroupLayoutEntry entries[4]{};
        for (uint32_t i = 0; i < 4; ++i) {
            entries[i].binding = i; entries[i].visibility = WGPUShaderStage_Compute;
            entries[i].buffer.type = i < 2 ? WGPUBufferBindingType_Storage : WGPUBufferBindingType_ReadOnlyStorage;
        }
        WGPUBindGroupLayoutDescriptor ld{}; ld.entryCount = 4; ld.entries = entries;
        const auto layout = wgpuDeviceCreateBindGroupLayout(device, &ld);
        WGPUPipelineLayoutDescriptor pd{}; pd.bindGroupLayoutCount = 1; pd.bindGroupLayouts = &layout;
        const auto pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pd);
        const auto module = shader("engine.wgsl");
        WGPUComputePipelineDescriptor cp{}; cp.layout = pipelineLayout; cp.compute.module = module;
        cp.compute.entryPoint = {"paint", WGPU_STRLEN}; paint = wgpuDeviceCreateComputePipeline(device, &cp);
        cp.compute.entryPoint = {"compose", WGPU_STRLEN}; compose = wgpuDeviceCreateComputePipeline(device, &cp);
        WGPUBindGroupEntry bindings[4]{};
        WGPUBuffer bound[]{document, projection, command, mapping};
        uint64_t sizes[]{documentBytes, projectionBytes, CommandBytes, s.mapping.size() * 4};
        for (uint32_t i = 0; i < 4; ++i) { bindings[i].binding = i; bindings[i].buffer = bound[i]; bindings[i].size = sizes[i]; }
        WGPUBindGroupDescriptor bg{}; bg.layout = layout; bg.entryCount = 4; bg.entries = bindings;
        computeGroup = wgpuDeviceCreateBindGroup(device, &bg);
        wgpuShaderModuleRelease(module); wgpuPipelineLayoutRelease(pipelineLayout); wgpuBindGroupLayoutRelease(layout);
        createDisplay();
        if (timing) {
            WGPUQuerySetDescriptor q{}; q.type = WGPUQueryType_Timestamp; q.count = 6;
            queries = wgpuDeviceCreateQuerySet(device, &q);
            resolve = buffer(48, WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc);
            times = buffer(48, WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);
        }
    }

    ~Impl()
    {
        owner.wait();
        if (queries) wgpuQuerySetRelease(queries);
        wgpuRenderPipelineRelease(display);
        wgpuComputePipelineRelease(paint); wgpuComputePipelineRelease(compose);
        wgpuBindGroupRelease(computeGroup); wgpuBindGroupRelease(displayGroup);
        wgpuTextureViewRelease(view); wgpuTextureRelease(target);
        for (auto b : buffers) wgpuBufferRelease(b);
        wgpuQueueRelease(queue); wgpuDeviceRelease(device);
        wgpuAdapterRelease(adapter); wgpuInstanceRelease(instance);
    }
    WGPUBuffer buffer(uint64_t size, WGPUBufferUsage usage)
    {
        WGPUBufferDescriptor desc{}; desc.size = size; desc.usage = usage;
        auto result = wgpuDeviceCreateBuffer(device, &desc);
        require(result, "allocate buffer"); buffers.push_back(result); owner.allocatedBytes += size;
        return result;
    }
    WGPUShaderModule shader(const char *name)
    {
        std::ifstream file(std::string(POC_SHADER_DIR) + '/' + name);
        require(bool(file), "read WGSL");
        const std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        WGPUShaderSourceWGSL source{}; source.chain.sType = WGPUSType_ShaderSourceWGSL;
        source.code = {code.data(), code.size()};
        WGPUShaderModuleDescriptor desc{}; desc.nextInChain = &source.chain;
        return wgpuDeviceCreateShaderModule(device, &desc);
    }
    void createDisplay()
    {
        WGPUBindGroupLayoutEntry entry[2]{};
        entry[0].binding = 0; entry[0].visibility = WGPUShaderStage_Fragment;
        entry[0].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
        entry[1].binding = 1; entry[1].visibility = WGPUShaderStage_Fragment;
        entry[1].buffer.type = WGPUBufferBindingType_Uniform;
        WGPUBindGroupLayoutDescriptor ld{}; ld.entryCount = 2; ld.entries = entry;
        auto layout = wgpuDeviceCreateBindGroupLayout(device, &ld);
        WGPUPipelineLayoutDescriptor pd{}; pd.bindGroupLayoutCount = 1; pd.bindGroupLayouts = &layout;
        auto pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pd);
        auto module = shader("display.wgsl");
        WGPUColorTargetState color{}; color.format = WGPUTextureFormat_RGBA8Unorm; color.writeMask = WGPUColorWriteMask_All;
        WGPUFragmentState fragment{}; fragment.module = module; fragment.entryPoint = {"fragmentMain", WGPU_STRLEN};
        fragment.targetCount = 1; fragment.targets = &color;
        WGPURenderPipelineDescriptor rp{}; rp.layout = pipelineLayout;
        rp.vertex.module = module; rp.vertex.entryPoint = {"vertexMain", WGPU_STRLEN};
        rp.fragment = &fragment; rp.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        rp.multisample.count = 1; rp.multisample.mask = ~0u;
        display = wgpuDeviceCreateRenderPipeline(device, &rp);
        WGPUBindGroupEntry entries[2]{};
        entries[0].binding = 0; entries[0].buffer = projection; entries[0].size = projectionBytes;
        entries[1].binding = 1; entries[1].buffer = grid; entries[1].size = 16;
        WGPUBindGroupDescriptor bg{}; bg.layout = layout; bg.entryCount = 2; bg.entries = entries;
        displayGroup = wgpuDeviceCreateBindGroup(device, &bg);
        WGPUTextureDescriptor td{}; td.dimension = WGPUTextureDimension_2D;
        td.size = {scene.options.size, scene.options.size, 1};
        td.format = WGPUTextureFormat_RGBA8Unorm; td.mipLevelCount = td.sampleCount = 1;
        td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        target = wgpuDeviceCreateTexture(device, &td); view = wgpuTextureCreateView(target, nullptr);
        owner.allocatedBytes += projectionBytes;
        wgpuShaderModuleRelease(module); wgpuPipelineLayoutRelease(pipelineLayout); wgpuBindGroupLayoutRelease(layout);
    }
    void submit(WGPUCommandEncoder encoder)
    {
        const auto commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(queue, 1, &commandBuffer);
        wgpuCommandBufferRelease(commandBuffer); wgpuCommandEncoderRelease(encoder);
        ++owner.counters.submissions;
    }
    void compute(WGPUCommandEncoder encoder, WGPUComputePipeline pipeline, uint32_t count, uint32_t stamp)
    {
        if (!count) return;
        WGPUComputePassDescriptor pd{};
        WGPUComputePassTimestampWrites ts{}; ts.querySet = queries;
        ts.beginningOfPassWriteIndex = stamp; ts.endOfPassWriteIndex = stamp + 1;
        if (timestamps) pd.timestampWrites = &ts;
        auto pass = wgpuCommandEncoderBeginComputePass(encoder, &pd);
        wgpuComputePassEncoderSetPipeline(pass, pipeline);
        wgpuComputePassEncoderSetBindGroup(pass, 0, computeGroup, 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, TilePixels / 64, count, 1);
        wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    }
    void map(WGPUBuffer b, uint64_t bytes)
    {
        std::atomic<int> ready{0};
        WGPUBufferMapCallbackInfo cb{}; cb.mode = WGPUCallbackMode_AllowSpontaneous; cb.userdata1 = &ready;
        cb.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *p, void *) {
            static_cast<std::atomic<int> *>(p)->store(status == WGPUMapAsyncStatus_Success ? 1 : -1);
        };
        wgpuBufferMapAsync(b, WGPUMapMode_Read, 0, bytes, cb);
        const double deadline = wallMs() + 30000;
        while (!ready.load()) {
            wgpuDevicePoll(device, false, nullptr);
            if (wallMs() > deadline) { std::cerr << "map timeout\n"; std::abort(); }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        require(ready == 1, "map readback");
    }
    std::vector<uint32_t> read(WGPUBuffer source, uint64_t bytes)
    {
        WGPUBufferDescriptor bd{}; bd.size = bytes; bd.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
        auto readback = wgpuDeviceCreateBuffer(device, &bd);
        auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
        wgpuCommandEncoderCopyBufferToBuffer(encoder, source, 0, readback, 0, bytes); submit(encoder);
        map(readback, bytes);
        std::vector<uint32_t> result(bytes / 4);
        std::memcpy(result.data(), wgpuBufferGetConstMappedRange(readback, 0, bytes), bytes);
        wgpuBufferUnmap(readback); wgpuBufferRelease(readback);
        owner.counters.readback += bytes;
        return result;
    }
};

Gpu::Gpu(const Scene &scene, bool timestamps, Mode mode) : d(std::make_unique<Impl>(*this, scene, timestamps, mode))
{
    checkpoint(); wait();
}
Gpu::~Gpu() = default;
void Gpu::wait()
{
    // Nonblocking poll avoids the native Metal blocking-poll sleep. Both paths
    // use the same 50 us completion observation; its delay is included in wall time.
    if (!d) return;
    const double deadline = wallMs() + 30000;
    while (!wgpuDevicePoll(d->device, false, nullptr)) {
        if (wallMs() > deadline) { std::cerr << "GPU completion timeout\n"; std::abort(); }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}
void Gpu::checkpoint()
{
    auto e = wgpuDeviceCreateCommandEncoder(d->device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(e, d->document, 0, d->backup, 0, d->activeBytes);
    counters.copyBytes += d->activeBytes; d->submit(e);
}
void Gpu::restore()
{
    auto e = wgpuDeviceCreateCommandEncoder(d->device, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(e, d->backup, 0, d->document, 0, d->activeBytes);
    counters.copyBytes += d->activeBytes; d->submit(e);
}
void Gpu::reset()
{
    auto e = wgpuDeviceCreateCommandEncoder(d->device, nullptr);
    wgpuCommandEncoderClearBuffer(e, d->document, 0, d->activeBytes); d->submit(e);
}
void Gpu::frame(const Batch &batch, Mode mode, const CpuDocument *cpu, bool restoreFirst)
{
    d->lastMode = mode;
    auto e = wgpuDeviceCreateCommandEncoder(d->device, nullptr);
    if (restoreFirst && mode == Mode::Gpu) {
        wgpuCommandEncoderCopyBufferToBuffer(e, d->backup, 0, d->document, 0, d->activeBytes);
        counters.copyBytes += d->activeBytes;
    }
    if (mode != Mode::Gpu) {
        require(cpu, "CPU document required");
        // Merge adjacent dirty tiles into writes, preserving partial updates.
        std::vector<uint32_t> slots;
        for (auto &j : batch.jobs) if (mode == Mode::Cpu || j.physical != Missing)
            slots.push_back(mode == Mode::Cpu ? j.logical : j.physical);
        std::sort(slots.begin(), slots.end());
        for (size_t i = 0; i < slots.size();) {
            size_t end = i + 1;
            while (end < slots.size() && slots[end] == slots[end - 1] + 1) ++end;
            const uint64_t offset = uint64_t(slots[i]) * TilePixels;
            const uint64_t bytes = (end - i) * TilePixels * 4;
            const auto &pixels = mode == Mode::Cpu ? cpu->projection : cpu->pixels;
            wgpuQueueWriteBuffer(d->queue, mode == Mode::Cpu ? d->projection : d->document,
                                 offset * 4, pixels.data() + offset, bytes);
            counters.pixelUpload += bytes; ++counters.writeCalls; i = end;
        }
    }
    if (mode != Mode::Cpu) {
        const auto data = batch.commands(d->scene.grid, d->scene.options.layers);
        require(data.size() * 4 <= Impl::CommandBytes, "command buffer capacity");
        wgpuQueueWriteBuffer(d->queue, d->command, 0, data.data(), data.size() * 4);
        counters.commandUpload += data.size() * 4; ++counters.writeCalls;
        if (mode == Mode::Gpu) { d->compute(e, d->paint, batch.jobs.size(), 0); counters.paintTiles += batch.jobs.size(); }
        d->compute(e, d->compose, batch.jobs.size(), 2); counters.composeTiles += batch.jobs.size();
    }
    WGPURenderPassColorAttachment attachment{}; attachment.view = d->view;
    attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    attachment.loadOp = WGPULoadOp_Clear; attachment.storeOp = WGPUStoreOp_Store;
    WGPURenderPassDescriptor rp{}; rp.colorAttachmentCount = 1; rp.colorAttachments = &attachment;
    WGPURenderPassTimestampWrites ts{}; ts.querySet = d->queries;
    ts.beginningOfPassWriteIndex = 4; ts.endOfPassWriteIndex = 5;
    if (d->timestamps) rp.timestampWrites = &ts;
    auto pass = wgpuCommandEncoderBeginRenderPass(e, &rp);
    wgpuRenderPassEncoderSetPipeline(pass, d->display);
    wgpuRenderPassEncoderSetBindGroup(pass, 0, d->displayGroup, 0, nullptr);
    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(pass); wgpuRenderPassEncoderRelease(pass);
    if (d->timestamps) {
        // Resolve only queries written by this mode; spare slots remain unused.
        const uint32_t first = mode == Mode::Cpu ? 4 : mode == Mode::Hybrid ? 2 : 0;
        wgpuCommandEncoderResolveQuerySet(e, d->queries, first, 6 - first, d->resolve, 0);
        wgpuCommandEncoderCopyBufferToBuffer(e, d->resolve, 0, d->times, 0, (6 - first) * 8);
    }
    d->submit(e);
}
std::vector<uint32_t> Gpu::readDocument() { return d->read(d->document, d->documentBytes); }
std::vector<uint32_t> Gpu::readProjection() { return d->read(d->projection, d->projectionBytes); }
std::vector<uint32_t> Gpu::readDisplay()
{
    WGPUBufferDescriptor bd{}; bd.size = d->projectionBytes; bd.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    auto b = wgpuDeviceCreateBuffer(d->device, &bd);
    auto e = wgpuDeviceCreateCommandEncoder(d->device, nullptr);
    WGPUTexelCopyTextureInfo src{}; src.texture = d->target;
    WGPUTexelCopyBufferInfo dst{}; dst.buffer = b; dst.layout.bytesPerRow = d->scene.options.size * 4;
    dst.layout.rowsPerImage = d->scene.options.size;
    WGPUExtent3D extent{d->scene.options.size, d->scene.options.size, 1};
    wgpuCommandEncoderCopyTextureToBuffer(e, &src, &dst, &extent); d->submit(e);
    d->map(b, d->projectionBytes);
    std::vector<uint32_t> result(d->projectionBytes / 4);
    std::memcpy(result.data(), wgpuBufferGetConstMappedRange(b, 0, d->projectionBytes), d->projectionBytes);
    wgpuBufferUnmap(b); wgpuBufferRelease(b); counters.readback += d->projectionBytes;
    return result;
}
std::array<double, 3> Gpu::gpuTimes()
{
    require(d->timestamps, "timestamps disabled");
    const uint32_t first = d->lastMode == Mode::Cpu ? 2 : d->lastMode == Mode::Hybrid ? 1 : 0;
    const uint64_t bytes = (3 - first) * 16;
    d->map(d->times, bytes);
    const auto *values = static_cast<const uint64_t *>(wgpuBufferGetConstMappedRange(d->times, 0, bytes));
    std::array<double, 3> result{};
    for (uint32_t i = first; i < 3; ++i) {
        const auto a = values[(i - first) * 2], b = values[(i - first) * 2 + 1];
        require(b >= a && a != 0, "invalid GPU timestamp");
        result[i] = double(b - a) * wgpuQueueGetTimestampPeriod(d->queue) / 1e6;
    }
    wgpuBufferUnmap(d->times); return result;
}
}
