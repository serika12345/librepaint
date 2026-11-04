/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_DEVICE_TRANSFER_P_H
#define KIS_GPU_PAINT_DEVICE_TRANSFER_P_H

#include "kis_types.h"
class KoColorSpace;
#include "kis_default_bounds_base.h"
#include "gpu/KisGpuPaintImage.h"
#include <kritaimage_export.h>

/** Convert canonical little-endian RGBA and native-endian BGRA pixels. */
KRITAIMAGE_EXPORT QByteArray kisGpuPixelTransfer(QByteArrayView source, LibrePaint::Image::Gpu::PixelFormat format, bool toGpu);

/** Copies full-resolution CPU RGBA U8/U16 pixels into canonical sparse tiles.
 * Preserves native allocation areas and the default pixel, including outside
 * the canvas. Only the layer's pixel fields change; rejection leaves it intact.
 */
KRITAIMAGE_EXPORT bool kisGpuRasterLayerFromPaintDevice(KisPaintDeviceSP source,
    LibrePaint::Image::Gpu::PixelFormat format, qsizetype maximumBytes,
    LibrePaint::Image::Gpu::RasterLayer &layer);

/** Temporary native pixels for synchronous CPU image-returning APIs.
 * The returned buffer owns its pixels; the GPU image retains editing ownership.
 */
KRITAIMAGE_EXPORT KisPaintDeviceSP kisGpuPaintDeviceTransfer(const LibrePaint::Image::Gpu::PaintImageSP &image,
                                         quint64 layer, KisDefaultBoundsBaseSP bounds,
                                         const LibrePaint::Image::Gpu::Snapshot &version,
                                         const KoColorSpace *colorSpace,
                                         qsizetype maximumBytes = 256 * 1024 * 1024);
#endif
