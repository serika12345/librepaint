/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuNonlinearRgba8_p.h"
#include "KisGpuRgba8Compositing_p.h"
#include "KisGpuRgba8AlphaBlend_p.h"
#include "KisGpuRgba16Compositing_p.h"
#include "KisGpuCopyCompositing_p.h"
#include <QFile>
#include <cmath>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
const QByteArray &Detail::rgba16PowerValues()
{
    static const auto bytes = [] {
        QFile resource(QStringLiteral(":/librepaint/gpu/blend/rgba16-power.bin"));
        if (!resource.open(QIODevice::ReadOnly)) throw std::logic_error("GPU RGBA16 power values are unavailable");
        auto result = resource.readAll();
        if (result.size() != Rgba16PowerBytes) throw std::logic_error("GPU RGBA16 power values are incomplete");
        return result;
    }();
    return bytes;
}
const QByteArray &Detail::rgba16CosineValues()
{
    static const auto bytes = [] {
        QFile resource(QStringLiteral(":/librepaint/gpu/blend/rgba16-cosine.bin"));
        if (!resource.open(QIODevice::ReadOnly)) throw std::logic_error("GPU RGBA16 cosine values are unavailable");
        auto result = resource.readAll();
        if (result.size() != Rgba16CosineBytes) throw std::logic_error("GPU RGBA16 cosine values are incomplete");
        return result;
    }();
    return bytes;
}
const QByteArray &Detail::nonlinearRgba8Values()
{
    static const auto bytes = [] {
        QFile resource(QStringLiteral(":/librepaint/gpu/blend/rgba8-nonlinear.bin"));
        if (!resource.open(QIODevice::ReadOnly)) throw std::logic_error("GPU nonlinear RGBA8 values are unavailable");
        auto result = resource.readAll();
        if (result.size() != NonlinearRgba8Bytes) throw std::logic_error("GPU nonlinear RGBA8 values are incomplete");
        return result;
    }();
    return bytes;
}
namespace {
using Detail::multiplyByte;
using Detail::multiplyThreeBytes;
using Detail::blendByte;
quint32 divideByte(quint32 a, quint32 b)
{ return (a * 255u + b / 2u) / b; }
quint32 glowByte(quint32 source, quint32 destination)
{
    return destination == 255 ? 255 : qMin(255u, divideByte(multiplyByte(source, source), 255 - destination));
}
quint32 heatByte(quint32 source, quint32 destination)
{
    if (source == 255) return 255;
    if (!destination) return 0;
    return 255 - qMin(255u, divideByte(multiplyByte(255 - source, 255 - source), destination));
}
quint32 heatGlowByte(quint32 source, quint32 destination)
{
    if (source + destination > 255) return heatByte(source, destination);
    return source ? glowByte(source, destination) : 0;
}
quint32 freezeReflectByte(quint32 source, quint32 destination)
{
    if (source + destination > 255) return heatByte(destination, source);
    return destination ? glowByte(destination, source) : 0;
}
quint32 glowHeatByte(quint32 source, quint32 destination)
{
    if (destination == 255) return 255;
    return source + destination > 255 ? glowByte(source, destination) : heatByte(source, destination);
}
quint32 penumbraByte(quint32 source, quint32 destination)
{
    if (destination == 255) return 255;
    if (source + destination < 255) return qMin(255u, divideByte(source, 255 - destination)) / 2;
    return source ? 255 - qMin(255u, divideByte(255 - destination, source) / 2) : 0;
}
quint32 quantizedBlendChannel(LayerBlendMode mode, quint32 source, quint32 destination)
{
    switch (mode) {
    case LayerBlendMode::Reflect: return glowByte(destination, source);
    case LayerBlendMode::Glow: return glowByte(source, destination);
    case LayerBlendMode::Freeze: return heatByte(destination, source);
    case LayerBlendMode::Heat: return heatByte(source, destination);
    case LayerBlendMode::GlowHeat: return glowHeatByte(source, destination);
    case LayerBlendMode::HeatGlow: return heatGlowByte(source, destination);
    case LayerBlendMode::ReflectFreeze: return glowHeatByte(destination, source);
    case LayerBlendMode::FreezeReflect: return freezeReflectByte(source, destination);
    case LayerBlendMode::QuadraticHybrid:
        return (freezeReflectByte(source, destination) + heatGlowByte(source, destination)) * 127 / 255;
    case LayerBlendMode::Parallel:
        return source && destination ? qMin(255u, 130050u / (divideByte(255, source) + divideByte(255, destination))) : 0;
    case LayerBlendMode::HardOverlay:
        if (!destination) return 0;
        if (source == 255) return 255;
        return source >= 127 ? qMin(255u, destination * 255 / (2 * (255 - source))) : destination * 2 * source / 255;
    case LayerBlendMode::SoftLightPegtopDelphi:
        return qMin(255u, multiplyByte(destination, source + destination - multiplyByte(source, destination)) +
                          multiplyByte(multiplyByte(source, destination), 255 - destination));
    case LayerBlendMode::Negation: return 255 - qAbs(255 - int(source) - int(destination));
    case LayerBlendMode::PenumbraA: return penumbraByte(destination, source);
    case LayerBlendMode::PenumbraB: return penumbraByte(source, destination);
    case LayerBlendMode::FlatLight:
        return !source ? 0 : destination > source ? penumbraByte(source, destination) : penumbraByte(destination, source);
    case LayerBlendMode::HardMixSofterPhotoshop:
        return quint32(qBound(0, 3 * int(destination) - 2 * (255 - int(source)), 255));
    default:
        return Detail::usesNonlinearRgba8(mode) ? Detail::nonlinearRgba8Channel(mode, source, destination) :
                                                 multiplyByte(source, destination);
    }
}
RgbaPixel separableProjectionPixel(RgbaPixel destination,
    const RgbaPixel &source, quint8 opacity, quint8 mask, LayerBlendMode mode)
{
    if (Detail::usesSpecialAlphaBlend(mode)) return Detail::specialAlphaRgba8Pixel(destination, source, opacity, mask, mode);
    if (Detail::isCoupledBlend(mode)) return Detail::coupledRgba8Pixel(destination, source, opacity, mask, mode);
    // RGBA8 generic separable composition quantizes opacity, products and blend
    // terms independently before the final integer division.
    if (mask != 15 && destination[3] == 0) destination = {};
    const auto alpha = multiplyThreeBytes(source[3], 255, opacity);
    const bool locked = !(mask & 8u);
    const auto oldAlpha = destination[3];
    if (mode == LayerBlendMode::CopyRed || mode == LayerBlendMode::CopyGreen || mode == LayerBlendMode::CopyBlue) {
        const int channel = int(mode) - int(LayerBlendMode::CopyRed);
        if (mask & (1u << channel)) destination[channel] = blendByte(destination[channel], source[channel], multiplyByte(source[3], opacity));
        return destination;
    }
    if (mode == LayerBlendMode::DestinationIn || mode == LayerBlendMode::DestinationAtop) {
        if (mode == LayerBlendMode::DestinationIn) {
            if (!locked) destination[3] = quint8(multiplyByte(oldAlpha, alpha));
        } else {
            if (source[3]) for (int channel = 0; channel < 3; ++channel)
                if (mask & (1u << channel))
                    destination[channel] = blendByte(source[channel], destination[channel], oldAlpha);
            if (!locked) destination[3] = quint8(alpha);
        }
        return destination;
    }
    if (!alpha) return destination;
    if (mode == LayerBlendMode::Erase) {
        destination[3] = quint8(multiplyByte(255 - multiplyByte(source[3], opacity), oldAlpha));
        return destination;
    }
    if (mode == LayerBlendMode::Behind) {
        if (oldAlpha == 255) return destination;
        const auto unionAlpha = oldAlpha + alpha - multiplyByte(oldAlpha, alpha);
        for (int channel = 0; channel < 3; ++channel) {
            if (!(mask & (1u << channel))) continue;
            const auto blended = blendByte(quint8(multiplyByte(source[channel], alpha)), destination[channel], oldAlpha);
            destination[channel] = oldAlpha ? quint8((quint32(blended) * 255u + unionAlpha / 2u) / unionAlpha) : source[channel];
        }
        if (!locked) destination[3] = quint8(unionAlpha);
        return destination;
    }
    const auto nextAlpha = locked ? oldAlpha : alpha + oldAlpha - multiplyByte(alpha, oldAlpha);
    for (int channel = 0; channel < 3; ++channel) {
        if (!(mask & (1u << channel))) continue;
        const auto multiplied = multiplyByte(source[channel], destination[channel]);
        const auto product = quint8(mode == LayerBlendMode::Addition || mode == LayerBlendMode::LinearDodge ?
            qMin(255, int(source[channel]) + destination[channel]) :
            mode == LayerBlendMode::Darken ? qMin(source[channel], destination[channel]) :
            mode == LayerBlendMode::Lighten ? qMax(source[channel], destination[channel]) :
            mode == LayerBlendMode::Screen ? source[channel] + destination[channel] - multiplied :
            mode == LayerBlendMode::Difference ? qAbs(int(source[channel]) - int(destination[channel])) :
            mode == LayerBlendMode::Exclusion ? qMin(255, int(source[channel]) + destination[channel] - 2 * int(multiplied)) :
            mode == LayerBlendMode::Subtract ? qMax(0, int(destination[channel]) - int(source[channel])) :
            mode == LayerBlendMode::LinearBurn || mode == LayerBlendMode::InverseSubtract ?
                qMax(0, int(source[channel]) + destination[channel] - 255) :
            mode == LayerBlendMode::GrainMerge ? qBound(0, int(source[channel]) + destination[channel] - 127, 255) :
            mode == LayerBlendMode::GrainExtract ? qBound(0, int(destination[channel]) + 127 - int(source[channel]), 255) :
            mode == LayerBlendMode::HardLight ?
                (source[channel] == 127 ? int(destination[channel]) :
                 source[channel] > 127 ?
                    qBound(0, int(2 * source[channel]) - 255 + destination[channel] -
                        int(multiplyByte(2 * source[channel] - 255, destination[channel])), 255) :
                    multiplyByte(2 * source[channel], destination[channel])) :
            mode == LayerBlendMode::Overlay ?
                (destination[channel] == 127 ? int(source[channel]) :
                 destination[channel] > 127 ?
                    qBound(0, int(2 * destination[channel]) - 255 + source[channel] -
                        int(multiplyByte(2 * destination[channel] - 255, source[channel])), 255) :
                    multiplyByte(2 * destination[channel], source[channel])) :
            mode == LayerBlendMode::PinLight ?
                qMax(qMax(0, 2 * int(source[channel]) - 255), qMin(int(destination[channel]), 2 * int(source[channel]))) :
            mode == LayerBlendMode::Divide ?
                (destination[channel] == 0 ? 0 : source[channel] == 0 ? 255 :
                    qMin(255, (int(destination[channel]) * 255 + source[channel] / 2) / int(source[channel]))) :
            mode == LayerBlendMode::ColorDodge ?
                (source[channel] == 255 ? (destination[channel] == 0 ? 0 : 255) :
                    qMin(255, (int(destination[channel]) * 255 + (255 - source[channel]) / 2) / (255 - int(source[channel])))) :
            mode == LayerBlendMode::ColorBurn ?
                (destination[channel] == 255 ? 255 : source[channel] == 0 ? 0 :
                    255 - qMin(255, ((255 - int(destination[channel])) * 255 + source[channel] / 2) / int(source[channel]))) :
            mode == LayerBlendMode::HardMix ?
                (destination[channel] > 127 ?
                    (source[channel] == 255 ? (destination[channel] == 0 ? 0 : 255) :
                        qMin(255, (int(destination[channel]) * 255 + (255 - source[channel]) / 2) / (255 - int(source[channel])))) :
                    (destination[channel] == 255 ? 255 : source[channel] == 0 ? 0 :
                        255 - qMin(255, ((255 - int(destination[channel])) * 255 + source[channel] / 2) / int(source[channel])))) :
            mode == LayerBlendMode::VividLight ?
                (source[channel] < 127 ?
                    (source[channel] == 0 ? (destination[channel] == 255 ? 255 : 0) :
                        qBound(0, 255 - ((255 - int(destination[channel])) * 255) / (2 * int(source[channel])), 255)) :
                    (source[channel] == 255 ? (destination[channel] == 0 ? 0 : 255) :
                        qMin(255, (int(destination[channel]) * 255) / (2 * (255 - int(source[channel])))))) :
            mode == LayerBlendMode::LinearLight ?
                qBound(0, 2 * int(source[channel]) + int(destination[channel]) - 255, 255) :
            mode == LayerBlendMode::Equivalence ? qAbs(int(destination[channel]) - int(source[channel])) :
            mode == LayerBlendMode::Allanon ? ((int(source[channel]) + destination[channel]) * 127) / 255 :
            mode == LayerBlendMode::HardMixPhotoshop ?
                (int(source[channel]) + destination[channel] > 255 ? 255 : 0) :
            mode == LayerBlendMode::BitwiseXor ? (source[channel] ^ destination[channel]) :
            mode == LayerBlendMode::BitwiseOr ? (source[channel] | destination[channel]) :
            mode == LayerBlendMode::BitwiseAnd ? (source[channel] & destination[channel]) :
            mode == LayerBlendMode::BitwiseNand ? (255u ^ (source[channel] & destination[channel])) :
            mode == LayerBlendMode::BitwiseNor ? (255u ^ (source[channel] | destination[channel])) :
            mode == LayerBlendMode::BitwiseXnor ? (255u ^ (source[channel] ^ destination[channel])) :
            mode == LayerBlendMode::Implication ? (source[channel] | (255u - destination[channel])) :
            mode == LayerBlendMode::NotImplication ? ((255u - source[channel]) & destination[channel]) :
            mode == LayerBlendMode::Converse ? ((255u - source[channel]) | destination[channel]) :
            mode == LayerBlendMode::NotConverse ? (source[channel] & (255u - destination[channel])) :
            quantizedBlendChannel(mode, source[channel], destination[channel]));
        if (locked || oldAlpha == 255) {
            if (oldAlpha) destination[channel] = blendByte(destination[channel], product, alpha);
        } else if (!oldAlpha) destination[channel] = source[channel];
        else if (alpha == 255) destination[channel] = blendByte(source[channel], product, oldAlpha);
        else {
            const auto result = quint8(multiplyThreeBytes(255 - alpha, oldAlpha, destination[channel]) +
                multiplyThreeBytes(255 - oldAlpha, alpha, source[channel]) + multiplyThreeBytes(oldAlpha, alpha, product));
            destination[channel] = quint8((quint32(result) * 255u + nextAlpha / 2u) / nextAlpha);
        }
    }
    destination[3] = quint8(nextAlpha);
    return destination;
}

RgbaPixel normalProjectionPixel16(RgbaPixel destination, const RgbaPixel &source,
                                 quint8 opacity, quint8 writeMask)
{
    const volatile float normalizedSource = float(source[3]) * (1.0f / 65535.0f);
    const float alpha = normalizedSource * (float(opacity) / 255.0f);
    if (alpha == 0.0f) return destination;
    const volatile float normalizedDestination = float(destination[3]) * (1.0f / 65535.0f);
    float nextAlpha = normalizedDestination;
    float blend;
    if (!(writeMask & 8u) || destination[3] == 65535) blend = alpha;
    else if (destination[3] == 0) {
        nextAlpha = alpha;
        blend = 1.0f;
        if (writeMask != 15) destination = {};
    } else {
        const volatile float complement = 1.0f - normalizedDestination;
        nextAlpha = std::fma(float(complement), alpha, float(normalizedDestination));
        blend = alpha / nextAlpha;
    }
    for (int channel = 0; channel < 3; ++channel) {
        if (writeMask & (1u << channel)) {
            const float color = std::fma(float(int(source[channel]) - destination[channel]),
                                         blend, float(destination[channel]));
            destination[channel] = quint16(std::nearbyint(color));
        }
    }
    if (writeMask & 8u) {
        const float scaled = nextAlpha * 65535.0f;
        destination[3] = quint16(std::nearbyint(scaled));
    }
    return destination;
}

QList<LayerProjectionInput> compositionInputs(const QHash<QUuid, QList<const Layer *>> &children, const QUuid &parent)
{
    struct Pending { const Layer *layer; quint8 opacity; quint8 parentMask; };
    QList<Pending> pending;
    const auto appendChildren = [&](const QUuid &uuid, quint8 opacity, quint8 mask) {
        const auto values = children.value(uuid);
        for (auto child = values.crbegin(); child != values.crend(); ++child)
            pending.append({*child, opacity, mask});
    };
    appendChildren(parent, 255, 15);
    QList<LayerProjectionInput> result;
    while (!pending.isEmpty()) {
        const auto value = pending.takeLast();
        const auto &layer = *value.layer;
        const auto opacity = quint8(int(layer.opacity) * value.opacity / 255);
        if (!layer.visible || (!opacity && layer.blendMode != LayerBlendMode::DestinationIn && layer.blendMode != LayerBlendMode::DestinationAtop && !Detail::transparentSourceAffectsBlend(layer.blendMode))) continue;
        if (layer.passThrough) appendChildren(layer.uuid, opacity, layer.projectionChannelMask);
        else result.append({layer.id, opacity, quint8(layer.projectionChannelMask & value.parentMask), layer.blendMode});
    }
    return result;
}
}

QList<LayerProjectionInput> DocumentVersion::projectionInputs(LayerId group) const
{
    QHash<QUuid, QList<const Layer *>> children;
    QUuid parent;
    for (const auto &layer : layers) {
        children[layer.parentUuid].append(&layer);
        if (layer.id == group && layer.kind == LayerKind::Group) parent = layer.uuid;
    }
    if (group && parent.isNull()) throw std::invalid_argument("GPU projection group is unavailable");
    return compositionInputs(children, parent);
}

QHash<LayerId, RgbaPixel> DocumentVersion::projectionDefaultPixels() const
{
    QHash<LayerId, RgbaPixel> result;
    QHash<LayerId, quint8> maskDefaults;
    QHash<LayerId, bool> implicitSources;
    QHash<QUuid, const Layer *> identities;
    QHash<QUuid, QList<const Layer *>> children;
    QHash<LayerId, int> remaining;
    QList<const Layer *> ready;
    for (const auto &layer : layers) {
        identities.insert(layer.uuid, &layer);
        children[layer.parentUuid].append(&layer);
        if (layer.kind == LayerKind::Paint) {
            result.insert(layer.id, layer.defaultPixel);
            if (layer.transparencyMask && layer.transparencyMaskVisible)
                maskDefaults.insert(layer.id, layer.transparencyMask->defaultCoverage());
            implicitSources.insert(layer.id, layer.defaultPixel[3] != 0);
        }
        else remaining.insert(layer.id, 0);
    }
    for (const auto &layer : layers) {
        if (layer.parentUuid.isNull() || layer.kind != LayerKind::Group) continue;
        const auto parent = identities.value(layer.parentUuid);
        if (!parent || parent->kind != LayerKind::Group) throw std::logic_error("Invalid retained group parent");
        ++remaining[parent->id];
    }
    for (const auto &layer : layers)
        if (layer.kind == LayerKind::Group && remaining.value(layer.id) == 0) ready.append(&layer);
    const auto maskedDefault = [&](LayerId id) {
        auto pixel = result.value(id);
        const auto coverage = maskDefaults.value(id, 255);
        if (coverage != 255) pixel[3] = format == PixelFormat::Rgba16 ?
            quint16(Detail::multiplyWord(pixel[3], quint32(coverage) * 257u)) :
            quint16(Detail::multiplyByte(pixel[3], coverage));
        return pixel;
    };
    for (qsizetype index = 0; index < ready.size(); ++index) {
        const auto group = ready[index];
        const auto ownChildren = children.value(group->uuid);
        const auto inputs = compositionInputs(children, group->uuid);
        RgbaPixel pixel{};
        bool allocated = false;
        if (ownChildren.size() == 1 && !ownChildren.first()->passThrough && inputs.size() == 1 &&
            inputs.first().opacity == 255 && inputs.first().channelMask == 15 && inputs.first().blendMode == LayerBlendMode::Normal &&
            maskDefaults.value(inputs.first().layer, 255) == 255) {
            pixel = result.value(inputs.first().layer);
            allocated = implicitSources.value(inputs.first().layer);
        } else {
            for (const auto &layer : inputs) {
                auto source = maskedDefault(layer.layer);
                const bool wholeRectangle = layer.blendMode == LayerBlendMode::Copy || layer.blendMode == LayerBlendMode::DestinationIn || layer.blendMode == LayerBlendMode::DestinationAtop;
                if ((!allocated && !(layer.channelMask & 8u) && layer.blendMode != LayerBlendMode::Copy) || (!implicitSources.value(layer.layer) && !wholeRectangle)) continue;
                allocated = true;
                if (layer.blendMode == LayerBlendMode::Copy) {
                    pixel = Detail::copyPixel(pixel, source, float(layer.opacity) / 255.0f, layer.channelMask, format);
                    continue;
                }
                if (format == PixelFormat::Rgba16 && layer.blendMode != LayerBlendMode::Normal) {
                    if (Detail::usesSpecialAlphaRgba16Blend(layer.blendMode)) {
                        pixel = Detail::specialAlphaRgba16Pixel(pixel, source, quint16(layer.opacity * 257u),
                            layer.channelMask, 65535, layer.blendMode);
                        continue;
                    }
                    auto alpha = quint16(quint64(source[3]) * (quint32(layer.opacity) * 257u) / 65535u);
                    if (layer.blendMode == LayerBlendMode::Erase ||
                        (layer.blendMode >= LayerBlendMode::CopyRed && layer.blendMode <= LayerBlendMode::CopyBlue))
                        alpha = Detail::multiplyWord(source[3], quint32(layer.opacity) * 257u);
                    pixel = Detail::isCoupledBlend(layer.blendMode) ?
                        Detail::coupledRgba16Pixel(pixel, source, alpha, layer.channelMask, layer.blendMode) :
                        Detail::separableRgba16Pixel(pixel, source, alpha, layer.channelMask, layer.blendMode);
                    continue;
                }
                if (layer.blendMode != LayerBlendMode::Normal) {
                    pixel = separableProjectionPixel(pixel, source, layer.opacity, layer.channelMask, layer.blendMode);
                    continue;
                }
                if (format == PixelFormat::Rgba16) {
                    pixel = normalProjectionPixel16(pixel, source, layer.opacity, layer.channelMask);
                    continue;
                }
                const float alpha = float(source[3]) * (float(layer.opacity) / 255.0f);
                if (alpha == 0.0f) continue;
                float destinationAlpha = pixel[3];
                float blend;
                if (!(layer.channelMask & 8u) || destinationAlpha == 255.0f) blend = alpha * (1.0f / 255.0f);
                else if (destinationAlpha == 0.0f) {
                    destinationAlpha = alpha;
                    blend = 1.0f;
                    if (layer.channelMask != 15) pixel = {};
                } else {
                    const float weightedAlpha = (255.0f - destinationAlpha) * alpha;
                    destinationAlpha = float(double(weightedAlpha) * double(1.0f / 255.0f) + double(destinationAlpha));
                    blend = alpha / destinationAlpha;
                }
                for (int channel = 0; channel < 3; ++channel) {
                    if (!(layer.channelMask & (1u << channel))) continue;
                    // Retain the byte-color product and sum in binary64, then
                    // round once to binary32 before nearest-even quantization.
                    const float color = float(double(int(source[channel]) - pixel[channel]) * double(blend) +
                                              double(pixel[channel]));
                    pixel[channel] = quint8(std::nearbyint(color));
                }
                if (layer.channelMask & 8u) pixel[3] = quint8(std::nearbyint(destinationAlpha));
            }
        }
        result.insert(group->id, pixel);
        implicitSources.insert(group->id, allocated);
        if (!group->parentUuid.isNull()) {
            const auto parent = identities.value(group->parentUuid);
            if (--remaining[parent->id] == 0) ready.append(parent);
        }
    }
    if (result.size() != layers.size()) throw std::logic_error("Invalid retained layer hierarchy");
    return result;
}
}
