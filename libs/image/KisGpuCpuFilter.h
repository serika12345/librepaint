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
 * A retained source supplies the original native pixels during dialog previews.
 */
KRITAIMAGE_EXPORT bool kisGpuApplyCpuFilter(KisImage *image, KisNodeSP layer, KisFilterConfigurationSP configuration,
                                          KisPaintDeviceSP source = {});
/** Independent native pixels for a CPU filter's settings widget. */
KRITAIMAGE_EXPORT KisPaintDeviceSP kisGpuFilterConfigurationDevice(KisImage *image, KisNodeSP node);
#endif
