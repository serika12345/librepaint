/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CPU_FILTER_H
#define KIS_GPU_CPU_FILTER_H

#include <kis_types.h>
#include <kritaimage_export.h>

/** Runs an existing CPU filter on the affected GPU paint-layer pixels and
 * publishes the result as one version. Rejections preserve pixels/history.
 */
KRITAIMAGE_EXPORT bool kisGpuApplyCpuFilter(KisImage *image, KisNodeSP layer, KisFilterConfigurationSP configuration);
#endif
