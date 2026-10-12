/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuDevice.h"
#include "KisGpuReadbackBatch_p.h"
#include <QGuiApplication>
#include <QThread>
#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

namespace Krita::Image::Gpu {
struct Device::Private
{
#if QT_CONFIG(vulkan)
    QVulkanInstance vulkan;
#endif
    // Results remain valid through native cleanup, including failed submissions.
    std::shared_ptr<Detail::ReadbackBatch> readback;
    std::unique_ptr<QRhi> rhi;
};

Device::Device(std::unique_ptr<Private> data) : m_d(std::move(data)) {}
Device::~Device()
{
    Q_ASSERT(m_d->rhi->thread() == QThread::currentThread());
    Q_ASSERT(!m_d->rhi->isRecordingFrame());
}
QRhi &Device::rhi() const { return *m_d->rhi; }
bool Device::readbackPending() const { return m_d->readback && m_d->readback->remaining > 0; }
void Device::releaseCompletedReadback()
{
    if (!readbackPending()) m_d->readback.reset();
}

std::shared_ptr<Detail::ReadbackBatch> Detail::ReadbackBatch::create(Device &device, qsizetype count)
{
    if (device.rhi().thread() != QThread::currentThread() || count <= 0 || device.readbackPending()) return {};
    auto batch = std::make_shared<ReadbackBatch>();
    batch->remaining = count;
    batch->results.reserve(count);
    for (qsizetype i = 0; i < count; ++i) {
        auto result = std::make_unique<QRhiReadbackResult>();
        result->completed = [owner = std::weak_ptr<ReadbackBatch>(batch)] {
            if (const auto batch = owner.lock()) --batch->remaining;
        };
        batch->results.push_back(std::move(result));
    }
    device.m_d->readback = batch;
    return batch;
}

QVulkanInstance *Device::vulkanInstance() const
{
#if QT_CONFIG(vulkan)
    return m_d->rhi->backend() == QRhi::Vulkan ? &m_d->vulkan : nullptr;
#else
    return nullptr;
#endif
}

DevicePtr Device::createNative(QString &diagnostic)
{
    diagnostic.clear();
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance()) ||
        QCoreApplication::instance()->thread() != QThread::currentThread()) {
        diagnostic = QStringLiteral("Native GPU creation requires the GUI application thread");
        return {};
    }
    auto data = std::make_unique<Private>();
    constexpr auto flags = QRhi::EnableDebugMarkers | QRhi::EnableTimestamps;
#if QT_CONFIG(metal)
    QRhiMetalInitParams params;
    data->rhi.reset(QRhi::create(QRhi::Metal, &params, flags));
#elif QT_CONFIG(vulkan)
    data->vulkan.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
    if (!data->vulkan.create()) {
        diagnostic = QStringLiteral("The platform Vulkan connection could not be created");
        return {};
    }
    QRhiVulkanInitParams params;
    params.inst = &data->vulkan;
    data->rhi.reset(QRhi::create(QRhi::Vulkan, &params, flags));
#else
    diagnostic = QStringLiteral("A native Metal or Vulkan backend is required");
    return {};
#endif
    if (!data->rhi || !data->rhi->isFeatureSupported(QRhi::Compute)) {
        diagnostic = QStringLiteral("The native GPU requires compute and storage buffers");
        return {};
    }
    return DevicePtr(new Device(std::move(data)));
}
}
