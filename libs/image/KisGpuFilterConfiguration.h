/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_FILTER_CONFIGURATION_H
#define KIS_GPU_FILTER_CONFIGURATION_H
#include <kritaimage_export.h>
#include <kis_types.h>
#include <gpu/KisGpuOperationDocument.h>
#include <optional>
class KoColorSpace;

/** Resolve scalar color/profile inputs once and retain native editable metadata. */
KRITAIMAGE_EXPORT std::optional<LibrePaint::Image::Gpu::FilterDefinition> freezeGpuFilterConfiguration(
    const KisFilterConfigurationSP &configuration, const KoColorSpace *colorSpace);
#endif
