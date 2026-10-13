/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_DEVICE_H
#define KIS_GPU_DEVICE_H

#include <kritaimage_export.h>
#include <QString>
#include <memory>

class QRhi;
class QVulkanInstance;

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
class KRITAIMAGE_EXPORT Device
{
public:
    static DevicePtr createNative(QString &diagnostic);
    ~Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    /** Borrowed native device; all frame operations use its owning thread. */
    QRhi &rhi() const;
    /** Borrowed platform connection for a Vulkan QWindow; null on Metal. */
    QVulkanInstance *vulkanInstance() const;

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
