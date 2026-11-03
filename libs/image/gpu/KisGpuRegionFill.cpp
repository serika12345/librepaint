/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <QSet>
#include <algorithm>
#include <type_traits>

namespace LibrePaint::Image::Gpu {
namespace {
void retainLabeledReference(DocumentVersion &source, LayerId active, const RegionFillSettings &settings)
{
    QHash<QUuid, QList<int>> children;
    for (int index = 0; index < source.layers.size(); ++index)
        children[source.layers[index].parentUuid].append(index);
    struct Visit { int index; bool descendant; };
    QList<Visit> pending;
    QList<Layer> layers;
    const auto appendChildren = [&](const QUuid &parent, bool descendant) {
        const auto siblings = children.value(parent);
        for (auto index = siblings.crbegin(); index != siblings.crend(); ++index)
            pending.append({*index, descendant});
    };
    appendChildren({}, false);
    while (!pending.isEmpty()) {
        const auto visit = pending.takeLast();
        auto layer = source.layers[visit.index];
        bool selected = visit.descendant;
        if (!selected) {
            if (!layer.visible) continue;
            selected = (settings.colorLabels.contains(layer.colorLabelIndex) ||
                (settings.includeActiveLayer && layer.id == active)) &&
                (layer.kind != LayerKind::Group || layer.colorLabelIndex != 0);
            if (selected) {
                // The ordinary labelled-layer reference merges detached copies
                // in tree order, retaining each root's own opacity and masks.
                layer.parentUuid = {};
                layer.blendMode = LayerBlendMode::Normal;
                layer.projectionChannelMask = 15;
            }
        }
        if (selected) layers.append(layer);
        if (layer.kind == LayerKind::Group) appendChildren(layer.uuid, selected);
    }
    source.layers = std::move(layers);
    source.projectionBackground = {};
}


}

bool OperationDocument::selectionFitsInputs(const SelectionMaskSP &mask) const
{
    return !mask || mask->byteCount() <= m_limits.rasterBytes;
}

Result OperationDocument::prepareRegionFill(LayerId layer, const RegionFillSettings &settings, RegionFillRequestSP *output) const
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!output || (settings.mode != RegionFillMode::Contiguous && settings.mode != RegionFillMode::Similar) ||
        (settings.reference != RegionFillReference::CurrentLayer && settings.reference != RegionFillReference::AllLayers &&
         settings.reference != RegionFillReference::ColorLabeledLayers && settings.reference != RegionFillReference::CurrentProjection) ||
        (settings.purpose != RegionCoveragePurpose::Paint && settings.purpose != RegionCoveragePurpose::Selection) ||
        (settings.purpose == RegionCoveragePurpose::Paint && settings.reference == RegionFillReference::CurrentProjection) || settings.colorLabels.size() > 9 ||
        std::any_of(settings.colorLabels.cbegin(), settings.colorLabels.cend(), [](int label) { return label < 0 || label > 8; }) ||
        settings.threshold < 0 || settings.threshold > 255 || settings.sizeMod < -400 || settings.sizeMod > 400 ||
        settings.feather < 0 || settings.feather > 400 ||
        settings.opacitySpread < 0 || settings.opacitySpread > 100 ||
        !QRect(QPoint(), snapshot()->size).contains(settings.seed) ||
        (settings.boundaryColor && (settings.mode != RegionFillMode::Contiguous || !validPixel(*settings.boundaryColor, snapshot()->format))))
        return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || (settings.purpose == RegionCoveragePurpose::Paint &&
        (snapshot()->layers[index].kind != LayerKind::Paint || snapshot()->layers[index].userLocked))) return Result::LayerUnavailable;
    auto source = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    if (settings.reference == RegionFillReference::CurrentLayer) {
        source->layers = {snapshot()->layers[index]};
        source->layers[0].parentUuid = {};
        source->layers[0].transparencyMask.clear();
    } else if (settings.reference == RegionFillReference::CurrentProjection) {
        // A selection can sample a locked or hidden node and its masked
        // projection. A group's descendants remain in their existing order.
        const auto root = snapshot()->layers[index];
        QSet<QUuid> included{root.uuid};
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto &candidate : snapshot()->layers) {
                if (included.contains(candidate.parentUuid) && !included.contains(candidate.uuid)) {
                    included.insert(candidate.uuid);
                    changed = true;
                }
            }
        }
        source->layers.clear();
        for (auto candidate : snapshot()->layers) {
            if (!included.contains(candidate.uuid)) continue;
            if (candidate.id == layer) {
                candidate.parentUuid = {};
                candidate.visible = true;
                candidate.opacity = 255;
                candidate.blendMode = LayerBlendMode::Normal;
                candidate.projectionChannelMask = 15;
            }
            source->layers.append(std::move(candidate));
        }
        source->projectionBackground = {};
    } else if (settings.reference == RegionFillReference::ColorLabeledLayers) {
        retainLabeledReference(*source, layer, settings);
    }
    if (settings.purpose == RegionCoveragePurpose::Selection &&
        (settings.mode == RegionFillMode::Similar || !settings.selectionAsBoundary)) {
        source->selection = {};
        source->selectionKind = SelectionKind::Rectangle;
        source->selectionInverted = false;
        source->selectionMask.clear();
    }
    if (settings.labConversion && (!settings.labConversion->isValid() ||
        settings.labConversion->input.iccProfile != source->colorProfile ||
        (settings.labConversion->input.depth == KoColorConversionStages::Encoding::U16) != (source->format == PixelFormat::Rgba16)))
        return Result::InvalidInput;
    auto region = QSharedPointer<RegionFill>::create();
    region->documentSize = source->size;
    region->preparedFor = snapshot();
    region->layer = layer;
    region->purpose = settings.purpose;
    const int border = (qMax(0, settings.sizeMod) + 63) / 64 * 64 + (settings.feather + 63) / 64 * 64;
    region->coverageOrigin = {-border, -border};
    region->coverageSize = settings.antiAlias || settings.sizeMod || settings.feather
        ? QSize((source->size.width() + 63) / 64 * 64 + 2 * border,
                (source->size.height() + 63) / 64 * 64 + 2 * border) : source->size;
    auto request = QSharedPointer<RegionFillRequest>::create();
    request->source = source; request->settings = settings; request->result = region;
    *output = request;
    return Result::Success;
}

Result OperationDocument::fillRegion(LayerId layer, const RegionFillSP &region, const RgbaPixel &color,
                                    const std::optional<PatternFill> &pattern, qreal opacity, bool erase,
                                    bool alphaLocked, LayerBlendMode blendMode)
{
    if (!region || region->layer != layer || region->preparedFor.toStrongRef() != snapshot()) return Result::StaleVersion;
    if (region->purpose != RegionCoveragePurpose::Paint) return Result::InvalidInput;
    Brush brush;
    brush.rgba = color;
    brush.opacity = opacity;
    brush.erase = erase;
    brush.alphaLocked = alphaLocked;
    brush.channelWriteMask = alphaLocked ? 7 : 15;
    FigureStyle style;
    style.outline = false;
    if (pattern) style.pattern = pattern;
    else style.fill = color;
    style.fillBlendMode = blendMode;
    const QRect bounds = region->bounds;
    if (bounds.isEmpty()) return Result::EmptyOperation;
    if (!QRect(region->coverageOrigin, region->coverageSize).contains(bounds)) return Result::InvalidInput;
    const QPointF start = bounds.topLeft(), end = start + QPointF(bounds.width(), bounds.height());
    const auto result = paintFigureWithin(layer, brush, {{start, 1}, {end, 1}},
        {ShapeKind::Rectangle, start, end, 0}, style, 0, bounds, region);
    if (result == Result::Success) m_layerHistoryChanges[m_index].filledLayer = true;
    return result;
}
}
