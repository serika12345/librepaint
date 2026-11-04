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
#include <limits>

enum class KisGpuPixelReadSource { Layer, LayerProjection, CanvasProjection, NativeProjection };

/** Validates native pixel dimensions and encoded capacity before allocation. */
KRITAIMAGE_EXPORT qsizetype kisNativePixelByteCount(QRect region, int pixelSize,
    qsizetype maximumBytes = std::numeric_limits<qsizetype>::max());

/** Convert canonical little-endian RGBA and native-endian BGRA pixels. */
KRITAIMAGE_EXPORT QByteArray kisGpuPixelTransfer(QByteArrayView source, LibrePaint::Image::Gpu::PixelFormat format, bool toGpu);

/** Converts consecutive native RGBA U8/U16 rows into canonical upload tiles.
 * Validates padded tile capacity before allocation and publishes only complete
 * input. Native allocation blocks and transparent padding survive format import.
 */
class KRITAIMAGE_EXPORT KisGpuRasterImport
{
public:
    KisGpuRasterImport(QSize size, const KoColorSpace *colorSpace, qsizetype maximumBytes);
    LibrePaint::Image::Gpu::PixelFormat format() const { return m_format; }
    void appendRow(QByteArrayView nativePixels);
    LibrePaint::Image::Gpu::RasterLayer takeLayer();

private:
    QSize m_size;
    LibrePaint::Image::Gpu::PixelFormat m_format;
    QList<LibrePaint::Image::Gpu::RgbaTile> m_tiles;
    int m_nextRow = 0;
};

/** Explicit fixed-version native pixel read. Layer reads retain off-canvas and
 * implicit pixels; CanvasProjection clips to the canvas. Rejection throws
 * before publishing any bytes. The returned buffer owns its native BGRA pixels.
 */
KRITAIMAGE_EXPORT QByteArray kisGpuReadNativePixels(const LibrePaint::Image::Gpu::PaintImageSP &image,
    const LibrePaint::Image::Gpu::Snapshot &version, quint64 layer, QRect region,
    qsizetype maximumBytes, KisGpuPixelReadSource source);

/** Publishes one complete native pixel write through the retained layer view.
 * Rejection preserves pixels, history and the retained reference's version.
 */
KRITAIMAGE_EXPORT LibrePaint::Image::Gpu::Result kisGpuWriteNativePixels(
    const LibrePaint::Image::Gpu::PaintImageSP &image,
    const LibrePaint::Image::Gpu::LayerPixelReferenceSP &pixels, QRect region, QByteArrayView bytes);

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
