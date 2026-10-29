/*
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-FileCopyrightText: 2004 Sun Microsystems, Inc.
 * SPDX-FileCopyrightText: 1993 Sun Microsystems, Inc.
 * SPDX-License-Identifier: GPL-2.0-or-later AND SunPro
 */
#include "KisGpuRgba16Compositing_p.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
constexpr quint32 Unit = 65535;
quint32 glowWord(quint32 source, quint32 destination)
{ return destination == Unit ? Unit : std::min(Unit, divideWord(multiplyWord(source, source), Unit - destination)); }
quint32 heatWord(quint32 source, quint32 destination)
{
    if (source == Unit) return Unit;
    if (!destination) return 0;
    return Unit - std::min(Unit, divideWord(multiplyWord(Unit - source, Unit - source), destination));
}
quint32 heatGlowWord(quint32 source, quint32 destination)
{ return source + destination > Unit ? heatWord(source, destination) : source ? glowWord(source, destination) : 0; }
quint32 freezeReflectWord(quint32 source, quint32 destination)
{ return source + destination > Unit ? heatWord(destination, source) : destination ? glowWord(destination, source) : 0; }
quint32 glowHeatWord(quint32 source, quint32 destination)
{
    if (destination == Unit) return Unit;
    return source + destination > Unit ? glowWord(source, destination) : heatWord(source, destination);
}
quint32 penumbraWord(quint32 source, quint32 destination)
{
    if (destination == Unit) return Unit;
    if (source + destination < Unit) return std::min(Unit, divideWord(source, Unit - destination)) / 2;
    return source ? Unit - std::min(Unit, divideWord(Unit - destination, source) / 2) : 0;
}
double colorAdd(double a, double b) { const volatile double result = a + b; return result; }
double colorSubtract(double a, double b) { const volatile double result = a - b; return result; }
double colorMultiply(double a, double b) { const volatile double result = a * b; return result; }
double colorDivide(double a, double b) { const volatile double result = a / b; return result; }
double normalizedWordColor(quint32 word)
{
    // Native integer-to-real colors use the single-precision lookup value.
    const volatile float normalized = float(word) / 65535.0f;
    return normalized;
}
quint32 scaledWordColor(double value)
{
    const double scaled = std::clamp(colorMultiply(value, 65535.0), 0.0, 65535.0);
    return quint32(colorAdd(scaled, 0.5));
}
double moduloUnit(double value)
{
    const double divisor = colorAdd(1.0, std::numeric_limits<double>::epsilon());
    const double quotient = std::floor(colorDivide(value, divisor));
    return colorSubtract(value, colorMultiply(quotient, divisor));
}
double moduloShiftValue(double source, double destination)
{
    if (source == 1.0 && destination == 0.0) return 0.0;
    return moduloUnit(colorAdd(destination, source));
}
double divisiveModuloValue(double source, double destination, bool realEpsilon)
{
    const double divisor = source == 0.0 ? (realEpsilon ? std::numeric_limits<double>::epsilon() : 1.0) : source;
    return moduloUnit(colorMultiply(colorDivide(1.0, divisor), destination));
}
quint32 divisiveModuloContinuousWord(quint32 sourceWord, quint32 destinationWord)
{
    if (!destinationWord) return 0;
    const auto source = normalizedWordColor(sourceWord), destination = normalizedWordColor(destinationWord);
    // The native zero-source branch returns the nested real result directly
    // as an integer, before the outer word-scale conversion.
    if (!sourceWord) return quint32(divisiveModuloValue(source, destination, true));
    auto value = divisiveModuloValue(source, destination, true);
    if (!(quint32(std::ceil(colorDivide(destination, source))) & 1u)) value = colorSubtract(1.0, value);
    return scaledWordColor(value);
}
quint32 softLightWordColor(quint32 source, quint32 destination, bool svg)
{
    const auto s = normalizedWordColor(source), d = normalizedWordColor(destination);
    if (s > 0.5) {
        const auto curve = !svg || d > 0.25 ? std::sqrt(d) :
            colorMultiply(colorAdd(colorMultiply(colorSubtract(colorMultiply(16.0, d), 12.0), d), 4.0), d);
        return scaledWordColor(colorAdd(d, colorMultiply(colorSubtract(colorMultiply(2.0, s), 1.0), colorSubtract(curve, d))));
    }
    return scaledWordColor(colorSubtract(d, colorMultiply(colorMultiply(colorSubtract(1.0, colorMultiply(2.0, s)), d), colorSubtract(1.0, d))));
}
quint32 ifsWordColor(LayerBlendMode mode, quint32 source, quint32 destination)
{
    const auto s = normalizedWordColor(source), d = normalizedWordColor(destination);
    const auto inverseSource = colorSubtract(1.0, s), inverseDestination = colorSubtract(1.0, d);
    switch (mode) {
    case LayerBlendMode::TintIFSIllusions:
        return scaledWordColor(colorAdd(colorMultiply(s, inverseDestination), std::sqrt(d)));
    case LayerBlendMode::ShadeIFSIllusions:
        return scaledWordColor(colorSubtract(1.0, colorAdd(colorMultiply(inverseDestination, s), std::sqrt(inverseSource))));
    case LayerBlendMode::FogLightenIFSIllusions:
        if (s < 0.5) return scaledWordColor(colorSubtract(colorSubtract(1.0, colorMultiply(inverseSource, s)), colorMultiply(inverseDestination, inverseSource)));
        return scaledWordColor(colorAdd(colorSubtract(s, colorMultiply(inverseDestination, inverseSource)), colorMultiply(inverseSource, inverseSource)));
    default:
        if (s < 0.5) return scaledWordColor(colorAdd(colorMultiply(inverseSource, s), colorMultiply(s, d)));
        return scaledWordColor(colorSubtract(colorAdd(colorMultiply(s, d), s), colorMultiply(s, s)));
    }
}
// Color powers and Greater supply finite exponents in [-40,40]. Reduction and coefficients
// derive from OpenLibm e_exp.c (SunPro); retained notice: LICENSES/SunPro.txt.
// https://github.com/JuliaMath/openlibm/blob/5fe399749f9276eaa0b8403e507470da05cbbb3f/src/e_exp.c
double finiteColorExp(double value) {
    const int magnitude = int(colorAdd(colorMultiply(std::abs(value), 0x1.71547652b82fep0), .5));
    const int exponent = std::signbit(value) ? -magnitude : magnitude;
    const auto high = colorSubtract(value, colorMultiply(double(exponent), 0x1.62e42fee00000p-1));
    const auto low = colorMultiply(double(exponent), 0x1.a39ef35793c76p-33);
    const auto reduced = colorSubtract(high, low), square = colorMultiply(reduced, reduced);
    auto polynomial = colorAdd(-0x1.bbd41c5d26bf1p-20, colorMultiply(square, 0x1.6376972bea4d0p-25));
    polynomial = colorAdd(0x1.1566aaf25de2cp-14, colorMultiply(square, polynomial));
    polynomial = colorAdd(-0x1.6c16c16bebd93p-9, colorMultiply(square, polynomial));
    polynomial = colorAdd(0x1.555555555553ep-3, colorMultiply(square, polynomial));
    const auto correction = colorSubtract(reduced, colorMultiply(square, polynomial));
    if (!exponent) return colorSubtract(1.0, colorSubtract(colorDivide(colorMultiply(reduced, correction), colorSubtract(correction, 2.0)), reduced));
    const auto result = colorSubtract(1.0, colorSubtract(colorSubtract(low, colorDivide(colorMultiply(reduced, correction), colorSubtract(2.0, correction))), high));
    return std::ldexp(result, exponent);
}
// Positive normal inputs supplied by word color powers. The reduction and
// coefficients derive from OpenLibm e_log.c; notice: LICENSES/SunPro.txt.
// https://github.com/JuliaMath/openlibm/blob/5fe399749f9276eaa0b8403e507470da05cbbb3f/src/e_log.c
double positiveColorLog(double value)
{
    quint64 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    const auto fraction = quint32(bits >> 32) & 0xfffffu;
    const auto reduction = (fraction + 0x95f64u) & 0x100000u;
    const int exponent = int(bits >> 52) - 1023 + int(reduction >> 20);
    bits = (bits & 0xffffffffu) | (quint64(fraction | (reduction ^ 0x3ff00000u)) << 32);
    std::memcpy(&value, &bits, sizeof(value));
    const auto f = colorSubtract(value, 1.0);
    const auto high = colorMultiply(double(exponent), 0x1.62e42fee00000p-1);
    const auto low = colorMultiply(double(exponent), 0x1.a39ef35793c76p-33);
    if (((2u + fraction) & 0xfffffu) < 3u) {
        if (f == 0.0) return colorAdd(high, low);
        const auto r = colorMultiply(colorMultiply(f, f), colorSubtract(.5, colorMultiply(0x1.5555555555555p-2, f)));
        return colorSubtract(high, colorSubtract(colorSubtract(r, low), f));
    }
    const auto s = colorDivide(f, colorAdd(2.0, f));
    const auto z = colorMultiply(s, s), w = colorMultiply(z, z);
    const auto t1 = colorMultiply(w, colorAdd(0x1.999999997fa04p-2,
        colorMultiply(w, colorAdd(0x1.c71c51d8e78afp-3, colorMultiply(w, 0x1.39a09d078c69fp-3)))));
    const auto t2 = colorMultiply(z, colorAdd(0x1.5555555555593p-1,
        colorMultiply(w, colorAdd(0x1.2492494229359p-2,
            colorMultiply(w, colorAdd(0x1.7466496cb03dep-3, colorMultiply(w, 0x1.2f112df3e5244p-3)))))));
    const auto r = colorAdd(t2, t1);
    if (((int(fraction) - 0x6147a) | (0x6b851 - int(fraction))) > 0) {
        const auto halfSquare = colorMultiply(colorMultiply(.5, f), f);
        return colorSubtract(high, colorSubtract(colorSubtract(halfSquare,
            colorAdd(colorMultiply(s, colorAdd(halfSquare, r)), low)), f));
    }
    return colorSubtract(high, colorSubtract(colorSubtract(colorMultiply(s, colorSubtract(f, r)), low), f));
}
double wordColorPower(double base, double exponent)
{
    if (exponent == 0.0 || base == 1.0) return 1.0;
    if (base == 0.0) return 0.0;
    if (exponent == 1.0) return base;
    const auto power = colorMultiply(exponent, positiveColorLog(base));
    // Fractional consumers quantize to a word, optionally after a complement.
    // exp(-40) is below either rounding boundary; raw P-Norm inputs are >=1.
    return power < -40.0 ? 0.0 : finiteColorExp(power);
}
quint32 powerWordColor(LayerBlendMode mode, quint32 source, quint32 destination)
{
    const auto s = normalizedWordColor(source), d = normalizedWordColor(destination);
    switch (mode) {
    case LayerBlendMode::PNormA:
        return quint32(std::min(double(Unit), wordColorPower(colorAdd(
            wordColorPower(double(destination), 2.3333333333333333),
            wordColorPower(double(source), 2.3333333333333333)), .428571428571434)));
    case LayerBlendMode::PNormB: {
        const auto s2 = source * source, d2 = destination * destination;
        return quint32(std::min(double(Unit), std::sqrt(std::sqrt(colorAdd(
            colorMultiply(double(d2), double(d2)), colorMultiply(double(s2), double(s2)))))));
    }
    case LayerBlendMode::GammaIllumination:
        return Unit - powerWordColor(LayerBlendMode::GammaDark, Unit - source, Unit - destination);
    case LayerBlendMode::GammaDark:
        if (!destination || destination == Unit) return destination;
        return !source ? 0u : scaledWordColor(wordColorPower(d, colorDivide(1.0, s)));
    case LayerBlendMode::GammaLight:
        if (!destination || destination == Unit) return destination;
        return scaledWordColor(wordColorPower(d, s));
    case LayerBlendMode::SoftLightIFSIllusions:
        return scaledWordColor(wordColorPower(d, wordColorPower(2.0, colorMultiply(2.0, colorSubtract(.5, s)))));
    case LayerBlendMode::EasyDodge:
        return source == Unit ? Unit : scaledWordColor(wordColorPower(d,
            colorMultiply(colorSubtract(1.0, s), 1.039999999)));
    case LayerBlendMode::SuperLight:
        if (s < .5) return scaledWordColor(colorSubtract(1.0, wordColorPower(colorAdd(
            wordColorPower(colorSubtract(1.0, d), 2.875),
            wordColorPower(colorSubtract(1.0, colorMultiply(2.0, s)), 2.875)), 1.0 / 2.875)));
        return scaledWordColor(wordColorPower(colorAdd(wordColorPower(d, 2.875),
            wordColorPower(colorSubtract(colorMultiply(2.0, s), 1.0), 2.875)), 1.0 / 2.875));
    default:
        return scaledWordColor(colorSubtract(1.0, wordColorPower(colorSubtract(1.0,
            source == Unit ? .999999999999 : s), colorMultiply(d, 1.039999999))));
    }
}
quint16 scaledWordColor(float value)
{
    const volatile float scaled = value * 65535.f;
    const volatile float rounded = std::clamp(float(scaled), 0.f, 65535.f) + .5f;
    return quint16(int(rounded));
}
quint16 saiWordChannel(quint16 destination, quint16 source, quint16 alpha)
{
    const volatile float s = float(source) / 65535.f, d = float(destination) / 65535.f, a = float(alpha) / 65535.f;
    const volatile float product = s * a;
    const volatile float combined = product + d;
    return scaledWordColor(float(combined));
}
quint32 channel(LayerBlendMode mode, quint32 source, quint32 destination)
{
    const auto dodge = [](quint32 s, quint32 d) { return s == Unit ? (d ? Unit : 0u) : std::min(Unit, divideWord(d, Unit - s)); };
    const auto burn = [](quint32 s, quint32 d) { return d == Unit ? Unit : !s ? 0u : Unit - std::min(Unit, divideWord(Unit - d, s)); };
    switch (mode) {
    case LayerBlendMode::Multiply: return multiplyWord(source, destination);
    case LayerBlendMode::Screen: return source + destination - multiplyWord(source, destination);
    case LayerBlendMode::Darken: return std::min(source, destination);
    case LayerBlendMode::Lighten: return std::max(source, destination);
    case LayerBlendMode::Addition:
    case LayerBlendMode::LinearDodge: return std::min(Unit, source + destination);
    case LayerBlendMode::Difference:
    case LayerBlendMode::Equivalence: return quint32(qAbs(int(destination) - int(source)));
    case LayerBlendMode::Exclusion: return qBound(0, int(source + destination) - 2 * int(multiplyWord(source, destination)), int(Unit));
    case LayerBlendMode::Subtract: return quint32(std::max(0, int(destination) - int(source)));
    case LayerBlendMode::LinearBurn:
    case LayerBlendMode::InverseSubtract: return quint32(std::max(0, int(source + destination) - int(Unit)));
    case LayerBlendMode::GrainMerge: return quint32(qBound(0, int(source + destination) - 32767, int(Unit)));
    case LayerBlendMode::GrainExtract: return quint32(qBound(0, int(destination) - int(source) + 32767, int(Unit)));
    case LayerBlendMode::Overlay: std::swap(source, destination); [[fallthrough]];
    case LayerBlendMode::HardLight:
        if (source == 32767) return destination;
        return source > 32767 ? 2 * source - Unit + destination - multiplyWord(2 * source - Unit, destination) : multiplyWord(2 * source, destination);
    case LayerBlendMode::PinLight: return quint32(std::max(2 * int(source) - int(Unit), std::min(int(destination), 2 * int(source))));
    case LayerBlendMode::Divide: return !destination ? 0 : !source ? Unit : std::min(Unit, divideWord(destination, source));
    case LayerBlendMode::ColorDodge: return dodge(source, destination);
    case LayerBlendMode::ColorBurn: return burn(source, destination);
    case LayerBlendMode::HardMix: return destination > 32767 ? dodge(source, destination) : burn(source, destination);
    case LayerBlendMode::VividLight:
        if (source < 32767) return !source ? (destination == Unit ? Unit : 0) : Unit - std::min(Unit, (Unit - destination) * Unit / (2 * source));
        return source == Unit ? (destination ? Unit : 0) : std::min(Unit, destination * Unit / (2 * (Unit - source)));
    case LayerBlendMode::LinearLight: return quint32(qBound(0, int(2 * source + destination) - int(Unit), int(Unit)));
    case LayerBlendMode::Allanon: return (source + destination) * 32767 / Unit;
    case LayerBlendMode::HardMixPhotoshop: return source + destination > Unit ? Unit : 0;
    case LayerBlendMode::BitwiseXor: return source ^ destination;
    case LayerBlendMode::BitwiseOr: return source | destination;
    case LayerBlendMode::BitwiseAnd: return source & destination;
    case LayerBlendMode::BitwiseNand: return Unit ^ (source & destination);
    case LayerBlendMode::BitwiseNor: return Unit ^ (source | destination);
    case LayerBlendMode::BitwiseXnor: return Unit ^ (source ^ destination);
    case LayerBlendMode::Implication: return source | (Unit - destination);
    case LayerBlendMode::NotImplication: return (Unit - source) & destination;
    case LayerBlendMode::Converse: return (Unit - source) | destination;
    case LayerBlendMode::NotConverse: return source & (Unit - destination);
    case LayerBlendMode::Reflect: return glowWord(destination, source);
    case LayerBlendMode::Glow: return glowWord(source, destination);
    case LayerBlendMode::Freeze: return heatWord(destination, source);
    case LayerBlendMode::Heat: return heatWord(source, destination);
    case LayerBlendMode::GlowHeat: return glowHeatWord(source, destination);
    case LayerBlendMode::HeatGlow: return heatGlowWord(source, destination);
    case LayerBlendMode::ReflectFreeze: return glowHeatWord(destination, source);
    case LayerBlendMode::FreezeReflect: return freezeReflectWord(source, destination);
    case LayerBlendMode::QuadraticHybrid:
        return (freezeReflectWord(source, destination) + heatGlowWord(source, destination)) * 32767u / Unit;
    case LayerBlendMode::Parallel:
        return source && destination ? quint32(std::min(quint64(Unit), 2 * quint64(Unit) * Unit /
            (quint64(divideWord(Unit, source)) + divideWord(Unit, destination)))) : 0;
    case LayerBlendMode::HardOverlay:
        if (!destination) return 0;
        if (source == Unit) return Unit;
        return source >= 32767 ? std::min(Unit, destination * Unit / (2 * (Unit - source))) : destination * 2 * source / Unit;
    case LayerBlendMode::SoftLightPegtopDelphi:
        return std::min(Unit, multiplyWord(destination, source + destination - multiplyWord(source, destination)) +
            multiplyWord(multiplyWord(source, destination), Unit - destination));
    case LayerBlendMode::Negation: return Unit - qAbs(int(Unit) - int(source) - int(destination));
    case LayerBlendMode::PenumbraA: return penumbraWord(destination, source);
    case LayerBlendMode::PenumbraB: return penumbraWord(source, destination);
    case LayerBlendMode::FlatLight:
        return !source ? 0 : destination > source ? penumbraWord(source, destination) : penumbraWord(destination, source);
    case LayerBlendMode::HardMixSofterPhotoshop:
        return quint32(qBound(0, 3 * int(destination) - 2 * (int(Unit) - int(source)), int(Unit)));
    case LayerBlendMode::GeometricMean:
        return scaledWordColor(std::sqrt(colorMultiply(normalizedWordColor(source), normalizedWordColor(destination))));
    case LayerBlendMode::SoftLightSvg: return softLightWordColor(source, destination, true);
    case LayerBlendMode::SoftLightPhotoshop: return softLightWordColor(source, destination, false);
    case LayerBlendMode::SoftLightIFSIllusions:
    case LayerBlendMode::GammaLight:
    case LayerBlendMode::GammaIllumination:
    case LayerBlendMode::GammaDark:
    case LayerBlendMode::EasyDodge:
    case LayerBlendMode::PNormA:
    case LayerBlendMode::PNormB:
    case LayerBlendMode::SuperLight:
    case LayerBlendMode::EasyBurn: return powerWordColor(mode, source, destination);
    case LayerBlendMode::TintIFSIllusions:
    case LayerBlendMode::ShadeIFSIllusions:
    case LayerBlendMode::FogLightenIFSIllusions:
    case LayerBlendMode::FogDarkenIFSIllusions: return ifsWordColor(mode, source, destination);
    case LayerBlendMode::AdditiveSubtractive:
        return scaledWordColor(std::abs(colorSubtract(std::sqrt(normalizedWordColor(destination)), std::sqrt(normalizedWordColor(source)))));
    case LayerBlendMode::Modulo: return destination % (source + 1u);
    case LayerBlendMode::ModuloContinuous:
        return multiplyWord(divisiveModuloContinuousWord(source, destination), source);
    case LayerBlendMode::DivisiveModulo:
        return scaledWordColor(divisiveModuloValue(normalizedWordColor(source), normalizedWordColor(destination), false));
    case LayerBlendMode::DivisiveModuloContinuous: return divisiveModuloContinuousWord(source, destination);
    case LayerBlendMode::ModuloShift:
        return scaledWordColor(moduloShiftValue(normalizedWordColor(source), normalizedWordColor(destination)));
    case LayerBlendMode::ModuloShiftContinuous: {
        if (source == Unit && !destination) return Unit;
        const auto s = normalizedWordColor(source), d = normalizedWordColor(destination);
        auto value = moduloShiftValue(s, d);
        if (destination && !(quint32(std::ceil(colorAdd(s, d))) & 1u)) value = colorSubtract(1.0, value);
        return scaledWordColor(value);
    }
    default: return source;
    }
}
}
RgbaPixel separableRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                             quint16 alpha, quint8 mask, LayerBlendMode mode)
{
    const quint32 oldAlpha = destination[3];
    if (mode == LayerBlendMode::Erase) {
        destination[3] = multiplyWord(Unit - alpha, oldAlpha);
        return destination;
    }
    if (mask != 15 && !oldAlpha) destination = {};
    const bool locked = !(mask & 8u);
    if (mode >= LayerBlendMode::CopyRed && mode <= LayerBlendMode::CopyBlue) {
        const int index = int(mode) - int(LayerBlendMode::CopyRed);
        if (mask & (1u << index)) destination[index] = interpolateWord(destination[index], source[index], alpha);
        return destination;
    }
    if (mode == LayerBlendMode::DestinationIn || mode == LayerBlendMode::DestinationAtop) {
        if (mode == LayerBlendMode::DestinationIn) {
            if (!locked) destination[3] = multiplyWord(oldAlpha, alpha);
        } else {
            if (source[3]) for (int index = 0; index < 3; ++index)
                if (mask & (1u << index)) destination[index] = interpolateWord(source[index], destination[index], oldAlpha);
            if (!locked) destination[3] = alpha;
        }
        return destination;
    }
    if (!alpha) return destination;
    if (mode == LayerBlendMode::Behind) {
        if (oldAlpha == Unit) return destination;
        const auto unionAlpha = oldAlpha + alpha - multiplyWord(oldAlpha, alpha);
        for (int index = 0; index < 3; ++index) {
            if (!(mask & (1u << index))) continue;
            const auto blended = interpolateWord(multiplyWord(source[index], alpha), destination[index], oldAlpha);
            destination[index] = oldAlpha ? divideWord(blended, unionAlpha) : source[index];
        }
        if (!locked) destination[3] = unionAlpha;
        return destination;
    }
    const quint32 nextAlpha = locked ? oldAlpha : alpha + oldAlpha - multiplyWord(alpha, oldAlpha);
    for (int index = 0; index < 3; ++index) {
        if (!(mask & (1u << index))) continue;
        const auto product = channel(mode, source[index], destination[index]);
        if (locked || oldAlpha == Unit) {
            if (oldAlpha) destination[index] = interpolateWord(destination[index], product, alpha);
        } else if (!oldAlpha) destination[index] = source[index];
        else if (alpha == Unit) destination[index] = interpolateWord(source[index], product, oldAlpha);
        else {
            const auto value = quint16(multiplyThreeWords(Unit - alpha, oldAlpha, destination[index]) +
                multiplyThreeWords(Unit - oldAlpha, alpha, source[index]) + multiplyThreeWords(oldAlpha, alpha, product));
            destination[index] = divideWord(value, nextAlpha);
        }
    }
    destination[3] = nextAlpha;
    return destination;
}

RgbaPixel specialAlphaRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                                 quint16 opacity, quint8 mask, quint16 selection, LayerBlendMode mode)
{
    const auto oldAlpha = destination[3];
    const bool locked = !(mask & 8u);
    if (mask != 15 && !oldAlpha) destination = {};
    if (mode == LayerBlendMode::Marker) {
        const auto selected = multiplyWord(source[3], selection);
        const auto alpha = multiplyWord(selected, opacity);
        const auto weight = oldAlpha ? std::min(Unit, divideWord(alpha, oldAlpha)) : Unit;
        for (int channel = 0; channel < 3; ++channel)
            if (mask & (1u << channel)) destination[channel] = interpolateWord(destination[channel], source[channel], weight);
        if (!locked) destination[3] = opacity > oldAlpha ? interpolateWord(oldAlpha, opacity, selected) : oldAlpha;
        return destination;
    }
    const auto alpha = quint16(multiplyThreeWords(source[3], selection, opacity));
    if (mode == LayerBlendMode::Greater) {
        if (!alpha || oldAlpha == Unit) return destination;
        const volatile float d = float(oldAlpha) / 65535.f, a = float(alpha) / 65535.f;
        const volatile float difference = d - a;
        const float weight = float(colorDivide(1.0, colorAdd(1.0, finiteColorExp(colorMultiply(-40.0, difference)))));
        const volatile float first = d * weight;
        const auto second = colorMultiply(a, colorSubtract(1.0, weight));
        const float combined = std::max(float(d), std::clamp(float(colorAdd(first, second)), 0.f, 1.f));
        const volatile float inverseAlpha = 1.f - combined, inverseDestination = 1.f - d;
        const volatile float denominator = inverseDestination + 1e-16f;
        const volatile float quotient = inverseAlpha / denominator;
        const volatile float fakeOpacity = 1.f - quotient;
        const auto nextAlpha = scaledWordColor(combined), colorWeight = scaledWordColor(float(fakeOpacity));
        for (int channel = 0; channel < 3; ++channel) {
            if (!(mask & (1u << channel))) continue;
            if (!oldAlpha) destination[channel] = source[channel];
            else if (nextAlpha) {
                const auto mixed = interpolateWord(multiplyWord(destination[channel], oldAlpha), source[channel], colorWeight);
                destination[channel] = std::min(Unit, divideWord(mixed, nextAlpha));
            }
        }
        if (!locked) destination[3] = nextAlpha;
        return destination;
    }
    const auto nextAlpha = alpha + oldAlpha - multiplyWord(alpha, oldAlpha);
    if (locked ? oldAlpha != 0 : nextAlpha != 0)
        for (int channel = 0; channel < 3; ++channel)
            if (mask & (1u << channel)) destination[channel] = saiWordChannel(destination[channel], source[channel], alpha);
    if (!locked) destination[3] = quint16(nextAlpha);
    return destination;
}
}
