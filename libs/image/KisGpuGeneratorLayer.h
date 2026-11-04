/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_GENERATOR_LAYER_H
#define KIS_GPU_GENERATOR_LAYER_H

#include <gpu/KisGpuOperationDocument.h>
#include <kis_types.h>
#include <kritaimage_export.h>
#include "KisGpuCpuLayer.h"

class KoColorSpace;
class KisGeneratorLayer;

KRITAIMAGE_EXPORT KisFilterConfigurationSP gpuGeneratorConfiguration(
    const LibrePaint::Image::Gpu::CpuLayerDefinition &definition);
/** Runs the native generator on CPU and supplies canonical raster tiles. */
KRITAIMAGE_EXPORT bool renderGpuGeneratorLayer(LibrePaint::Image::Gpu::RasterLayer &layer,
    QSize size, const KoColorSpace *space, qsizetype maximumBytes,
    const KisGpuCpuLayerContext &context = {});
KRITAIMAGE_EXPORT bool writeGpuGeneratorConfiguration(KisGeneratorLayer *layer,
    const KisFilterConfigurationSP &configuration);

#endif
