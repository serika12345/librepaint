/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BLEND_MODE_H
#define KIS_GPU_BLEND_MODE_H

#include "gpu/KisGpuOperationDocument.h"
#include "KoCompositeOpIds.h"
#include <QString>
#include <optional>

namespace LibrePaint::Image::Gpu {

/** Single mapping between the product composite id and the GPU blend mode.
 * Painting, node synchronization, the public API and file serialization share
 * it so a new mode cannot be accepted in one path and dropped in another.
 */
inline std::optional<LayerBlendMode> layerBlendModeFromCompositeOp(const QString &compositeOp)
{
    if (compositeOp == COMPOSITE_OVER) return LayerBlendMode::Normal;
    if (compositeOp == COMPOSITE_MULT) return LayerBlendMode::Multiply;
    if (compositeOp == COMPOSITE_SCREEN) return LayerBlendMode::Screen;
    if (compositeOp == COMPOSITE_DARKEN) return LayerBlendMode::Darken;
    if (compositeOp == COMPOSITE_LIGHTEN) return LayerBlendMode::Lighten;
    if (compositeOp == COMPOSITE_ADD) return LayerBlendMode::Addition;
    if (compositeOp == COMPOSITE_LINEAR_DODGE) return LayerBlendMode::LinearDodge;
    if (compositeOp == COMPOSITE_DIFF) return LayerBlendMode::Difference;
    if (compositeOp == COMPOSITE_EXCLUSION) return LayerBlendMode::Exclusion;
    if (compositeOp == COMPOSITE_SUBTRACT) return LayerBlendMode::Subtract;
    return std::nullopt;
}

inline QString compositeOpFromLayerBlendMode(LayerBlendMode mode)
{
    switch (mode) {
    case LayerBlendMode::Multiply: return COMPOSITE_MULT;
    case LayerBlendMode::Screen: return COMPOSITE_SCREEN;
    case LayerBlendMode::Darken: return COMPOSITE_DARKEN;
    case LayerBlendMode::Lighten: return COMPOSITE_LIGHTEN;
    case LayerBlendMode::Addition: return COMPOSITE_ADD;
    case LayerBlendMode::LinearDodge: return COMPOSITE_LINEAR_DODGE;
    case LayerBlendMode::Difference: return COMPOSITE_DIFF;
    case LayerBlendMode::Exclusion: return COMPOSITE_EXCLUSION;
    case LayerBlendMode::Subtract: return COMPOSITE_SUBTRACT;
    case LayerBlendMode::Normal: break;
    }
    return COMPOSITE_OVER;
}

}
#endif
