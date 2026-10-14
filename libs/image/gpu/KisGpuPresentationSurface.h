/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PRESENTATION_SURFACE_H
#define KIS_GPU_PRESENTATION_SURFACE_H

#include "KisGpuExecution.h"
struct WGPUSurfaceDescriptor;

namespace LibrePaint::Image::Gpu {
/** Owns one native window surface and its acquired texture on the device thread.
 * The native window outlives this owner. Acquisition lasts until present/discard;
 * Execution borrows this owner through submission or cancellation.
 */
class KRITAIMAGE_EXPORT PresentationSurface
{
public:
    PresentationSurface(DevicePtr device, const WGPUSurfaceDescriptor &descriptor);
    ~PresentationSurface();
    PresentationSurface(const PresentationSurface &) = delete;
    PresentationSurface &operator=(const PresentationSurface &) = delete;
    DevicePtr device() const;
    bool configure(QSize size);
    QSize size() const;
    quint32 format() const;
    ExecutionOutcome acquire();
    WGPUTextureView target() const;
    bool present();
    void discard();
private:
    struct Private;
    std::unique_ptr<Private> m_d;
};
}
#endif
