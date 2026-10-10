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
int tileCoordinate(int pixel) { return pixel >= 0 ? pixel / 64 : (pixel + 1) / 64 - 1; }
}

KisGpuTestDevice::KisGpuTestDevice()
{
    try {
        WGPUInstanceExtras extras{};
        extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
#ifdef __APPLE__
        extras.backends = WGPUInstanceBackend_Metal;
#else
        extras.backends = WGPUInstanceBackend_Vulkan;
#endif
        extras.flags = WGPUInstanceFlag_Validation;
        WGPUInstanceDescriptor instanceDescriptor{};
        instanceDescriptor.nextInChain = &extras.chain;
        instance = wgpuCreateInstance(&instanceDescriptor);
        if (!instance) throw std::runtime_error("Cannot create GPU instance");

        std::promise<WGPUAdapter> adapterPromise;
        auto adapterFuture = adapterPromise.get_future();
        WGPURequestAdapterCallbackInfo adapterCallback{};
        adapterCallback.mode = WGPUCallbackMode_AllowSpontaneous;
        adapterCallback.userdata1 = &adapterPromise;
        adapterCallback.callback = [](WGPURequestAdapterStatus, WGPUAdapter adapter, WGPUStringView, void *data, void *) {
            static_cast<std::promise<WGPUAdapter> *>(data)->set_value(adapter);
        };
        WGPURequestAdapterOptions options{};
        wgpuInstanceRequestAdapter(instance, &options, adapterCallback);
        adapter = adapterFuture.get();
        if (!adapter) throw std::runtime_error("A hardware Metal/Vulkan adapter is required");
        WGPUAdapterInfo info{};
        wgpuAdapterGetInfo(adapter, &info);
        const auto adapterType = info.adapterType;
        name = QString::fromUtf8(info.device.data, qsizetype(info.device.length));
        wgpuAdapterInfoFreeMembers(info);
        if (adapterType == WGPUAdapterType_CPU) throw std::runtime_error("A hardware GPU is required");

        std::promise<WGPUDevice> devicePromise;
        auto deviceFuture = devicePromise.get_future();
        WGPURequestDeviceCallbackInfo deviceCallback{};
        deviceCallback.mode = WGPUCallbackMode_AllowSpontaneous;
        deviceCallback.userdata1 = &devicePromise;
        deviceCallback.callback = [](WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView, void *data, void *) {
            static_cast<std::promise<WGPUDevice> *>(data)->set_value(device);
        };
        WGPUDeviceDescriptor descriptor{};
        descriptor.uncapturedErrorCallbackInfo.userdata1 = &errors;
        descriptor.uncapturedErrorCallbackInfo.callback = [](const WGPUDevice *, WGPUErrorType, WGPUStringView message, void *data, void *) {
            static_cast<std::atomic<int> *>(data)->fetch_add(1);
            qWarning() << "GPU validation:" << QByteArray(message.data, qsizetype(message.length));
        };
        wgpuAdapterRequestDevice(adapter, &descriptor, deviceCallback);
        device = deviceFuture.get();
        if (!device) throw std::runtime_error("Cannot create GPU device");
        queue = wgpuDeviceGetQueue(device);
        if (!queue) throw std::runtime_error("Cannot get GPU queue");
    } catch (...) {
        release();
        throw;
    }
}

KisGpuTestDevice::~KisGpuTestDevice() { release(); }

void KisGpuTestDevice::release()
{
    if (device) wgpuDevicePoll(device, true, nullptr);
    if (queue) wgpuQueueRelease(queue);
    if (device) wgpuDeviceRelease(device);
    if (adapter) wgpuAdapterRelease(adapter);
    if (instance) wgpuInstanceRelease(instance);
}

QByteArray KisGpuTestDevice::read(const KisGpuTileStore::Version &version, QRect bounds)
{
    std::map<std::pair<int, int>, QByteArray> tiles;
    QByteArray result(bounds.width() * bounds.height() * 4, '\0');
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const auto coordinate = std::make_pair(tileCoordinate(x), tileCoordinate(y));
            const WGPUBuffer source = version.tile(QPoint(coordinate.first, coordinate.second));
            if (!source) continue;
            auto found = tiles.find(coordinate);
            if (found == tiles.end()) {
                WGPUBufferDescriptor descriptor{};
                descriptor.size = KisGpuTileStore::TileBytes;
                descriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
                const auto staging = wgpuDeviceCreateBuffer(device, &descriptor);
                const auto encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
                wgpuCommandEncoderCopyBufferToBuffer(encoder, source, 0, staging, 0, descriptor.size);
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

