/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LIBREPAINT_GPU_WINDOW_SURFACE_P_H
#define LIBREPAINT_GPU_WINDOW_SURFACE_P_H
#include "KisGpuDevice.h"
#include <QSize>
#include <memory>
class QWindow;
namespace Krita::Canvas {
/** Native presentation resources; the dedicated window and device remain borrowed. */
class GpuWindowSurface
{
public:
    GpuWindowSurface(KisGpuDevice &device, QWindow &window);
    ~GpuWindowSurface();
    WGPUSurface surface() const;
    bool resize(QSize physicalSize);
private:
    struct Private;
    std::unique_ptr<Private> d;
};
}
#endif
