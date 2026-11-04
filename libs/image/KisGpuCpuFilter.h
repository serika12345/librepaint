/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CPU_FILTER_H
#define KIS_GPU_CPU_FILTER_H

#include <kis_types.h>
#include <kritaimage_export.h>

/** Runs an existing CPU filter on affected GPU paint-layer or Alpha8 pixels and
 * publishes the result as one version. Rejections preserve pixels/history.
 */
KRITAIMAGE_EXPORT bool kisGpuApplyCpuFilter(KisImage *image, KisNodeSP layer, KisFilterConfigurationSP configuration);
/** Independent native pixels for a CPU filter's settings widget. */
KRITAIMAGE_EXPORT KisPaintDeviceSP kisGpuFilterConfigurationDevice(KisImage *image, KisNodeSP node);
#endif
