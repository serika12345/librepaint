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
    if (compositeOp == COMPOSITE_LINEAR_BURN) return LayerBlendMode::LinearBurn;
    if (compositeOp == COMPOSITE_INVERSE_SUBTRACT) return LayerBlendMode::InverseSubtract;
    if (compositeOp == COMPOSITE_GRAIN_MERGE) return LayerBlendMode::GrainMerge;
    if (compositeOp == COMPOSITE_GRAIN_EXTRACT) return LayerBlendMode::GrainExtract;
    if (compositeOp == COMPOSITE_OVERLAY) return LayerBlendMode::Overlay;
    if (compositeOp == COMPOSITE_HARD_LIGHT) return LayerBlendMode::HardLight;
    if (compositeOp == COMPOSITE_PIN_LIGHT) return LayerBlendMode::PinLight;
    if (compositeOp == COMPOSITE_DIVIDE) return LayerBlendMode::Divide;
    if (compositeOp == COMPOSITE_DODGE) return LayerBlendMode::ColorDodge;
    if (compositeOp == COMPOSITE_BURN) return LayerBlendMode::ColorBurn;
    if (compositeOp == COMPOSITE_HARD_MIX) return LayerBlendMode::HardMix;
    if (compositeOp == COMPOSITE_VIVID_LIGHT) return LayerBlendMode::VividLight;
    if (compositeOp == COMPOSITE_LINEAR_LIGHT) return LayerBlendMode::LinearLight;
    if (compositeOp == COMPOSITE_EQUIVALENCE) return LayerBlendMode::Equivalence;
    if (compositeOp == COMPOSITE_ALLANON) return LayerBlendMode::Allanon;
    if (compositeOp == COMPOSITE_HARD_MIX_PHOTOSHOP) return LayerBlendMode::HardMixPhotoshop;
    if (compositeOp == COMPOSITE_ERASE) return LayerBlendMode::Erase;
    if (compositeOp == COMPOSITE_BEHIND) return LayerBlendMode::Behind;
    if (compositeOp == COMPOSITE_COPY_RED) return LayerBlendMode::CopyRed;
    if (compositeOp == COMPOSITE_COPY_GREEN) return LayerBlendMode::CopyGreen;
    if (compositeOp == COMPOSITE_COPY_BLUE) return LayerBlendMode::CopyBlue;
    if (compositeOp == COMPOSITE_XOR) return LayerBlendMode::BitwiseXor;
    if (compositeOp == COMPOSITE_OR) return LayerBlendMode::BitwiseOr;
    if (compositeOp == COMPOSITE_AND) return LayerBlendMode::BitwiseAnd;
    if (compositeOp == COMPOSITE_NAND) return LayerBlendMode::BitwiseNand;
    if (compositeOp == COMPOSITE_NOR) return LayerBlendMode::BitwiseNor;
    if (compositeOp == COMPOSITE_XNOR) return LayerBlendMode::BitwiseXnor;
    if (compositeOp == COMPOSITE_IMPLICATION) return LayerBlendMode::Implication;
    if (compositeOp == COMPOSITE_NOT_IMPLICATION) return LayerBlendMode::NotImplication;
    if (compositeOp == COMPOSITE_CONVERSE) return LayerBlendMode::Converse;
    if (compositeOp == COMPOSITE_NOT_CONVERSE) return LayerBlendMode::NotConverse;
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
    case LayerBlendMode::LinearBurn: return COMPOSITE_LINEAR_BURN;
    case LayerBlendMode::InverseSubtract: return COMPOSITE_INVERSE_SUBTRACT;
    case LayerBlendMode::GrainMerge: return COMPOSITE_GRAIN_MERGE;
    case LayerBlendMode::GrainExtract: return COMPOSITE_GRAIN_EXTRACT;
    case LayerBlendMode::Overlay: return COMPOSITE_OVERLAY;
    case LayerBlendMode::HardLight: return COMPOSITE_HARD_LIGHT;
    case LayerBlendMode::PinLight: return COMPOSITE_PIN_LIGHT;
    case LayerBlendMode::Divide: return COMPOSITE_DIVIDE;
    case LayerBlendMode::ColorDodge: return COMPOSITE_DODGE;
    case LayerBlendMode::ColorBurn: return COMPOSITE_BURN;
    case LayerBlendMode::HardMix: return COMPOSITE_HARD_MIX;
    case LayerBlendMode::VividLight: return COMPOSITE_VIVID_LIGHT;
    case LayerBlendMode::LinearLight: return COMPOSITE_LINEAR_LIGHT;
    case LayerBlendMode::Equivalence: return COMPOSITE_EQUIVALENCE;
    case LayerBlendMode::Allanon: return COMPOSITE_ALLANON;
    case LayerBlendMode::HardMixPhotoshop: return COMPOSITE_HARD_MIX_PHOTOSHOP;
    case LayerBlendMode::Erase: return COMPOSITE_ERASE;
    case LayerBlendMode::Behind: return COMPOSITE_BEHIND;
    case LayerBlendMode::CopyRed: return COMPOSITE_COPY_RED;
    case LayerBlendMode::CopyGreen: return COMPOSITE_COPY_GREEN;
    case LayerBlendMode::CopyBlue: return COMPOSITE_COPY_BLUE;
    case LayerBlendMode::BitwiseXor: return COMPOSITE_XOR;
    case LayerBlendMode::BitwiseOr: return COMPOSITE_OR;
    case LayerBlendMode::BitwiseAnd: return COMPOSITE_AND;
    case LayerBlendMode::BitwiseNand: return COMPOSITE_NAND;
    case LayerBlendMode::BitwiseNor: return COMPOSITE_NOR;
    case LayerBlendMode::BitwiseXnor: return COMPOSITE_XNOR;
    case LayerBlendMode::Implication: return COMPOSITE_IMPLICATION;
    case LayerBlendMode::NotImplication: return COMPOSITE_NOT_IMPLICATION;
    case LayerBlendMode::Converse: return COMPOSITE_CONVERSE;
    case LayerBlendMode::NotConverse: return COMPOSITE_NOT_CONVERSE;
    case LayerBlendMode::Normal: break;
    }
    return COMPOSITE_OVER;
}

}
#endif
