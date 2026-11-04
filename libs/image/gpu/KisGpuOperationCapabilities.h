/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_OPERATION_CAPABILITIES_H
#define KIS_GPU_OPERATION_CAPABILITIES_H

#include <QString>
#include <kritaimagegpuoperations_export.h>

namespace LibrePaint::Image::Gpu {
enum class ExecutionLocation { Gpu, Cpu };
enum class FilterTarget { Rgba, Alpha };
enum class ToolOperation { Gradient, Fill, SmartPatch, EncloseAndFill };
enum class LayerFeature {
    Paint, Group, TransparencyMask, SelectionMask,
    Adjustment, Generator, Clone, File, Vector, Text, LayerStyle, ColorizeMask
};

/** Current execution location for a registered destructive filter. CPU filters
 * run on transferred pixels and publish their result in the GPU document.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT ExecutionLocation filterExecutionLocation(const QString &id,
    FilterTarget target = FilterTarget::Rgba);
/** Current freehand brush implementation. Other engines use native CPU pixels. */
KRITAIMAGEGPUOPERATIONS_EXPORT ExecutionLocation brushExecutionLocation(const QString &id);
/** One-shot tools compute on transferred native pixels, per the migration scope. */
KRITAIMAGEGPUOPERATIONS_EXPORT ExecutionLocation toolExecutionLocation(ToolOperation operation);
/** Placement of layer computation, independent of destructive filter support. */
KRITAIMAGEGPUOPERATIONS_EXPORT ExecutionLocation layerExecutionLocation(LayerFeature feature);
}
#endif
