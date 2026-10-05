/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KRITA_CANVAS_WGPU_IMAGE_RENDERER_H
#define KRITA_CANVAS_WGPU_IMAGE_RENDERER_H

#include <QImage>
#include <QRect>
#include <QString>
#include <memory>
#include <functional>
#include <array>
#include <webgpu/webgpu.h>

namespace Krita::Canvas
{
/** Owns one GPU device, image texture and optional borrowed native surface.
 * All operations and destruction run on the creating thread. The native
 * surface source must outlive this object. Readback waits for GPU completion;
 * surface acquisition may wait for a display drawable on the calling thread.
 */
class WgpuImageRenderer
{
public:
    /** Optional monotonic CPU stage boundaries, in nanoseconds. */
    struct PresentationTiming {
        qint64 acquireStartNs = 0;
        qint64 acquireEndNs = 0;
        qint64 beforeDrawEndNs = 0;
        qint64 submitEndNs = 0;
        qint64 presentEndNs = 0;
        qint64 gpuCompletionEndNs = 0;
    };
    WgpuImageRenderer();
    ~WgpuImageRenderer();
    WgpuImageRenderer(const WgpuImageRenderer &) = delete;
    WgpuImageRenderer &operator=(const WgpuImageRenderer &) = delete;

    bool initialize(const WGPUChainedStruct *surfaceSource = nullptr);
    bool upload(const QImage &image, const QRect &dirty);
    bool uploadPatch(const QImage &patch, const QRect &destination);
    void setProjectionGeometry(const std::array<float,16> &geometry);
    QImage readback(QSize size = {});
    bool configureSurface(QSize size, WGPUPresentMode mode = WGPUPresentMode_Fifo);
    bool present(PresentationTiming *timing = nullptr, const std::function<bool()> &beforeDraw = {});
    QImage readbackSurface();
    QString error() const;
    WGPUBackendType backend() const;
    quint64 uploadedBytes() const;

private:
    struct Private;
    std::unique_ptr<Private> m_d;
};
}
#endif
