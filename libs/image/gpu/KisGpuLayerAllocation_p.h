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
/** Native group projection storage extent; group zero denotes the image root. */
std::optional<QRect> groupProjectionExtent(const Snapshot &document, LayerId group, qsizetype maximumRectangles);
/** Source pixels required by a native 3x3 filter, clipped to the coordinate domain. */
QRect convolutionInputRegion(QRect output);
/** Before/after regions for the parent's ordered filter masks. */
QList<QRect> filterMaskRegions(const Snapshot &document, const Layer &layer, QRect output);
struct FilterProjectionRegions
{
    QHash<LayerId, QRect> output;
    QHash<LayerId, QRect> input;
};
/** Propagate adjustment input demands backwards through a composition sequence. */
FilterProjectionRegions filterProjectionRegions(const Snapshot &document,
    const QList<LayerProjectionInput> &inputs, QRect output);
/** Stable repeat bounds for full persistent evaluation use the original allocation and canvas. */
std::optional<QRect> filterRepeatBounds(const Snapshot &document, const Layer &layer, qsizetype maximumRectangles);
}
#endif
