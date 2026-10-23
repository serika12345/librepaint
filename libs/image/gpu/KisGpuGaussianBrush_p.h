/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_GAUSSIAN_BRUSH_P_H
#define KIS_GPU_GAUSSIAN_BRUSH_P_H

#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
/** Geometry and frozen arithmetic inputs; mask pixels are generated on the GPU. */
struct GaussianDab
{
    std::array<quint32, 24> words{};
    QRect bounds() const { return {qint32(words[18]), qint32(words[19]), int(words[0]), int(words[1])}; }
};
static_assert(sizeof(GaussianDab) == 96);

std::optional<QList<GaussianDab>> gaussianDabs(const PaintOperation &operation);
}
#endif
