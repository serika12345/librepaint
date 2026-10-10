/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
template<typename T, void (*Release)(T)>
struct Handle {
    T value;
    explicit Handle(T handle) : value(handle) {
        if (!value) throw std::runtime_error("Cannot create GPU tile resource");
    }
    ~Handle() { if (value) Release(value); }
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
};

struct DeviceState {
    WGPUDevice device;
    WGPUQueue queue;
    std::atomic<quint64> residentBytes{0};
    explicit DeviceState(WGPUDevice value) : device(value), queue(wgpuDeviceGetQueue(value)) {
        wgpuDeviceAddRef(device);
    }
    ~DeviceState() { wgpuQueueRelease(queue); wgpuDeviceRelease(device); }
};

struct Allocation {
    std::shared_ptr<DeviceState> owner;
    WGPUBuffer buffer;
    quint64 bytes;
    Allocation(std::shared_ptr<DeviceState> state, quint64 size, WGPUBufferUsage usage)
        : owner(std::move(state)), bytes(size) {
        WGPUBufferDescriptor descriptor{};
        descriptor.size = bytes;
        descriptor.usage = usage;
        buffer = wgpuDeviceCreateBuffer(owner->device, &descriptor);
        if (!buffer) throw std::runtime_error("Cannot allocate GPU tile buffer");
        owner->residentBytes.fetch_add(bytes);
    }
    ~Allocation() {
        wgpuBufferRelease(buffer);
        owner->residentBytes.fetch_sub(bytes);
    }
};

using Coordinate = std::pair<int, int>;
int tileCoordinate(qint64 pixel) { return int(pixel >= 0 ? pixel / 64 : (pixel + 1) / 64 - 1); }
struct FillParameters {
    quint32 left, top, right, bottom, color, operation, opacity, coverage;
};
static_assert(sizeof(FillParameters) == 32);

constexpr char FillShader[] = R"(
struct FillParameters {
    lower: vec2<u32>, upper: vec2<u32>, color: u32,
    operation: u32, opacity: u32, coverage: u32,
}
@group(0) @binding(0) var<storage, read_write> pixels: array<u32>;
@group(0) @binding(1) var<uniform> parameters: FillParameters;

fn multiply8(a: u32, b: u32) -> u32 {
    let product = a * b + 128u;
    return (product + (product >> 8u)) >> 8u;
}

fn unpack(pixel: u32) -> vec4<u32> {
    return vec4<u32>(pixel, pixel >> 8u, pixel >> 16u, pixel >> 24u) & vec4<u32>(255u);
}

fn quantize(value: f32) -> u32 {
    let bounded = clamp(value, 0.0, 255.0);
    let lower = u32(floor(bounded));
    let fraction = bounded - f32(lower);
    return lower + select(0u, 1u, fraction > 0.5 || (fraction == 0.5 && (lower & 1u) != 0u));
}

fn over(source: vec4<u32>, destination: vec4<u32>) -> vec4<u32> {
    var alpha = f32(source.a) * (f32(parameters.opacity) * (1.0 / 255.0));
    alpha *= f32(parameters.coverage) * (1.0 / 255.0);
    if (alpha == 0.0) { return destination; }
    let resultAlpha = f32(destination.a) + (255.0 - f32(destination.a)) * alpha * (1.0 / 255.0);
    let blend = alpha / resultAlpha;
    let color = blend * (vec3<f32>(source.rgb) - vec3<f32>(destination.rgb)) + vec3<f32>(destination.rgb);
    return vec4<u32>(quantize(color.r), quantize(color.g), quantize(color.b), quantize(resultAlpha));
}

@compute @workgroup_size(8, 8)
fn fill(@builtin(global_invocation_id) position: vec3<u32>) {
    if (all(position.xy >= parameters.lower) && all(position.xy < parameters.upper)) {
        let index = position.y * 64u + position.x;
        if (parameters.operation == 0u) {
            pixels[index] = parameters.color;
        } else {
            let source = unpack(parameters.color);
            var destination = unpack(pixels[index]);
            if (parameters.operation == 2u) {
                let alpha = multiply8(multiply8(source.a, parameters.coverage), parameters.opacity);
                destination.a = multiply8(destination.a, 255u - alpha);
            } else {
                destination = over(source, destination);
            }
            pixels[index] = destination.r | (destination.g << 8u) | (destination.b << 16u) | (destination.a << 24u);
        }
    }
}
)";
}

struct KisGpuTileStore::VersionData {
    std::shared_ptr<DeviceState> owner;
    std::shared_ptr<CompletionData> completion;
    std::map<Coordinate, std::shared_ptr<Allocation>> tiles;
};

struct KisGpuTileStore::CompletionData {
    std::atomic<Status> status{Status::Pending};
    std::atomic<unsigned> remaining{3};
    std::atomic<bool> failed{false};
    quint64 sequence = 0;
    void complete(bool success) {
        if (!success) failed.store(true);
        if (remaining.fetch_sub(1) == 1) status.store(failed.load() ? Status::Failed : Status::Succeeded);
    }
};

struct KisGpuTileStore::Private {
    struct ErrorScopes {
        WGPUDevice device;
        std::shared_ptr<CompletionData> completion;
        std::array<std::unique_ptr<std::shared_ptr<CompletionData>>, 2> callbacks;
        bool submitted = false;
        ErrorScopes(WGPUDevice value, std::shared_ptr<CompletionData> operation)
            : device(value), completion(std::move(operation)) {
            for (auto &callback : callbacks) callback = std::make_unique<std::shared_ptr<CompletionData>>(completion);
            for (auto filter : {WGPUErrorFilter_Validation, WGPUErrorFilter_OutOfMemory}) {
                wgpuDevicePushErrorScope(device, filter);
            }
        }
        ~ErrorScopes() {
            for (auto &data : callbacks) {
                WGPUPopErrorScopeCallbackInfo callback{};
                callback.mode = WGPUCallbackMode_AllowSpontaneous;
                callback.userdata1 = data.release();
                callback.callback = [](WGPUPopErrorScopeStatus status, WGPUErrorType error, WGPUStringView, void *data, void *) {
                    std::unique_ptr<std::shared_ptr<CompletionData>> completion(
                        static_cast<std::shared_ptr<CompletionData> *>(data));
                    (*completion)->complete(status == WGPUPopErrorScopeStatus_Success && error == WGPUErrorType_NoError);
                };
                wgpuDevicePopErrorScope(device, callback);
            }
            if (!submitted) completion->complete(false);
        }
    };
    struct Pending {
        std::shared_ptr<CompletionData> completion;
        Version source, result;
        std::shared_ptr<Allocation> parameters;
    };
    std::shared_ptr<DeviceState> state;
    quint64 budget;
    WGPULimits limits{};
    WGPUBindGroupLayout layout = nullptr;
    WGPUComputePipeline pipeline = nullptr;
    WGPUSubmissionIndex lastSubmission = 0;
    Statistics statistics;
    std::vector<Pending> pending;

    Private(WGPUDevice device, quint64 bytes) : budget(bytes) {
        if (!device || wgpuGetVersion() != 0x1b000400) {
            throw std::runtime_error("GPU tiles require a device from wgpu-native 27.0.4.0");
        }
        state = std::make_shared<DeviceState>(device);
        if (wgpuDeviceGetLimits(device, &limits) != WGPUStatus_Success
            || limits.maxStorageBufferBindingSize < TileBytes) {
            throw std::runtime_error("GPU device cannot bind a document tile");
        }
        WGPUBindGroupLayoutEntry entries[2]{};
        entries[0].binding = 0;
        entries[0].visibility = WGPUShaderStage_Compute;
        entries[0].buffer.type = WGPUBufferBindingType_Storage;
        entries[0].buffer.minBindingSize = TileBytes;
        entries[1].binding = 1;
        entries[1].visibility = WGPUShaderStage_Compute;
        entries[1].buffer.type = WGPUBufferBindingType_Uniform;
        entries[1].buffer.hasDynamicOffset = true;
        entries[1].buffer.minBindingSize = sizeof(FillParameters);
        WGPUBindGroupLayoutDescriptor layoutDescriptor{};
        layoutDescriptor.entryCount = 2;
        layoutDescriptor.entries = entries;
        Handle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease> groupLayout(
            wgpuDeviceCreateBindGroupLayout(device, &layoutDescriptor));
        WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
        pipelineLayoutDescriptor.bindGroupLayouts = &groupLayout.value;
        Handle<WGPUPipelineLayout, wgpuPipelineLayoutRelease> pipelineLayout(
            wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor));
        WGPUShaderSourceWGSL source{};
        source.chain.sType = WGPUSType_ShaderSourceWGSL;
        source.code = {FillShader, sizeof(FillShader) - 1};
        WGPUShaderModuleDescriptor shaderDescriptor{};
        shaderDescriptor.nextInChain = &source.chain;
        Handle<WGPUShaderModule, wgpuShaderModuleRelease> shader(
            wgpuDeviceCreateShaderModule(device, &shaderDescriptor));
        WGPUComputePipelineDescriptor descriptor{};
        descriptor.layout = pipelineLayout.value;
        descriptor.compute.module = shader.value;
        descriptor.compute.entryPoint = {"fill", WGPU_STRLEN};
        Handle<WGPUComputePipeline, wgpuComputePipelineRelease> fillPipeline(
            wgpuDeviceCreateComputePipeline(device, &descriptor));
        layout = groupLayout.value;
        groupLayout.value = nullptr;
        pipeline = fillPipeline.value;
        fillPipeline.value = nullptr;
    }

    ~Private() {
        if (lastSubmission) wgpuDevicePoll(state->device, true, &lastSubmission);
        pending.clear();
        wgpuComputePipelineRelease(pipeline);
        wgpuBindGroupLayoutRelease(layout);
    }
};

KisGpuTileStore::KisGpuTileStore(WGPUDevice device, quint64 budgetBytes)
    : d(new Private(device, budgetBytes)) {}
KisGpuTileStore::~KisGpuTileStore() = default;

qsizetype KisGpuTileStore::Version::tileCount() const { return d ? qsizetype(d->tiles.size()) : 0; }

WGPUBuffer KisGpuTileStore::Version::tile(QPoint coordinate) const
{
    if (!d) return nullptr;
    const auto found = d->tiles.find({coordinate.x(), coordinate.y()});
    return found == d->tiles.end() ? nullptr : found->second->buffer;
}

KisGpuTileStore::Status KisGpuTileStore::Completion::status() const
{
    return d ? d->status.load() : Status::Failed;
}

quint64 KisGpuTileStore::Completion::sequence() const { return d ? d->sequence : 0; }

KisGpuTileStore::Version KisGpuTileStore::emptyVersion() const
{
    Version result;
    auto data = std::make_shared<VersionData>();
    data->owner = d->state;
    data->completion = std::make_shared<CompletionData>();
    data->completion->status.store(Status::Succeeded);
    result.d = std::move(data);
    return result;
}

KisGpuTileStore::Edit KisGpuTileStore::fill(const Version &base, QRect rectangle, quint32 rgba)
{
    return update(base, rectangle, rgba, UpdateKind::Fill, 255, 255);
}

KisGpuTileStore::Edit KisGpuTileStore::paint(const Version &base, QRect rectangle, quint32 rgba,
                                          CompositeOp operation, quint8 opacity, quint8 coverage)
{
    return update(base, rectangle, rgba, operation == CompositeOp::Erase ? UpdateKind::Erase : UpdateKind::Over,
                  opacity, coverage);
}

KisGpuTileStore::Edit KisGpuTileStore::update(const Version &base, QRect rectangle, quint32 rgba,
                                           UpdateKind kind, quint8 opacity, quint8 coverage)
{
    Edit result;
    if (!base.d || base.d->owner != d->state) {
        result.error = Error::InvalidVersion;
        return result;
    }
    if (rectangle.isEmpty() || (kind != UpdateKind::Fill && (opacity == 0 || coverage == 0 || (rgba >> 24) == 0))) {
        result.version = base;
        result.completion.d = base.d->completion;
        return result;
    }

    // Widen before adding: QRect's inclusive right/bottom can overflow int.
    const qint64 left = rectangle.x(), top = rectangle.y();
    const qint64 right = left + rectangle.width(), bottom = top + rectangle.height();
    const int firstX = tileCoordinate(left), lastX = tileCoordinate(right - 1);
    const int firstY = tileCoordinate(top), lastY = tileCoordinate(bottom - 1);
    const quint64 count = quint64(lastX - firstX + 1) * quint64(lastY - firstY + 1);
    const quint64 alignment = d->limits.minUniformBufferOffsetAlignment;
    const quint64 commandBytes = (count - 1) * alignment + sizeof(FillParameters);
    const quint64 resident = d->state->residentBytes.load();
    if (commandBytes > d->limits.maxBufferSize || commandBytes > std::numeric_limits<quint32>::max()
        || resident > d->budget || commandBytes > d->budget - resident
        || count > (d->budget - resident - commandBytes) / TileBytes) {
        result.error = Error::BudgetExceeded;
        return result;
    }

    result.completion.d = std::make_shared<CompletionData>();
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto data = std::make_shared<VersionData>(*base.d);
    data->completion = result.completion.d;
    auto parameters = std::make_shared<Allocation>(d->state, commandBytes,
        WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    std::vector<char> commands(size_t(commandBytes), 0);
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(
        wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
    quint64 copiedBytes = 0;
    quint32 index = 0;
    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x, ++index) {
            const qint64 tileLeft = qint64(x) * 64, tileTop = qint64(y) * 64;
            const FillParameters fill {
                quint32(std::max(left, tileLeft) - tileLeft),
                quint32(std::max(top, tileTop) - tileTop),
                quint32(std::min(right, tileLeft + 64) - tileLeft),
                quint32(std::min(bottom, tileTop + 64) - tileTop), rgba, quint32(kind), opacity, coverage
            };
            std::memcpy(commands.data() + index * alignment, &fill, sizeof(fill));
            auto tile = std::make_shared<Allocation>(d->state, TileBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst);
            const auto oldTile = base.d->tiles.find({x, y});
            if (oldTile != base.d->tiles.end()
                && (kind != UpdateKind::Fill || fill.left != 0 || fill.top != 0 || fill.right != 64 || fill.bottom != 64)) {
                wgpuCommandEncoderCopyBufferToBuffer(encoder.value, oldTile->second->buffer, 0, tile->buffer, 0, TileBytes);
                copiedBytes += TileBytes;
            }
            data->tiles[{x, y}] = tile;
            WGPUBindGroupEntry entries[2]{};
            entries[0].binding = 0;
            entries[0].buffer = tile->buffer;
            entries[0].size = TileBytes;
            entries[1].binding = 1;
            entries[1].buffer = parameters->buffer;
            entries[1].size = sizeof(FillParameters);
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->layout;
            descriptor.entryCount = 2;
            descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> group(
                wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(
                wgpuCommandEncoderBeginComputePass(encoder.value, nullptr));
            wgpuComputePassEncoderSetPipeline(pass.value, d->pipeline);
            const quint32 offset = quint32(index * alignment);
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, group.value, 1, &offset);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, 1);
            wgpuComputePassEncoderEnd(pass.value);
        }
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commandBuffer(
        wgpuCommandEncoderFinish(encoder.value, nullptr));
    result.version.d = std::move(data);
    result.completion.d->sequence = d->statistics.submissions + 1;
    auto callbackData = std::make_unique<std::shared_ptr<CompletionData>>(result.completion.d);
    d->pending.push_back({result.completion.d, base, result.version, parameters});
    wgpuQueueWriteBuffer(d->state->queue, parameters->buffer, 0, commands.data(), commands.size());
    d->lastSubmission = wgpuQueueSubmitForIndex(d->state->queue, 1, &commandBuffer.value);
    WGPUQueueWorkDoneCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = callbackData.release();
    callback.callback = [](WGPUQueueWorkDoneStatus status, void *data, void *) {
        // Spontaneous callbacks only publish a value; GPU references are released by poll().
        std::unique_ptr<std::shared_ptr<CompletionData>> completion(
            static_cast<std::shared_ptr<CompletionData> *>(data));
        (*completion)->complete(status == WGPUQueueWorkDoneStatus_Success);
    };
    wgpuQueueOnSubmittedWorkDone(d->state->queue, callback);
    errors.submitted = true;
    ++d->statistics.submissions;
    d->statistics.commandUploadBytes += commandBytes;
    d->statistics.tileCopyBytes += copiedBytes;
    return result;
}

void KisGpuTileStore::poll()
{
    wgpuDevicePoll(d->state->device, false, nullptr);
    d->pending.erase(std::remove_if(d->pending.begin(), d->pending.end(), [](const Private::Pending &operation) {
        return operation.completion->status.load() != Status::Pending;
    }), d->pending.end());
}

KisGpuTileStore::Statistics KisGpuTileStore::statistics() const
{
    Statistics result = d->statistics;
    result.residentBytes = d->state->residentBytes.load();
    return result;
}
