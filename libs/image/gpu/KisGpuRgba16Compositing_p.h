/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_RGBA16_COMPOSITING_P_H
#define KIS_GPU_RGBA16_COMPOSITING_P_H
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
inline bool usesSeparableRgba16Blend(LayerBlendMode mode)
{
    return (mode >= LayerBlendMode::Multiply && mode <= LayerBlendMode::GeometricMean) ||
        (mode >= LayerBlendMode::Modulo && mode <= LayerBlendMode::ModuloShiftContinuous);
}
inline quint32 multiplyWord(quint32 a, quint32 b)
{
    const auto product = a * b + 32768u;
    return ((product >> 16) + product) >> 16;
}
/** Native U16 channel evaluation with staged rounding and integer alpha. */
RgbaPixel separableRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                             quint16 appliedAlpha, quint8 mask, LayerBlendMode mode);
}
#endif
