/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuGaussianBrush_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QTransform>
#include <QVector2D>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
qreal response(qreal pressure, const QList<qreal> &values)
{
    const qreal x = std::clamp(255 * pressure, qreal(0), qreal(255));
    const auto low = std::size_t(std::floor(x)), high = std::size_t(std::ceil(x));
    const qreal fraction = x - std::floor(x);
    if (fraction < 1e-6) return values[low];
    if (fraction > 1 - 1e-6) return values[high];
    return values[low] + fraction * (values[high] - values[low]);
}
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
QSizeF characteristicSize(qreal diameter, qreal scale, qreal angle)
{
    constexpr qreal circle = 6.283185307179586476925286766559;
    angle = std::fmod(angle, circle);
    if (angle < 0) angle += circle;
    QTransform transform;
    transform.scale(scale, scale);
    if (!qFuzzyCompare(angle, qreal(0))) transform = transform * QTransform().rotateRadians(angle);
    const int side = std::max(1, int(diameter));
    return transform.mapRect(QRectF(0, 0, side, side)).size();
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
    if (!brush.gaussianWash || brush.basicSmoothing || brush.pixelPerfect ||
        operation.figureStyle.fill || brush.mirrorHorizontal || brush.mirrorVertical ||
        brush.erase || brush.alphaLocked || brush.channelWriteMask != 15) return std::nullopt;
    const auto &settings = *brush.gaussianWash;
    auto input = operation.inputSamples();
    if (operation.shape.kind == ShapeKind::Line && !input.isEmpty()) {
        const auto first = input.first().position;
        const auto delta = input.last().position - first;
        const qreal lengthSquared = QPointF::dotProduct(delta, delta);
        const auto lineDelta = operation.shape.second - operation.shape.first;
        if (input.size() == 1) input.append(input.first());
        for (qsizetype i = 1; i + 1 < input.size(); ++i) {
            const qreal fraction = lengthSquared > 0 ?
                std::clamp(QPointF::dotProduct(input[i].position - first, delta) / lengthSquared, qreal(0), qreal(1)) :
                qreal(i) / (input.size() - 1);
            input[i].position = operation.shape.first + lineDelta * fraction;
        }
        input.first().position = operation.shape.first;
        input.last().position = operation.shape.second;
    }
    QList<GaussianDab> result;
    qreal spacing = .5, accumulated = 0, averageOpacity = 0;
    const auto append = [&](const InputSample &point) {
        const qreal scale = settings.sizeUsesPressureDirectly ? point.pressure : response(point.pressure, settings.sizePressure);
        if (scale * std::max(1, int(brush.diameter)) < .01) { spacing = .5; return true; }
        if (result.size() >= PathGeometrySegmentLimit) return false;
        const auto metric = characteristicSize(brush.diameter, scale, settings.angle);
        const QPointF hotSpot(std::max(qreal(1), metric.width()) / 2,
                              std::max(qreal(1), metric.height()) / 2);
        const auto offset = point.position - hotSpot;
        const QPoint origin(int(std::floor(offset.x())), int(std::floor(offset.y())));
        const auto subpixel = offset - origin;
        const double radius = brush.diameter * scale / 2;
        const double fadeStart = std::max(0., radius - 1.);
        const qreal opacity = settings.opacityUsesPressureDirectly ? point.pressure : response(point.pressure, settings.opacityPressure);
        constexpr float exponent = .1f;
        averageOpacity = averageOpacity < opacity ? opacity : exponent * opacity + (1.0 - exponent) * averageOpacity;
        GaussianDab dab;
        dab.words[0] = maskSide(metric.width(), subpixel.x());
        dab.words[1] = maskSide(metric.height(), subpixel.y());
        const qreal effectiveDiameter = brush.diameter * scale;
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
        spacing = std::max(qreal(.5), metric.width() * brush.spacing);
        return true;
    };
    const auto line = [&](InputSample first, const InputSample &last) {
        while (first.position != last.position) {
            const qreal length = QVector2D(last.position - first.position).length();
            const qreal next = spacing - accumulated;
            if (next > length) { accumulated += length; return true; }
            const qreal fraction = next <= 0 ? 0 : next / length;
            const auto previous = first.position;
            first.position = (1 - fraction) * first.position + fraction * last.position;
            first.pressure = (1 - fraction) * first.pressure + fraction * last.pressure;
            accumulated = 0;
            if (!append(first)) return false;
            if (first.position == previous && fraction != 0) return false;
        }
        return true;
    };
    qsizetype cubicSegments = 0;
    const auto cubic = [&](InputSample first, QPointF control1, QPointF control2, InputSample last) {
        struct Curve { InputSample first; QPointF control1, control2; InputSample last; };
        QList<Curve> pending{{first, control1, control2, last}};
        const auto same = [](QPointF a, QPointF b) { return a.x() == b.x() && a.y() == b.y(); };
        while (!pending.isEmpty()) {
            const auto curve = pending.takeLast();
            bool flat;
            if (same(curve.first.position, curve.last.position)) {
                flat = same(curve.control1, curve.first.position) && same(curve.control2, curve.first.position);
            } else {
                const auto delta = curve.last.position - curve.first.position;
                const qreal length = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                const QPointF normal = length > 0 ? QPointF(-delta.y() / length, delta.x() / length) :
                                                   QPointF(-delta.y(), delta.x());
                const qreal offset = -QPointF::dotProduct(curve.first.position, normal);
                const qreal d1 = std::abs(QPointF::dotProduct(normal, curve.control1) + offset);
                const qreal d2 = std::abs(QPointF::dotProduct(normal, curve.control2) + offset);
                flat = (d1 < .5 && d2 < .5) || std::isnan(d1) || std::isnan(d2);
            }
            if (flat) {
                if (cubicSegments++ >= PathGeometrySegmentLimit || !line(curve.first, curve.last)) return false;
                continue;
            }
            // Match ordinary cubic brush calls before deriving Gaussian dabs.
            const auto left = (curve.first.position + curve.control1) / 2;
            const auto center = (curve.control1 + curve.control2) / 2;
            const auto right = (curve.control2 + curve.last.position) / 2;
            const auto leftInner = (left + center) / 2;
            const auto rightInner = (center + right) / 2;
            auto middle = curve.first;
            middle.position = (leftInner + rightInner) / 2;
            middle.pressure = .5 * curve.first.pressure + .5 * curve.last.pressure;
            pending.append({middle, rightInner, right, curve.last});
            pending.append({curve.first, left, leftInner, middle});
        }
        return true;
    };
    if (input.isEmpty()) return result;
    if (operation.shape.kind == ShapeKind::Path) {
        qsizetype index = 0, call = 0;
        qreal lineSpacing = spacing, lineAccumulated = accumulated;
        InputSample previous;
        for (const auto command : operation.shape.pathCommands) {
            // A painter-path call owns its distance, while opacity averaging
            // belongs to the whole brush stroke, including intervening lines.
            if (call < operation.shape.pathCalls.size() && index == operation.shape.pathCalls[call].first) {
                lineSpacing = spacing;
                lineAccumulated = accumulated;
                spacing = .5;
                accumulated = 0;
            }
            if (command == PathCommand::CubicTo) {
                if (!cubic(previous, input[index].position, input[index + 1].position, input[index + 2])) return std::nullopt;
                previous = input[index + 2];
                index += 3;
            } else {
                const auto point = input[index++];
                if (command == PathCommand::LineTo && !line(previous, point)) return std::nullopt;
                previous = point;
            }
            if (call < operation.shape.pathCalls.size() && index == operation.shape.pathCalls[call].second) {
                spacing = lineSpacing;
                accumulated = lineAccumulated;
                ++call;
            }
        }
    } else if (operation.shape.kind == ShapeKind::Line || operation.shape.kind == ShapeKind::Freehand ||
               operation.shape.kind == ShapeKind::Polyline) {
        const bool stationaryFreehand = operation.shape.kind == ShapeKind::Freehand &&
            std::all_of(input.cbegin(), input.cend(), [&](const InputSample &point) {
                return point.position.x() == input.first().position.x() &&
                       point.position.y() == input.first().position.y();
            });
        if (stationaryFreehand && !pending && !append(input.last())) return std::nullopt;
        for (qsizetype i = 1; i < input.size(); ++i)
            if (!line(input[i - 1], input[i])) return std::nullopt;
    } else return std::nullopt;
    return result;
}
}
