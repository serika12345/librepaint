/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_OPERATION_VALIDATION_P_H
#define KIS_GPU_OPERATION_VALIDATION_P_H

#include "KisGpuOperationDocument.h"
#include <cmath>

namespace Krita::Image::Gpu::Detail {
constexpr qsizetype LayerNameByteLimit = 65536;

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
    return std::isfinite(brush.diameter) && brush.diameter > 0 &&
        brush.diameter <= OperationDocument::CoordinateLimit &&
        std::isfinite(brush.spacing) && brush.spacing > 0 && brush.spacing <= 1;
}

inline bool validShape(const Shape &shape)
{
    if (!validPoint(shape.first) || !validPoint(shape.second) || !std::isfinite(shape.rotation)) {
        return false;
    }
    switch (shape.kind) {
    case ShapeKind::Freehand:
    case ShapeKind::Line:
        return true;
    case ShapeKind::Ellipse:
        return shape.first.x() != shape.second.x() && shape.first.y() != shape.second.y();
    }
    return false;
}
}
#endif
