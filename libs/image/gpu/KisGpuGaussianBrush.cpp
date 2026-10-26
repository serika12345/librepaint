/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuGaussianBrush_p.h"
#include "KisGpuBrushDabs_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
quint32 floatBits(float value)
{
    quint32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}
void doubleWords(GaussianDab &dab, std::size_t index, double value)
{
    quint64 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    dab.words[index] = quint32(bits);
    dab.words[index + 1] = quint32(bits >> 32);
}
int maskSide(qreal metric, qreal subpixel)
{
    if (std::abs(metric - qRound(metric)) < 1e-6) metric = qRound(metric);
    if (subpixel < 1e-6) subpixel = 0;
    return std::max(1, int(std::ceil(metric + subpixel)));
}
}

std::optional<QList<GaussianDab>> gaussianDabs(const PaintOperation &operation, bool pending)
{
    const auto &brush = operation.brush;
    if (!brush.gaussianWash || operation.figureStyle.fill || brush.mirrorHorizontal || brush.mirrorVertical ||
        brush.alphaLocked || brush.channelWriteMask != 15) return std::nullopt;
    if (operation.shape.kind != ShapeKind::Freehand && operation.shape.kind != ShapeKind::Line &&
        operation.shape.kind != ShapeKind::Polyline && operation.shape.kind != ShapeKind::Path) return std::nullopt;
    const auto &settings = *brush.gaussianWash;
    QList<GaussianDab> result;
    qreal averageOpacity = 0;
    const bool complete = forEachBrushDab(operation, pending, [&](const BrushDab &point) {
        const auto &metric = point.size;
        const QPointF hotSpot(std::max(qreal(1), metric.width()) / 2, std::max(qreal(1), metric.height()) / 2);
        const auto offset = point.position - hotSpot;
        const QPoint origin(int(std::floor(offset.x())), int(std::floor(offset.y())));
        const auto subpixel = offset - origin;
        const double radius = brush.diameter * point.scale / 2;
        const double fadeStart = std::max(0., radius - 1.);
        const qreal opacity = pressureValue(settings.opacityResponse, point.pressure);
        constexpr float exponent = .1f;
        averageOpacity = averageOpacity < opacity ? opacity : exponent * opacity + (1.0 - exponent) * averageOpacity;
        GaussianDab dab;
        dab.words[0] = maskSide(metric.width(), subpixel.x());
        dab.words[1] = maskSide(metric.height(), subpixel.y());
        const qreal effectiveDiameter = brush.diameter * point.scale;
        dab.words[2] = effectiveDiameter < 1 ? 6 : effectiveDiameter < 10 ? 3 : 1;
        doubleWords(dab, 4, hotSpot.x() - .5 + subpixel.x());
        doubleWords(dab, 6, hotSpot.y() - .5 + subpixel.y());
        doubleWords(dab, 8, std::cos(settings.angle));
        doubleWords(dab, 10, std::sin(settings.angle));
        doubleWords(dab, 12, radius);
        doubleWords(dab, 14, fadeStart);
        doubleWords(dab, 16, 255. / (radius - fadeStart));
        dab.words[18] = quint32(origin.x());
        dab.words[19] = quint32(origin.y());
        dab.words[20] = floatBits(float(opacity));
        dab.words[21] = floatBits(float(settings.flow));
        dab.words[22] = floatBits(float(averageOpacity));
        dab.words[23] = quint32(settings.creamy);
        result.append(dab);
        return true;
    });
    if (!complete) return std::nullopt;
    return result;
}
}
