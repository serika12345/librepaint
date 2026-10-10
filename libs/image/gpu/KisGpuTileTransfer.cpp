/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTileStore_p.h"
#include <algorithm>
#include <cstring>
#include <limits>

using namespace KisGpuTileStorage;

QByteArray KisGpuTileStore::Readback::bytes() const
{
    return d && completion.status() == Status::Succeeded ? d->bytes : QByteArray();
}

KisGpuTileStore::Readback KisGpuTileStore::readback(const Version &source, QRect bounds)
{
    Readback result;
    if (!source.d || source.d->owner != d->state) {
        result.error = Error::InvalidVersion;
        return result;
    }
    if (!deviceAvailable()) {
        result.error = Error::DeviceLost;
        return result;
    }
    if (bounds.isEmpty()) {
        result.completion.d = source.d->completion;
        return result;
    }
    if (d->pending.size() >= d->maximumPending) {
        result.error = Error::QueueFull;
        return result;
    }
    const quint64 byteCount = quint64(bounds.width()) * quint64(bounds.height()) * 4;
    const quint64 resident = d->state->residentBytes.load();
    const quint64 available = resident <= d->budget ? d->budget - resident : 0;
    if (byteCount > available || byteCount > d->limits.maxBufferSize
        || byteCount > quint64(std::numeric_limits<int>::max())) {
        result.error = Error::BudgetExceeded;
        return result;
    }
    result.d = std::make_shared<ReadbackData>();
    result.d->bytes = QByteArray(int(byteCount), '\0');
    result.completion.d = std::make_shared<CompletionData>(d->state->availability);
    result.completion.d->remaining.store(4); // validation, allocation, submission, mapping
    Private::ErrorScopes errors(d->state->device, result.completion.d);
    auto staging = std::make_shared<Allocation>(d->state, byteCount,
        WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);
    Handle<WGPUCommandEncoder, wgpuCommandEncoderRelease> encoder(
        wgpuDeviceCreateCommandEncoder(d->state->device, nullptr));
    const qint64 left = bounds.x(), top = bounds.y();
    const qint64 right = left + bounds.width(), bottom = top + bounds.height();
    for (const auto &entry : source.d->tiles) {
        const qint64 tileLeft = qint64(entry.first.first) * 64, tileTop = qint64(entry.first.second) * 64;
        const qint64 x0 = std::max(left, tileLeft), y0 = std::max(top, tileTop);
        const qint64 x1 = std::min(right, tileLeft + 64), y1 = std::min(bottom, tileTop + 64);
        if (x0 >= x1 || y0 >= y1) continue;
        for (qint64 y = y0; y < y1; ++y) {
            const quint64 sourceOffset = entry.second.offset + ((y - tileTop) * 64 + x0 - tileLeft) * 4;
            const quint64 targetOffset = ((y - top) * bounds.width() + x0 - left) * 4;
            wgpuCommandEncoderCopyBufferToBuffer(encoder.value, entry.second.allocation->buffer, sourceOffset,
                staging->buffer, targetOffset, (x1 - x0) * 4);
        }
    }
    Handle<WGPUCommandBuffer, wgpuCommandBufferRelease> commands(
        wgpuCommandEncoderFinish(encoder.value, nullptr));
    struct MapResult {
        std::shared_ptr<ReadbackData> data;
        std::shared_ptr<CompletionData> completion;
        WGPUBuffer buffer;
        quint64 size;
        std::shared_ptr<Availability> availability;
    };
    auto callbackData = std::make_unique<MapResult>(MapResult{
        result.d, result.completion.d, staging->buffer, byteCount, d->state->availability});
    d->submit({result.completion.d, source, {}, staging, {}, {}}, commands.value, {}, {}, 0, 0);
    WGPUBufferMapCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = callbackData.release();
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *) {
        std::unique_ptr<MapResult> result(static_cast<MapResult *>(data));
        std::lock_guard<std::mutex> lock(result->availability->mapping);
        bool success = status == WGPUMapAsyncStatus_Success && result->availability->available.load();
        if (success) {
            const void *mapped = wgpuBufferGetConstMappedRange(result->buffer, 0, result->size);
            success = mapped != nullptr;
            if (success) std::memcpy(result->data->bytes.data(), mapped, size_t(result->size));
            wgpuBufferUnmap(result->buffer);
        }
        result->completion->complete(success);
    };
    wgpuBufferMapAsync(staging->buffer, WGPUMapMode_Read, 0, byteCount, callback);
    d->statistics.pixelReadbackBytes += byteCount;
    errors.submitted = true;
    return result;
}
