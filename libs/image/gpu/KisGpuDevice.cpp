/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDevice.h"
#include "KisGpuDevice_p.h"
#include <algorithm>
#include <cstring>
#include <future>
#include <stdexcept>

namespace {
QString message(WGPUStringView value)
{
    return value.data ? QString::fromUtf8(value.data,
        value.length == WGPU_STRLEN ? qsizetype(std::strlen(value.data)) : qsizetype(value.length)) : QString();
}
}

using namespace KisGpuTileStorage;

NativeDevice::NativeDevice(quint64 storageBindingLimit, bool timestamps, quint64 maximumBytes)
    : maximumResidentBytes(maximumBytes) {
    try {
        if (wgpuGetVersion() != 0x1b000400) throw std::runtime_error("GPU document requires wgpu-native 27.0.4.0");
        if (wgpuLibrePaintRecoveryRevision() != 3) throw std::runtime_error("GPU document requires recovery revision 3");
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
        name = message(info.device);
        wgpuAdapterInfoFreeMembers(info);
        if (adapterType == WGPUAdapterType_CPU) throw std::runtime_error("A hardware GPU is required");
        WGPULimits requestedLimits{};
        if (storageBindingLimit) {
            if (wgpuAdapterGetLimits(adapter, &requestedLimits) != WGPUStatus_Success) {
                throw std::runtime_error("Cannot query GPU limits");
            }
            requestedLimits.maxStorageBufferBindingSize = storageBindingLimit;
        }
        WGPUDeviceDescriptor descriptor{};
        const WGPUFeatureName timingFeature = WGPUFeatureName_TimestampQuery;
        if (timestamps) {
            if (!wgpuAdapterHasFeature(adapter, timingFeature)) throw std::runtime_error("GPU timestamp profiling is unsupported");
            descriptor.requiredFeatureCount = 1;
            descriptor.requiredFeatures = &timingFeature;
        }
        if (storageBindingLimit) descriptor.requiredLimits = &requestedLimits;
        descriptor.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        descriptor.deviceLostCallbackInfo.userdata1 = this;
        descriptor.deviceLostCallbackInfo.callback = [](const WGPUDevice *, WGPUDeviceLostReason,
            WGPUStringView value, void *data, void *) {
            auto &state = *static_cast<NativeDevice *>(data);
            state.availability->available.store(false);
            std::lock_guard<std::mutex> lock(state.diagnostics);
            state.error = message(value);
        };
        descriptor.uncapturedErrorCallbackInfo.userdata1 = this;
        descriptor.uncapturedErrorCallbackInfo.callback = [](const WGPUDevice *, WGPUErrorType,
            WGPUStringView value, void *data, void *) {
            auto &state = *static_cast<NativeDevice *>(data);
            state.errors.fetch_add(1);
            state.availability->available.store(false);
            std::lock_guard<std::mutex> lock(state.diagnostics);
            state.error = message(value);
        };
        std::promise<WGPUDevice> devicePromise;
        auto deviceFuture = devicePromise.get_future();
        WGPURequestDeviceCallbackInfo deviceCallback{};
        deviceCallback.mode = WGPUCallbackMode_AllowSpontaneous;
        deviceCallback.userdata1 = &devicePromise;
        deviceCallback.callback = [](WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView, void *data, void *) {
            static_cast<std::promise<WGPUDevice> *>(data)->set_value(device);
        };
        wgpuAdapterRequestDevice(adapter, &descriptor, deviceCallback);
        device = deviceFuture.get();
        if (!device) throw std::runtime_error("Cannot create GPU device");
        if (timestamps) {
            WGPUQuerySetDescriptor queries{};
            queries.type = WGPUQueryType_Timestamp;
            queries.count = TimestampQueryCount; // wgpu-types 27.0.1 QUERY_SET_MAX_QUERIES
            timestampQueries = wgpuDeviceCreateQuerySet(device, &queries);
            if (!timestampQueries || !availability->available.load()) throw std::runtime_error("Cannot create GPU timestamp storage");
        }
    } catch (...) {
        release();
        throw;
    }
}

void NativeDevice::destroy() {
    if (!device || destroyed) return;
    {
        std::lock_guard<std::mutex> lock(availability->mapping);
        availability->available.store(false);
    }
    destroyed = true;
    wgpuDeviceDestroy(device);
    wgpuDevicePoll(device, true, nullptr);
}
void NativeDevice::release() {
    destroy();
    if (timestampQueries) wgpuQuerySetRelease(timestampQueries);
    if (device) wgpuDeviceRelease(device);
    if (adapter) wgpuAdapterRelease(adapter);
    if (instance) wgpuInstanceRelease(instance);
}
NativeDevice::~NativeDevice() { release(); }

bool NativeDevice::queriesAvailable(quint32 count) const
{
    return timestampQueries && availableQueryStart(count) < TimestampQueryCount;
}
quint32 NativeDevice::availableQueryStart(quint32 count) const
{
    if (!count || count > TimestampQueryCount) return TimestampQueryCount;
    quint32 available = 0;
    for (quint32 i = 0; i < TimestampQueryCount; ++i) {
        available = occupiedQueries[i] ? 0 : available + 1;
        if (available == count) return i + 1 - count;
    }
    return TimestampQueryCount;
}
quint32 NativeDevice::acquireQueries(quint32 count)
{
    const auto first = availableQueryStart(count);
    if (first == TimestampQueryCount) throw std::runtime_error("GPU timestamp slots were not reserved before submission");
    std::fill(occupiedQueries.begin() + first, occupiedQueries.begin() + first + count, true);
    return first;
}
void NativeDevice::releaseQueries(quint32 first, quint32 count)
{
    std::fill(occupiedQueries.begin() + first, occupiedQueries.begin() + first + count, false);
}

KisGpuDevice::KisGpuDevice(quint64 storageBindingLimit, bool timestamps, quint64 maximumResidentBytes)
    : d(std::make_shared<NativeDevice>(storageBindingLimit, timestamps, maximumResidentBytes)) {}
KisGpuDevice::~KisGpuDevice() { d->destroy(); }
WGPUInstance KisGpuDevice::instance() const { return d->instance; }
WGPUAdapter KisGpuDevice::adapter() const { return d->adapter; }
WGPUDevice KisGpuDevice::device() const { return d->device; }
QString KisGpuDevice::adapterName() const { return d->name; }
bool KisGpuDevice::available() const { return d->availability->available.load(); }
int KisGpuDevice::errorCount() const { return d->errors.load(); }
QString KisGpuDevice::lastError() const { std::lock_guard<std::mutex> lock(d->diagnostics); return d->error; }
void KisGpuDevice::destroy() { d->destroy(); }
