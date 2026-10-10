/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TILE_STORE_P_H
#define KIS_GPU_TILE_STORE_P_H

#include "KisGpuTileStore.h"
#include "KisGpuDevice_p.h"
#include <array>
#include <atomic>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace KisGpuTileStorage {
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
    std::shared_ptr<Availability> availability;
    std::shared_ptr<NativeDevice> nativeOwner;
    explicit DeviceState(std::shared_ptr<NativeDevice> native)
        : device(native->device), queue(wgpuDeviceGetQueue(device)), availability(native->availability),
          nativeOwner(std::move(native)) {}
    ~DeviceState() { wgpuQueueRelease(queue); }
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
inline int tileCoordinate(qint64 pixel) { return int(pixel >= 0 ? pixel / 64 : (pixel + 1) / 64 - 1); }
struct TileCommand {
    quint32 left, top, right, bottom, color, operation, opacity, coverage;
};
static_assert(sizeof(TileCommand) == 32);
// A dab has one raster command followed by one equally sized shape record.
struct DabParameters {
    float centerX, centerY, xcoef, ycoef, fadeX, fadeY;
    quint32 padding[2];
};
static_assert(sizeof(DabParameters) == sizeof(TileCommand));
struct TileParameters {
    quint32 firstCommand, commandCount, selectionOffset, reserved;
    quint32 textureX, textureY, textureWidth, textureHeight;
};
static_assert(sizeof(TileParameters) == 32);
struct CompositeParameters {
    quint32 sourceTile, destinationTile, left, top, right, bottom, operation, opacity, coverage, maskTile;
};
static_assert(sizeof(CompositeParameters) == 40);
}

struct KisGpuTileStore::VersionData {
    std::shared_ptr<KisGpuTileStorage::DeviceState> owner;
    std::shared_ptr<CompletionData> completion;
    struct Tile {
        std::shared_ptr<KisGpuTileStorage::Allocation> allocation;
        quint64 offset;
    };
    std::map<KisGpuTileStorage::Coordinate, Tile> tiles;
};

struct KisGpuTileStore::CompletionData {
    std::optional<quint64> gpuNanoseconds;
    std::atomic<Status> status{Status::Pending};
    std::atomic<unsigned> remaining{3};
    std::atomic<bool> failed{false};
    quint64 sequence = 0;
    std::vector<std::shared_ptr<CompletionData>> dependencies;
    std::shared_ptr<KisGpuTileStorage::Availability> availability;
    explicit CompletionData(std::shared_ptr<KisGpuTileStorage::Availability> value) : availability(std::move(value)) {}
    Status currentStatus() const {
        const auto current = status.load();
        return current == Status::Pending && !availability->available.load() ? Status::Failed : current;
    }
    void complete(bool success) {
        if (!success) failed.store(true);
        remaining.fetch_sub(1);
    }
    void publish() {
        if (remaining.load() != 0) return;
        if (!availability->available.load()) failed.store(true);
        for (const auto &dependency : dependencies) {
            const auto state = dependency->currentStatus();
            if (state == Status::Pending && !failed.load()) return;
            if (state == Status::Failed) failed.store(true);
        }
        status.store(failed.load() ? Status::Failed : Status::Succeeded);
        dependencies.clear();
    }
};

struct KisGpuTileStore::ReadbackData {
    QByteArray bytes;
};

struct KisGpuTileStore::TextureData {
    std::shared_ptr<KisGpuTileStorage::DeviceState> owner;
    WGPUTexture texture;
    QRect bounds;
    quint64 bytes;
    TextureData(std::shared_ptr<KisGpuTileStorage::DeviceState> state, QRect rectangle);
    ~TextureData();
    TextureData(const TextureData &) = delete;
    TextureData &operator=(const TextureData &) = delete;
};

struct KisGpuTileStore::BrushTextureData {
    std::shared_ptr<KisGpuTileStorage::Allocation> allocation;
    std::shared_ptr<CompletionData> completion;
    QSize size;
};

struct KisGpuTileStore::Private {
    struct Timing {
        std::shared_ptr<KisGpuTileStorage::NativeDevice> owner;
        std::shared_ptr<KisGpuTileStorage::Allocation> resolved, staging;
        WGPUQuerySet queries = nullptr;
        quint32 passes, firstQuery;
        Timing(const std::shared_ptr<KisGpuTileStorage::DeviceState> &state, quint32 passes);
        ~Timing();
        Timing(const Timing &) = delete;
        Timing &operator=(const Timing &) = delete;
    };
    struct TimingReadbackData {
        std::shared_ptr<CompletionData> completion;
        WGPUBuffer buffer;
        float period;
        quint32 passes;
    };
    struct Recording {
        WGPUCommandEncoder value;
        std::shared_ptr<Timing> timing;
        quint32 nextPass = 0;
        Recording(const std::shared_ptr<KisGpuTileStorage::DeviceState> &state, quint32 passes);
        ~Recording();
        Recording(const Recording &) = delete;
        Recording &operator=(const Recording &) = delete;
        WGPUComputePassEncoder beginComputePass();
        WGPUCommandBuffer finish();
    };
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
        std::shared_ptr<KisGpuTileStorage::Allocation> parameters, commands;
        QVector<Version> inputs;
        std::shared_ptr<Timing> timing = {};
        std::shared_ptr<TextureData> texture = {};
        std::shared_ptr<BrushTextureData> brushTexture = {};
    };
    std::shared_ptr<KisGpuTileStorage::DeviceState> state;
    quint64 budget;
    quint32 maximumPending;
    bool timestamps = false;
    WGPULimits limits{};
    quint64 tilesPerAllocation = 0;
    WGPUBindGroupLayout layout = nullptr;
    WGPUComputePipeline pipeline = nullptr;
    WGPUBindGroupLayout dabSelectionLayout = nullptr;
    WGPUComputePipeline dabSelectionPipeline = nullptr;
    struct DabPipelines {
        std::array<WGPUBindGroupLayout, 2> layouts{};
        std::array<WGPUComputePipeline, 2> pipelines{};
        ~DabPipelines();
        DabPipelines() = default;
        DabPipelines(const DabPipelines &) = delete;
        DabPipelines &operator=(const DabPipelines &) = delete;
    };
    std::unique_ptr<DabPipelines> texturePipelines;
    WGPUBindGroupLayout compositeLayout = nullptr;
    WGPUComputePipeline compositePipeline = nullptr;
    WGPUSubmissionIndex lastSubmission = 0;
    Statistics statistics;
    std::vector<Pending> pending;

    Private(std::shared_ptr<KisGpuTileStorage::NativeDevice> nativeOwner, quint64 bytes, quint32 maximum);
    std::unique_ptr<DabPipelines> createDabPipelines(WGPUShaderModule shader, quint32 inputBinding,
                                                   const char *entryPoint, const char *selectedEntryPoint);
    quint64 availableForOperation(quint32 passes = 1) const;
    void mapTiming(std::unique_ptr<TimingReadbackData> result);

    void submit(Pending operation, WGPUCommandBuffer commandBuffer, const std::vector<char> &parameters,
                const std::vector<KisGpuTileStorage::TileCommand> &commands, quint64 copiedBytes, quint64 dispatches,
                const QByteArray &pixelInput = {}, std::shared_ptr<Timing> timing = {});

    void collect();

    ~Private();
};

#endif
