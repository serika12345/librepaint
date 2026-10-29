/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuRgba8Compositing_p.h"
#include "KisGpuRgba16Compositing_p.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
using Rgb = std::array<float, 3>;
float add(float a, float b) { const volatile float r = a + b; return r; }
float subtract(float a, float b) { const volatile float r = a - b; return r; }
float multiply(float a, float b) { const volatile float r = a * b; return r; }
float divide(float a, float b) { const volatile float r = a / b; return r; }
float smallest(const Rgb &v) { return std::min(std::min(v[0], v[1]), v[2]); }
float largest(const Rgb &v) { return std::max(std::max(v[0], v[1]), v[2]); }
float lightness(int model, const Rgb &v)
{
    if (model == 0) return add(add(multiply(.299f, v[0]), multiply(.587f, v[1])), multiply(.114f, v[2]));
    if (model == 1) return multiply(add(add(v[0], v[1]), v[2]), .33333333333333333333f);
    if (model == 2) return multiply(add(largest(v), smallest(v)), .5f);
    return largest(v);
}
float saturation(int model, const Rgb &v)
{
    const auto maximum = largest(v), minimum = smallest(v);
    const auto chroma = subtract(maximum, minimum);
    constexpr auto epsilon = std::numeric_limits<float>::epsilon();
    if (model == 0) return chroma;
    if (model == 1) return chroma > epsilon ? subtract(1.f, divide(minimum, lightness(model, v))) : 0.f;
    if (model == 2) {
        const auto light = multiply(add(maximum, minimum), .5f);
        const auto divisor = subtract(1.f, std::abs(subtract(multiply(2.f, light), 1.f)));
        return divisor > epsilon ? divide(chroma, divisor) : 0.f;
    }
    return maximum > epsilon ? divide(chroma, maximum) : 0.f;
}
Rgb toneMapping(int model, Rgb rgb)
{
    constexpr auto epsilon = std::numeric_limits<float>::epsilon();
    const auto light = lightness(model, rgb), minimum = smallest(rgb), maximum = largest(rgb);
    if (minimum < 0) {
        const auto stretch = subtract(light, minimum);
        if (light <= .00001f || stretch < epsilon) rgb = {};
        else {
            const auto inverse = divide(1.f, stretch);
            for (auto &v : rgb) v = add(light, multiply(multiply(subtract(v, light), light), inverse));
        }
    }
    if (maximum > 1) {
        const auto stretch = subtract(maximum, light);
        if (light > 1 || stretch < epsilon) {
            if (model != 3) rgb = {1, 1, 1};
            else for (auto &v : rgb) v = std::min(v, 1.f);
        } else {
            const auto complement = subtract(1.f, light), inverse = divide(1.f, stretch);
            for (auto &v : rgb) v = add(light, multiply(multiply(subtract(v, light), complement), inverse));
        }
    }
    return rgb;
}
Rgb addLightness(int model, Rgb rgb, float light)
{
    for (auto &v : rgb) v = add(v, light);
    return toneMapping(model, rgb);
}
Rgb setLightness(int model, const Rgb &rgb, float light)
{ return addLightness(model, rgb, subtract(light, lightness(model, rgb))); }
Rgb setSaturation(Rgb rgb, float sat)
{
    int low = 0, mid = 1, high = 2;
    if (rgb[mid] < rgb[low]) std::swap(mid, low);
    if (rgb[high] < rgb[mid]) std::swap(mid, high);
    if (rgb[mid] < rgb[low]) std::swap(mid, low);
    const auto chroma = subtract(rgb[high], rgb[low]);
    if (chroma <= std::numeric_limits<float>::epsilon()) return {};
    rgb[mid] = divide(multiply(subtract(rgb[mid], rgb[low]), sat), chroma);
    rgb[high] = sat;
    rgb[low] = 0;
    return rgb;
}
Rgb recombine(LayerBlendMode mode, const Rgb &source, Rgb destination)
{
    if (mode == LayerBlendMode::DarkerColor || mode == LayerBlendMode::LighterColor) {
        const auto old = lightness(0, destination), replacement = lightness(0, source);
        return (mode == LayerBlendMode::DarkerColor ? old > replacement : old < replacement) ? source : destination;
    }
    if (mode == LayerBlendMode::TangentNormalMap) {
        destination[0] = add(source[0], subtract(destination[0], .5f));
        destination[1] = add(source[1], subtract(destination[1], .5f));
        destination[2] = add(source[2], subtract(destination[2], 1.f));
        return destination;
    }
    if (mode == LayerBlendMode::ReorientedNormalMap) {
        if (source[2] <= .00001f) return destination;
        Rgb t{ subtract(multiply(2, source[0]), 1), subtract(multiply(2, source[1]), 1), multiply(2, source[2]) };
        Rgb u{ add(multiply(-2, destination[0]), 1), add(multiply(-2, destination[1]), 1), subtract(multiply(2, destination[2]), 1) };
        auto factor = divide(add(add(multiply(t[0], u[0]), multiply(t[1], u[1])), multiply(t[2], u[2])), t[2]);
        Rgb r;
        for (int i = 0; i < 3; ++i) r[i] = subtract(multiply(t[i], factor), u[i]);
        const auto length = add(add(multiply(r[0], r[0]), multiply(r[1], r[1])), multiply(r[2], r[2]));
        if (length == 0) return destination;
        const volatile float root = std::sqrt(length);
        factor = divide(1.f, root);
        for (int i = 0; i < 3; ++i) {
            const auto normalized = multiply(r[i], factor);
            const volatile double scaled = double(normalized) * .5;
            destination[i] = float(scaled + .5);
        }
        return destination;
    }
    if (mode == LayerBlendMode::LambertLighting || mode == LayerBlendMode::LambertLightingGamma22) {
        const bool gamma = mode == LayerBlendMode::LambertLightingGamma22;
        for (int i = 0; i < 3; ++i) {
            const auto product = multiply(source[i], destination[i]);
            const volatile double scaled = double(product) * (gamma ? 2.0 : 1.0 / .215686);
            const float value = float(scaled);
            if (value > 1) {
                const volatile double shifted = double(value) - 1.0;
                const volatile double squared = shifted * shifted;
                const volatile double weighted = squared * (gamma ? .4 : .01925);
                destination[i] = float(1.0 + weighted);
            } else destination[i] = value;
        }
        return toneMapping(1, destination);
    }
    int model, operation;
    if (mode <= LayerBlendMode::DecreaseLightnessHsy) {
        model = 0;
        operation = int(mode) - int(LayerBlendMode::ColorHsy);
        operation = operation == 2 ? 8 : operation > 2 ? operation - 1 : operation;
    } else {
        const auto offset = int(mode) - int(LayerBlendMode::ColorHsi);
        model = 1 + offset / 8;
        operation = offset % 8;
    }
    const auto light = lightness(model, destination);
    switch (operation) {
    case 0: return setLightness(model, source, light);
    case 1: return setLightness(model, setSaturation(source, saturation(model, destination)), light);
    case 2: return setLightness(model, setSaturation(destination, saturation(model, source)), light);
    case 3: {
        const auto old = saturation(model, destination), weight = saturation(model, source);
        const auto sat = add(multiply(subtract(1.f, old), weight), old);
        return setLightness(model, setSaturation(destination, sat), light);
    }
    case 4: {
        const auto sat = multiply(saturation(model, destination), saturation(model, source));
        return setLightness(model, setSaturation(destination, sat), light);
    }
    case 5: return setLightness(model, destination, lightness(model, source));
    case 6: return addLightness(model, destination, lightness(model, source));
    case 7: return addLightness(model, destination, subtract(lightness(model, source), 1.f));
    default:
        for (int i = 0; i < 3; ++i) destination[i] = add(light, multiply(source[i], subtract(1.f, light)));
        return destination;
    }
}
quint8 componentByte(float value)
{
    const auto scaled = multiply(value, 255.f);
    const auto bounded = std::max(0.f, std::min(255.f, scaled));
    return quint8(int(add(bounded, .5f)));
}
}

RgbaPixel coupledRgba8Pixel(RgbaPixel destination, const RgbaPixel &source,
                           quint8 opacity, quint8 mask, LayerBlendMode mode)
{
    if (mask != 15 && !destination[3]) destination = {};
    const auto alpha = multiplyThreeBytes(source[3], 255, opacity);
    const auto oldAlpha = destination[3];
    const bool locked = !(mask & 8);
    const auto nextAlpha = locked ? oldAlpha : alpha + oldAlpha - multiplyByte(alpha, oldAlpha);
    Rgb mixed{};
    if (alpha && oldAlpha && (mask & 7)) {
        Rgb s, d;
        for (int i = 0; i < 3; ++i) { s[i] = divide(source[i], 255.f); d[i] = divide(destination[i], 255.f); }
        mixed = recombine(mode, s, d);
    }
    for (int i = 0; i < 3; ++i) {
        if (!(mask & (1 << i))) continue;
        const auto product = componentByte(mixed[i]);
        if (locked) {
            if (oldAlpha) destination[i] = blendByte(destination[i], product, alpha);
        } else if (nextAlpha) {
            const auto weighted = quint8(multiplyThreeBytes(255 - alpha, oldAlpha, destination[i]) +
                multiplyThreeBytes(255 - oldAlpha, alpha, source[i]) + multiplyThreeBytes(oldAlpha, alpha, product));
            destination[i] = quint8((quint32(weighted) * 255u + nextAlpha / 2u) / nextAlpha);
        }
    }
    destination[3] = quint16(nextAlpha);
    return destination;
}

RgbaPixel coupledRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                            quint16 alpha, quint8 mask, LayerBlendMode mode)
{
    constexpr quint32 unit = 65535;
    if (mask != 15 && !destination[3]) destination = {};
    const auto oldAlpha = destination[3];
    const bool locked = !(mask & 8);
    const auto nextAlpha = locked ? oldAlpha : alpha + oldAlpha - multiplyWord(alpha, oldAlpha);
    Rgb mixed{};
    if (alpha && oldAlpha && (mask & 7)) {
        Rgb s, d;
        for (int i = 0; i < 3; ++i) { s[i] = divide(source[i], 65535.f); d[i] = divide(destination[i], 65535.f); }
        mixed = recombine(mode, s, d);
    }
    for (int i = 0; i < 3; ++i) {
        if (!(mask & (1 << i))) continue;
        const auto scaled = multiply(mixed[i], 65535.f);
        const auto product = quint16(int(add(std::clamp(scaled, 0.f, 65535.f), .5f)));
        if (locked) {
            if (oldAlpha) destination[i] = interpolateWord(destination[i], product, alpha);
        } else if (nextAlpha) {
            const auto weighted = quint16(multiplyThreeWords(unit - alpha, oldAlpha, destination[i]) +
                multiplyThreeWords(unit - oldAlpha, alpha, source[i]) + multiplyThreeWords(oldAlpha, alpha, product));
            destination[i] = divideWord(weighted, nextAlpha);
        }
    }
    destination[3] = quint16(nextAlpha);
    return destination;
}
}
