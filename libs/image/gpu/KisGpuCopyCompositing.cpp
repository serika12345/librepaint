/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCopyCompositing_p.h"
#include <algorithm>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
RgbaPixel copyPixel(RgbaPixel destination, const RgbaPixel &source,
                   float opacity, quint8 writeMask, PixelFormat format)
{
    const bool locked = !(writeMask & 8u);
    if ((locked && !source[3]) || opacity == 0.0f) return destination;
    if (opacity == 1.0f) {
        if (writeMask == 15) return source;
        if (!locked && !destination[3]) destination = {};
        for (int channel = 0; channel < 4; ++channel)
            if (writeMask & (1u << channel)) destination[channel] = source[channel];
        return destination;
    }
    const float maximum = format == PixelFormat::Rgba16 ? 65535.0f : 255.0f;
    const volatile float reciprocal = 1.0f / maximum;
    const volatile float sourceAlpha = float(source[3]) * reciprocal;
    const volatile float destinationAlpha = float(destination[3]) * reciprocal;
    const volatile float difference = sourceAlpha - destinationAlpha;
    const float alpha = std::fma(opacity, float(difference), float(destinationAlpha));
    if (alpha == 0.0f) {
        if (writeMask == 15 || destinationAlpha == 0.0f) return {};
        for (int channel = 0; channel < 4; ++channel)
            if (writeMask & (1u << channel)) destination[channel] = 0;
        return destination;
    }
    for (int channel = 0; channel < 3; ++channel) {
        if (!(writeMask & (1u << channel))) {
            if (!locked && destinationAlpha == 0.0f) destination[channel] = 0;
            continue;
        }
        const volatile float sourceColor = float(source[channel]) * sourceAlpha;
        const volatile float destinationColor = float(destination[channel]) * destinationAlpha;
        const volatile float colorDifference = sourceColor - destinationColor;
        float color = std::fma(opacity, float(colorDifference), float(destinationColor));
        if (alpha != 1.0f) color = std::min(color / alpha, maximum);
        destination[channel] = !locked && destinationAlpha == 0.0f && (writeMask & 7u) != 7u ?
            quint16(color) : quint16(std::nearbyint(color));
    }
    if (!locked) {
        const float scaled = alpha * maximum;
        destination[3] = quint16(std::nearbyint(scaled));
    }
    return destination;
}
}
