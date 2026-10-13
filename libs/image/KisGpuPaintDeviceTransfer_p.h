/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_DEVICE_TRANSFER_P_H
#define KIS_GPU_PAINT_DEVICE_TRANSFER_P_H

#include "kis_types.h"
#include "kis_default_bounds_base.h"
#include "gpu/KisGpuPaintImage.h"

/** Temporary native pixels for synchronous CPU image-returning APIs.
 * The returned buffer owns its pixels; the GPU image retains editing ownership.
 */
KisPaintDeviceSP kisGpuPaintDeviceTransfer(const LibrePaint::Image::Gpu::PaintImageSP &image,
                                         quint64 layer, KisDefaultBoundsBaseSP bounds);
#endif
