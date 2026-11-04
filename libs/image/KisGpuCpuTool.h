/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CPU_TOOL_H
#define KIS_GPU_CPU_TOOL_H

#include <kritaimage_export.h>
#include <kis_types.h>
#include <QRect>
#include <functional>

namespace LibrePaint::Image::Gpu { enum class HistoryAction; }

/** Run a synchronous native tool on independent paint-layer or Alpha8 pixels.
 * The callback merges the supplied selection while drawing inside area.
 * Only that rectangle is transferred; selection geometry is retained for tools
 * that derive their result from its outline. Success publishes one named GPU
 * history entry. A false callback or rejected transfer preserves the document.
 */
KRITAIMAGE_EXPORT bool kisGpuApplyCpuTool(KisImageSP image, KisNodeSP node, QRect area,
    LibrePaint::Image::Gpu::HistoryAction action,
    const std::function<bool(KisPaintDeviceSP, KisSelectionSP)> &process);

#endif
