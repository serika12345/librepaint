/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuMarkerBrush_p.h"
#include "KisGpuBrushDabs_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <algorithm>
#include <cstring>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
void floatWord(MarkerDab &dab, std::size_t index, qreal value)
{
    const float scalar = float(value);
    std::memcpy(&dab.words[index], &scalar, sizeof(scalar));
}
float floatValue(const MarkerDab &dab, std::size_t index)
{
    float value;
    std::memcpy(&value, &dab.words[index], sizeof(value));
    return value;
}
}

bool markerDabIntersects(const MarkerDab &dab, const QRect &region)
{
    const auto bounds = dab.bounds().intersected(region);
    if (bounds.isEmpty()) return false;
    const double x = double(floatValue(dab, 0)) + dab.bounds().x(), y = double(floatValue(dab, 1)) + dab.bounds().y();
    const double dx = std::clamp(qRound(x), bounds.left(), bounds.right()) - x;
    const double dy = std::clamp(qRound(y), bounds.top(), bounds.bottom()) - y;
    return ((dx * dx + dy * dy) - floatValue(dab, 2)) * floatValue(dab, 3) < 1;
}

std::optional<QList<MarkerDab>> markerDabs(const PaintOperation &operation, bool pending, const QRect &region,
                                         bool includeEmptySource)
{
    if (operation.brush.autoBrush) return std::nullopt;
    QList<MarkerDab> result;
    if (!operation.figureStyle.outline || region.isEmpty()) return result;
    const auto &brush = operation.brush;
    const int mirror = int(brush.mirrorHorizontal) | (int(brush.mirrorVertical) << 1);
    const bool complete = forEachBrushDab(operation, pending, [&](const BrushDab &point) {
        const double radius = point.size.width() / 2;
        const double squared = radius * radius;
        const double faded = radius + 1;
        for (int reflection = 0; reflection < 4; ++reflection) {
            if ((reflection & mirror) != reflection) continue;
            auto position = point.position;
            if (reflection & 1) position.rx() = 2 * qreal(brush.mirrorCenter.x()) - position.x();
            if (reflection & 2) position.ry() = 2 * qreal(brush.mirrorCenter.y()) - position.y();
            position -= QPointF(.5, .5);
            const auto bounds = QRectF(position.x() - faded, position.y() - faded, 2 * faded, 2 * faded).toAlignedRect();
            MarkerDab dab;
            floatWord(dab, 0, position.x() - bounds.x());
            floatWord(dab, 1, position.y() - bounds.y());
            floatWord(dab, 2, squared);
            floatWord(dab, 3, 1.0 / (faded * faded - squared));
            dab.words[4] = quint32(bounds.x());
            dab.words[5] = quint32(bounds.y());
            dab.words[6] = quint32(bounds.width());
            dab.words[7] = quint32(bounds.height());
            if (includeEmptySource ? !bounds.intersects(region) : !markerDabIntersects(dab, region)) continue;
            if (result.size() >= PaintGeometryElementLimit) return false;
            result.append(dab);
        }
        return true;
    });
    if (!complete) return std::nullopt;
    return result;
}
}
