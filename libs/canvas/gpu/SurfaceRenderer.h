/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LIBREPAINT_GPU_SURFACE_RENDERER_H
#define LIBREPAINT_GPU_SURFACE_RENDERER_H
#include "GpuRenderer.h"
#include <QSize>
#include <optional>
class QWindow;
namespace Krita::Canvas {
struct GpuPresentationData;
/** Owns the native presentation surface of a dedicated QWindow.
 * The window and device outlive this owner. All operations share the GUI/submitting
 * thread. Polling collects rendering and presentation callbacks; presenting does not wait for the GPU.
 */
class SurfaceRenderer
{
public:
    enum class Error { None, WindowUnavailable, DeviceLost, ImageRejected, QueueFull,
                       ResizePending, BudgetExceeded, SurfaceUnavailable, PresentFailed };
    class Presentation {
    public:
        enum class Status { Unavailable, Pending, Presented, Skipped, Abandoned };
        Presentation() = default;
        Status status() const;
        /** Actual display time on the presentation clock; absent for every other status. */
        std::optional<quint64> hostTimeNanoseconds() const;
    private:
        friend class SurfaceRenderer;
        friend class GpuWindowSurface;
        explicit Presentation(std::shared_ptr<GpuPresentationData> state) : d(std::move(state)) {}
        void abandon();
        std::shared_ptr<GpuPresentationData> d;
    };
    struct Frame {
        Error error = Error::None;
        GpuRenderer::Frame rendering;
        Presentation presentation;
        bool accepted() const { return error == Error::None && rendering.sequence() != 0; }
    };
    struct Statistics {
        quint64 presentationRequests = 0;
        quint64 reservedSurfaceBytes = 0;
        QSize physicalSize;
        GpuRenderer::Statistics rendering;
        quint64 displayedFrames = 0, skippedFrames = 0, abandonedFrames = 0;
        quint32 pendingPresentations = 0;
    };
    /** Call before the dedicated window acquires a native handle. */
    static void prepareWindow(QWindow &window);
    /** Use this clock for input timestamps compared with Presentation::hostTimeNanoseconds().
     * Unavailable when the platform cannot measure actual presentation time.
     */
    static std::optional<quint64> presentationClockNanoseconds();
    /** Unavailable platform/window/device, format or zero limits throw std::runtime_error.
     * The pending limit is one or two. The budget reserves three RGBA8 presentation images, including the displayed image.
     */
    SurfaceRenderer(KisGpuDevice &device, QWindow &window,
                    quint64 surfaceBudget = 64 * 1024 * 1024, quint32 maximumPending = 2);
    ~SurfaceRenderer();
    SurfaceRenderer(const SurfaceRenderer &) = delete;
    SurfaceRenderer &operator=(const SurfaceRenderer &) = delete;
    Frame present(const KisGpuTileStore::TextureSnapshot &image, const GpuRenderer::View &view);
    void poll();
    Statistics statistics() const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
#endif
