/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_MARKER_BRUSH_P_H
#define KIS_GPU_MARKER_BRUSH_P_H

#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
/** Binary64 native circle inputs and its integer dispatch bounds. */
struct MarkerDab
{
    std::array<quint32, 12> words{};
    QRect bounds() const { return {qint32(words[8]), qint32(words[9]), int(words[10]), int(words[11])}; }
};
static_assert(sizeof(MarkerDab) == 48);

std::optional<QList<MarkerDab>> markerDabs(const PaintOperation &operation, bool pending, const QRect &region);
/** Conservative native circle/rectangle intersection for sparse tile dispatch. */
bool markerDabIntersects(const MarkerDab &dab, const QRect &region);
}
#endif
