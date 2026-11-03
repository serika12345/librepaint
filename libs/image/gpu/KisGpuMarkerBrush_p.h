/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_MARKER_BRUSH_P_H
#define KIS_GPU_MARKER_BRUSH_P_H

#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
/** Native f32 circle inputs relative to their dispatch bounds and its integer dispatch bounds. */
struct MarkerDab
{
    std::array<quint32, 8> words{};
    QRect bounds() const { return {qint32(words[4]), qint32(words[5]), int(words[6]), int(words[7])}; }
};
static_assert(sizeof(MarkerDab) == 32);

/** Empty source pixels in the allocation rectangle participate in alpha-replacing merges. */
std::optional<QList<MarkerDab>> markerDabs(const PaintOperation &operation, bool pending, const QRect &region,
                                         bool includeEmptySource = false);
/** Conservative native circle/rectangle intersection for sparse tile dispatch. */
bool markerDabIntersects(const MarkerDab &dab, const QRect &region);
}
#endif
