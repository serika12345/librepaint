/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DEVICE_P_H
#define KIS_GPU_DEVICE_P_H
#include <QString>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <webgpu/wgpu.h>

namespace KisGpuTileStorage {
struct Availability {
    std::mutex mapping;
    std::atomic<bool> available{true};
};

struct NativeDevice {
    static constexpr quint32 TimestampQueryCount = 4096;
    // Metal records timestamped passes into separate native command buffers.
    // Bound one unsubmitted recording below the native queue capacity.
    static constexpr quint32 MaximumTimedPasses = 32;
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUQuerySet timestampQueries = nullptr;
    std::array<bool, TimestampQueryCount> occupiedQueries{};
    QString name, error;
    std::mutex diagnostics;
    std::atomic<int> errors{0};
    std::shared_ptr<Availability> availability = std::make_shared<Availability>();
    bool destroyed = false;
    explicit NativeDevice(quint64 storageBindingLimit, bool timestamps);
    void destroy();
    void release();
    bool queriesAvailable(quint32 count) const;
    quint32 availableQueryStart(quint32 count) const;
    quint32 acquireQueries(quint32 count);
    void releaseQueries(quint32 first, quint32 count);
    ~NativeDevice();
};
}
#endif
