/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuRgba8AlphaBlend_p.h"
#include "KisGpuRgba8Compositing_p.h"
#include "KisGpuNonlinearBlend_p.h"
#include <algorithm>
#include <stdexcept>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
quint8 saiChannel(quint8 destination, quint8 source, quint32 alpha)
{
    const volatile float s = float(source) / 255.f, d = float(destination) / 255.f, a = float(alpha) / 255.f;
    const volatile float product = s * a;
    const volatile float combined = product + d;
    const volatile float scaled = std::min(1.f, float(combined)) * 255.f;
    const volatile float rounded = scaled + .5f;
    return quint8(int(rounded));
}
}
RgbaPixel specialAlphaRgba8Pixel(RgbaPixel destination, const RgbaPixel &source,
                               quint8 opacity, quint8 mask, LayerBlendMode mode)
{
    const auto oldAlpha = destination[3];
    const bool locked = !(mask & 8u);
    if (mask != 15 && !oldAlpha) destination = {};
    if (mode == LayerBlendMode::Marker) {
        const auto alpha = multiplyByte(source[3], opacity);
        const auto weight = oldAlpha ? std::min(255u, (alpha * 255u + oldAlpha / 2u) / oldAlpha) : 255u;
        for (int channel = 0; channel < 3; ++channel)
            if (mask & (1u << channel)) destination[channel] = oldAlpha ? blendByte(destination[channel], source[channel], weight) : source[channel];
        if (!locked) destination[3] = opacity > oldAlpha ? blendByte(oldAlpha, opacity, source[3]) : oldAlpha;
        return destination;
    }
    const auto alpha = multiplyThreeBytes(source[3], 255, opacity);
    if (mode == LayerBlendMode::Greater) {
        if (!alpha || oldAlpha == 255) return destination;
        const auto pair = greaterAlphaUnit(oldAlpha, alpha, 255);
        const auto nextAlpha = quint8(pair.first);
        const auto weight = quint8(pair.second);
        for (int channel = 0; channel < 3; ++channel) {
            if (!(mask & (1u << channel))) continue;
            if (!oldAlpha) destination[channel] = source[channel];
            else if (nextAlpha) {
                const auto premultiplied = multiplyByte(destination[channel], oldAlpha);
                const auto mixed = blendByte(quint8(premultiplied), source[channel], weight);
                destination[channel] = std::min(255u, (quint32(mixed) * 255u + nextAlpha / 2u) / nextAlpha);
            }
        }
        if (!locked) destination[3] = nextAlpha;
        return destination;
    }
    const auto nextAlpha = alpha + oldAlpha - multiplyByte(alpha, oldAlpha);
    if (locked ? oldAlpha != 0 : nextAlpha != 0)
        for (int channel = 0; channel < 3; ++channel)
            if (mask & (1u << channel)) destination[channel] = saiChannel(destination[channel], source[channel], alpha);
    if (!locked) destination[3] = nextAlpha;
    return destination;
}
}
