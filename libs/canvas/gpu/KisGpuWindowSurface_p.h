/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_WINDOW_SURFACE_P_H
#define KIS_GPU_WINDOW_SURFACE_P_H
#include <gpu/KisGpuPresentationSurface.h>
#include <QWindow>
namespace LibrePaint::Canvas::Gpu::Detail {
void setNativeSurfaceType(QWindow &window);
std::unique_ptr<Image::Gpu::PresentationSurface> createWindowSurface(Image::Gpu::DevicePtr device, QWindow &window);
}
#endif
