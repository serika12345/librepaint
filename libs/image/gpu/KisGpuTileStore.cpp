/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"

#include <QFile>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <cmath>
#include <limits>
#include <map>
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

KisGpuTileStore::Private::Private(WGPUDevice device, quint64 bytes, quint32 maximum) : budget(bytes), maximumPending(maximum) {
    if (!device || !maximum || wgpuGetVersion() != 0x1b000400) {
        throw std::runtime_error("GPU tiles require a device from wgpu-native 27.0.4.0");
    }
    state = std::make_shared<DeviceState>(device);
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
            const std::vector<TileCommand> &commands, quint64 copiedBytes, quint64 dispatches, const QByteArray &pixelInput) {
    const auto completion = operation.completion;
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

KisGpuTileStore::KisGpuTileStore(WGPUDevice device, quint64 budgetBytes, quint32 maximumPending)
    : d(new Private(device, budgetBytes, maximumPending)) {}
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

KisGpuTileStore::Edit KisGpuTileStore::fill(const Version &base, QRect rectangle, quint32 rgba)
{
    return update(base, {{rectangle, rgba, UpdateKind::Fill, 255, 255, {}, {}, {}}});
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
                           command.opacity, command.coverage, {}, {}, {}});
    }
    return update(base, updates);
}

KisGpuTileStore::Edit KisGpuTileStore::paintDabs(const Version &base, const QVector<DabCommand> &commands, QRect clip)
{
    if (!base.d || base.d->owner != d->state) return {Error::InvalidVersion, {}, {}};
    if (!deviceAvailable()) return {Error::DeviceLost, {}, {}};
    QVector<UpdateCommand> updates;
    updates.reserve(commands.size());
    for (const auto &command : commands) {
        const double cx = command.center.x(), cy = command.center.y();
        const double dx = command.diameter.width(), dy = command.diameter.height();
        const double fx = command.fade.width(), fy = command.fade.height();
        if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(dx) || !std::isfinite(dy)
            || !std::isfinite(fx) || !std::isfinite(fy) || dx <= 0 || dy <= 0 || fx <= 0 || fy <= 0
            || fx > 1 || fy > 1 || std::abs(cx) > std::numeric_limits<int>::max()
            || std::abs(cy) > std::numeric_limits<int>::max()
            || dx > std::numeric_limits<int>::max() || dy > std::numeric_limits<int>::max()
            || !std::isfinite(float(2 / dx / fx))
            || !std::isfinite(float(2 / dy / fy))) {
            return {Error::InvalidCommand, {}, {}};
        }
        if (clip.isEmpty()) continue;
        const qint64 left = std::max(qint64(clip.x()), qint64(std::floor(cx - dx / 2)));
        const qint64 top = std::max(qint64(clip.y()), qint64(std::floor(cy - dy / 2)));
        const qint64 right = std::min(qint64(clip.x()) + clip.width(), qint64(std::ceil(cx + dx / 2)) + 1);
        const qint64 bottom = std::min(qint64(clip.y()) + clip.height(), qint64(std::ceil(cy + dy / 2)) + 1);
        if (left >= right || top >= bottom) continue;
        updates.push_back({QRect(int(left), int(top), int(right - left), int(bottom - top)), command.rgba,
            command.operation == CompositeOp::Erase ? UpdateKind::DabErase : UpdateKind::DabOver,
            command.opacity, command.coverage, command.center, command.diameter, command.fade});
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
    if (!deviceAvailable()) {
        result.error = Error::DeviceLost;
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
    const quint64 maximumCommands = std::min<quint64>({d->limits.maxBufferSize,
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
        const bool dab = command.kind == UpdateKind::DabOver || command.kind == UpdateKind::DabErase;
        const quint64 records = dab ? 2 : 1;
        if (count > available / TileBytes || count > (maximumCommands - commandCount) / records) {
            result.error = Error::BudgetExceeded;
            return result;
        }
        for (int y = firstY; y <= lastY; ++y) {
            for (int x = firstX; x <= lastX; ++x) {
                auto &list = tileCommands[{x, y}];
                commandCount += records;
                if (!fits(tileCommands.size(), commandCount)) {
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
                if (dab) {
                    const DabParameters shape {float(command.center.x() - tileLeft), float(command.center.y() - tileTop),
                        float(2 / command.diameter.width()), float(2 / command.diameter.height()),
                        float(2 / command.diameter.width() / command.fade.width()),
                        float(2 / command.diameter.height() / command.fade.height()), {0, 0}};
                    TileCommand record;
                    std::memcpy(&record, &shape, sizeof(record));
                    list.push_back(record);
                }
            }
        }
    }
    if (tileCommands.empty()) {
        result.version = base;
        result.completion.d = base.d->completion;
        return result;
    }
    if (d->pending.size() >= d->maximumPending) {
        result.error = Error::QueueFull;
        return result;
    }
    const quint64 parameterBytes = parameterSize(tileCommands.size());
    const quint64 storageBytes = commandCount * sizeof(TileCommand);
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
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
    d->submit({result.completion.d, base, result.version, parameters, commandStorage, {}}, commandBuffer.value,
              packedParameters, packedCommands, copiedBytes, allocations.size());
    errors.submitted = true;
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
