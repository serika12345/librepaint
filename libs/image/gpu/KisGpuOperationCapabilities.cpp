/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationCapabilities.h"

namespace LibrePaint::Image::Gpu {
ExecutionLocation filterExecutionLocation(const QString &id, FilterTarget target)
{
    if (target == FilterTarget::Alpha) return ExecutionLocation::Cpu;
    static constexpr const char *gpuFilters[] = {
        "invert", "desaturate", "hsvadjustment", "maximize", "minimize",
        "resettransparent", "fastcoloroverlay", "sharpen", "mean removal",
        "emboss laplascian", "emboss all directions", "emboss horizontal and vertical",
        "emboss vertical only", "emboss horizontal only"
    };
    for (const auto *filter : gpuFilters)
        if (id == QLatin1String(filter)) return ExecutionLocation::Gpu;
    return ExecutionLocation::Cpu;
}
}
