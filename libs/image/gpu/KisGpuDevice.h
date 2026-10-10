/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DEVICE_H
#define KIS_GPU_DEVICE_H

#include <QString>
#include <memory>
#include <webgpu/wgpu.h>

class KisGpuTileStore;
namespace KisGpuTileStorage { struct NativeDevice; }

/**
 * Owns a hardware Metal/Vulkan device and its loss notifications. Creation,
 * submission and destroy() share one thread. Stores retain the native device
 * and share its availability. Destruction invalidates every store before
 * destroying native resources and draining callbacks. Borrowed device handles
 * are used for GPU commands; this owner controls their destruction.
 */
class KisGpuDevice
{
public:
    /** Zero uses default limits; a positive storage limit requests the adapter's other limits. */
    explicit KisGpuDevice(quint64 storageBindingLimit = 0);
    ~KisGpuDevice();
    KisGpuDevice(const KisGpuDevice &) = delete;
    KisGpuDevice &operator=(const KisGpuDevice &) = delete;

    WGPUDevice device() const;
    QString adapterName() const;
    bool available() const;
    int errorCount() const;
    QString lastError() const;
    void destroy();

private:
    friend class KisGpuTileStore;
    std::shared_ptr<KisGpuTileStorage::NativeDevice> d;
};

#endif
