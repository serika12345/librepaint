/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_RGBA16_COMPOSITING_P_H
#define KIS_GPU_RGBA16_COMPOSITING_P_H
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
inline bool usesIntegerRgba16Blend(LayerBlendMode mode)
{
    return (mode >= LayerBlendMode::Multiply && mode <= LayerBlendMode::HardMixPhotoshop) ||
           (mode >= LayerBlendMode::BitwiseXor && mode <= LayerBlendMode::NotConverse) ||
           (mode >= LayerBlendMode::Reflect && mode <= LayerBlendMode::HardMixSofterPhotoshop);
}
/** Native U16 separable composition, including integer truncation boundaries. */
RgbaPixel separableRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                             quint16 appliedAlpha, quint8 mask, LayerBlendMode mode);
}
#endif
