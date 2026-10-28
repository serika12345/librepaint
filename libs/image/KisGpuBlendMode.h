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
    if (compositeOp == COMPOSITE_MARKER) return LayerBlendMode::Marker;
    if (compositeOp == COMPOSITE_GREATER) return LayerBlendMode::Greater;
    if (compositeOp == COMPOSITE_LUMINOSITY_SAI) return LayerBlendMode::LuminositySai;
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
    if (compositeOp == COMPOSITE_DESTINATION_IN) return LayerBlendMode::DestinationIn;
    if (compositeOp == COMPOSITE_DESTINATION_ATOP) return LayerBlendMode::DestinationAtop;
    if (compositeOp == COMPOSITE_REFLECT) return LayerBlendMode::Reflect;
    if (compositeOp == COMPOSITE_GLOW) return LayerBlendMode::Glow;
    if (compositeOp == COMPOSITE_FREEZE) return LayerBlendMode::Freeze;
    if (compositeOp == COMPOSITE_HEAT) return LayerBlendMode::Heat;
    if (compositeOp == COMPOSITE_GLEAT) return LayerBlendMode::GlowHeat;
    if (compositeOp == COMPOSITE_HELOW) return LayerBlendMode::HeatGlow;
    if (compositeOp == COMPOSITE_REEZE) return LayerBlendMode::ReflectFreeze;
    if (compositeOp == COMPOSITE_FRECT) return LayerBlendMode::FreezeReflect;
    if (compositeOp == COMPOSITE_FHYRD) return LayerBlendMode::QuadraticHybrid;
    if (compositeOp == COMPOSITE_PARALLEL) return LayerBlendMode::Parallel;
    if (compositeOp == COMPOSITE_HARD_OVERLAY) return LayerBlendMode::HardOverlay;
    if (compositeOp == COMPOSITE_SOFT_LIGHT_PEGTOP_DELPHI) return LayerBlendMode::SoftLightPegtopDelphi;
    if (compositeOp == COMPOSITE_NEGATION) return LayerBlendMode::Negation;
    if (compositeOp == COMPOSITE_PENUMBRAA) return LayerBlendMode::PenumbraA;
    if (compositeOp == COMPOSITE_PENUMBRAB) return LayerBlendMode::PenumbraB;
    if (compositeOp == COMPOSITE_FLAT_LIGHT) return LayerBlendMode::FlatLight;
    if (compositeOp == COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP) return LayerBlendMode::HardMixSofterPhotoshop;
    if (compositeOp == COMPOSITE_GEOMETRIC_MEAN) return LayerBlendMode::GeometricMean;
    if (compositeOp == COMPOSITE_INTERPOLATION) return LayerBlendMode::Interpolation;
    if (compositeOp == COMPOSITE_INTERPOLATIONB) return LayerBlendMode::InterpolationB;
    if (compositeOp == COMPOSITE_PENUMBRAC) return LayerBlendMode::PenumbraC;
    if (compositeOp == COMPOSITE_PENUMBRAD) return LayerBlendMode::PenumbraD;
    if (compositeOp == COMPOSITE_SOFT_LIGHT_IFS_ILLUSIONS) return LayerBlendMode::SoftLightIFSIllusions;
    if (compositeOp == COMPOSITE_SOFT_LIGHT_SVG) return LayerBlendMode::SoftLightSvg;
    if (compositeOp == COMPOSITE_SOFT_LIGHT_PHOTOSHOP) return LayerBlendMode::SoftLightPhotoshop;
    if (compositeOp == COMPOSITE_GAMMA_LIGHT) return LayerBlendMode::GammaLight;
    if (compositeOp == COMPOSITE_GAMMA_ILLUMINATION) return LayerBlendMode::GammaIllumination;
    if (compositeOp == COMPOSITE_PNORM_A) return LayerBlendMode::PNormA;
    if (compositeOp == COMPOSITE_PNORM_B) return LayerBlendMode::PNormB;
    if (compositeOp == COMPOSITE_SUPER_LIGHT) return LayerBlendMode::SuperLight;
    if (compositeOp == COMPOSITE_TINT_IFS_ILLUSIONS) return LayerBlendMode::TintIFSIllusions;
    if (compositeOp == COMPOSITE_FOG_LIGHTEN_IFS_ILLUSIONS) return LayerBlendMode::FogLightenIFSIllusions;
    if (compositeOp == COMPOSITE_EASY_DODGE) return LayerBlendMode::EasyDodge;
    if (compositeOp == COMPOSITE_GAMMA_DARK) return LayerBlendMode::GammaDark;
    if (compositeOp == COMPOSITE_SHADE_IFS_ILLUSIONS) return LayerBlendMode::ShadeIFSIllusions;
    if (compositeOp == COMPOSITE_FOG_DARKEN_IFS_ILLUSIONS) return LayerBlendMode::FogDarkenIFSIllusions;
    if (compositeOp == COMPOSITE_EASY_BURN) return LayerBlendMode::EasyBurn;
    if (compositeOp == COMPOSITE_MOD) return LayerBlendMode::Modulo;
    if (compositeOp == COMPOSITE_MOD_CON) return LayerBlendMode::ModuloContinuous;
    if (compositeOp == COMPOSITE_DIVISIVE_MOD) return LayerBlendMode::DivisiveModulo;
    if (compositeOp == COMPOSITE_DIVISIVE_MOD_CON) return LayerBlendMode::DivisiveModuloContinuous;
    if (compositeOp == COMPOSITE_MODULO_SHIFT) return LayerBlendMode::ModuloShift;
    if (compositeOp == COMPOSITE_MODULO_SHIFT_CON) return LayerBlendMode::ModuloShiftContinuous;
    if (compositeOp == COMPOSITE_ARC_TANGENT) return LayerBlendMode::ArcTangent;
    if (compositeOp == COMPOSITE_ADDITIVE_SUBTRACTIVE) return LayerBlendMode::AdditiveSubtractive;
    if (compositeOp == COMPOSITE_COLOR) return LayerBlendMode::ColorHsy;
    if (compositeOp == COMPOSITE_HUE) return LayerBlendMode::HueHsy;
    if (compositeOp == COMPOSITE_TINT) return LayerBlendMode::TintHsy;
    if (compositeOp == COMPOSITE_SATURATION) return LayerBlendMode::SaturationHsy;
    if (compositeOp == COMPOSITE_INC_SATURATION) return LayerBlendMode::IncreaseSaturationHsy;
    if (compositeOp == COMPOSITE_DEC_SATURATION) return LayerBlendMode::DecreaseSaturationHsy;
    if (compositeOp == COMPOSITE_LUMINIZE) return LayerBlendMode::LightnessHsy;
    if (compositeOp == COMPOSITE_INC_LUMINOSITY) return LayerBlendMode::IncreaseLightnessHsy;
    if (compositeOp == COMPOSITE_DEC_LUMINOSITY) return LayerBlendMode::DecreaseLightnessHsy;
    if (compositeOp == COMPOSITE_COLOR_HSI) return LayerBlendMode::ColorHsi;
    if (compositeOp == COMPOSITE_HUE_HSI) return LayerBlendMode::HueHsi;
    if (compositeOp == COMPOSITE_SATURATION_HSI) return LayerBlendMode::SaturationHsi;
    if (compositeOp == COMPOSITE_INC_SATURATION_HSI) return LayerBlendMode::IncreaseSaturationHsi;
    if (compositeOp == COMPOSITE_DEC_SATURATION_HSI) return LayerBlendMode::DecreaseSaturationHsi;
    if (compositeOp == COMPOSITE_INTENSITY) return LayerBlendMode::LightnessHsi;
    if (compositeOp == COMPOSITE_INC_INTENSITY) return LayerBlendMode::IncreaseLightnessHsi;
    if (compositeOp == COMPOSITE_DEC_INTENSITY) return LayerBlendMode::DecreaseLightnessHsi;
    if (compositeOp == COMPOSITE_COLOR_HSL) return LayerBlendMode::ColorHsl;
    if (compositeOp == COMPOSITE_HUE_HSL) return LayerBlendMode::HueHsl;
    if (compositeOp == COMPOSITE_SATURATION_HSL) return LayerBlendMode::SaturationHsl;
    if (compositeOp == COMPOSITE_INC_SATURATION_HSL) return LayerBlendMode::IncreaseSaturationHsl;
    if (compositeOp == COMPOSITE_DEC_SATURATION_HSL) return LayerBlendMode::DecreaseSaturationHsl;
    if (compositeOp == COMPOSITE_LIGHTNESS) return LayerBlendMode::LightnessHsl;
    if (compositeOp == COMPOSITE_INC_LIGHTNESS) return LayerBlendMode::IncreaseLightnessHsl;
    if (compositeOp == COMPOSITE_DEC_LIGHTNESS) return LayerBlendMode::DecreaseLightnessHsl;
    if (compositeOp == COMPOSITE_COLOR_HSV) return LayerBlendMode::ColorHsv;
    if (compositeOp == COMPOSITE_HUE_HSV) return LayerBlendMode::HueHsv;
    if (compositeOp == COMPOSITE_SATURATION_HSV) return LayerBlendMode::SaturationHsv;
    if (compositeOp == COMPOSITE_INC_SATURATION_HSV) return LayerBlendMode::IncreaseSaturationHsv;
    if (compositeOp == COMPOSITE_DEC_SATURATION_HSV) return LayerBlendMode::DecreaseSaturationHsv;
    if (compositeOp == COMPOSITE_VALUE) return LayerBlendMode::LightnessHsv;
    if (compositeOp == COMPOSITE_INC_VALUE) return LayerBlendMode::IncreaseLightnessHsv;
    if (compositeOp == COMPOSITE_DEC_VALUE) return LayerBlendMode::DecreaseLightnessHsv;
    if (compositeOp == COMPOSITE_DARKER_COLOR) return LayerBlendMode::DarkerColor;
    if (compositeOp == COMPOSITE_LIGHTER_COLOR) return LayerBlendMode::LighterColor;
    if (compositeOp == COMPOSITE_TANGENT_NORMALMAP) return LayerBlendMode::TangentNormalMap;
    if (compositeOp == COMPOSITE_COMBINE_NORMAL) return LayerBlendMode::ReorientedNormalMap;
    if (compositeOp == COMPOSITE_LAMBERT_LIGHTING) return LayerBlendMode::LambertLighting;
    if (compositeOp == COMPOSITE_LAMBERT_LIGHTING_GAMMA_2_2) return LayerBlendMode::LambertLightingGamma22;
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
    case LayerBlendMode::DestinationIn: return COMPOSITE_DESTINATION_IN;
    case LayerBlendMode::DestinationAtop: return COMPOSITE_DESTINATION_ATOP;
    case LayerBlendMode::Reflect: return COMPOSITE_REFLECT;
    case LayerBlendMode::Glow: return COMPOSITE_GLOW;
    case LayerBlendMode::Freeze: return COMPOSITE_FREEZE;
    case LayerBlendMode::Heat: return COMPOSITE_HEAT;
    case LayerBlendMode::GlowHeat: return COMPOSITE_GLEAT;
    case LayerBlendMode::HeatGlow: return COMPOSITE_HELOW;
    case LayerBlendMode::ReflectFreeze: return COMPOSITE_REEZE;
    case LayerBlendMode::FreezeReflect: return COMPOSITE_FRECT;
    case LayerBlendMode::QuadraticHybrid: return COMPOSITE_FHYRD;
    case LayerBlendMode::Parallel: return COMPOSITE_PARALLEL;
    case LayerBlendMode::HardOverlay: return COMPOSITE_HARD_OVERLAY;
    case LayerBlendMode::SoftLightPegtopDelphi: return COMPOSITE_SOFT_LIGHT_PEGTOP_DELPHI;
    case LayerBlendMode::Negation: return COMPOSITE_NEGATION;
    case LayerBlendMode::PenumbraA: return COMPOSITE_PENUMBRAA;
    case LayerBlendMode::PenumbraB: return COMPOSITE_PENUMBRAB;
    case LayerBlendMode::FlatLight: return COMPOSITE_FLAT_LIGHT;
    case LayerBlendMode::HardMixSofterPhotoshop: return COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP;
    case LayerBlendMode::GeometricMean: return COMPOSITE_GEOMETRIC_MEAN;
    case LayerBlendMode::Interpolation: return COMPOSITE_INTERPOLATION;
    case LayerBlendMode::InterpolationB: return COMPOSITE_INTERPOLATIONB;
    case LayerBlendMode::PenumbraC: return COMPOSITE_PENUMBRAC;
    case LayerBlendMode::PenumbraD: return COMPOSITE_PENUMBRAD;
    case LayerBlendMode::SoftLightIFSIllusions: return COMPOSITE_SOFT_LIGHT_IFS_ILLUSIONS;
    case LayerBlendMode::SoftLightSvg: return COMPOSITE_SOFT_LIGHT_SVG;
    case LayerBlendMode::SoftLightPhotoshop: return COMPOSITE_SOFT_LIGHT_PHOTOSHOP;
    case LayerBlendMode::GammaLight: return COMPOSITE_GAMMA_LIGHT;
    case LayerBlendMode::GammaIllumination: return COMPOSITE_GAMMA_ILLUMINATION;
    case LayerBlendMode::PNormA: return COMPOSITE_PNORM_A;
    case LayerBlendMode::PNormB: return COMPOSITE_PNORM_B;
    case LayerBlendMode::SuperLight: return COMPOSITE_SUPER_LIGHT;
    case LayerBlendMode::TintIFSIllusions: return COMPOSITE_TINT_IFS_ILLUSIONS;
    case LayerBlendMode::FogLightenIFSIllusions: return COMPOSITE_FOG_LIGHTEN_IFS_ILLUSIONS;
    case LayerBlendMode::EasyDodge: return COMPOSITE_EASY_DODGE;
    case LayerBlendMode::GammaDark: return COMPOSITE_GAMMA_DARK;
    case LayerBlendMode::ShadeIFSIllusions: return COMPOSITE_SHADE_IFS_ILLUSIONS;
    case LayerBlendMode::FogDarkenIFSIllusions: return COMPOSITE_FOG_DARKEN_IFS_ILLUSIONS;
    case LayerBlendMode::EasyBurn: return COMPOSITE_EASY_BURN;
    case LayerBlendMode::Modulo: return COMPOSITE_MOD;
    case LayerBlendMode::ModuloContinuous: return COMPOSITE_MOD_CON;
    case LayerBlendMode::DivisiveModulo: return COMPOSITE_DIVISIVE_MOD;
    case LayerBlendMode::DivisiveModuloContinuous: return COMPOSITE_DIVISIVE_MOD_CON;
    case LayerBlendMode::ModuloShift: return COMPOSITE_MODULO_SHIFT;
    case LayerBlendMode::ModuloShiftContinuous: return COMPOSITE_MODULO_SHIFT_CON;
    case LayerBlendMode::ArcTangent: return COMPOSITE_ARC_TANGENT;
    case LayerBlendMode::AdditiveSubtractive: return COMPOSITE_ADDITIVE_SUBTRACTIVE;
    case LayerBlendMode::ColorHsy: return COMPOSITE_COLOR;
    case LayerBlendMode::HueHsy: return COMPOSITE_HUE;
    case LayerBlendMode::TintHsy: return COMPOSITE_TINT;
    case LayerBlendMode::SaturationHsy: return COMPOSITE_SATURATION;
    case LayerBlendMode::IncreaseSaturationHsy: return COMPOSITE_INC_SATURATION;
    case LayerBlendMode::DecreaseSaturationHsy: return COMPOSITE_DEC_SATURATION;
    case LayerBlendMode::LightnessHsy: return COMPOSITE_LUMINIZE;
    case LayerBlendMode::IncreaseLightnessHsy: return COMPOSITE_INC_LUMINOSITY;
    case LayerBlendMode::DecreaseLightnessHsy: return COMPOSITE_DEC_LUMINOSITY;
    case LayerBlendMode::ColorHsi: return COMPOSITE_COLOR_HSI;
    case LayerBlendMode::HueHsi: return COMPOSITE_HUE_HSI;
    case LayerBlendMode::SaturationHsi: return COMPOSITE_SATURATION_HSI;
    case LayerBlendMode::IncreaseSaturationHsi: return COMPOSITE_INC_SATURATION_HSI;
    case LayerBlendMode::DecreaseSaturationHsi: return COMPOSITE_DEC_SATURATION_HSI;
    case LayerBlendMode::LightnessHsi: return COMPOSITE_INTENSITY;
    case LayerBlendMode::IncreaseLightnessHsi: return COMPOSITE_INC_INTENSITY;
    case LayerBlendMode::DecreaseLightnessHsi: return COMPOSITE_DEC_INTENSITY;
    case LayerBlendMode::ColorHsl: return COMPOSITE_COLOR_HSL;
    case LayerBlendMode::HueHsl: return COMPOSITE_HUE_HSL;
    case LayerBlendMode::SaturationHsl: return COMPOSITE_SATURATION_HSL;
    case LayerBlendMode::IncreaseSaturationHsl: return COMPOSITE_INC_SATURATION_HSL;
    case LayerBlendMode::DecreaseSaturationHsl: return COMPOSITE_DEC_SATURATION_HSL;
    case LayerBlendMode::LightnessHsl: return COMPOSITE_LIGHTNESS;
    case LayerBlendMode::IncreaseLightnessHsl: return COMPOSITE_INC_LIGHTNESS;
    case LayerBlendMode::DecreaseLightnessHsl: return COMPOSITE_DEC_LIGHTNESS;
    case LayerBlendMode::ColorHsv: return COMPOSITE_COLOR_HSV;
    case LayerBlendMode::HueHsv: return COMPOSITE_HUE_HSV;
    case LayerBlendMode::SaturationHsv: return COMPOSITE_SATURATION_HSV;
    case LayerBlendMode::IncreaseSaturationHsv: return COMPOSITE_INC_SATURATION_HSV;
    case LayerBlendMode::DecreaseSaturationHsv: return COMPOSITE_DEC_SATURATION_HSV;
    case LayerBlendMode::LightnessHsv: return COMPOSITE_VALUE;
    case LayerBlendMode::IncreaseLightnessHsv: return COMPOSITE_INC_VALUE;
    case LayerBlendMode::DecreaseLightnessHsv: return COMPOSITE_DEC_VALUE;
    case LayerBlendMode::DarkerColor: return COMPOSITE_DARKER_COLOR;
    case LayerBlendMode::LighterColor: return COMPOSITE_LIGHTER_COLOR;
    case LayerBlendMode::TangentNormalMap: return COMPOSITE_TANGENT_NORMALMAP;
    case LayerBlendMode::ReorientedNormalMap: return COMPOSITE_COMBINE_NORMAL;
    case LayerBlendMode::LambertLighting: return COMPOSITE_LAMBERT_LIGHTING;
    case LayerBlendMode::LambertLightingGamma22: return COMPOSITE_LAMBERT_LIGHTING_GAMMA_2_2;
    case LayerBlendMode::Marker: return COMPOSITE_MARKER;
    case LayerBlendMode::Greater: return COMPOSITE_GREATER;
    case LayerBlendMode::LuminositySai: return COMPOSITE_LUMINOSITY_SAI;
    case LayerBlendMode::Normal: break;
    }
    return COMPOSITE_OVER;
}

}
#endif
