/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_VALIDATION_P_H
#define KIS_GPU_OPERATION_VALIDATION_P_H

#include "KisGpuOperationDocument.h"
#include "KisGpuPaintGeometry_p.h"
#include <QSet>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
constexpr qsizetype LayerNameByteLimit = 65536;

inline bool validPixelWriteArea(qint32 x, qint32 y, qint32 width, qint32 height)
{
    if (width <= 0 || height <= 0 || width > PixelHistoryBlockSize || height > PixelHistoryBlockSize ||
        x < -RgbaTile::CoordinateLimit || y < -RgbaTile::CoordinateLimit ||
        qint64(x) + width > RgbaTile::CoordinateLimit || qint64(y) + height > RgbaTile::CoordinateLimit) return false;
    return pixelTileIndex(x, PixelHistoryBlockSize) == pixelTileIndex(x + width - 1, PixelHistoryBlockSize) &&
        pixelTileIndex(y, PixelHistoryBlockSize) == pixelTileIndex(y + height - 1, PixelHistoryBlockSize);
}

inline bool validWrittenAreas(const TileWriteOperation &write)
{
    if (write.writtenAreas.isEmpty()) return write.writeGroup == 0;
    if (!write.writeGroup || write.writeGroup > write.id.sequence ||
        write.writtenAreas.size() > write.tiles.size() * 16) return false;
    QSet<QPoint> tiles, blocks, referencedTiles;
    for (const auto &tile : write.tiles) tiles.insert(tile->coordinate());
    for (const auto &area : write.writtenAreas) {
        if (!validPixelWriteArea(area.x(), area.y(), area.width(), area.height())) return false;
        const QPoint block(pixelTileIndex(area.left(), PixelHistoryBlockSize), pixelTileIndex(area.top(), PixelHistoryBlockSize));
        if (blocks.contains(block) ||
            !tiles.contains({pixelTileIndex(area.left(), RgbaTile::Size), pixelTileIndex(area.top(), RgbaTile::Size)})) return false;
        blocks.insert(block);
        referencedTiles.insert({pixelTileIndex(area.left(), RgbaTile::Size), pixelTileIndex(area.top(), RgbaTile::Size)});
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
        std::isfinite(sample.rotation);
}

inline bool validBrush(const Brush &brush)
{
    return std::abs(qint64(brush.mirrorCenter.x())) <= OperationDocument::CoordinateLimit &&
        std::abs(qint64(brush.mirrorCenter.y())) <= OperationDocument::CoordinateLimit &&
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
        (shape.kind != ShapeKind::Path && (!shape.pathCommands.isEmpty() || shape.windingFill))) {
        return false;
    }
    switch (shape.kind) {
    case ShapeKind::Freehand:
    case ShapeKind::Line:
    case ShapeKind::Polyline:
    case ShapeKind::Polygon:
        return true;
    case ShapeKind::Path:
        if (!shape.pathCommands.isEmpty() && shape.pathCommands.first() != PathCommand::MoveTo) return false;
        for (const auto command : shape.pathCommands) {
            if (command != PathCommand::MoveTo && command != PathCommand::LineTo && command != PathCommand::CubicTo) return false;
        }
        return true;
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
        for (const auto command : shape.pathCommands) {
            const qsizetype points = command == PathCommand::CubicTo ? 3 : 1;
            if (count < points) return false;
            count -= points;
        }
        return count == 0;
    }
    return (shape.kind != ShapeKind::Polyline && shape.kind != ShapeKind::Polygon) || count >= 2;
}

inline bool validFigureStyle(const Shape &shape, const FigureStyle &style)
{
    return !style.fill || (shape.kind != ShapeKind::Freehand && shape.kind != ShapeKind::Line);
}
}
#endif
