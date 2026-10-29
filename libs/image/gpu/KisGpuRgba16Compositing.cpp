/*
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuRgba16Compositing_p.h"
#include <algorithm>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
constexpr quint32 Unit = 65535;
quint32 multiplyWord(quint32 a, quint32 b)
{
    const auto product = a * b + 32768u;
    return ((product >> 16) + product) >> 16;
}
quint32 tripleWord(quint32 a, quint32 b, quint32 c)
{ return quint64(a) * b * c / (quint64(Unit) * Unit); }
quint32 divideWord(quint32 a, quint32 b)
{ return (a * Unit + b / 2) / b; }
quint16 interpolateWord(quint16 destination, quint16 source, quint32 alpha)
{ return qint64(destination) + (qint64(source) - destination) * alpha / Unit; }
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
    default: return source;
    }
}
}
RgbaPixel separableRgba16Pixel(RgbaPixel destination, const RgbaPixel &source,
                             quint16 alpha, quint8 mask, LayerBlendMode mode)
{
    const quint32 oldAlpha = destination[3];
    if (mask != 15 && !oldAlpha) destination = {};
    if (!alpha) return destination;
    const bool locked = !(mask & 8u);
    const quint32 nextAlpha = locked ? oldAlpha : alpha + oldAlpha - multiplyWord(alpha, oldAlpha);
    for (int index = 0; index < 3; ++index) {
        if (!(mask & (1u << index))) continue;
        const auto product = channel(mode, source[index], destination[index]);
        if (locked || oldAlpha == Unit) {
            if (oldAlpha) destination[index] = interpolateWord(destination[index], product, alpha);
        } else if (!oldAlpha) destination[index] = source[index];
        else if (alpha == Unit) destination[index] = interpolateWord(source[index], product, oldAlpha);
        else {
            const auto value = quint16(tripleWord(Unit - alpha, oldAlpha, destination[index]) +
                tripleWord(Unit - oldAlpha, alpha, source[index]) + tripleWord(oldAlpha, alpha, product));
            destination[index] = divideWord(value, nextAlpha);
        }
    }
    destination[3] = nextAlpha;
    return destination;
}
}
