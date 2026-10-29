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
        if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) origin -= (*translation)->offset;
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
            allocated.translate((*translation)->offset);
            allocated &= domain;
            origin += (*translation)->offset;
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
}
