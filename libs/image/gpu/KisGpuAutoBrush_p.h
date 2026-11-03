/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_AUTO_BRUSH_P_H
#define KIS_GPU_AUTO_BRUSH_P_H

#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
/** Geometry and frozen arithmetic inputs; mask pixels are generated on the GPU. */
struct AutoBrushDab
{
    std::array<quint32, 32> words{};
    /** Image-tip row ranges and fixed-point chunk origins; coordinate metadata only. */
    QSharedPointer<const QList<quint32>> imageCoordinates;
    qsizetype hand = 0;
    QRect bounds() const { return {qint32(words[12]), qint32(words[13]), int(words[0]), int(words[1])}; }
};
static_assert(sizeof(AutoBrushDab::words) == 128);

/** Pending intent belongs to the pinned version; stationary input stamps its last point after commit. */
std::optional<QList<AutoBrushDab>> autoBrushDabs(const PaintOperation &operation, bool pending);
}
#endif
