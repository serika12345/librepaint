/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerAllocation_p.h"
#include "KisGpuPaintGeometry_p.h"
#include "KisGpuAutoBrush_p.h"
#include "KisGpuMarkerBrush_p.h"

namespace LibrePaint::Image::Gpu::Detail {
std::optional<LayerAllocation> paintLayerAllocation(const Snapshot &document, const Layer &layer, qsizetype maximumRectangles)
{
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    QPoint origin = layer.position;
    for (const auto &operation : layer.operations)
        if (const auto translation = std::get_if<TranslationSnapshot>(&operation); translation && !(*translation)->selectedPixels)
            origin -= (*translation)->offset;
    for (const auto &operation : layer.operations)
        if (const auto affine = std::get_if<AffineSnapshot>(&operation); affine && !(*affine)->selection)
            origin -= (*affine)->plan->positionDelta;
    if (layer.operations.size() > PaintGeometryElementLimit) return std::nullopt;
    LayerAllocation result;
    auto &allocated = result.blocks;
    result.beforeOperations.reserve(layer.operations.size());
    const auto aligned = [&](QRect area) {
        if (area.isEmpty()) return QRect{};
        area.translate(-origin);
        const auto first = QPoint(pixelTileIndex(area.left(), PixelHistoryBlockSize), pixelTileIndex(area.top(), PixelHistoryBlockSize));
        const auto last = QPoint(pixelTileIndex(area.right(), PixelHistoryBlockSize), pixelTileIndex(area.bottom(), PixelHistoryBlockSize));
        return QRect(first * PixelHistoryBlockSize + origin, (last + QPoint(1, 1)) * PixelHistoryBlockSize + origin - QPoint(1, 1)).intersected(domain);
    };
    const auto append = [&](QRect area) { allocated += aligned(area); return allocated.rectCount() <= maximumRectangles; };
    if (layer.initialProjection) for (const auto &block : layer.initialProjection->allocatedBlocks) {
        allocated += block;
        if (allocated.rectCount() > maximumRectangles) return std::nullopt;
    }
    for (const auto &operation : layer.operations) {
        auto precedingExtent = allocated.boundingRect();
        if (layer.defaultPixel[3]) precedingExtent |= QRect(QPoint(), document->size);
        result.beforeOperations.append(precedingExtent);
        if (const auto write = std::get_if<TileWriteSnapshot>(&operation)) {
            if ((*write)->writtenAreas.isEmpty()) {
                for (const auto &tile : (*write)->tiles)
                    if (!append(QRect(tile->coordinate() * RgbaTile::Size, QSize(RgbaTile::Size, RgbaTile::Size)))) return std::nullopt;
            } else for (const auto &area : (*write)->writtenAreas) if (!append(area)) return std::nullopt;
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
            if ((*translation)->selectedPixels) {
                const auto copied = allocated.intersected((*translation)->copyRect).translated((*translation)->offset);
                allocated |= copied;
                if (layer.defaultPixel[3] && !append((*translation)->copyRect.translated((*translation)->offset))) return std::nullopt;
                allocated &= domain;
                if (allocated.rectCount() > maximumRectangles) return std::nullopt;
            } else {
                allocated.translate((*translation)->offset);
                allocated &= domain;
                origin += (*translation)->offset;
            }
        } else if (const auto affine = std::get_if<AffineSnapshot>(&operation)) {
            if (!(*affine)->selection) origin += (*affine)->plan->positionDelta;
            if ((*affine)->allocatedBlocks) {
                allocated = QRegion();
                for (const auto &block : *(*affine)->allocatedBlocks) allocated += block;
            } else {
                QRegion transformed = (*affine)->selection ? allocated.intersected((*affine)->selection->copyRect) : allocated;
                for (const auto &pass : (*affine)->plan->passes) {
                    QRegion next;
                    for (const auto &area : transformed) next += affineOutputRegion(pass, area);
                    if (pass.kind != AffinePass::Remap || layer.defaultPixel[3]) next += pass.destination;
                    transformed = next;
                }
                if ((*affine)->selection) allocated |= transformed;
                else allocated = transformed;
                allocated &= domain;
            }
        } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
            allocated &= aligned((*crop)->rect.normalized());
        } else if (const auto filter = std::get_if<FilterSnapshot>(&operation)) {
            if (layer.defaultPixel == RgbaPixel{}) continue;
            auto area = QRect(QPoint(), document->size);
            if (!(*filter)->selectionRect.isEmpty() && !(*filter)->selectionInverted) area &= (*filter)->selectionRect;
            if (!append(area)) return std::nullopt;
        } else {
            const auto &paint = *std::get<PaintSnapshot>(operation);
            auto clip = domain;
            if ((paint.brush.alphaLocked || !(paint.brush.channelWriteMask & 8u)) && !hasCopyBlend(paint)) clip &= precedingExtent;
            if (paint.compositionBounds) clip &= *paint.compositionBounds;
            if (!paint.selectionRect.isEmpty() && !paint.selectionInverted) clip &= paint.selectionRect;
            if (paint.figureStyle.outline) {
                const auto addDabs = [&](const auto &dabs) {
                    if (!dabs) return false;
                    for (const auto &dab : *dabs) if (!append(dab.bounds().intersected(clip))) return false;
                    return true;
                };
                if (paint.brush.autoBrush) {
                    if (!addDabs(autoBrushDabs(paint, document->pendingOperation() == paint.id))) return std::nullopt;
                } else if (!addDabs(markerDabs(paint, document->pendingOperation() == paint.id, clip, true))) return std::nullopt;
            }
            if (hasFigureFill(paint.figureStyle)) {
                const auto edges = figureFillEdges(paint);
                if (!edges) return std::nullopt;
                const auto blocks = figureFillTiles(*edges, paint.brush, clip, maximumRectangles, PixelHistoryBlockSize);
                if (!blocks) return std::nullopt;
                for (const auto block : *blocks)
                    if (!append(QRect(coordinate(block) * PixelHistoryBlockSize, QSize(PixelHistoryBlockSize, PixelHistoryBlockSize)).intersected(clip))) return std::nullopt;
            }
        }
        if (allocated.rectCount() > maximumRectangles) return std::nullopt;
    }
    result.extent = allocated.boundingRect();
    if (layer.defaultPixel[3]) result.extent |= QRect(QPoint(), document->size);
    return result;
}
std::optional<QRect> groupProjectionExtent(const Snapshot &document, LayerId group, qsizetype maximumRectangles)
{
    QHash<LayerId, const Layer *> nodes;
    QHash<QUuid, QList<const Layer *>> children;
    for (const auto &layer : document->layers) { nodes.insert(layer.id, &layer); children[layer.parentUuid].append(&layer); }
    Layer imageRoot;
    imageRoot.kind = LayerKind::Group;
    const auto root = group ? nodes.value(group) : &imageRoot;
    if (!root || root->kind != LayerKind::Group) return std::nullopt;
    QList<const Layer *> ordered{root};
    for (qsizetype i = 0; i < ordered.size(); ++i)
        for (const auto child : children.value(ordered[i]->uuid)) ordered.append(child);
    QHash<LayerId, QRect> extents;
    const auto defaults = document->projectionDefaultPixels();
    for (qsizetype i = ordered.size(); i-- > 0;) {
        const auto node = ordered[i];
        if (node->kind == LayerKind::Paint) {
            const auto allocated = paintLayerAllocation(document, *node, maximumRectangles);
            if (!allocated) return std::nullopt;
            extents.insert(node->id, allocated->extent);
            continue;
        }
        const auto inputs = document->projectionInputs(node->id);
        const auto direct = inputs.size() == 1 ? nodes.value(inputs.first().layer) : nullptr;
        if (direct && (group || node != root || document->directProjectionLayer() == direct->id) &&
            children.value(node->uuid).size() == 1 && children.value(node->uuid).first()->id == direct->id && direct->opacity == 255 &&
            direct->projectionChannelMask == 15 && !direct->passThrough && direct->blendMode == LayerBlendMode::Normal &&
            !hasEnabledTransparencyMask(*direct)) {
            extents.insert(node->id, extents.value(direct->id));
            continue;
        }
        QRect destination;
        for (const auto &input : inputs) {
            auto extent = extents.value(input.layer);
            if (input.blendMode == LayerBlendMode::Copy || input.blendMode == LayerBlendMode::DestinationIn ||
                input.blendMode == LayerBlendMode::DestinationAtop) extent = QRect(QPoint(), document->size);
            if (!(input.channelMask & 8u) && input.blendMode != LayerBlendMode::Copy) extent &= destination;
            if (extent.isEmpty()) continue;
            const auto first = QPoint(pixelTileIndex(extent.left() - node->position.x(), PixelHistoryBlockSize),
                pixelTileIndex(extent.top() - node->position.y(), PixelHistoryBlockSize));
            const auto last = QPoint(pixelTileIndex(extent.right() - node->position.x(), PixelHistoryBlockSize),
                pixelTileIndex(extent.bottom() - node->position.y(), PixelHistoryBlockSize));
            destination |= QRect(first * PixelHistoryBlockSize + node->position,
                (last + QPoint(1, 1)) * PixelHistoryBlockSize + node->position - QPoint(1, 1));
        }
        if (!group && node == root) {
            const auto alpha = document->projectionBackground[3];
            const auto opacity = document->format == PixelFormat::Rgba16 ? (alpha - (alpha >> 8) + 128) >> 8 : alpha;
            if (opacity) destination |= QRect(QPoint(), document->size);
        } else if (defaults.value(node->id)[3]) destination |= QRect(QPoint(), document->size);
        extents.insert(node->id, destination);
    }
    return extents.value(group);
}

}
