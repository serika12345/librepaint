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
namespace KisGpuTileStorage { struct NativeDevice; struct MemoryReservationData; }

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
    struct MemoryStatistics { quint64 limitBytes, reservedBytes; };
    /** Move-only logical GPU memory reservation. Creation, resize and destruction use the submitting thread.
     * Retain through the resource lifetime; shrinking and destruction return capacity, including after device loss.
     * Covers owned buffers, textures and the estimated presentation images, excluding driver bookkeeping.
     */
    class MemoryReservation {
    public:
        MemoryReservation() noexcept;
        ~MemoryReservation();
        MemoryReservation(MemoryReservation &&) noexcept;
        MemoryReservation &operator=(MemoryReservation &&) noexcept;
        MemoryReservation(const MemoryReservation &) = delete;
        MemoryReservation &operator=(const MemoryReservation &) = delete;
        explicit operator bool() const;
        quint64 bytes() const;
        /** Rejects growth beyond the device limit without changing this reservation. */
        bool tryResize(quint64 bytes);
    private:
        friend struct KisGpuTileStorage::NativeDevice;
        std::unique_ptr<KisGpuTileStorage::MemoryReservationData> d;
    };
    /**
     * Zero uses default limits; a positive storage limit requests the adapter's other limits.
     * Timestamps enable compute profiling; unsupported hardware throws std::runtime_error.
     */
    explicit KisGpuDevice(quint64 storageBindingLimit = 0, bool timestamps = false,
                          quint64 maximumResidentBytes = 512 * 1024 * 1024);
    ~KisGpuDevice();
    KisGpuDevice(const KisGpuDevice &) = delete;
    KisGpuDevice &operator=(const KisGpuDevice &) = delete;

    /** Borrowed handles for native presentation; this owner controls their lifetime. */
    WGPUInstance instance() const;
    WGPUAdapter adapter() const;
    WGPUDevice device() const;
    QString adapterName() const;
    bool available() const;
    int errorCount() const;
    QString lastError() const;
    /** Reserves bytes across all document and canvas owners; empty on zero, loss or insufficient capacity. */
    MemoryReservation reserveMemory(quint64 bytes);
    MemoryStatistics memoryStatistics() const;
    quint64 availableMemory() const;
    void destroy();

private:
    friend class KisGpuTileStore;
    std::shared_ptr<KisGpuTileStorage::NativeDevice> d;
};

#endif
