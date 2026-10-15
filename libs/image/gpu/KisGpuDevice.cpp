/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDevice.h"
#include "KisGpuReadbackBatch_p.h"
#include <QGuiApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <cstring>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
QString messageText(WGPUStringView message)
{
    if (!message.data) return {};
    const auto size = message.length == WGPU_STRLEN ? std::strlen(message.data) : message.length;
    return QString::fromUtf8(message.data, qsizetype(size));
}
}

struct Device::Private
{
    QThread *thread = QThread::currentThread();
    mutable QMutex mutex;
    QString error;
    std::atomic<bool> lost{false}, failed{false};
    // Callback addresses survive native device and instance cleanup.
    std::shared_ptr<ReadbackBatch> readback;
    NativeInstance instance;
    NativeAdapter adapter;
    NativeDevice device;
    NativeQueue queue;
    NativeCommands commands;

    void fail(WGPUStringView message) noexcept
    {
        failed.store(true, std::memory_order_release);
        try {
            QMutexLocker guard(&mutex);
            if (error.isEmpty()) error = messageText(message);
        } catch (...) {}
    }
};

Device::Device(std::unique_ptr<Private> data) : m_d(std::move(data)) {}
Device::~Device()
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(!isRecordingFrame());
    if (readbackPending()) finish();
    wgpuDeviceDestroy(m_d->device.get());
    wgpuDevicePoll(m_d->device.get(), true, nullptr);
}
WGPUInstance Device::instance() const { return m_d->instance.get(); }
WGPUAdapter Device::adapter() const { return m_d->adapter.get(); }
WGPUDevice Device::nativeDevice() const { return m_d->device.get(); }
WGPUQueue Device::queue() const { return m_d->queue.get(); }
QThread *Device::thread() const { return m_d->thread; }
bool Device::isLost() const { return m_d->lost.load(std::memory_order_acquire); }
bool Device::isRecordingFrame() const { return bool(m_d->commands); }
bool Device::ownsCommands(WGPUCommandEncoder commands) const { return commands && commands == m_d->commands.get(); }
QString Device::diagnostic() const
{
    QMutexLocker guard(&m_d->mutex);
    if (!m_d->error.isEmpty()) return m_d->error;
    return m_d->failed.load(std::memory_order_acquire) ? QStringLiteral("The native GPU reported an error") : QString{};
}
bool Device::readbackPending() const
{ return m_d->readback && m_d->readback->remaining.load(std::memory_order_acquire) > 0; }
void Device::releaseCompletedReadback()
{ if (!readbackPending()) m_d->readback.reset(); }

WGPUCommandEncoder Device::beginCommands()
{
    if (thread() != QThread::currentThread() || isRecordingFrame() || isLost() || !diagnostic().isEmpty()) return nullptr;
    wgpuDevicePoll(nativeDevice(), false, nullptr);
    if (readbackPending()) return nullptr;
    releaseCompletedReadback();
    m_d->commands.reset(wgpuDeviceCreateCommandEncoder(nativeDevice(), nullptr));
    return m_d->commands.get();
}
bool Device::submitCommands(bool wait)
{
    if (thread() != QThread::currentThread() || !isRecordingFrame()) return false;
    auto commands = std::move(m_d->commands);
    NativeSubmission submission(wgpuCommandEncoderFinish(commands.get(), nullptr));
    if (!submission) return false;
    const auto buffer = submission.get();
    wgpuQueueSubmit(queue(), 1, &buffer);
    if (readbackPending()) m_d->readback->start(*this);
    if (wait) return finish();
    wgpuDevicePoll(nativeDevice(), false, nullptr);
    return !isLost() && diagnostic().isEmpty();
}
bool Device::finish()
{
    if (thread() != QThread::currentThread() || isRecordingFrame()) return false;
    wgpuDevicePoll(nativeDevice(), true, nullptr);
    return !isLost() && diagnostic().isEmpty() && !readbackPending();
}

DevicePtr Device::createNative(QString &diagnostic)
{ return createNative(diagnostic, false); }

DevicePtr Device::createNative(QString &diagnostic, bool enableTimestampQueries)
{
    diagnostic.clear();
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance()) ||
        QCoreApplication::instance()->thread() != QThread::currentThread()) {
        diagnostic = QStringLiteral("Native GPU creation requires the GUI application thread");
        return {};
    }
    if ((wgpuGetVersion() >> 24) != 27) {
        diagnostic = QStringLiteral("The native GPU library must match the pinned wgpu-native 27 C API");
        return {};
    }
    auto data = std::make_unique<Private>();
    WGPUInstanceExtras extras{};
    extras.chain.sType = WGPUSType(WGPUSType_InstanceExtras);
#ifdef Q_OS_DARWIN
    extras.backends = WGPUInstanceBackend_Metal;
#else
    extras.backends = WGPUInstanceBackend_Vulkan;
#endif
    WGPUInstanceDescriptor instanceDescriptor{};
    instanceDescriptor.nextInChain = &extras.chain;
    data->instance.reset(wgpuCreateInstance(&instanceDescriptor));
    if (!data->instance) {
        diagnostic = QStringLiteral("The native GPU instance could not be created");
        return {};
    }
    WGPURequestAdapterOptions options{};
#ifdef Q_OS_DARWIN
    options.backendType = WGPUBackendType_Metal;
#else
    options.backendType = WGPUBackendType_Vulkan;
#endif
    options.powerPreference = WGPUPowerPreference_HighPerformance;
    WGPURequestAdapterCallbackInfo adapterCallback{};
    adapterCallback.mode = WGPUCallbackMode_AllowSpontaneous;
    adapterCallback.userdata1 = data.get();
    adapterCallback.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *context, void *) noexcept {
        auto *data = static_cast<Private *>(context);
        if (status == WGPURequestAdapterStatus_Success) data->adapter.reset(adapter);
        else data->fail(message);
    };
    wgpuInstanceRequestAdapter(data->instance.get(), &options, adapterCallback);
    if (!data->adapter) {
        diagnostic = data->error.isEmpty() ? QStringLiteral("The required native GPU adapter is unavailable") : data->error;
        return {};
    }
    const WGPUFeatureName timestampFeature = WGPUFeatureName_TimestampQuery;
    if (enableTimestampQueries && !wgpuAdapterHasFeature(data->adapter.get(), timestampFeature)) {
        diagnostic = QStringLiteral("The native GPU does not support diagnostic timestamp queries");
        return {};
    }
    WGPUDeviceDescriptor descriptor{};
    if (enableTimestampQueries) {
        descriptor.requiredFeatureCount = 1;
        descriptor.requiredFeatures = &timestampFeature;
    }
    descriptor.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    descriptor.deviceLostCallbackInfo.userdata1 = data.get();
    descriptor.deviceLostCallbackInfo.callback = [](WGPUDevice const *, WGPUDeviceLostReason reason, WGPUStringView message, void *context, void *) noexcept {
        if (reason == WGPUDeviceLostReason_Destroyed || reason == WGPUDeviceLostReason_InstanceDropped) return;
        auto *data = static_cast<Private *>(context);
        data->lost.store(true, std::memory_order_release);
        data->fail(message);
    };
    descriptor.uncapturedErrorCallbackInfo.userdata1 = data.get();
    descriptor.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const *, WGPUErrorType, WGPUStringView message, void *context, void *) noexcept {
        static_cast<Private *>(context)->fail(message);
    };
    WGPURequestDeviceCallbackInfo deviceCallback{};
    deviceCallback.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceCallback.userdata1 = data.get();
    deviceCallback.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *context, void *) noexcept {
        auto *data = static_cast<Private *>(context);
        if (status == WGPURequestDeviceStatus_Success) data->device.reset(device);
        else data->fail(message);
    };
    wgpuAdapterRequestDevice(data->adapter.get(), &descriptor, deviceCallback);
    if (!data->device) {
        diagnostic = data->error.isEmpty() ? QStringLiteral("The native GPU device could not be created") : data->error;
        return {};
    }
    data->queue.reset(wgpuDeviceGetQueue(data->device.get()));
    if (!data->queue) {
        diagnostic = QStringLiteral("The native GPU queue could not be created");
        return {};
    }
    return DevicePtr(new Device(std::move(data)));
}

std::shared_ptr<ReadbackBatch> ReadbackBatch::create(Device &device, qsizetype count)
{
    if (device.thread() != QThread::currentThread() || count <= 0 || device.readbackPending()) return {};
    auto batch = std::make_shared<ReadbackBatch>();
    batch->results.reserve(size_t(count));
    for (qsizetype i = 0; i < count; ++i) {
        auto result = std::make_unique<Result>();
        result->owner = batch.get();
        batch->results.push_back(std::move(result));
    }
    batch->remaining.store(count, std::memory_order_relaxed);
    device.m_d->readback = batch;
    return batch;
}
bool ReadbackBatch::recordBuffer(Device &device, WGPUCommandEncoder commands, qsizetype index, WGPUBuffer source, quint64 size)
{
    if (!device.ownsCommands(commands) || !source || index < 0 || size_t(index) >= results.size() || !size || size % 4) return false;
    auto &result = *results[size_t(index)];
    WGPUBufferDescriptor descriptor{};
    descriptor.size = size;
    descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    result.buffer.reset(wgpuDeviceCreateBuffer(device.nativeDevice(), &descriptor));
    if (!result.buffer) return false;
    result.size = size;
    wgpuCommandEncoderCopyBufferToBuffer(commands, source, 0, result.buffer.get(), 0, size);
    return true;
}
void ReadbackBatch::start(Device &)
{
    for (const auto &result : results) {
        if (!result->buffer) {
            remaining.fetch_sub(1, std::memory_order_release);
            continue;
        }
        WGPUBufferMapCallbackInfo callback{};
        callback.mode = WGPUCallbackMode_AllowSpontaneous;
        callback.userdata1 = result.get();
        callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, void *context, void *) noexcept {
            auto *result = static_cast<Result *>(context);
            try {
                if (status == WGPUMapAsyncStatus_Success) {
                    const auto bytes = wgpuBufferGetConstMappedRange(result->buffer.get(), 0, result->size);
                    if (bytes) result->data = QByteArray(static_cast<const char *>(bytes), qsizetype(result->size));
                    wgpuBufferUnmap(result->buffer.get());
                } else result->diagnostic = messageText(message);
                if (result->completed) result->completed();
            } catch (...) {}
            result->owner->remaining.fetch_sub(1, std::memory_order_release);
        };
        wgpuBufferMapAsync(result->buffer.get(), WGPUMapMode_Read, 0, result->size, callback);
    }
}
}
