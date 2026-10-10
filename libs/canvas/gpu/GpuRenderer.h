/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LIBREPAINT_GPU_CANVAS_RENDERER_H
#define LIBREPAINT_GPU_CANVAS_RENDERER_H

#include "KisGpuDevice.h"
#include "KisGpuTileStore.h"
#include <QTransform>
#include <memory>

namespace Krita::Canvas {
/**
 * Renders immutable straight sRGB RGBA8 images to RGBA8Unorm/BGRA8Unorm targets.
 * The borrowed device owner outlives this renderer. Calls, polling and destruction
 * share its submitting thread. Destruction waits for this renderer's final submission.
 */
class GpuRenderer
{
    struct CompletionData;
public:
    using Status = KisGpuTileStore::Status;
    enum class Error { None, InvalidImage, ImagePending, InvalidTarget, InvalidTransform, QueueFull, DeviceLost };
    enum class Sampling { Nearest, Linear };
    struct View {
        QTransform canvasToTarget;
        quint32 backgroundRgba = 0xFFFFFFFF;
        Sampling sampling = Sampling::Linear;
    };
    struct Frame {
        Error error = Error::None;
        Status status() const;
        quint64 sequence() const;
    private:
        friend class GpuRenderer;
        std::shared_ptr<CompletionData> d;
    };
    struct Statistics {
        quint64 submissions = 0;
        quint64 parameterUploadBytes = 0;
        quint64 residentParameterBytes = 0;
        quint32 pendingFrames = 0;
    };
    /** Unsupported format, unavailable device or zero pending limit throw std::runtime_error. */
    GpuRenderer(KisGpuDevice &device, WGPUTextureFormat targetFormat, quint32 maximumPending = 3);
    ~GpuRenderer();
    GpuRenderer(const GpuRenderer &) = delete;
    GpuRenderer &operator=(const GpuRenderer &) = delete;
    /**
     * Renders a succeeded image. Canvas coordinates map to physical target pixels.
     * Pins image and target through all completion notifications. The caller owns
     * target and adopts its pixels only after success. Rejection leaves the queue unchanged.
     */
    Frame render(const KisGpuTileStore::TextureSnapshot &image, WGPUTexture target, const View &view);
    void poll();
    Statistics statistics() const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
#endif
