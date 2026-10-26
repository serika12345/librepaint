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
void doubleWords(MarkerDab &dab, std::size_t index, double value)
{
    quint64 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    dab.words[index] = quint32(bits);
    dab.words[index + 1] = quint32(bits >> 32);
}
double doubleValue(const MarkerDab &dab, std::size_t index)
{
    const quint64 bits = quint64(dab.words[index]) | (quint64(dab.words[index + 1]) << 32);
    double value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
}

bool markerDabIntersects(const MarkerDab &dab, const QRect &region)
{
    const auto bounds = dab.bounds().intersected(region);
    if (bounds.isEmpty()) return false;
    const double x = doubleValue(dab, 0), y = doubleValue(dab, 2);
    const double dx = std::clamp(qRound(x), bounds.left(), bounds.right()) - x;
    const double dy = std::clamp(qRound(y), bounds.top(), bounds.bottom()) - y;
    return ((dx * dx + dy * dy) - doubleValue(dab, 4)) * doubleValue(dab, 6) < 1;
}

std::optional<QList<MarkerDab>> markerDabs(const PaintOperation &operation, bool pending, const QRect &region)
{
    if (operation.brush.gaussianWash) return std::nullopt;
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
            doubleWords(dab, 0, position.x());
            doubleWords(dab, 2, position.y());
            doubleWords(dab, 4, squared);
            doubleWords(dab, 6, 1.0 / (faded * faded - squared));
            dab.words[8] = quint32(bounds.x());
            dab.words[9] = quint32(bounds.y());
            dab.words[10] = quint32(bounds.width());
            dab.words[11] = quint32(bounds.height());
            if (!markerDabIntersects(dab, region)) continue;
            if (result.size() >= PaintGeometryElementLimit) return false;
            result.append(dab);
        }
        return true;
    });
    if (!complete) return std::nullopt;
    return result;
}
}
