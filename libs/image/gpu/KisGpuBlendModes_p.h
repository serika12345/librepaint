/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BLEND_MODES_P_H
#define KIS_GPU_BLEND_MODES_P_H
#include "KisGpuOperationDocument.h"
namespace LibrePaint::Image::Gpu::Detail {
inline bool usesSeparableRgba16Blend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::Multiply && mode <= LayerBlendMode::AdditiveSubtractive; }
inline bool usesSpecialAlphaBlend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::Marker && mode <= LayerBlendMode::LuminositySai; }
inline bool usesSpecialAlphaRgba16Blend(LayerBlendMode mode)
{ return usesSpecialAlphaBlend(mode); }
inline bool isCoupledBlend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::ColorHsy && mode <= LayerBlendMode::LambertLightingGamma22; }
inline bool transparentSourceAffectsBlend(LayerBlendMode mode)
{ return isCoupledBlend(mode) || usesSpecialAlphaBlend(mode); }
}
#endif
