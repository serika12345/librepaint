/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuAutoBrush_p.h"
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
void doubleWords(AutoBrushDab &dab, std::size_t index, double value)
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

std::optional<QList<AutoBrushDab>> autoBrushDabs(const PaintOperation &operation, bool pending)
{
    const auto &brush = operation.brush;
    if (!brush.autoBrush || operation.figureStyle.fill) return std::nullopt;
    if (operation.shape.kind != ShapeKind::Freehand && operation.shape.kind != ShapeKind::Line &&
        operation.shape.kind != ShapeKind::Polyline && operation.shape.kind != ShapeKind::Path) return std::nullopt;
    const auto &settings = *brush.autoBrush;
    QList<AutoBrushDab> result;
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
        AutoBrushDab dab;
        dab.words[0] = maskSide(metric.width(), subpixel.x());
        dab.words[1] = maskSide(metric.height(), subpixel.y());
        const qreal effectiveDiameter = brush.diameter * point.scale;
        dab.words[2] = settings.antialiasEdges ? (effectiveDiameter < 1 ? 6 : effectiveDiameter < 10 ? 3 : 1) : 1;
        doubleWords(dab, 4, hotSpot.x() - .5 + subpixel.x());
        doubleWords(dab, 6, hotSpot.y() - .5 + subpixel.y());
        doubleWords(dab, 8, std::cos(settings.angle));
        doubleWords(dab, 10, std::sin(settings.angle));
        if (settings.mask == AutoBrush::Mask::Circle) {
            dab.words[3] = 1u | (quint32(settings.antialiasEdges) << 1);
            doubleWords(dab, 12, 1. / radius);
            doubleWords(dab, 14, settings.fade == 0 ? 1. : 1. / (settings.fade * radius));
        } else {
            doubleWords(dab, 12, radius);
            doubleWords(dab, 14, fadeStart);
            doubleWords(dab, 16, 255. / (radius - fadeStart));
        }
        dab.words[18] = quint32(origin.x());
        dab.words[19] = quint32(origin.y());
        dab.words[20] = floatBits(float(opacity));
        dab.words[21] = floatBits(float(settings.flow * pressureValue(settings.flowResponse, point.pressure)));
        dab.words[22] = floatBits(float(averageOpacity));
        dab.words[23] = quint32(settings.creamy);
        result.append(dab);
        const quint32 mirrors = quint32(brush.mirrorHorizontal) | (quint32(brush.mirrorVertical) << 1);
        for (quint32 reflection = 1; reflection < 4; ++reflection) {
            if ((reflection & mirrors) != reflection) continue;
            auto mirrored = dab;
            if (reflection & 1u) {
                mirrored.words[18] = quint32(2 * brush.mirrorCenter.x() - origin.x() - int(dab.words[0]));
                mirrored.words[3] |= 4u;
            }
            if (reflection & 2u) {
                mirrored.words[19] = quint32(2 * brush.mirrorCenter.y() - origin.y() - int(dab.words[1]));
                mirrored.words[3] |= 8u;
            }
            result.append(mirrored);
        }
        return result.size() <= PaintGeometryElementLimit;
    });
    if (!complete) return std::nullopt;
    return result;
}
}
