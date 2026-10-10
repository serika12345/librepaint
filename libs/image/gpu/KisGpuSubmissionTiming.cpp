/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <cmath>
#include <cstring>
#include <limits>

using namespace KisGpuTileStorage;

KisGpuTileStore::Private::Timing::Timing(const std::shared_ptr<DeviceState> &state, quint32 count)
    : owner(state->nativeOwner), passes(count), firstQuery(owner->acquireQueries(count * 2))
{
    queries = owner->timestampQueries;
    try {
        resolved = std::make_shared<Allocation>(state, 16 * count, WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc);
        staging = std::make_shared<Allocation>(state, 16 * count, WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead);
    } catch (...) {
        owner->releaseQueries(firstQuery, count * 2);
        throw;
    }
}
KisGpuTileStore::Private::Timing::~Timing() { owner->releaseQueries(firstQuery, passes * 2); }

KisGpuTileStore::Private::Recording::Recording(const std::shared_ptr<DeviceState> &state, quint32 passes)
    : value(wgpuDeviceCreateCommandEncoder(state->device, nullptr))
{
    if (!value) throw std::runtime_error("Cannot create GPU command encoder");
    if (passes) {
        try {
            timing = std::make_shared<Timing>(state, passes);
        } catch (...) {
            wgpuCommandEncoderRelease(value);
            throw;
        }
    }
}
KisGpuTileStore::Private::Recording::~Recording() { wgpuCommandEncoderRelease(value); }
WGPUComputePassEncoder KisGpuTileStore::Private::Recording::beginComputePass()
{
    WGPUComputePassDescriptor descriptor{};
    WGPUComputePassTimestampWrites writes{};
    if (timing) {
        writes.querySet = timing->queries;
        writes.beginningOfPassWriteIndex = timing->firstQuery + nextPass * 2;
        writes.endOfPassWriteIndex = timing->firstQuery + nextPass * 2 + 1;
        ++nextPass;
        descriptor.timestampWrites = &writes;
    }
    return wgpuCommandEncoderBeginComputePass(value, &descriptor);
}

WGPUCommandBuffer KisGpuTileStore::Private::Recording::finish()
{
    if (timing) {
        wgpuCommandEncoderResolveQuerySet(value, timing->queries, timing->firstQuery, 2 * timing->passes, timing->resolved->buffer, 0);
        wgpuCommandEncoderCopyBufferToBuffer(value, timing->resolved->buffer, 0, timing->staging->buffer, 0, 16 * timing->passes);
    }
    return wgpuCommandEncoderFinish(value, nullptr);
}

quint64 KisGpuTileStore::Private::availableForOperation(quint32 passes) const
{
    const quint64 resident = state->residentBytes.load(), overhead = timestamps ? 32 * quint64(passes) : 0;
    const quint64 free = resident <= budget ? budget - resident : 0;
    return free >= overhead ? free - overhead : 0;
}

void KisGpuTileStore::Private::mapTiming(std::unique_ptr<TimingReadbackData> result)
{
    const auto buffer = result->buffer;
    const quint64 bytes = 16 * result->passes;
    WGPUBufferMapCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = result.release();
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *) {
        std::unique_ptr<TimingReadbackData> result(static_cast<TimingReadbackData *>(data));
        std::lock_guard<std::mutex> lock(result->completion->availability->mapping);
        bool success = status == WGPUMapAsyncStatus_Success && result->completion->availability->available.load()
            && std::isfinite(result->period) && result->period > 0;
        if (success) {
            const void *mapped = wgpuBufferGetConstMappedRange(result->buffer, 0, 16 * result->passes);
            success = mapped != nullptr;
            if (success) {
                double duration = 0;
                const auto *bytes = static_cast<const char *>(mapped);
                for (quint32 i = 0; i < result->passes; ++i) {
                    quint64 ticks[2];
                    std::memcpy(ticks, bytes + i * 16, sizeof(ticks));
                    success &= ticks[0] != 0 && ticks[1] >= ticks[0];
                    duration += double(ticks[1] - ticks[0]) * result->period;
                }
                success &= std::isfinite(duration) && duration >= 0 && duration < double(std::numeric_limits<quint64>::max());
                if (success) result->completion->gpuNanoseconds = quint64(duration);
            }
            wgpuBufferUnmap(result->buffer);
        }
        result->completion->complete(success);
    };
    wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, bytes, callback);
    statistics.timingReadbackBytes += bytes;
}

std::optional<quint64> KisGpuTileStore::Completion::gpuComputeNanoseconds() const
{
    return status() == Status::Succeeded ? d->gpuNanoseconds : std::nullopt;
}
