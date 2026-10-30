/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <QSet>
#include <type_traits>

namespace LibrePaint::Image::Gpu {
namespace {
// Count shared retained inputs once, including predecessor recipes. The bound
// covers old pixels kept alive by a fill after an untracked write or history
// trimming, as well as the copied operation-pointer arrays themselves.
bool boundedReference(const Snapshot &source, const RegionFillSettings &settings, qsizetype maximum)
{
    QSet<const void *> seen;
    QList<Snapshot> pending{source};
    qsizetype bytes = 0;
    const auto add = [&](const void *identity, qsizetype size) {
        if (!identity || seen.contains(identity)) return true;
        if (size < 0 || size > maximum - bytes) return false;
        seen.insert(identity);
        bytes += size;
        return true;
    };
    const auto mask = [&](const SelectionMaskSP &value) {
        return !value || add(value.data(), sizeof(SelectionMask) + value->byteCount());
    };
    const auto lab = [&](const LibrePaint::Pigment::Color::NativeLabConversionSP &value) {
        if (!value) return true;
        if (!add(value.data(), sizeof(*value)) || !add(value->input.iccProfile.constData(), value->input.iccProfile.size())) return false;
        const auto *clut = std::get_if<LibrePaint::Pigment::Color::NativeLabConversion::IntegerClut>(&value->definition);
        return !clut || add(clut->table.constData(), value->tableByteCount());
    };
    if (!lab(settings.labConversion)) return false;
    while (!pending.isEmpty()) {
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
                        for (const auto &block : value->sampleBlocks)
                            if (!add(block.data(), sizeof(*block)) ||
                                !add(block->constData(), block->size() * qsizetype(sizeof(InputSample)))) return false;
                        if (value->figureStyle.pattern && !add(value->figureStyle.pattern->rgba.constData(), value->figureStyle.pattern->byteCount())) return false;
                        if (value->region && add(value->region.data(), sizeof(RegionFill)) && lab(value->region->settings.labConversion)) pending.append(value->region->source);
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

Result OperationDocument::prepareRegionFill(LayerId layer, const RegionFillSettings &settings, RegionFillSP *output) const
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!output || (settings.mode != RegionFillMode::Contiguous && settings.mode != RegionFillMode::Similar) ||
        (settings.reference != RegionFillReference::CurrentLayer && settings.reference != RegionFillReference::AllLayers) || settings.threshold < 0 || settings.threshold > 255 ||
        settings.opacitySpread < 0 || settings.opacitySpread > 100 || settings.antiAlias ||
        !QRect(QPoint(), snapshot()->size).contains(settings.seed)) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || (snapshot()->layers[index].kind != LayerKind::Paint || snapshot()->layers[index].userLocked)) return Result::LayerUnavailable;
    auto source = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    if (settings.reference == RegionFillReference::CurrentLayer) {
        source->layers = {snapshot()->layers[index]};
        source->layers[0].parentUuid = {};
        source->layers[0].transparencyMask.clear();
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
    *output = region;
    return Result::Success;
}

Result OperationDocument::fillRegion(LayerId layer, const RegionFillSP &region, const RgbaPixel &color,
                                    const std::optional<PatternFill> &pattern, qreal opacity, bool erase,
                                    bool alphaLocked, LayerBlendMode blendMode)
{
    if (!region || region->layer != layer || region->preparedFor.toStrongRef() != snapshot()) return Result::StaleVersion;
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
    if (!QRect(QPoint(), snapshot()->size).contains(bounds)) return Result::InvalidInput;
    const QPointF start = bounds.topLeft(), end = start + QPointF(bounds.width(), bounds.height());
    const auto result = paintFigureWithin(layer, brush, {{start, 1}, {end, 1}},
        {ShapeKind::Rectangle, start, end, 0}, style, 0, bounds, region);
    if (result == Result::Success) m_layerHistoryChanges[m_index].filledLayer = true;
    return result;
}
}
