/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCloneProjection_p.h"
#include "KisGpuLayerAllocation_p.h"
#include <QRegion>
#include <QSet>

namespace LibrePaint::Image::Gpu::Detail {
std::optional<Layer> CloneLayerProjection::resolve(const Snapshot &document, const Layer &clone,
                                                 qsizetype maximumRectangles)
{
    if (!document || clone.kind != LayerKind::Clone || maximumRectangles <= 0) return std::nullopt;
    Layer result = clone;
    result.kind = LayerKind::Paint;
    result.cloneSourceUuid = {};
    result.cloneFromOriginal = false;
    result.operations.clear();
    result.initialProjection.clear();
    result.defaultPixel = {};
    QHash<QUuid, const Layer *> nodes;
    for (const auto &layer : document->layers) nodes.insert(layer.uuid, &layer);
    auto source = nodes.value(clone.cloneSourceUuid);
    bool original = clone.cloneFromOriginal;
    QSet<QUuid> visited;
    while (source && original && source->kind == LayerKind::Clone) {
        if (visited.contains(source->uuid)) return std::nullopt;
        visited.insert(source->uuid);
        original = source->cloneFromOriginal;
        source = nodes.value(source->cloneSourceUuid);
    }
    if (!source) return clone.cloneSourceUuid.isNull() || original ? std::optional<Layer>(result) : std::nullopt;
    auto scope = QSharedPointer<DocumentVersion>(new DocumentVersion(*document));
    scope->projectionBackground = {};
    scope->selection = {};
    scope->selectionMask.clear();
    scope->selectionInverted = false;
    scope->selectionKind = SelectionKind::Rectangle;
    QSet<LayerId> included{source->id};
    QList<const Layer *> required{source};
    for (qsizetype i = 0; i < required.size(); ++i) {
        const auto node = required[i];
        const auto add = [&](const Layer *value) {
            if (value && !included.contains(value->id)) {
                included.insert(value->id);
                required.append(value);
            }
        };
        if (node->kind == LayerKind::Clone) add(nodes.value(node->cloneSourceUuid));
        for (const auto &child : document->layers) {
            if (child.parentUuid != node->uuid || !child.visible || !child.opacity) continue;
            if (child.kind == LayerKind::FilterMask) {
                if (!(original && node->id == source->id)) add(&child);
            } else if (node->kind == LayerKind::Group) add(&child);
        }
    }
    QSet<QUuid> includedUuids;
    for (const auto node : required) includedUuids.insert(node->uuid);
    scope->layers.clear();
    scope->sampleCount = scope->rasterBytes = 0;
    for (auto layer : document->layers) {
        if (!included.contains(layer.id)) continue;
        if (!includedUuids.contains(layer.parentUuid)) layer.parentUuid = {};
        if (layer.parentUuid.isNull()) layer.visible = false;
        if (layer.id == source->id) {
            layer.parentUuid = {};
            layer.visible = true;
            layer.opacity = 255;
            layer.projectionChannelMask = 15;
            layer.blendMode = LayerBlendMode::Normal;
            layer.passThrough = false;
            if (original) layer.transparencyMask.clear();
        }
        scope->sampleCount += layer.inputSampleCount();
        scope->rasterBytes += layer.retainedRasterBytes();
        scope->layers.append(std::move(layer));
    }
    scope->activeLayer = source->id;
    scope->selectedLayers = {source->id};
    const Snapshot frozenScope = scope;
    const auto extent = groupProjectionExtent(frozenScope, source->id, maximumRectangles);
    if (!extent) return std::nullopt;
    QRegion blocks;
    QList<const Layer *> pending;
    for (const auto &layer : scope->layers) if (layer.id == source->id) pending.append(&layer);
    for (qsizetype i = 0; i < pending.size(); ++i) {
        const auto &layer = *pending[i];
        if (layer.kind == LayerKind::Group) {
            for (const auto &child : scope->layers)
                if (child.parentUuid == layer.uuid && child.visible && child.opacity && child.kind != LayerKind::FilterMask)
                    pending.append(&child);
        } else if (layer.kind == LayerKind::Paint || layer.kind == LayerKind::Clone) {
            const auto allocated = paintLayerAllocation(frozenScope, layer, maximumRectangles);
            if (!allocated) return std::nullopt;
            blocks |= allocated->blocks;
        }
        if (blocks.rectCount() > maximumRectangles) return std::nullopt;
    }
    auto initial = QSharedPointer<FrozenLayerProjection>::create();
    initial->source = frozenScope;
    initial->extent = *extent;
    initial->sourceLayer = source->id;
    initial->sourceBounds.insert(source->id, *extent);
    initial->nativeSources = true;
    initial->retainedBytes = sizeof(FrozenLayerProjection) + sizeof(DocumentVersion) +
        scope->layers.size() * sizeof(Layer) + blocks.rectCount() * sizeof(QRect);
    for (const auto &block : blocks) initial->allocatedBlocks.append(block);
    result.initialProjection = initial;
    result.defaultPixel = document->projectionDefaultPixels().value(clone.id);
    if (!clone.position.isNull()) {
        auto translation = QSharedPointer<TranslationOperation>::create();
        translation->layer = clone.id;
        translation->offset = clone.position;
        result.operations.append(TranslationSnapshot(translation));
    }
    return result;
}
}
