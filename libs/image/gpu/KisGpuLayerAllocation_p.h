/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_LAYER_ALLOCATION_P_H
#define KIS_GPU_LAYER_ALLOCATION_P_H

#include "KisGpuOperationDocument.h"
#include <QRegion>
#include <optional>

namespace LibrePaint::Image::Gpu::Detail {
struct LayerAllocation
{
    QRegion blocks;
    QList<QRect> beforeOperations;
    QRect extent;
};
/** Native 64-pixel allocation and preceding extents derived without pixel access. */
std::optional<LayerAllocation> paintLayerAllocation(const Snapshot &document, const Layer &layer, qsizetype maximumRectangles);
/** Native projection storage extent for the current group merge consumer. */
std::optional<QRect> groupProjectionExtent(const Snapshot &document, LayerId group, qsizetype maximumRectangles);
}
#endif
