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

/** Current execution location for a registered destructive filter. CPU filters
 * run on transferred pixels and publish their result in the GPU document.
 */
KRITAIMAGEGPUOPERATIONS_EXPORT ExecutionLocation filterExecutionLocation(const QString &id,
    FilterTarget target = FilterTarget::Rgba);
}
#endif
