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

// Count shared retained inputs once, including predecessor recipes. The bound
// covers old pixels kept alive by a fill after an untracked write or history
// trimming, as well as the copied operation-pointer arrays themselves.
bool boundedReference(const Snapshot &source, const RegionFillSettings &settings, qsizetype maximum,
                      const SelectionMaskSP &extraMask = {})
{
    QSet<const void *> seen;
    QList<Snapshot> pending{source};
    QList<SelectionMaskSP> pendingMasks;
    qsizetype bytes = 0;
    const auto add = [&](const void *identity, qsizetype size) {
        if (!identity || seen.contains(identity)) return true;
        if (size < 0 || size > maximum - bytes) return false;
        seen.insert(identity);
        bytes += size;
        return true;
    };
    const auto lab = [&](const LibrePaint::Pigment::Color::NativeLabConversionSP &value) {
        if (!value) return true;
        if (!add(value.data(), sizeof(*value)) || !add(value->input.iccProfile.constData(), value->input.iccProfile.size())) return false;
        const auto *clut = std::get_if<LibrePaint::Pigment::Color::NativeLabConversion::IntegerClut>(&value->definition);
        return !clut || add(clut->table.constData(), value->tableByteCount());
    };
    const auto inputs = [&](const RegionFillSettings &value) {
        return lab(value.labConversion) && add(value.colorLabels.constData(), value.colorLabels.size() * qsizetype(sizeof(int)));
    };
    const auto mask = [&](const SelectionMaskSP &value) {
        if (!value || seen.contains(value.data())) return true;
        if (!add(value.data(), sizeof(SelectionMask) + value->byteCount())) return false;
        if (const auto &region = value->regionSource()) {
            if (!add(region.data(), sizeof(RegionFill)) || !inputs(region->settings)) return false;
            pending.append(region->source);
        }
        if (const auto &combination = value->combinationSource()) {
            if (!add(combination.data(), sizeof(SelectionCombinationSource)) ||
                !add(combination->tiles.constData(), combination->tiles.size() * qsizetype(sizeof(QPoint)))) return false;
            pendingMasks.append(combination->first);
            pendingMasks.append(combination->second);
            pendingMasks.append(combination->replacement);
        }
        if (const auto &translation = value->translationSource()) {
            if (!add(translation.data(), sizeof(SelectionTranslationSource)) ||
                !add(translation->tiles.constData(), translation->tiles.size() * qsizetype(sizeof(QPoint)))) return false;
            pendingMasks.append(translation->source);
        }
        if (const auto &paint = value->paintSource()) {
            if (!add(paint.data(), sizeof(SelectionPaintSource)) ||
                !add(paint->tiles.constData(), paint->tiles.size() * qsizetype(sizeof(QPoint)))) return false;
            pending.append(paint->source);
        }
        return true;
    };
    if (!inputs(settings) || !mask(extraMask)) return false;
    while (!pending.isEmpty() || !pendingMasks.isEmpty()) {
        if (!pendingMasks.isEmpty()) {
            if (!mask(pendingMasks.takeLast())) return false;
            continue;
        }
        const auto version = pending.takeLast();
        if (!version || seen.contains(version.data())) continue;
        if (!add(version.data(), sizeof(DocumentVersion)) ||
            !add(version->layers.constData(), version->layers.size() * qsizetype(sizeof(Layer))) ||
            !mask(version->selectionMask) ||
            !add(version->colorProfile.constData(), version->colorProfile.size())) return false;
        for (const auto &layer : version->layers) {
            if (!mask(layer.transparencyMask)) return false;
            if (!add(layer.operations.constData(), layer.operations.size() * qsizetype(sizeof(LayerOperation)))) return false;
            for (const auto &operation : layer.operations) {
                const bool valid = std::visit([&](const auto &value) {
                    if (seen.contains(value.data())) return true;
                    if (!add(value.data(), sizeof(*value))) return false;
                    using T = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<T, PaintSnapshot>) {
                        if (!mask(value->selectionMask) ||
                            !add(value->shape.pathCommands.constData(), value->shape.pathCommands.size() * qsizetype(sizeof(PathCommand))) ||
                            !add(value->shape.pathCalls.constData(), value->shape.pathCalls.size() * qsizetype(sizeof(QPair<qsizetype, qsizetype>))) ||
                            !add(value->brush.sizeResponse.curve.constData(), value->brush.sizeResponse.curve.size() * qsizetype(sizeof(qreal))) ||
                            !add(value->sampleBlocks.constData(), value->sampleBlocks.size() * qsizetype(sizeof(value->sampleBlocks[0]))) ||
                            !add(value->smoothingFlushes.constData(), value->smoothingFlushes.size() * qsizetype(sizeof(qsizetype)))) return false;
                        if (value->brush.autoBrush) {
                            for (const auto *response : {&value->brush.autoBrush->opacityResponse, &value->brush.autoBrush->flowResponse})
                                if (!add(response->curve.constData(), response->curve.size() * qsizetype(sizeof(qreal)))) return false;
                        }
                        if (value->brush.texture) {
                            const auto &paper = *value->brush.texture;
                            if (!add(paper.alpha.constData(), paper.alpha.size())) return false;
                            for (const auto *response : {&paper.pressure, &paper.fade})
                                if (*response && !add((*response)->curve.constData(), (*response)->curve.size() * qsizetype(sizeof(qreal)))) return false;
                        }
                        for (const auto &block : value->sampleBlocks)
                            if (!add(block.data(), sizeof(*block)) ||
                                !add(block->constData(), block->size() * qsizetype(sizeof(InputSample)))) return false;
                        if (value->figureStyle.pattern && !add(value->figureStyle.pattern->rgba.constData(), value->figureStyle.pattern->byteCount())) return false;
                        if (value->region && add(value->region.data(), sizeof(RegionFill)) && inputs(value->region->settings)) pending.append(value->region->source);
                        else if (value->region) return false;
                    } else if constexpr (std::is_same_v<T, TileWriteSnapshot>) {
                        if (!add(value->tiles.constData(), value->tiles.size() * qsizetype(sizeof(value->tiles[0])))) return false;
                        for (const auto &tile : value->tiles)
                            if (!add(tile.data(), sizeof(RasterTile) + tile->byteCount())) return false;
                    } else if constexpr (std::is_same_v<T, TranslationSnapshot> || std::is_same_v<T, FilterSnapshot>) {
                        if (!mask(value->selectionMask)) return false;
                    } else if constexpr (std::is_same_v<T, AffineSnapshot>) {
                        if (value->selection && !mask(value->selection->selectionMask)) return false;
                        if (value->plan && !seen.contains(value->plan.data())) {
                            if (!add(value->plan.data(), sizeof(AffinePlan)) ||
                                !add(value->plan->passes.constData(), value->plan->passes.size() * qsizetype(sizeof(AffinePass)))) return false;
                            for (const auto &pass : value->plan->passes)
                                if (pass.kernel && (!add(pass.kernel.data(), sizeof(AffineKernel)) ||
                                    !add(pass.kernel->weights.constData(), pass.kernel->weights.size() * qsizetype(sizeof(qint32))))) return false;
                        }
                    }
                    return true;
                }, operation);
                if (!valid) return false;
            }
        }
    }
    return true;
}
}

bool OperationDocument::selectionFitsRetainedInputs(const SelectionMaskSP &mask) const
{
    return !mask || !mask->isGpuGenerated() || boundedReference({}, {}, m_limits.rasterBytes, mask);
}

Result OperationDocument::prepareRegionFill(LayerId layer, const RegionFillSettings &settings, RegionFillSP *output) const
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
    if (!boundedReference(source, settings, m_limits.rasterBytes)) return Result::CapacityExceeded;
    auto region = QSharedPointer<RegionFill>::create();
    region->source = source;
    region->preparedFor = snapshot();
    region->layer = layer;
    region->settings = settings;
    const int border = (qMax(0, settings.sizeMod) + 63) / 64 * 64;
    region->coverageOrigin = {-border, -border};
    region->coverageSize = settings.antiAlias || settings.sizeMod
        ? QSize((source->size.width() + 63) / 64 * 64 + 2 * border,
                (source->size.height() + 63) / 64 * 64 + 2 * border) : source->size;
    *output = region;
    return Result::Success;
}

Result OperationDocument::fillRegion(LayerId layer, const RegionFillSP &region, const RgbaPixel &color,
                                    const std::optional<PatternFill> &pattern, qreal opacity, bool erase,
                                    bool alphaLocked, LayerBlendMode blendMode)
{
    if (!region || region->layer != layer || region->preparedFor.toStrongRef() != snapshot()) return Result::StaleVersion;
    if (region->settings.purpose != RegionCoveragePurpose::Paint) return Result::InvalidInput;
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
