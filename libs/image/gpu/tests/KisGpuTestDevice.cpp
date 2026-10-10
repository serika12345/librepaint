/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTestDevice.h"

#include <QDebug>
#include <cstring>
#include <future>
#include <map>
#include <stdexcept>

namespace {
int tileCoordinate(qint64 pixel) { return int(pixel >= 0 ? pixel / 64 : (pixel + 1) / 64 - 1); }
}

KisGpuTestDevice::KisGpuTestDevice(quint64 storageBindingLimit, bool timestamps, quint64 maximumResidentBytes)
    : owner(storageBindingLimit, timestamps, maximumResidentBytes), device(owner.device()), queue(wgpuDeviceGetQueue(device)), name(owner.adapterName())
{
    if (!queue) throw std::runtime_error("Cannot get GPU queue");
}

KisGpuTestDevice::~KisGpuTestDevice()
{
    wgpuDevicePoll(device, true, nullptr);
    wgpuQueueRelease(queue);
}

QByteArray KisGpuTestDevice::read(const KisGpuTileStore::Version &version, QRect bounds)
{
    std::map<std::pair<int, int>, QByteArray> tiles;
    QByteArray result(bounds.width() * bounds.height() * 4, '\0');
    for (qint64 y = bounds.y(); y < qint64(bounds.y()) + bounds.height(); ++y) {
        for (qint64 x = bounds.x(); x < qint64(bounds.x()) + bounds.width(); ++x) {
            const auto coordinate = std::make_pair(tileCoordinate(x), tileCoordinate(y));
            const auto source = version.tile(QPoint(coordinate.first, coordinate.second));
            if (!source.buffer) continue;
            auto found = tiles.find(coordinate);
            if (found == tiles.end()) {
                WGPUBufferDescriptor descriptor{};
                descriptor.size = KisGpuTileStore::TileBytes;
                descriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
                const auto staging = wgpuDeviceCreateBuffer(device, &descriptor);
                const auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
                wgpuCommandEncoderCopyBufferToBuffer(encoder, source.buffer, source.offset, staging, 0, descriptor.size);
                const auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
                wgpuQueueSubmit(queue, 1, &commands);
                wgpuCommandBufferRelease(commands);
                wgpuCommandEncoderRelease(encoder);
                std::promise<WGPUMapAsyncStatus> promise;
                auto future = promise.get_future();
                WGPUBufferMapCallbackInfo callback{};
                callback.mode = WGPUCallbackMode_AllowSpontaneous;
                callback.userdata1 = &promise;
                callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *) {
                    static_cast<std::promise<WGPUMapAsyncStatus> *>(data)->set_value(status);
                };
                wgpuBufferMapAsync(staging, WGPUMapMode_Read, 0, descriptor.size, callback);
                wgpuDevicePoll(device, true, nullptr);
                const auto status = future.get();
                QByteArray bytes;
                if (status == WGPUMapAsyncStatus_Success) {
                    bytes = QByteArray(static_cast<const char *>(wgpuBufferGetConstMappedRange(staging, 0, descriptor.size)), descriptor.size);
                    wgpuBufferUnmap(staging);
                }
                wgpuBufferRelease(staging);
                if (status != WGPUMapAsyncStatus_Success) return {};
                found = tiles.emplace(coordinate, bytes).first;
            }
            const int sourceOffset = ((y - coordinate.second * 64) * 64 + x - coordinate.first * 64) * 4;
            const int targetOffset = ((y - bounds.y()) * bounds.width() + x - bounds.x()) * 4;
            std::memcpy(result.data() + targetOffset, found->second.constData() + sourceOffset, 4);
        }
    }
    return result;
}

QByteArray KisGpuTestDevice::read(WGPUTexture texture, QSize size)
{
    const quint32 stride = (size.width() * 4 + 255) / 256 * 256;
    WGPUBufferDescriptor descriptor{};
    descriptor.size = quint64(stride) * size.height();
    descriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    const auto staging = wgpuDeviceCreateBuffer(device, &descriptor);
    const auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
    WGPUTexelCopyTextureInfo source{};
    source.texture = texture;
    source.aspect = WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo destination{};
    destination.buffer = staging;
    destination.layout.bytesPerRow = stride;
    destination.layout.rowsPerImage = size.height();
    const WGPUExtent3D extent{quint32(size.width()), quint32(size.height()), 1};
    wgpuCommandEncoderCopyTextureToBuffer(encoder, &source, &destination, &extent);
    const auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(queue, 1, &commands);
    wgpuCommandBufferRelease(commands);
    wgpuCommandEncoderRelease(encoder);
    std::promise<WGPUMapAsyncStatus> promise;
    auto future = promise.get_future();
    WGPUBufferMapCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = &promise;
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *) {
        static_cast<std::promise<WGPUMapAsyncStatus> *>(data)->set_value(status);
    };
    wgpuBufferMapAsync(staging, WGPUMapMode_Read, 0, descriptor.size, callback);
    wgpuDevicePoll(device, true, nullptr);
    const auto status = future.get();
    QByteArray result;
    if (status == WGPUMapAsyncStatus_Success) {
        const auto mapped = static_cast<const char *>(wgpuBufferGetConstMappedRange(staging, 0, descriptor.size));
        result.resize(size.width() * size.height() * 4);
        for (int y = 0; y < size.height(); ++y) {
            std::memcpy(result.data() + y * size.width() * 4, mapped + y * stride, size.width() * 4);
        }
        wgpuBufferUnmap(staging);
    }
    wgpuBufferRelease(staging);
    return result;
}
