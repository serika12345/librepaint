/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TILE_STORE_P_H
#define KIS_GPU_TILE_STORE_P_H

#include "KisGpuTileStore.h"
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

struct Availability {
    std::mutex mapping;
    std::atomic<bool> available{true};
};

struct DeviceState {
    WGPUDevice device;
    WGPUQueue queue;
    std::atomic<quint64> residentBytes{0};
    std::shared_ptr<Availability> availability = std::make_shared<Availability>();
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
    quint32 firstCommand, commandCount, padding[2];
};
static_assert(sizeof(TileParameters) == 16);
struct CompositeParameters {
    quint32 sourceTile, destinationTile, left, top, right, bottom, operation, opacity, coverage, padding;
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
        std::shared_ptr<KisGpuTileStorage::Allocation> parameters, commands;
        Version input;
    };
    std::shared_ptr<KisGpuTileStorage::DeviceState> state;
    quint64 budget;
    quint32 maximumPending;
    WGPULimits limits{};
    quint64 tilesPerAllocation = 0;
    WGPUBindGroupLayout layout = nullptr;
    WGPUComputePipeline pipeline = nullptr;
    WGPUBindGroupLayout compositeLayout = nullptr;
    WGPUComputePipeline compositePipeline = nullptr;
    WGPUSubmissionIndex lastSubmission = 0;
    Statistics statistics;
    std::vector<Pending> pending;

    Private(WGPUDevice device, quint64 bytes, quint32 maximum);

    void submit(Pending operation, WGPUCommandBuffer commandBuffer, const std::vector<char> &parameters,
                const std::vector<KisGpuTileStorage::TileCommand> &commands, quint64 copiedBytes, quint64 dispatches,
                const QByteArray &pixelInput = {});

    void collect();

    ~Private();
};

#endif
