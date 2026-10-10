/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore.h"

#include <QFile>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include <map>
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
struct TileCommand {
    quint32 left, top, right, bottom, color, operation, opacity, coverage;
};
static_assert(sizeof(TileCommand) == 32);
struct TileParameters {
    quint32 firstCommand, commandCount, padding[2];
};
static_assert(sizeof(TileParameters) == 16);
}

struct KisGpuTileStore::VersionData {
    std::shared_ptr<DeviceState> owner;
    std::shared_ptr<CompletionData> completion;
    struct Tile {
        std::shared_ptr<Allocation> allocation;
        quint64 offset;
    };
    std::map<Coordinate, Tile> tiles;
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
        std::shared_ptr<Allocation> parameters, commands;
    };
    std::shared_ptr<DeviceState> state;
    quint64 budget;
    WGPULimits limits{};
    quint64 tilesPerAllocation = 0;
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
            || limits.maxStorageBufferBindingSize < TileBytes || limits.maxBufferSize < TileBytes
            || limits.minStorageBufferOffsetAlignment == 0 || limits.maxComputeWorkgroupsPerDimension < 8) {
            throw std::runtime_error("GPU device cannot bind a document tile");
        }
        tilesPerAllocation = std::min({quint64(64), limits.maxStorageBufferBindingSize / TileBytes,
            limits.maxBufferSize / TileBytes, quint64(limits.maxComputeWorkgroupsPerDimension)});
        WGPUBindGroupLayoutEntry entries[3]{};
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

KisGpuTileStore::TileView KisGpuTileStore::Version::tile(QPoint coordinate) const
{
    if (!d) return {};
    const auto found = d->tiles.find({coordinate.x(), coordinate.y()});
    return found == d->tiles.end() ? TileView{} : TileView{found->second.allocation->buffer, found->second.offset};
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
    return update(base, {{rectangle, rgba, UpdateKind::Fill, 255, 255}});
}

KisGpuTileStore::Edit KisGpuTileStore::paint(const Version &base, QRect rectangle, quint32 rgba,
                                          CompositeOp operation, quint8 opacity, quint8 coverage)
{
    return paint(base, {{rectangle, rgba, operation, opacity, coverage}});
}

KisGpuTileStore::Edit KisGpuTileStore::paint(const Version &base, const QVector<PaintCommand> &commands)
{
    QVector<UpdateCommand> updates;
    updates.reserve(commands.size());
    for (const auto &command : commands) {
        updates.push_back({command.rectangle, command.rgba,
                           command.operation == CompositeOp::Erase ? UpdateKind::Erase : UpdateKind::Over,
                           command.opacity, command.coverage});
    }
    return update(base, updates);
}

KisGpuTileStore::Edit KisGpuTileStore::update(const Version &base, const QVector<UpdateCommand> &commands)
{
    Edit result;
    if (!base.d || base.d->owner != d->state) {
        result.error = Error::InvalidVersion;
        return result;
    }
    const quint64 resident = d->state->residentBytes.load();
    const quint64 available = resident <= d->budget ? d->budget - resident : 0;
    const quint64 alignment = d->limits.minStorageBufferOffsetAlignment;
    const quint64 capacity = d->tilesPerAllocation;
    const quint64 parameterStride = (capacity * sizeof(TileParameters) + alignment - 1) / alignment * alignment;
    auto parameterSize = [&](quint64 tiles) {
        return (tiles - 1) / capacity * parameterStride + ((tiles - 1) % capacity + 1) * sizeof(TileParameters);
    };
    const quint64 maximumCommands = std::min({d->limits.maxBufferSize,
        d->limits.maxStorageBufferBindingSize, quint64(std::numeric_limits<quint32>::max())}) / sizeof(TileCommand);
    quint64 commandCount = 0;
    std::map<Coordinate, std::vector<TileCommand>> tileCommands;
    auto fits = [&](quint64 tiles, quint64 count) {
        if (tiles > available / TileBytes || count > maximumCommands) return false;
        const quint64 parameterBytes = parameterSize(tiles);
        const quint64 storageBytes = count * sizeof(TileCommand);
        const quint64 remaining = available - tiles * TileBytes;
        return parameterBytes <= d->limits.maxBufferSize
            && parameterBytes <= remaining && storageBytes <= remaining - parameterBytes;
    };
    for (const auto &command : commands) {
        const QRect rectangle = command.rectangle;
        if (rectangle.isEmpty() || (command.kind != UpdateKind::Fill
            && (command.opacity == 0 || command.coverage == 0 || (command.rgba >> 24) == 0))) continue;
        // Widen before adding: QRect's inclusive right/bottom can overflow int.
        const qint64 left = rectangle.x(), top = rectangle.y();
        const qint64 right = left + rectangle.width(), bottom = top + rectangle.height();
        const int firstX = tileCoordinate(left), lastX = tileCoordinate(right - 1);
        const int firstY = tileCoordinate(top), lastY = tileCoordinate(bottom - 1);
        const quint64 count = quint64(lastX - firstX + 1) * quint64(lastY - firstY + 1);
        if (count > available / TileBytes || count > maximumCommands - commandCount) {
            result.error = Error::BudgetExceeded;
            return result;
        }
        for (int y = firstY; y <= lastY; ++y) {
            for (int x = firstX; x <= lastX; ++x) {
                auto &list = tileCommands[{x, y}];
                if (!fits(tileCommands.size(), ++commandCount)) {
                    result.error = Error::BudgetExceeded;
                    return result;
                }
                const qint64 tileLeft = qint64(x) * 64, tileTop = qint64(y) * 64;
                list.push_back({
                    quint32(std::max(left, tileLeft) - tileLeft),
                    quint32(std::max(top, tileTop) - tileTop),
                    quint32(std::min(right, tileLeft + 64) - tileLeft),
                    quint32(std::min(bottom, tileTop + 64) - tileTop),
                    command.rgba, quint32(command.kind), command.opacity, command.coverage
                });
            }
        }
    }
    if (tileCommands.empty()) {
        result.version = base;
        result.completion.d = base.d->completion;
        return result;
    }
    const quint64 parameterBytes = parameterSize(tileCommands.size());
    const quint64 storageBytes = commandCount * sizeof(TileCommand);
    result.completion.d = std::make_shared<CompletionData>();
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto data = std::make_shared<VersionData>(*base.d);
    data->completion = result.completion.d;
    auto parameters = std::make_shared<Allocation>(d->state, parameterBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto commandStorage = std::make_shared<Allocation>(d->state, storageBytes,
        WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    std::vector<char> packedParameters(size_t(parameterBytes), 0);
    std::vector<std::shared_ptr<Allocation>> allocations;
    std::vector<TileCommand> packedCommands;
    packedCommands.reserve(size_t(commandCount));
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(
        wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
    quint64 copiedBytes = 0;
    quint32 index = 0;
    for (const auto &entry : tileCommands) {
        const TileParameters tileParameters {quint32(packedCommands.size()), quint32(entry.second.size()), {0, 0}};
        const quint64 parameterOffset = index / capacity * parameterStride + index % capacity * sizeof(TileParameters);
        std::memcpy(packedParameters.data() + parameterOffset, &tileParameters, sizeof(tileParameters));
        packedCommands.insert(packedCommands.end(), entry.second.begin(), entry.second.end());
        if (index % capacity == 0) {
            const quint64 count = std::min(capacity, quint64(tileCommands.size()) - index);
            allocations.push_back(std::make_shared<Allocation>(d->state, count * TileBytes,
                WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst));
        }
        const auto &allocation = allocations.back();
        const quint64 tileOffset = index % capacity * TileBytes;
        const auto oldTile = base.d->tiles.find(entry.first);
        const auto &first = entry.second.front();
        if (oldTile != base.d->tiles.end()
            && (first.operation != quint32(UpdateKind::Fill)
                || first.left != 0 || first.top != 0 || first.right != 64 || first.bottom != 64)) {
            wgpuCommandEncoderCopyBufferToBuffer(encoder.value, oldTile->second.allocation->buffer, oldTile->second.offset,
                                                 allocation->buffer, tileOffset, TileBytes);
            copiedBytes += TileBytes;
        }
        data->tiles[entry.first] = {allocation, tileOffset};
        ++index;
    }
    {
        Handle<WGPUComputePassEncoder, wgpuComputePassEncoderRelease> pass(
            wgpuCommandEncoderBeginComputePass(encoder.value, nullptr));
        wgpuComputePassEncoderSetPipeline(pass.value, d->pipeline);
        for (size_t groupIndex = 0; groupIndex < allocations.size(); ++groupIndex) {
            const auto &allocation = allocations[groupIndex];
            const quint64 count = allocation->bytes / TileBytes;
            WGPUBindGroupEntry entries[3]{};
            entries[0].binding = 0;
            entries[0].buffer = allocation->buffer;
            entries[0].size = allocation->bytes;
            entries[1].binding = 1;
            entries[1].buffer = parameters->buffer;
            entries[1].offset = groupIndex * parameterStride;
            entries[1].size = count * sizeof(TileParameters);
            entries[2].binding = 2;
            entries[2].buffer = commandStorage->buffer;
            entries[2].size = storageBytes;
            WGPUBindGroupDescriptor descriptor{};
            descriptor.layout = d->layout;
            descriptor.entryCount = 3;
            descriptor.entries = entries;
            Handle<WGPUBindGroup, wgpuBindGroupRelease> group(
                wgpuDeviceCreateBindGroup(d->state->device, &descriptor));
            wgpuComputePassEncoderSetBindGroup(pass.value, 0, group.value, 0, nullptr);
            wgpuComputePassEncoderDispatchWorkgroups(pass.value, 8, 8, quint32(count));
        }
        wgpuComputePassEncoderEnd(pass.value);
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commandBuffer(
        wgpuCommandEncoderFinish(encoder.value, nullptr));
    result.version.d = std::move(data);
    result.completion.d->sequence = d->statistics.submissions + 1;
    auto callbackData = std::make_unique<std::shared_ptr<CompletionData>>(result.completion.d);
    d->pending.push_back({result.completion.d, base, result.version, parameters, commandStorage});
    wgpuQueueWriteBuffer(d->state->queue, parameters->buffer, 0, packedParameters.data(), packedParameters.size());
    wgpuQueueWriteBuffer(d->state->queue, commandStorage->buffer, 0, packedCommands.data(), storageBytes);
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
    d->statistics.computeDispatches += allocations.size();
    d->statistics.commandUploadBytes += parameterBytes + storageBytes;
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
