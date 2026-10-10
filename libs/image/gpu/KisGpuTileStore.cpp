/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include "KisGpuDevice.h"

#include <QFile>
#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <vector>

static void initializeGpuTileResources()
{
    static const bool initialized = [] {
        Q_INIT_RESOURCE(gpu_document);
        return true;
    }();
    Q_UNUSED(initialized);
}

using namespace KisGpuTileStorage;

KisGpuTileStore::Private::Private(std::shared_ptr<NativeDevice> nativeOwner, quint64 bytes, quint32 maximum)
    : budget(bytes), maximumPending(maximum) {
    if (!nativeOwner || !nativeOwner->availability->available.load() || !maximum) {
        throw std::runtime_error("GPU tiles require an available device and a nonzero pending limit");
    }
    state = std::make_shared<DeviceState>(std::move(nativeOwner));
    const auto device = state->device;
    timestamps = wgpuDeviceHasFeature(device, WGPUFeatureName_TimestampQuery);
    if (wgpuDeviceGetLimits(device, &limits) != WGPUStatus_Success
        || limits.maxStorageBufferBindingSize < TileBytes || limits.maxBufferSize < TileBytes
        || limits.minStorageBufferOffsetAlignment == 0 || limits.maxComputeWorkgroupsPerDimension < 8) {
        throw std::runtime_error("GPU device cannot bind a document tile");
    }
    tilesPerAllocation = std::min<quint64>({quint64(64), limits.maxStorageBufferBindingSize / TileBytes,
        limits.maxBufferSize / TileBytes, quint64(limits.maxComputeWorkgroupsPerDimension)});
    WGPUBindGroupLayoutEntry entries[4]{};
    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Compute;
    entries[0].buffer.type = WGPUBufferBindingType_Storage;
    entries[0].buffer.minBindingSize = TileBytes;
    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Compute;
    entries[1].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entries[1].buffer.minBindingSize = sizeof(TileParameters);
    entries[2].binding = 2;
    entries[2].visibility = WGPUShaderStage_Compute;
    entries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entries[2].buffer.minBindingSize = sizeof(TileCommand);
    WGPUBindGroupLayoutDescriptor layoutDescriptor{};
    layoutDescriptor.entryCount = 3;
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
    initializeGpuTileResources();
    QFile shaderFile(QStringLiteral(":/librepaint/gpu/KisGpuTilePaint.wgsl"));
    if (!shaderFile.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Cannot load GPU tile shader resource");
    }
    const QByteArray shaderCode = shaderFile.readAll();
    source.code = {shaderCode.constData(), size_t(shaderCode.size())};
    WGPUShaderModuleDescriptor shaderDescriptor{};
    shaderDescriptor.nextInChain = &source.chain;
    Handle<WGPUShaderModule, wgpuShaderModuleRelease> shader(
        wgpuDeviceCreateShaderModule(device, &shaderDescriptor));
    WGPUComputePipelineDescriptor descriptor{};
    descriptor.layout = pipelineLayout.value;
    descriptor.compute.module = shader.value;
    descriptor.compute.entryPoint = {"paint", WGPU_STRLEN};
    Handle<WGPUComputePipeline, wgpuComputePipelineRelease> fillPipeline(
        wgpuDeviceCreateComputePipeline(device, &descriptor));
    entries[1].binding = 3;
    entries[1].buffer.minBindingSize = TileBytes;
    entries[2].binding = 4;
    entries[2].buffer.minBindingSize = sizeof(CompositeParameters);
    entries[3].binding = 5;
    entries[3].visibility = WGPUShaderStage_Compute;
    entries[3].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
    entries[3].buffer.minBindingSize = TileBytes;
    layoutDescriptor.entryCount = 4;
    Handle<WGPUBindGroupLayout, wgpuBindGroupLayoutRelease> imageLayout(
        wgpuDeviceCreateBindGroupLayout(device, &layoutDescriptor));
    pipelineLayoutDescriptor.bindGroupLayouts = &imageLayout.value;
    Handle<WGPUPipelineLayout, wgpuPipelineLayoutRelease> imagePipelineLayout(
        wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDescriptor));
    descriptor.layout = imagePipelineLayout.value;
    descriptor.compute.entryPoint = {"composite", WGPU_STRLEN};
    Handle<WGPUComputePipeline, wgpuComputePipelineRelease> imagePipeline(
        wgpuDeviceCreateComputePipeline(device, &descriptor));
    layout = groupLayout.value;
    groupLayout.value = nullptr;
    pipeline = fillPipeline.value;
    fillPipeline.value = nullptr;
    compositeLayout = imageLayout.value;
    imageLayout.value = nullptr;
    compositePipeline = imagePipeline.value;
    imagePipeline.value = nullptr;
}

void KisGpuTileStore::Private::submit(Pending operation, WGPUCommandBuffer commandBuffer, const std::vector<char> &parameters,
            const std::vector<TileCommand> &commands, quint64 copiedBytes, quint64 dispatches, const QByteArray &pixelInput, std::shared_ptr<Timing> timing) {
    const auto completion = operation.completion;
    // Prepare notification storage before any command can reach the queue.
    std::unique_ptr<TimingReadbackData> timingResult;
    if (timing) {
        timingResult = std::make_unique<TimingReadbackData>(TimingReadbackData{
            completion, timing->staging->buffer, wgpuQueueGetTimestampPeriod(state->queue), timing->passes});
        completion->remaining.fetch_add(1);
        operation.timing = timing;
    }
    if (operation.source.d) completion->dependencies.push_back(operation.source.d->completion);
    for (const auto &input : operation.inputs) {
        if (input.d && !(input == operation.source)) completion->dependencies.push_back(input.d->completion);
    }
    completion->sequence = statistics.submissions + 1;
    auto callbackData = std::make_unique<std::shared_ptr<CompletionData>>(completion);
    pending.push_back(std::move(operation));
    const auto &resources = pending.back();
    if (!parameters.empty()) {
        wgpuQueueWriteBuffer(state->queue, resources.parameters->buffer, 0, parameters.data(), parameters.size());
    }
    if (!pixelInput.isEmpty()) {
        wgpuQueueWriteBuffer(state->queue, resources.parameters->buffer, 0, pixelInput.constData(), pixelInput.size());
    }
    if (!commands.empty()) {
        wgpuQueueWriteBuffer(state->queue, resources.commands->buffer, 0,
                             commands.data(), commands.size() * sizeof(TileCommand));
    }
    lastSubmission = wgpuQueueSubmitForIndex(state->queue, 1, &commandBuffer);
    WGPUQueueWorkDoneCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = callbackData.release();
    callback.callback = [](WGPUQueueWorkDoneStatus status, void *data, void *) {
        // Spontaneous callbacks only publish a value; GPU references are released by poll().
        std::unique_ptr<std::shared_ptr<CompletionData>> completion(
            static_cast<std::shared_ptr<CompletionData> *>(data));
        (*completion)->complete(status == WGPUQueueWorkDoneStatus_Success);
    };
    wgpuQueueOnSubmittedWorkDone(state->queue, callback);
    if (timingResult) mapTiming(std::move(timingResult));
    ++statistics.submissions;
    statistics.computeDispatches += dispatches;
    statistics.commandUploadBytes += parameters.size() + commands.size() * sizeof(TileCommand);
    statistics.tileCopyBytes += copiedBytes;
    statistics.pixelUploadBytes += pixelInput.size();
}

void KisGpuTileStore::Private::collect() {
    // Submission order is also dependency order. Publish results on the owning thread.
    for (auto &operation : pending) operation.completion->publish();
    pending.erase(std::remove_if(pending.begin(), pending.end(), [](const Pending &operation) {
        return operation.completion->remaining.load() == 0
            && operation.completion->status.load() != Status::Pending;
    }), pending.end());
}

KisGpuTileStore::Private::~Private() {
    if (lastSubmission) wgpuDevicePoll(state->device, true, &lastSubmission);
    collect();
    pending.clear();
    wgpuComputePipelineRelease(pipeline);
    wgpuBindGroupLayoutRelease(layout);
    wgpuComputePipelineRelease(compositePipeline);
    wgpuBindGroupLayoutRelease(compositeLayout);
}

KisGpuTileStore::KisGpuTileStore(KisGpuDevice &device, quint64 budgetBytes, quint32 maximumPending)
    : d(new Private(device.d, budgetBytes, maximumPending)) {}
KisGpuTileStore::~KisGpuTileStore() = default;

qsizetype KisGpuTileStore::Version::tileCount() const { return d ? qsizetype(d->tiles.size()) : 0; }

KisGpuTileStore::TileView KisGpuTileStore::Version::tile(QPoint coordinate) const
{
    if (!d) return {};
    const auto found = d->tiles.find({coordinate.x(), coordinate.y()});
    return found == d->tiles.end() ? TileView{} : TileView{found->second.allocation->buffer, found->second.offset};
}

KisGpuTileStore::Status KisGpuTileStore::Completion::status() const
{
    return d ? d->currentStatus() : Status::Failed;
}

quint64 KisGpuTileStore::Completion::sequence() const { return d ? d->sequence : 0; }

KisGpuTileStore::Version KisGpuTileStore::emptyVersion() const
{
    Version result;
    auto data = std::make_shared<VersionData>();
    data->owner = d->state;
    data->completion = std::make_shared<CompletionData>(d->state->availability);
    data->completion->remaining.store(0);
    data->completion->status.store(Status::Succeeded);
    result.d = std::move(data);
    return result;
}

void KisGpuTileStore::poll()
{
    wgpuDevicePoll(d->state->device, false, nullptr);
    d->collect();
}

void KisGpuTileStore::invalidateDevice()
{
    std::lock_guard<std::mutex> lock(d->state->availability->mapping);
    d->state->availability->available.store(false);
}

bool KisGpuTileStore::deviceAvailable() const { return d->state->availability->available.load(); }

KisGpuTileStore::Statistics KisGpuTileStore::statistics() const
{
    Statistics result = d->statistics;
    result.residentBytes = d->state->residentBytes.load();
    return result;
}
