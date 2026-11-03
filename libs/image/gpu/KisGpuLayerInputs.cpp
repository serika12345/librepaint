/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuBlendModes_p.h"
#include <QSet>
#include <stdexcept>
namespace LibrePaint::Image::Gpu {
namespace {
QList<LayerProjectionInput> compositionInputs(const QHash<QUuid, QList<const Layer *>> &children,
    const QUuid &parent, const QSet<LayerId> &forced = {})
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
        if (layer.kind == LayerKind::FilterMask) continue;
        const auto opacity = quint8(int(layer.opacity) * value.opacity / 255);
        if (!forced.contains(layer.id) && (!layer.visible || (!opacity && layer.blendMode != LayerBlendMode::DestinationIn && layer.blendMode != LayerBlendMode::DestinationAtop && !Detail::transparentSourceAffectsBlend(layer.blendMode)))) continue;
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

QList<LayerProjectionInput> DocumentVersion::projectionInputsThrough(LayerId adjustment, LayerId *scope) const
{
    QHash<QUuid, const Layer *> identities;
    QHash<QUuid, QList<const Layer *>> children;
    const Layer *target = nullptr;
    for (const auto &layer : layers) {
        identities.insert(layer.uuid, &layer);
        children[layer.parentUuid].append(&layer);
        if (layer.id == adjustment) target = &layer;
    }
    if (!target || target->kind != LayerKind::Adjustment) throw std::invalid_argument("GPU adjustment node is unavailable");
    QSet<LayerId> forced{target->id};
    QUuid parent;
    LayerId group = 0;
    for (auto ancestor = identities.value(target->parentUuid); ancestor;
         ancestor = identities.value(ancestor->parentUuid)) {
        if (ancestor->kind == LayerKind::Group && !ancestor->passThrough) {
            parent = ancestor->uuid; group = ancestor->id; break;
        }
        forced.insert(ancestor->id);
    }
    auto result = compositionInputs(children, parent, forced);
    for (qsizetype i = 0; i < result.size(); ++i) if (result[i].layer == adjustment) {
        result.resize(i + 1);
        if (scope) *scope = group;
        return result;
    }
    throw std::invalid_argument("GPU adjustment projection scope is unavailable");
}

}
