/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_VALIDATION_P_H
#define KIS_GPU_OPERATION_VALIDATION_P_H

#include "KisGpuOperationDocument.h"
#include "KisGpuPaintGeometry_p.h"
#include <QSet>
#include <QHash>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
constexpr qsizetype LayerNameByteLimit = 65536;

template<class LayerValue>
bool validLayerHierarchy(const QList<LayerValue> &layers)
{
    QHash<QUuid, const LayerValue *> identities;
    for (const auto &layer : layers) {
        if (layer.kind != LayerKind::Paint && layer.kind != LayerKind::Group) return false;
        if (layer.kind != LayerKind::Group && layer.passThrough) return false;
        if (layer.blendMode > LayerBlendMode::LinearDodge) return false;
        if (layer.kind == LayerKind::Group && layer.transparencyMask) return false;
        if (layer.transparencyMask &&
            (layer.transparencyMask->isEmpty() ||
             layer.transparencyMask->byteCount() > SelectionMask::MaximumBytes)) return false;
        if (!layer.uuid.isNull()) {
            if (identities.contains(layer.uuid)) return false;
            identities.insert(layer.uuid, &layer);
        }
    }
    for (const auto &layer : layers) {
        if (layer.parentUuid.isNull()) continue;
        const auto parent = identities.constFind(layer.parentUuid);
        if (parent == identities.cend() || (*parent)->kind != LayerKind::Group) return false;
    }
    QHash<QUuid, quint8> colours;
    for (auto entry = identities.cbegin(); entry != identities.cend(); ++entry) {
        auto current = entry.key();
        while (!current.isNull() && colours.value(current) == 0) {
            colours[current] = 1;
            current = identities.value(current)->parentUuid;
        }
        if (!current.isNull() && colours.value(current) == 1) return false;
        current = entry.key();
        while (!current.isNull() && colours.value(current) == 1) {
            colours[current] = 2;
            current = identities.value(current)->parentUuid;
        }
    }
    return true;
}

inline bool validCropRect(qint32 x, qint32 y, qint32 width, qint32 height)
{
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    return x >= -limit && x <= limit && y >= -limit && y <= limit &&
        qint64(x) + width >= -limit && qint64(x) + width <= limit &&
        qint64(y) + height >= -limit && qint64(y) + height <= limit;
}
inline bool validCropRect(const QRect &rect)
{ return validCropRect(rect.x(), rect.y(), rect.width(), rect.height()); }
/** Null/empty rectangles disable selection; nonempty rectangles use positive
 * extents inside the absolute document coordinate limit.
 */
inline bool validSelectionRect(const QRect &rect)
{
    if (rect.isEmpty()) return rect.x() == 0 && rect.y() == 0;
    return validCropRect(rect) && rect.width() > 0 && rect.height() > 0;
}
/** Rectangular and elliptical kinds carry no coverage; Freehand requires a
 * nonempty mask whose tight bounds equal the recorded selection rectangle.
 */
inline bool validSelectionState(const QRect &rect, SelectionKind kind, const SelectionMaskSP &mask)
{
    if (kind != SelectionKind::Rectangle && kind != SelectionKind::Ellipse && kind != SelectionKind::Freehand)
        return false;
    if (kind == SelectionKind::Freehand)
        return mask && !mask->isEmpty() && !rect.isEmpty() && rect == mask->bounds() &&
            mask->byteCount() <= SelectionMask::MaximumBytes;
    return !mask;
}
inline bool validCanvasRect(const QRect &rect)
{
    return validCropRect(rect) && rect.width() > 0 && rect.height() > 0 &&
        rect.width() <= OperationDocument::CoordinateLimit && rect.height() <= OperationDocument::CoordinateLimit;
}

// A shifted 64-pixel history grid cuts a 256-pixel GPU tile into at most 5 by 5 pieces.
constexpr qsizetype PixelWriteAreasPerTile = 25;

inline bool validPixelWriteArea(qint32 x, qint32 y, qint32 width, qint32 height, bool alignedHistoryBlocks = false)
{
    if (width <= 0 || height <= 0 || width > PixelHistoryBlockSize || height > PixelHistoryBlockSize ||
        x < -RgbaTile::CoordinateLimit || y < -RgbaTile::CoordinateLimit ||
        qint64(x) + width > RgbaTile::CoordinateLimit || qint64(y) + height > RgbaTile::CoordinateLimit) return false;
    if (pixelTileIndex(x, RgbaTile::Size) != pixelTileIndex(x + width - 1, RgbaTile::Size) ||
        pixelTileIndex(y, RgbaTile::Size) != pixelTileIndex(y + height - 1, RgbaTile::Size)) return false;
    return !alignedHistoryBlocks ||
        (pixelTileIndex(x, PixelHistoryBlockSize) == pixelTileIndex(x + width - 1, PixelHistoryBlockSize) &&
         pixelTileIndex(y, PixelHistoryBlockSize) == pixelTileIndex(y + height - 1, PixelHistoryBlockSize));
}

inline bool validWrittenAreas(const TileWriteOperation &write, bool alignedHistoryBlocks = false)
{
    if (write.writtenAreas.isEmpty()) return write.writeGroup == 0;
    const qsizetype maximumAreas = alignedHistoryBlocks ? 16 : PixelWriteAreasPerTile;
    if (!write.writeGroup || write.writeGroup > write.id.sequence ||
        write.writtenAreas.size() > write.tiles.size() * maximumAreas) return false;
    QSet<QPoint> tiles, blocks, referencedTiles;
    QHash<QPoint, QList<QRect>> occupied;
    for (const auto &tile : write.tiles) tiles.insert(tile->coordinate());
    for (const auto &area : write.writtenAreas) {
        if (!validPixelWriteArea(area.x(), area.y(), area.width(), area.height(), alignedHistoryBlocks)) return false;
        const QPoint block(pixelTileIndex(area.left(), PixelHistoryBlockSize), pixelTileIndex(area.top(), PixelHistoryBlockSize));
        const QPoint tile(pixelTileIndex(area.left(), RgbaTile::Size), pixelTileIndex(area.top(), RgbaTile::Size));
        if ((alignedHistoryBlocks && blocks.contains(block)) || !tiles.contains(tile)) return false;
        if (alignedHistoryBlocks) blocks.insert(block);
        auto &areas = occupied[tile];
        if (areas.size() >= maximumAreas) return false;
        for (const auto &previous : areas) if (previous.intersects(area)) return false;
        areas.append(area);
        referencedTiles.insert(tile);
    }
    return referencedTiles == tiles;
}

inline bool validLayerName(const QString &name)
{
    if (name.size() > LayerNameByteLimit) return false;
    const auto bytes = name.toUtf8();
    return bytes.size() <= LayerNameByteLimit && QString::fromUtf8(bytes) == name;
}

inline bool validPoint(const QPointF &point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y()) &&
        std::abs(point.x()) <= OperationDocument::CoordinateLimit &&
        std::abs(point.y()) <= OperationDocument::CoordinateLimit;
}

inline bool validSample(const InputSample &sample)
{
    return validPoint(sample.position) && std::isfinite(sample.pressure) &&
        sample.pressure >= 0 && sample.pressure <= 1 &&
        std::isfinite(sample.xTilt) && std::abs(sample.xTilt) <= 90 &&
        std::isfinite(sample.yTilt) && std::abs(sample.yTilt) <= 90 &&
        std::isfinite(sample.rotation) && std::isfinite(sample.speed) && sample.speed >= 0 && sample.speed <= 1;
}

inline bool validBrush(const Brush &brush)
{
    if (brush.gaussianWash) {
        const auto &value = *brush.gaussianWash;
        const auto unit = [](qreal number) { return std::isfinite(number) && number >= 0 && number <= 1; };
        if (value.resourceDigest.size() != 32 || value.sizePressure.size() != 256 ||
            value.opacityPressure.size() != 256 || !std::isfinite(value.angle) ||
            !unit(value.canvasOpacity) || !unit(value.flow)) return false;
        for (const auto number : value.sizePressure) if (!unit(number)) return false;
        for (const auto number : value.opacityPressure) if (!unit(number)) return false;
    }
    if (brush.weightedSmoothing) {
        const auto &value = *brush.weightedSmoothing;
        if (!brush.basicSmoothing || !std::isfinite(value.distanceMin) || !std::isfinite(value.distanceMax) ||
            value.distanceMin < 0 || value.distanceMax < 0 ||
            value.distanceMin > OperationDocument::CoordinateLimit || value.distanceMax > OperationDocument::CoordinateLimit ||
            !std::isfinite(value.tailAggressiveness) || value.tailAggressiveness < 0 || value.tailAggressiveness > 1) return false;
    }
    return !(brush.basicSmoothing && brush.pixelPerfect) &&
        std::abs(qint64(brush.mirrorCenter.x())) <= OperationDocument::CoordinateLimit &&
        std::abs(qint64(brush.mirrorCenter.y())) <= OperationDocument::CoordinateLimit &&
        brush.blendMode <= LayerBlendMode::LinearDodge &&
        (!brush.erase || brush.blendMode == LayerBlendMode::Normal) &&
        brush.channelWriteMask <= 15 && std::isfinite(brush.diameter) && brush.diameter > 0 &&
        brush.diameter <= OperationDocument::CoordinateLimit &&
        std::isfinite(brush.spacing) && brush.spacing > 0 && brush.spacing <= 1;
}

inline bool validShape(const Shape &shape)
{
    if (!validPoint(shape.first) || !validPoint(shape.second) || !std::isfinite(shape.rotation) ||
        !std::isfinite(shape.cornerRadii.width()) || !std::isfinite(shape.cornerRadii.height()) ||
        shape.cornerRadii.width() < 0 || shape.cornerRadii.height() < 0 ||
        shape.cornerRadii.width() > OperationDocument::CoordinateLimit ||
        shape.cornerRadii.height() > OperationDocument::CoordinateLimit ||
        (shape.kind != ShapeKind::RoundedRectangle && shape.cornerRadii != QSizeF(0, 0)) ||
        (shape.kind != ShapeKind::Path && (!shape.pathCommands.isEmpty() || shape.windingFill || !shape.pathCalls.isEmpty()))) {
        return false;
    }
    switch (shape.kind) {
    case ShapeKind::Freehand:
    case ShapeKind::Line:
    case ShapeKind::Polyline:
    case ShapeKind::Polygon:
        return true;
    case ShapeKind::Path: {
        if (!shape.pathCommands.isEmpty() && shape.pathCommands.first() != PathCommand::MoveTo) return false;
        for (const auto command : shape.pathCommands) {
            if (command != PathCommand::MoveTo && command != PathCommand::LineTo && command != PathCommand::CubicTo) return false;
        }
        qsizetype previousEnd = 0;
        for (const auto &call : shape.pathCalls) {
            if (call.first < previousEnd || call.second <= call.first) return false;
            previousEnd = call.second;
        }
        return true;
    }
    case ShapeKind::Ellipse:
    case ShapeKind::Rectangle:
    case ShapeKind::RoundedRectangle:
        return shape.first.x() != shape.second.x() && shape.first.y() != shape.second.y();
    }
    return false;
}

inline bool validShapeSampleCount(const Shape &shape, qsizetype count)
{
    if (shape.kind == ShapeKind::Path) {
        qsizetype position = 0, call = 0;
        bool insideCall = false;
        for (const auto command : shape.pathCommands) {
            if (call < shape.pathCalls.size() && position == shape.pathCalls[call].first) {
                if (command != PathCommand::MoveTo) return false;
                insideCall = true;
            }
            const qsizetype points = command == PathCommand::CubicTo ? 3 : 1;
            if (count < points) return false;
            count -= points;
            position += points;
            if (call < shape.pathCalls.size()) {
                const auto &range = shape.pathCalls[call];
                if ((!insideCall && range.first < position) || (insideCall && range.second < position)) return false;
                if (insideCall && range.second == position) { ++call; insideCall = false; }
            }
        }
        return count == 0 && call == shape.pathCalls.size() && !insideCall;
    }
    return (shape.kind != ShapeKind::Polyline && shape.kind != ShapeKind::Polygon) || count >= 2;
}

inline bool validFigureStyle(const Shape &shape, const FigureStyle &style, const Brush &brush)
{
    if (style.pattern && !style.pattern->isValid()) return false;
    if (style.canvasOpacity && (!std::isfinite(*style.canvasOpacity) || *style.canvasOpacity < 0 ||
        *style.canvasOpacity > 1 || style.outline || !style.fill || style.pattern ||
        style.fillBlendMode != LayerBlendMode::Normal || brush.erase || brush.gaussianWash)) return false;
    if (style.fillBlendMode > LayerBlendMode::LinearDodge) return false;
    // A non-normal fill mode owns only the fill source. Outline brush
    // compositing stays source-over, so this combination is rejected rather
    // than silently blending the outline.
    if (style.fillBlendMode != LayerBlendMode::Normal && (!hasFigureFill(style) || style.outline)) return false;
    return !hasFigureFill(style) || (shape.kind != ShapeKind::Freehand && shape.kind != ShapeKind::Line);
}
}
#endif
