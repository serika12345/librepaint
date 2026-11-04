/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CPU_LAYER_H
#define KIS_GPU_CPU_LAYER_H

#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <kis_types.h>
#include <kritaimage_export.h>
#include <functional>
#include <optional>

class KoColorSpace;

/** Pins the version used by a CPU render. inputLayer requests independent native
 * pixels; zero reads the composition, and no value skips the pixel read.
 */
struct KisGpuCpuLayerContext
{
    LibrePaint::Image::Gpu::PaintImageSP image;
    LibrePaint::Image::Gpu::Snapshot version;
    std::optional<LibrePaint::Image::Gpu::LayerId> inputLayer;
};

/** Runs a native CPU layer renderer and prepares canonical sparse RGBA U8/U16
 * tiles. maximumBytes bounds each input/output transfer. The callback returns
 * an independent native device. Success replaces only the raster pixel fields;
 * the caller publishes the definition and pixels together in one version.
 * A stale context, failed computation or rejected transfer leaves layer intact.
 * An empty context supports format import before attaching a GPU document.
 */
KRITAIMAGE_EXPORT bool kisGpuRenderCpuLayer(LibrePaint::Image::Gpu::LayerFeature feature,
    LibrePaint::Image::Gpu::RasterLayer &layer, QSize size, const KoColorSpace *space,
    qsizetype maximumBytes, const std::function<KisPaintDeviceSP(KisPaintDeviceSP)> &render,
    const KisGpuCpuLayerContext &context = {});

#endif
