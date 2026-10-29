/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_RGBA16_COMPOSITING_P_H
#define KIS_GPU_RGBA16_COMPOSITING_P_H
#include "KisGpuOperationDocument.h"

namespace LibrePaint::Image::Gpu::Detail {
constexpr qsizetype Rgba16CosineBytes = 2 * 65536 * 4;
constexpr qsizetype Rgba16PowerBytes = 8 * 65536 * 4;
constexpr bool usesRgba16Cosine(LayerBlendMode mode)
{ return mode == LayerBlendMode::Interpolation || mode == LayerBlendMode::InterpolationB; }
/** Immutable binary64 cosine words shared by projection defaults and GPU colors. */
KRITAIMAGEGPUOPERATIONS_EXPORT const QByteArray &rgba16CosineValues();
constexpr bool usesRgba16PowerValues(LayerBlendMode mode)
{ return mode == LayerBlendMode::SuperLight || mode == LayerBlendMode::PNormA; }
/** Native fixed-exponent powers, in four pairs of binary64 word layers. */
KRITAIMAGEGPUOPERATIONS_EXPORT const QByteArray &rgba16PowerValues();
inline bool usesSeparableRgba16Blend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::Multiply && mode <= LayerBlendMode::AdditiveSubtractive; }
inline bool usesSpecialAlphaRgba16Blend(LayerBlendMode mode)
{ return mode >= LayerBlendMode::Marker && mode <= LayerBlendMode::LuminositySai; }
inline quint32 multiplyWord(quint32 a, quint32 b)
{
    const auto product = a * b + 32768u;
    return ((product >> 16) + product) >> 16;
}
inline quint32 multiplyThreeWords(quint32 a, quint32 b, quint32 c)
{ return quint64(a) * b * c / (quint64(65535) * 65535); }
inline quint32 divideWord(quint32 a, quint32 b)
{ return (a * 65535u + b / 2) / b; }
inline quint16 interpolateWord(quint16 destination, quint16 source, quint32 alpha)
{ return qint64(destination) + (qint64(source) - destination) * alpha / 65535; }
/** Native U16 channel evaluation with staged rounding and integer alpha. */
RgbaPixel separableRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                             quint16 appliedAlpha, quint8 mask, LayerBlendMode mode);
/** Coupled RGB evaluation followed by native U16 quantization and alpha. */
RgbaPixel coupledRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                            quint16 appliedAlpha, quint8 mask, LayerBlendMode mode);
RgbaPixel specialAlphaRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                                 quint16 opacity, quint8 mask, quint16 selection, LayerBlendMode mode);
}
#endif
