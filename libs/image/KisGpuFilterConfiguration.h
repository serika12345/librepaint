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
class KisNode;

/** Resolve scalar color/profile inputs once and retain native editable metadata. */
KRITAIMAGE_EXPORT std::optional<LibrePaint::Image::Gpu::FilterDefinition> freezeGpuFilterConfiguration(
    const KisFilterConfigurationSP &configuration, const KoColorSpace *colorSpace);

/** Handles a direct setter on an attached GPU filter node. Returns false for
 * native/detached nodes and view synchronization. Rejection throws before
 * changing node metadata or document state. */
bool writeGpuNodeFilterConfiguration(KisNode *node, const KisFilterConfigurationSP &configuration);
#endif
