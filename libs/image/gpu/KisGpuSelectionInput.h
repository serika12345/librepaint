/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_SELECTION_INPUT_H
#define KIS_GPU_SELECTION_INPUT_H

#include "KisGpuSelectionMask.h"
#include <kis_types.h>
#include <kritaimage_export.h>

class QPainterPath;

namespace LibrePaint::Image::Gpu {
/** Converts geometric tool input into immutable native Alpha8 coverage.
 * The image supplies finite default bounds and remains borrowed for the call.
 * No document pixels are read. Native path rasterization and selection filters
 * prepare an input resource; drawing and history consume the frozen result.
 * Null reports invalid input/capacity; empty() is valid zero coverage.
 */
KRITAIMAGE_EXPORT SelectionMaskSP selectionMaskFromPath(const QPainterPath &path, const KisImageSP &image,
                                                       bool antiAlias, int grow, int feather);
}
#endif
