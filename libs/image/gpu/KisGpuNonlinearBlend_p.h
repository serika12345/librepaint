/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_NONLINEAR_BLEND_P_H
#define KIS_GPU_NONLINEAR_BLEND_P_H

#include "KisGpuOperationDocument.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace LibrePaint::Image::Gpu::Detail {
/** Nonlinear separable modes GeometricMean through AdditiveSubtractive (60..87)
 * and the Greater alpha curve in float, matching separable.wgsl. Integer colors
 * are normalized by their depth maximum (255 or 65535). */
constexpr bool isNonlinearBlend(LayerBlendMode mode)
{ return int(mode) >= int(LayerBlendMode::GeometricMean) && int(mode) <= int(LayerBlendMode::AdditiveSubtractive); }

namespace NonlinearBlend {
inline float unit(quint32 value, quint32 maximum) { return float(value) / float(maximum); }
inline quint32 scaled(float value, quint32 maximum)
{
    if (value < 0.f) return 0;
    const float result = value * float(maximum);
    if (!(result <= float(maximum))) return maximum;
    return std::min(maximum, quint32(result + .5f));
}
inline quint32 multiply(quint32 a, quint32 b, quint32 maximum)
{ return quint32((quint64(a) * b + maximum / 2) / maximum); }
inline float modulo(float value) { return value - std::floor(value); }
inline bool ceilingIsOdd(float value)
{
    const float integral = std::floor(value);
    return ((quint32(integral) + (integral != value ? 1u : 0u)) & 1u) != 0;
}
inline quint32 arcTangent(quint32 source, quint32 destination, quint32 maximum)
{
    if (!source) return 0;
    if (!destination) return maximum;
    return scaled(2.f * std::atan(unit(source, maximum) / unit(destination, maximum)) / 3.14159265358979f, maximum);
}
inline quint32 gammaDark(quint32 source, quint32 destination, quint32 maximum)
{
    if (!destination || destination == maximum) return destination;
    if (!source) return 0;
    return scaled(std::pow(unit(destination, maximum), 1.f / unit(source, maximum)), maximum);
}
inline quint32 interpolation(quint32 source, quint32 destination, quint32 maximum)
{
    if (!source && !destination) return 0;
    const float pi = 3.14159265358979f;
    return scaled(.5f - .25f * std::cos(pi * unit(source, maximum)) - .25f * std::cos(pi * unit(destination, maximum)), maximum);
}
inline quint32 divisiveModuloContinuous(quint32 source, quint32 destination, quint32 maximum)
{
    if (!destination || !source) return 0;
    const float s = unit(source, maximum), d = unit(destination, maximum);
    float value = modulo((1.f / s) * d);
    if (!ceilingIsOdd(d / s)) value = 1.f - value;
    return scaled(value, maximum);
}
}

inline quint32 nonlinearUnitColor(LayerBlendMode mode, quint32 source, quint32 destination, quint32 maximum)
{
    using namespace NonlinearBlend;
    const float s = unit(source, maximum), d = unit(destination, maximum);
    const bool lowSource = source <= maximum / 2;
    switch (int(mode)) {
    case 60: return scaled(std::sqrt(s * d), maximum);
    case 61: return interpolation(source, destination, maximum);
    case 62: { const auto first = interpolation(source, destination, maximum); return interpolation(first, first, maximum); }
    case 63: return source == maximum ? maximum : arcTangent(destination, maximum - source, maximum);
    case 64: return destination == maximum ? maximum : arcTangent(source, maximum - destination, maximum);
    case 65: return scaled(std::pow(d, std::pow(2.f, 2.f * (.5f - s))), maximum);
    case 66:
    case 67:
        if (source > maximum / 2) {
            float curve = std::sqrt(d);
            if (int(mode) == 66 && destination <= maximum / 4) curve = ((16.f * d - 12.f) * d + 4.f) * d;
            return scaled(d + (2.f * s - 1.f) * (curve - d), maximum);
        }
        return scaled(d - (1.f - 2.f * s) * d * (1.f - d), maximum);
    case 68: return !destination || destination == maximum ? destination : scaled(std::pow(d, s), maximum);
    case 69: return maximum - gammaDark(maximum - source, maximum - destination, maximum);
    case 70:
    case 71: {
        // Native P-Norm uses raw integer colors and truncates its result.
        const float rawSource = float(source), rawDestination = float(destination);
        float value;
        if (int(mode) == 70) {
            value = std::pow(std::pow(rawDestination, 2.3333333333333333f) + std::pow(rawSource, 2.3333333333333333f), .428571428571434f);
        } else {
            const float s2 = rawSource * rawSource, d2 = rawDestination * rawDestination;
            value = std::sqrt(std::sqrt(d2 * d2 + s2 * s2));
        }
        return !(value <= float(maximum)) ? maximum : quint32(value);
    }
    case 72:
        if (lowSource) return scaled(1.f - std::pow(std::pow(1.f - d, 2.875f) + std::pow(1.f - 2.f * s, 2.875f), 1.f / 2.875f), maximum);
        return scaled(std::pow(std::pow(d, 2.875f) + std::pow(2.f * s - 1.f, 2.875f), 1.f / 2.875f), maximum);
    case 73: return scaled(s * (1.f - d) + std::sqrt(d), maximum);
    case 74:
        if (lowSource) return scaled((1.f - (1.f - s) * s) - (1.f - d) * (1.f - s), maximum);
        return scaled(s - (1.f - d) * (1.f - s) + (1.f - s) * (1.f - s), maximum);
    case 75: return source == maximum ? maximum : scaled(std::pow(d, (1.f - s) * 1.039999999f), maximum);
    case 76: return gammaDark(source, destination, maximum);
    case 77: return scaled(1.f - ((1.f - d) * s + std::sqrt(1.f - s)), maximum);
    case 78:
        if (lowSource) return scaled((1.f - s) * s + s * d, maximum);
        return scaled(s * d + s - s * s, maximum);
    case 79: {
        const float base = source == maximum ? 1e-12f : 1.f - s;
        return scaled(1.f - std::pow(base, d * 1.039999999f), maximum);
    }
    case 80: return destination % (source + 1u);
    case 81: return multiply(divisiveModuloContinuous(source, destination, maximum), source, maximum);
    case 82: return scaled(modulo((source ? 1.f / s : 1.f) * d), maximum);
    case 83: return divisiveModuloContinuous(source, destination, maximum);
    case 84: return source == maximum && !destination ? 0 : scaled(modulo(s + d), maximum);
    case 85: {
        if (source == maximum && !destination) return maximum;
        float value = modulo(s + d);
        if (destination && !ceilingIsOdd(s + d)) value = 1.f - value;
        return scaled(value, maximum);
    }
    case 86: return arcTangent(source, destination, maximum);
    case 87: return scaled(std::abs(std::sqrt(d) - std::sqrt(s)), maximum);
    default: return source;
    }
}

/** Native Greater alpha: the resulting alpha and the color weight. */
inline std::pair<quint32, quint32> greaterAlphaUnit(quint32 oldAlpha, quint32 alpha, quint32 maximum)
{
    using namespace NonlinearBlend;
    const float destination = unit(oldAlpha, maximum), source = unit(alpha, maximum);
    const float weight = 1.f / (1.f + std::exp(-40.f * (destination - source)));
    const float result = std::max(std::clamp(destination * weight + source * (1.f - weight), 0.f, 1.f), destination);
    const float opacity = 1.f - (1.f - result) / (1.f - destination + 1e-16f);
    return {scaled(result, maximum), scaled(opacity, maximum)};
}
}
#endif
