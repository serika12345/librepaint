/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LIBREPAINT_GPU_SURFACE_RENDERER_H
#define LIBREPAINT_GPU_SURFACE_RENDERER_H
#include "GpuRenderer.h"
#include <QSize>
class QWindow;
namespace Krita::Canvas {
/** Owns the native presentation surface of a dedicated QWindow.
 * The window and device outlive this owner. All operations share the GUI/submitting
 * thread. Polling collects rendering callbacks; presenting does not wait for the GPU.
 */
class SurfaceRenderer
{
public:
    enum class Error { None, WindowUnavailable, DeviceLost, ImageRejected, QueueFull,
                       ResizePending, BudgetExceeded, SurfaceUnavailable, PresentFailed };
    struct Frame {
        Error error = Error::None;
        GpuRenderer::Frame rendering;
        bool accepted() const { return error == Error::None && rendering.sequence() != 0; }
    };
    struct Statistics {
        quint64 presentationRequests = 0;
        quint64 reservedSurfaceBytes = 0;
        QSize physicalSize;
        GpuRenderer::Statistics rendering;
    };
    /** Call before the dedicated window acquires a native handle. */
    static void prepareWindow(QWindow &window);
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
