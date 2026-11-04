/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationCapabilities.h"

namespace LibrePaint::Image::Gpu {
ExecutionLocation layerExecutionLocation(LayerFeature feature)
{
    switch (feature) {
    case LayerFeature::Paint:
    case LayerFeature::Group:
    case LayerFeature::TransparencyMask:
    case LayerFeature::SelectionMask:
        return ExecutionLocation::Gpu;
    case LayerFeature::Adjustment:
    case LayerFeature::Generator:
    case LayerFeature::Clone:
    case LayerFeature::File:
    case LayerFeature::Vector:
    case LayerFeature::Text:
    case LayerFeature::LayerStyle:
    case LayerFeature::ColorizeMask:
        return ExecutionLocation::Cpu;
    }
    return ExecutionLocation::Cpu;
}

ExecutionLocation toolExecutionLocation(ToolOperation operation)
{
    switch (operation) {
    case ToolOperation::Gradient:
    case ToolOperation::Fill:
    case ToolOperation::SmartPatch:
    case ToolOperation::EncloseAndFill:
        return ExecutionLocation::Cpu;
    }
    return ExecutionLocation::Cpu;
}

ExecutionLocation brushExecutionLocation(const QString &id)
{
    return id == QLatin1String("paintbrush") || id == QLatin1String("roundmarker") ?
        ExecutionLocation::Gpu : ExecutionLocation::Cpu;
}

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
