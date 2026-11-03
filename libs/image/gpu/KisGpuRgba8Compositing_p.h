/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_RGBA8_COMPOSITING_P_H
#define KIS_GPU_RGBA8_COMPOSITING_P_H
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
inline quint32 multiplyByte(quint32 a, quint32 b)
{
    const auto product = a * b + 128u;
    return ((product >> 8) + product) >> 8;
}
inline quint32 multiplyThreeBytes(quint32 a, quint32 b, quint32 c)
{
    const auto product = a * b * c + 0x7f5bu;
    return ((product >> 7) + product) >> 16;
}
inline quint8 blendByte(quint8 destination, quint8 source, quint32 alpha)
{
    const auto weighted = (int(source) - destination) * int(alpha) + 128;
    return quint8((((weighted >> 8) + weighted) >> 8) + destination);
}
inline bool isCoupledBlend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::ColorHsy && mode <= LayerBlendMode::LambertLightingGamma22; }

inline bool transparentSourceAffectsBlend(LayerBlendMode mode)
{ return isCoupledBlend(mode) || (mode >= LayerBlendMode::Marker && mode <= LayerBlendMode::LuminositySai); }

/** Pure uniform-pixel counterpart of the GPU's coupled RGB compositor.
 * Intermediate binary32/binary64 operations retain native rounding stages.
 */
RgbaPixel coupledRgba8Pixel(RgbaPixel destination, const RgbaPixel &source,
                           quint8 opacity, quint8 mask, LayerBlendMode mode);
}
#endif
