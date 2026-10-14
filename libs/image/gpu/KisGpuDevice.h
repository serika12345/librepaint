/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_DEVICE_H
#define KIS_GPU_DEVICE_H

#include <kritagpuimage_export.h>
#include <QString>
#include <memory>

class QThread;
struct WGPUInstanceImpl;
struct WGPUAdapterImpl;
struct WGPUDeviceImpl;
struct WGPUQueueImpl;
struct WGPUBufferImpl;
struct WGPUCommandEncoderImpl;
struct WGPUSurfaceImpl;
struct WGPUTextureViewImpl;
using WGPUInstance = WGPUInstanceImpl *;
using WGPUAdapter = WGPUAdapterImpl *;
using WGPUDevice = WGPUDeviceImpl *;
using WGPUQueue = WGPUQueueImpl *;
using WGPUBuffer = WGPUBufferImpl *;
using WGPUCommandEncoder = WGPUCommandEncoderImpl *;
using WGPUSurface = WGPUSurfaceImpl *;
using WGPUTextureView = WGPUTextureViewImpl *;

namespace LibrePaint::Image::Gpu {
namespace Detail { struct ReadbackBatch; }
class Device;
using DevicePtr = std::shared_ptr<Device>;

/**
 * Owns a native GPU and its platform connection on the GUI thread.
 * Metal serves Apple platforms; Vulkan serves the other supported platforms.
 * GPU versions and display resources share this owner. Hold a DevicePtr through
 * frame submission and release the last reference on the owning thread, before
 * QGuiApplication exits. A retained version can keep a retired device alive.
 */
class KRITAGPUIMAGE_EXPORT Device
{
public:
    static DevicePtr createNative(QString &diagnostic);
    ~Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    /** Native handles are borrowed through this owner on its application thread. */
    WGPUInstance instance() const;
    WGPUAdapter adapter() const;
    WGPUDevice nativeDevice() const;
    WGPUQueue queue() const;
    QThread *thread() const;
    bool isLost() const;
    QString diagnostic() const;
    /** One command encoder belongs to the device until submission. */
    WGPUCommandEncoder beginCommands();
    bool ownsCommands(WGPUCommandEncoder commands) const;
    bool isRecordingFrame() const;
    bool submitCommands(bool wait = false);
    /** Waits at explicit external read and retirement boundaries. */
    bool finish();

private:
    friend class Execution;
    friend struct Detail::ReadbackBatch;
    struct Private;
    explicit Device(std::unique_ptr<Private> data);
    bool readbackPending() const;
    void releaseCompletedReadback();
    std::unique_ptr<Private> m_d;
};
}
#endif
