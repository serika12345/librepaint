/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_RGBA8_ALPHA_BLEND_P_H
#define KIS_GPU_RGBA8_ALPHA_BLEND_P_H

#include "KisGpuOperationDocument.h"
#include <QByteArray>

namespace LibrePaint::Image::Gpu::Detail {
inline bool usesSpecialAlphaBlend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::Marker && mode <= LayerBlendMode::LuminositySai; }
RgbaPixel specialAlphaRgba8Pixel(RgbaPixel destination, const RgbaPixel &source,
                               quint8 opacity, quint8 mask, LayerBlendMode mode);
}
#endif
