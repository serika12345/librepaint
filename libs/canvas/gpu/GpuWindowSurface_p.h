/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LIBREPAINT_GPU_WINDOW_SURFACE_P_H
#define LIBREPAINT_GPU_WINDOW_SURFACE_P_H
#include "KisGpuDevice.h"
#include "SurfaceRenderer.h"
#include <QSize>
#include <memory>
#include <atomic>
class QWindow;
namespace Krita::Canvas {
/** Native callbacks own only this value state; publication follows the timestamp write. */
struct GpuPresentationData {
    using Status = SurfaceRenderer::Presentation::Status;
    std::atomic<Status> status{Status::Pending};
    std::atomic<quint64> hostTime{0};
    void abandon() {
        auto expected = Status::Pending;
        status.compare_exchange_strong(expected, Status::Abandoned, std::memory_order_release, std::memory_order_relaxed);
    }
    void finish(quint64 nanoseconds) {
        hostTime.store(nanoseconds, std::memory_order_relaxed);
        auto expected = Status::Pending;
        status.compare_exchange_strong(expected, nanoseconds ? Status::Presented : Status::Skipped,
                                       std::memory_order_release, std::memory_order_relaxed);
    }
};
/** Native presentation resources; the dedicated window and device remain borrowed. */
class GpuWindowSurface
{
public:
    GpuWindowSurface(KisGpuDevice &device, QWindow &window);
    ~GpuWindowSurface();
    WGPUSurface surface() const;
    bool resize(QSize physicalSize);
    SurfaceRenderer::Presentation presentation() const;
    static std::optional<quint64> presentationClockNanoseconds();
private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
#endif
