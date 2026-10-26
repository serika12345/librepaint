/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuBrushDabs_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QPainterPath>
#include <QTransform>
#include <QVector2D>
#include <algorithm>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
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
}

qreal pressureValue(const PressureResponse &response, qreal pressure)
{
    if (response.kind == PressureResponse::Kind::Constant) return response.strength;
    qreal value = pressure;
    if (response.kind == PressureResponse::Kind::Curve) {
        const qreal x = std::clamp(255 * pressure, qreal(0), qreal(255));
        const auto low = std::size_t(std::floor(x)), high = std::size_t(std::ceil(x));
        const qreal fraction = x - std::floor(x);
        value = fraction < 1e-6 ? response.curve[low] : fraction > 1 - 1e-6 ? response.curve[high] :
            response.curve[low] + fraction * (response.curve[high] - response.curve[low]);
    }
    return std::clamp(response.strength * value, qreal(0), qreal(1));
}
bool forEachBrushDab(const PaintOperation &operation, bool pending,
                     const std::function<bool(const BrushDab &)> &visit)
{
    const auto &brush = operation.brush;
    if ((brush.basicSmoothing || brush.pixelPerfect) && operation.shape.kind != ShapeKind::Freehand)
        return false;
    const auto originalInput = operation.inputSamples();
    auto input = originalInput;
    if (brush.weightedSmoothing) {
        const auto corrected = weightedInput(input, *brush.weightedSmoothing);
        if (!corrected) return false;
        input = *corrected;
    } else if (brush.pixelPerfect) input = pixelPerfectInput(input);
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
    qsizetype count = 0;
    qreal spacing = .5, accumulated = 0;
    const auto append = [&](const InputSample &point) {
        const qreal scale = pressureValue(brush.sizeResponse, point.pressure);
        const qreal diameter = brush.diameter * scale;
        const bool tooSmall = brush.gaussianWash ? scale * std::max(1, int(brush.diameter)) < .01 :
                                                  scale * diameter < .01;
        if (tooSmall) { spacing = .5; return true; }
        if (count >= PaintGeometryElementLimit) return false;
        const auto metric = brush.gaussianWash ? characteristicSize(brush.diameter, scale, brush.gaussianWash->angle) :
                                                QSizeF(diameter, diameter);
        if (!visit({point.position, point.pressure, scale, metric})) return false;
        ++count;
        const qreal distance = !brush.gaussianWash && brush.autoSpacing ?
            brush.autoSpacingCoefficient * (diameter < 1 ? diameter : std::sqrt(diameter)) :
            metric.width() * brush.spacing;
        spacing = std::max(qreal(.5), distance);
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
                if (cubicSegments++ >= PaintGeometryElementLimit || !line(curve.first, curve.last)) return false;
                continue;
            }
            // Native brush calls subdivide cubics before placing dabs.
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
    if (input.isEmpty()) return true;
    if (brush.basicSmoothing) {
        InputSample previous = input.first(), older;
        QPointF previousTangent;
        bool haveTangent = false, paintIssued = false, inputMoved = false;
        qsizetype flushIndex = 0;
        const auto tangent = [](const InputSample &a, const InputSample &b) {
            return (b.position - a.position) / qreal(std::max<quint64>(1, b.timestamp - a.timestamp));
        };
        const auto basicCurve = [&](const InputSample &first, const InputSample &last,
                                    QPointF firstTangent, QPointF lastTangent) {
            const auto controls = basicCurveControls(first.position, last.position, firstTangent, lastTangent);
            if (!controls) return true;
            paintIssued = true;
            return cubic(first, (*controls)[0], (*controls)[1], last);
        };
        const auto finish = [&] {
            if (!haveTangent) return true;
            haveTangent = false;
            return basicCurve(older, previous, previousTangent, tangent(older, previous));
        };
        for (qsizetype index = 1; index < input.size(); ++index) {
            const auto &point = input[index];
            inputMoved = inputMoved || originalInput[index].position.x() != originalInput.first().position.x() ||
                                       originalInput[index].position.y() != originalInput.first().position.y();
            if (!haveTangent) {
                haveTangent = true;
                previousTangent = tangent(previous, point);
            } else {
                const auto nextTangent = tangent(older, point);
                if (nextTangent.isNull() || previousTangent.isNull()) {
                    paintIssued = true;
                    if (!line(previous, point)) return false;
                } else if (!basicCurve(older, previous, previousTangent, nextTangent)) {
                    return false;
                }
                previousTangent = nextTangent;
            }
            older = previous;
            previous = point;
            if (flushIndex < operation.smoothingFlushes.size() &&
                operation.smoothingFlushes[flushIndex] == index + 1) {
                if (!finish()) return false;
                ++flushIndex;
            }
        }
        if (operation.smoothingFinished) {
            const bool completeStationary = paintIssued && !inputMoved;
            if (!paintIssued) {
                if (!append(previous)) return false;
            } else if (!finish()) return false;
            if (completeStationary && count == 0) {
                auto tap = previous;
                // A stroke with unchanged raw positions finishes at that exact position.
                tap.position = originalInput.last().position;
                if (!append(tap)) return false;
            }
        }
        return true;
    }
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
                if (!cubic(previous, input[index].position, input[index + 1].position, input[index + 2])) return false;
                previous = input[index + 2];
                index += 3;
            } else {
                const auto point = input[index++];
                if (command == PathCommand::LineTo && !line(previous, point)) return false;
                previous = point;
            }
            if (call < operation.shape.pathCalls.size() && index == operation.shape.pathCalls[call].second) {
                spacing = lineSpacing;
                accumulated = lineAccumulated;
                ++call;
            }
        }
    } else if (operation.shape.kind == ShapeKind::Rectangle || operation.shape.kind == ShapeKind::Ellipse ||
               operation.shape.kind == ShapeKind::RoundedRectangle) {
        const auto &shape = operation.shape;
        const QRectF bounds = QRectF(shape.first, shape.second).normalized();
        QTransform transform;
        if (shape.rotation != 0) {
            const auto center = bounds.center();
            transform.translate(center.x(), center.y()).rotateRadians(shape.rotation).translate(-center.x(), -center.y());
        }
        const auto point = [&](QPointF position) {
            auto sample = input.first();
            sample.position = transform.map(position);
            return sample;
        };
        if (shape.kind == ShapeKind::Rectangle) {
            const QList<QPointF> corners{bounds.topLeft(), bounds.bottomLeft(), bounds.bottomRight(), bounds.topRight()};
            for (qsizetype i = 0; i < corners.size(); ++i)
                if (!line(point(corners[i]), point(corners[(i + 1) % corners.size()]))) return false;
        } else if (shape.kind == ShapeKind::Ellipse) {
            if (bounds.isEmpty()) return true;
            constexpr qreal kappa = .5522847498;
            const auto center = bounds.center();
            const qreal lx = (bounds.width() / 2) * kappa, ly = (bounds.height() / 2) * kappa;
            const QList<QPointF> outline{{bounds.left(), center.y()}, {bounds.left(), center.y() - ly},
                {center.x() - lx, bounds.top()}, {center.x(), bounds.top()},
                {center.x() + lx, bounds.top()}, {bounds.right(), center.y() - ly}, {bounds.right(), center.y()},
                {bounds.right(), center.y() + ly}, {center.x() + lx, bounds.bottom()}, {center.x(), bounds.bottom()},
                {center.x() - lx, bounds.bottom()}, {bounds.left(), center.y() + ly}, {bounds.left(), center.y()}};
            for (qsizetype i = 0; i < 12; i += 3)
                if (!cubic(point(outline[i]), transform.map(outline[i + 1]), transform.map(outline[i + 2]),
                           point(outline[i + 3]))) return false;
        } else {
            QPainterPath path;
            path.addRoundedRect(bounds, shape.cornerRadii.width(), shape.cornerRadii.height(), Qt::AbsoluteSize);
            path = transform.map(path);
            InputSample previous;
            bool havePoint = false;
            for (int i = 0; i < path.elementCount(); ++i) {
                const auto element = path.elementAt(i);
                auto sample = input.first();
                sample.position = {element.x, element.y};
                if (element.isMoveTo()) { previous = sample; havePoint = true; }
                else if (element.isLineTo()) {
                    if (havePoint && !line(previous, sample)) return false;
                    previous = sample;
                } else if (element.type == QPainterPath::CurveToElement) {
                    if (i + 2 >= path.elementCount()) return false;
                    const auto control2 = path.elementAt(i + 1), last = path.elementAt(i + 2);
                    auto end = input.first(); end.position = {last.x, last.y};
                    if (!cubic(previous, sample.position, {control2.x, control2.y}, end)) return false;
                    previous = end;
                    i += 2;
                }
            }
        }
    } else if (operation.shape.kind == ShapeKind::Line || operation.shape.kind == ShapeKind::Freehand ||
               operation.shape.kind == ShapeKind::Polyline || operation.shape.kind == ShapeKind::Polygon) {
        const bool stationaryFreehand = operation.shape.kind == ShapeKind::Freehand &&
            std::all_of(input.cbegin(), input.cend(), [&](const InputSample &point) {
                return point.position.x() == input.first().position.x() &&
                       point.position.y() == input.first().position.y();
            });
        if (stationaryFreehand && !pending && !append(input.last())) return false;
        for (qsizetype i = 1; i < input.size(); ++i)
            if (!line(input[i - 1], input[i])) return false;
        if (operation.shape.kind == ShapeKind::Polygon && input.size() > 1 &&
            !line(input.last(), input.first())) return false;
    } else return false;
    return true;
}

}
