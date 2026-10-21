/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <cmath>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
namespace {
quint32 multiplyByte(quint32 a, quint32 b)
{
    const auto product = a * b + 128u;
    return ((product >> 8) + product) >> 8;
}
quint32 multiplyThreeBytes(quint32 a, quint32 b, quint32 c)
{
    const auto product = a * b * c + 0x7f5bu;
    return ((product >> 7) + product) >> 16;
}
quint8 blendByte(quint8 destination, quint8 source, quint32 alpha)
{
    const auto weighted = (int(source) - destination) * int(alpha) + 128;
    return quint8((((weighted >> 8) + weighted) >> 8) + destination);
}
std::array<quint8, 4> separableProjectionPixel(std::array<quint8, 4> destination,
    const std::array<quint8, 4> &source, quint8 opacity, quint8 mask, LayerBlendMode mode)
{
    // RGBA8 generic separable composition quantizes opacity, products and blend
    // terms independently before the final integer division.
    if (mask != 15 && destination[3] == 0) destination = {};
    const auto alpha = multiplyThreeBytes(source[3], 255, opacity);
    if (!alpha) return destination;
    const bool locked = !(mask & 8u);
    const auto oldAlpha = destination[3];
    const auto nextAlpha = locked ? oldAlpha : alpha + oldAlpha - multiplyByte(alpha, oldAlpha);
    for (int channel = 0; channel < 3; ++channel) {
        if (!(mask & (1u << channel))) continue;
        const auto multiplied = multiplyByte(source[channel], destination[channel]);
        const auto product = quint8(mode == LayerBlendMode::Darken ? qMin(source[channel], destination[channel]) :
            mode == LayerBlendMode::Lighten ? qMax(source[channel], destination[channel]) :
            mode == LayerBlendMode::Screen ? source[channel] + destination[channel] - multiplied : multiplied);
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
        if (!layer.visible || !opacity) continue;
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

QHash<LayerId, std::array<quint8, 4>> DocumentVersion::projectionDefaultPixels() const
{
    QHash<LayerId, std::array<quint8, 4>> result;
    QHash<QUuid, const Layer *> identities;
    QHash<QUuid, QList<const Layer *>> children;
    QHash<LayerId, int> remaining;
    QList<const Layer *> ready;
    for (const auto &layer : layers) {
        identities.insert(layer.uuid, &layer);
        children[layer.parentUuid].append(&layer);
        if (layer.kind == LayerKind::Paint) result.insert(layer.id, layer.defaultPixel);
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
    for (qsizetype index = 0; index < ready.size(); ++index) {
        const auto group = ready[index];
        const auto ownChildren = children.value(group->uuid);
        const auto inputs = compositionInputs(children, group->uuid);
        std::array<quint8, 4> pixel{};
        if (ownChildren.size() == 1 && !ownChildren.first()->passThrough && inputs.size() == 1 &&
            inputs.first().opacity == 255 && inputs.first().channelMask == 15 && inputs.first().blendMode == LayerBlendMode::Normal) {
            pixel = result.value(inputs.first().layer);
        } else {
            for (const auto &layer : inputs) {
                auto source = result.value(layer.layer);
                if (layer.blendMode != LayerBlendMode::Normal) {
                    pixel = separableProjectionPixel(pixel, source, layer.opacity, layer.channelMask, layer.blendMode);
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
        if (!group->parentUuid.isNull()) {
            const auto parent = identities.value(group->parentUuid);
            if (--remaining[parent->id] == 0) ready.append(parent);
        }
    }
    if (result.size() != layers.size()) throw std::logic_error("Invalid retained layer hierarchy");
    return result;
}
}
