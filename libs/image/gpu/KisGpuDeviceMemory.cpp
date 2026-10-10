/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDevice_p.h"

namespace KisGpuTileStorage {
struct MemoryReservationData {
    std::shared_ptr<NativeDevice> owner;
    quint64 bytes = 0;
    ~MemoryReservationData() { owner->reservedBytes.fetch_sub(bytes); }
};
quint64 NativeDevice::availableMemory() const
{
    if (!availability->available.load()) return 0;
    const auto reserved = reservedBytes.load();
    return reserved <= maximumResidentBytes ? maximumResidentBytes - reserved : 0;
}
KisGpuDevice::MemoryReservation NativeDevice::reserveMemory(std::shared_ptr<NativeDevice> owner, quint64 bytes)
{
    KisGpuDevice::MemoryReservation result;
    if (!bytes || !owner->availability->available.load()) return result;
    result.d = std::make_unique<MemoryReservationData>();
    result.d->owner = std::move(owner);
    if (!result.tryResize(bytes)) result.d.reset();
    return result;
}
}

KisGpuDevice::MemoryReservation::MemoryReservation() noexcept = default;
KisGpuDevice::MemoryReservation::~MemoryReservation() = default;
KisGpuDevice::MemoryReservation::MemoryReservation(MemoryReservation &&) noexcept = default;
KisGpuDevice::MemoryReservation &KisGpuDevice::MemoryReservation::operator=(MemoryReservation &&) noexcept = default;
KisGpuDevice::MemoryReservation::operator bool() const { return d && d->bytes; }
quint64 KisGpuDevice::MemoryReservation::bytes() const { return d ? d->bytes : 0; }
bool KisGpuDevice::MemoryReservation::tryResize(quint64 bytes)
{
    if (!d) return false;
    if (bytes <= d->bytes) {
        d->owner->reservedBytes.fetch_sub(d->bytes - bytes);
        d->bytes = bytes;
        return true;
    }
    if (!d->owner->availability->available.load()) return false;
    const auto growth = bytes - d->bytes;
    auto reserved = d->owner->reservedBytes.load();
    do {
        if (reserved > d->owner->maximumResidentBytes || growth > d->owner->maximumResidentBytes - reserved) return false;
    } while (!d->owner->reservedBytes.compare_exchange_weak(reserved, reserved + growth));
    d->bytes = bytes;
    return true;
}
KisGpuDevice::MemoryReservation KisGpuDevice::reserveMemory(quint64 bytes)
{
    return KisGpuTileStorage::NativeDevice::reserveMemory(d, bytes);
}
KisGpuDevice::MemoryStatistics KisGpuDevice::memoryStatistics() const { return {d->maximumResidentBytes, d->reservedBytes.load()}; }
quint64 KisGpuDevice::availableMemory() const { return d->availableMemory(); }
