/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
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
        const auto inputs = children.value(group->uuid);
        std::array<quint8, 4> pixel{};
        if (inputs.size() == 1 && inputs.first()->visible && inputs.first()->opacity == 255) {
            pixel = result.value(inputs.first()->id);
        } else {
            for (const auto layer : inputs) {
                if (!layer->visible) continue;
                auto source = result.value(layer->id);
                const quint32 alpha = (quint32(source[3]) * layer->opacity + 127u) / 255u;
                if (!alpha) continue;
                const quint32 destinationAlpha = pixel[3];
                const quint32 denominator = alpha * 255u + destinationAlpha * (255u - alpha);
                for (int channel = 0; channel < 3; ++channel) {
                    pixel[channel] = quint8((quint32(source[channel]) * alpha * 255u +
                        quint32(pixel[channel]) * destinationAlpha * (255u - alpha) + denominator / 2u) / denominator);
                }
                pixel[3] = quint8((denominator + 127u) / 255u);
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
