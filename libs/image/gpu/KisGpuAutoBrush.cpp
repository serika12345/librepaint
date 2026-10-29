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
    if (!brush.autoBrush || hasFigureFill(operation.figureStyle)) return std::nullopt;
    const auto &settings = *brush.autoBrush;
    const quint32 mirrors = quint32(brush.mirrorHorizontal) | (quint32(brush.mirrorVertical) << 1);
    const qsizetype reflectionCount = (brush.mirrorHorizontal ? 2 : 1) * (brush.mirrorVertical ? 2 : 1);
    QList<AutoBrushDab> result;
    qreal averageOpacity = 0;
    const bool complete = forEachBrushDab(operation, pending, [&](const BrushDab &point) {
        const auto &metric = point.size;
        const QPointF hotSpot(std::max(qreal(1), metric.width()) / 2, std::max(qreal(1), metric.height()) / 2);
        const auto offset = point.position - hotSpot;
        QPoint origin(int(std::floor(offset.x())), int(std::floor(offset.y())));
        auto subpixel = offset - origin;
        if (point.mirroredH) {
            const auto value = -(point.position.x() + hotSpot.x());
            subpixel.setX(value - std::floor(value));
            origin.setX(qRound(point.position.x() + subpixel.x() + hotSpot.x()) - maskSide(metric.width(), subpixel.x()));
        }
        if (point.mirroredV) {
            const auto value = -(point.position.y() + hotSpot.y());
            subpixel.setY(value - std::floor(value));
            origin.setY(qRound(point.position.y() + subpixel.y() + hotSpot.y()) - maskSide(metric.height(), subpixel.y()));
        }
        const double radius = brush.diameter * point.scale / 2;
        const double fadeStart = std::max(0., radius - 1.);
        const qreal opacity = pressureValue(settings.opacityResponse, point.pressure);
        constexpr float exponent = .1f;
        averageOpacity = averageOpacity < opacity ? opacity : exponent * opacity + (1.0 - exponent) * averageOpacity;
        AutoBrushDab dab;
        dab.words[0] = maskSide(metric.width(), subpixel.x());
        dab.words[1] = maskSide(metric.height(), subpixel.y());
        dab.words[3] = (point.mirroredH ? 4u : 0u) | (point.mirroredV ? 8u : 0u);
        const qreal narrowSide = brush.diameter * settings.ratio * point.scale;
        dab.words[2] = settings.antialiasEdges ? (narrowSide < 1 ? 6 : narrowSide < 10 ? 3 : 1) : 1;
        doubleWords(dab, 4, hotSpot.x() - .5 + subpixel.x());
        doubleWords(dab, 6, hotSpot.y() - .5 + subpixel.y());
        doubleWords(dab, 8, std::cos(point.angle));
        doubleWords(dab, 10, std::sin(point.angle));
        if (settings.mask != AutoBrush::Mask::GaussianCircle) {
            dab.words[3] |= 1u | (quint32(settings.antialiasEdges) << 1) |
                (settings.mask == AutoBrush::Mask::Rectangle ? 16u : 0u);
            doubleWords(dab, 12, 1. / radius);
            doubleWords(dab, 14, settings.fade == 0 ? 1. : 1. / (settings.fade * radius));
            const qreal verticalFade = settings.verticalFade.value_or(settings.fade);
            doubleWords(dab, 24, 2. / narrowSide);
            doubleWords(dab, 26, verticalFade == 0 ? 1. : 2. / (verticalFade * narrowSide));
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
        return result.size() <= PaintGeometryElementLimit / reflectionCount;
    });
    if (!complete) return std::nullopt;
    const auto originalCount = result.size();
    // The native brush paints the prepared dabs before each reflected batch.
    for (quint32 reflection : {1u, 3u, 2u}) {
        if ((reflection & mirrors) != reflection) continue;
        for (qsizetype i = 0; i < originalCount; ++i) {
            auto mirrored = result[i];
            if (reflection & 1u) {
                mirrored.words[18] = quint32(2 * brush.mirrorCenter.x() - qint32(mirrored.words[18]) - int(mirrored.words[0]));
                mirrored.words[3] ^= 4u;
            }
            if (reflection & 2u) {
                mirrored.words[19] = quint32(2 * brush.mirrorCenter.y() - qint32(mirrored.words[19]) - int(mirrored.words[1]));
                mirrored.words[3] ^= 8u;
            }
            result.append(mirrored);
        }
    }
    return result;
}
}
