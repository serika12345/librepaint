/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CLONE_PROJECTION_P_H
#define KIS_GPU_CLONE_PROJECTION_P_H
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
/** Resolves a live reference against one immutable document version. The
 * returned paint input contains GPU-evaluated predecessor projections. */
struct CloneLayerProjection
{
    static std::optional<Layer> resolve(const Snapshot &document, const Layer &clone,
                                       qsizetype maximumRectangles);
};
}
#endif
