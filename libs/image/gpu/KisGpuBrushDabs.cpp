/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuBrushDabs_p.h"
#include "KisGpuPaintGeometry_p.h"
#include <QPainterPath>
#include <QTransform>
#include <QVector2D>
#include <QDebug>
#include <boost/random/taus88.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <algorithm>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
QSizeF characteristicSize(qreal diameter, qreal ratio, qreal scale, qreal angle)
{
    constexpr qreal circle = 6.283185307179586476925286766559;
    angle = std::fmod(angle, circle);
    if (angle < 0) angle += circle;
    QTransform transform;
    transform.scale(scale, scale);
    if (!qFuzzyCompare(angle, qreal(0))) transform = transform * QTransform().rotateRadians(angle);
    return transform.mapRect(QRectF(0, 0, std::max(1, int(diameter)), std::max(1, int(diameter * ratio)))).size();
}
}

namespace {
qreal tiltElevation(const InputSample &sample)
{
    const qreal x = std::clamp(sample.xTilt / 60., -1., 1.);
    const qreal y = std::clamp(sample.yTilt / 60., -1., 1.);
    const qreal minor = std::abs(x) > std::abs(y) ? y : x;
    return std::acos(std::sqrt(x * x + y * y) / std::sqrt(1 + minor * minor)) / (M_PI * .5);
}
qreal tiltDirection(const InputSample &sample)
{
    qreal angle = qFuzzyIsNull(sample.xTilt) && qFuzzyIsNull(sample.yTilt) ?
        -(sample.xTilt >= 0 ? 1 : -1) * (sample.yTilt >= 0 ? 1 : -1) * M_PI_2 :
        std::atan2(-sample.xTilt, sample.yTilt);
    if (!qFuzzyIsNull(sample.tiltDirectionOffset)) {
        angle += sample.tiltDirectionOffset * M_PI / 180.;
        if (angle < -M_PI) angle += 2 * M_PI;
        else if (angle > M_PI) angle -= 2 * M_PI;
    }
    return angle / (2 * M_PI) + .5;
}
qreal responseCurveValue(const PressureResponse &response, qreal pressure)
{
    if (response.kind == PressureResponse::Kind::Constant) return 1;
    qreal value = pressure;
    if (response.kind == PressureResponse::Kind::Curve) {
        const qreal x = std::clamp(255 * pressure, qreal(0), qreal(255));
        const auto low = std::size_t(std::floor(x)), high = std::size_t(std::ceil(x));
        const qreal fraction = x - std::floor(x);
        value = fraction < 1e-6 ? response.curve[low] : fraction > 1 - 1e-6 ? response.curve[high] :
            response.curve[low] + fraction * (response.curve[high] - response.curve[low]);
    }
    return value;
}
}
qreal pressureValue(const PressureResponse &response, qreal pressure)
{ return std::clamp(response.strength * responseCurveValue(response, pressure), qreal(0), qreal(1)); }
bool forEachBrushDab(const PaintOperation &operation, bool pending,
                     const std::function<bool(const BrushDab &)> &visit)
{
    const auto &brush = operation.brush;
    if ((brush.basicSmoothing || brush.pixelPerfect) && operation.shape.kind != ShapeKind::Freehand)
        return false;
    const auto originalInput = operation.inputSamples();
    const bool directLines = brush.autoBrush && brush.autoBrush->sharpnessResponse &&
        (brush.autoBrush->imageTips ? brush.autoBrush->imageTips->originalSize == QSize(1, 1) :
         std::max(1, int(brush.diameter)) == 1 && std::max(1, int(brush.diameter * brush.autoBrush->ratio)) == 1);
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
    const auto tipMetric = [&](qreal scale, qreal angle) {
        if (brush.autoBrush && brush.autoBrush->imageTips) {
            const auto &size = brush.autoBrush->imageTips->originalSize;
            QTransform transform;
            const qreal ratio = brush.diameter / std::max(size.width(), size.height());
            transform.scale(scale * ratio, scale * ratio);
            angle = std::fmod(angle, 2 * M_PI);
            if (angle < 0) angle += 2 * M_PI;
            if (!qFuzzyCompare(angle, qreal(0))) transform = transform * QTransform().rotateRadians(angle);
            return transform.mapRect(QRectF(QPointF(), size)).size();
        }
        return brush.autoBrush ? characteristicSize(brush.diameter, brush.autoBrush->ratio, scale, angle) :
                                 QSizeF(brush.diameter * scale, brush.diameter * scale);
    };
    qsizetype count = 0;
    qsizetype sequence = 0;
    QPointF spacing(.5, .5), accumulated;
    qreal spacingRotation = 0;
    boost::taus88 random(quint32(operation.randomSeed));
    QPointF lastDabPosition = operation.shape.kind == ShapeKind::Freehand && !input.isEmpty() ? input.first().position : QPointF();
    qreal lastDrawingAngle = input.isEmpty() ? 0 : input.first().initialDrawingAngle;
    InputSample lastPaintInformation;
    bool haveLastPaintInformation = false;
    const auto direction = [](QPointF first, QPointF last, qreal fallback) -> qreal {
        if (qFuzzyCompare(first.x(), last.x()) && qFuzzyCompare(first.y(), last.y())) return fallback;
        const QVector2D delta(last - first);
        return std::atan2(delta.y(), delta.x());
    };
    const auto wrap = [](qreal value, qreal low, qreal high) {
        value = std::fmod(value - low, high - low);
        return (value < 0 ? value + high - low : value) + low;
    };
    const auto append = [&](const InputSample &point, bool explicitPoint = false,
                            std::optional<qreal> overrideAngle = {}) {
        if (directLines && operation.shape.kind != ShapeKind::Freehand && !explicitPoint) return true;
        const auto dabSequence = sequence++;
        const qreal drawingAngle = overrideAngle.value_or(direction(lastDabPosition, point.position, lastDrawingAngle));
        lastDabPosition = point.position;
        lastDrawingAngle = drawingAngle;
        lastPaintInformation = point;
        haveLastPaintInformation = true;
        const qreal scale = brush.sizeSpeedResponse ?
            std::clamp(brush.sizeResponse.strength * (responseCurveValue(brush.sizeResponse, point.pressure) *
                responseCurveValue(*brush.sizeSpeedResponse, point.speed)), qreal(0), qreal(1)) :
            pressureValue(brush.sizeResponse, brush.sizeSensor == Brush::SizeSensor::TiltElevation ? tiltElevation(point) : point.pressure);
        const qreal diameter = brush.diameter * scale;
        const auto originalSize = brush.autoBrush && brush.autoBrush->imageTips ? brush.autoBrush->imageTips->originalSize : QSize();
        const bool tooSmall = !originalSize.isEmpty() ?
            diameter * originalSize.width() / std::max(originalSize.width(), originalSize.height()) < .01 ||
            diameter * originalSize.height() / std::max(originalSize.width(), originalSize.height()) < .01 :
            brush.autoBrush ?
            scale * std::max(1, int(brush.diameter)) < .01 ||
            scale * std::max(1, int(brush.diameter * brush.autoBrush->ratio)) < .01 :
                                                  scale * diameter < .01;
        if (tooSmall) { spacing = {.5, .5}; return true; }
        if (count >= PaintGeometryElementLimit) {
            qWarning() << "GPU brush dab count exceeds the geometry limit" << count;
            return false;
        }
        qreal sensorRotation = point.canvasRotation * M_PI / 180.;
        if (brush.autoBrush && brush.autoBrush->constantRotation) {
            // Preserve the native wrap before converting checked rotation to radians.
            const qreal normalizedBaseAngle = -point.canvasRotation / 360.;
            qreal value = std::fmod(2 * normalizedBaseAngle + 1., 2.);
            if (value < 0) value += 2.;
            value -= 1.;
            sensorRotation = (1 - value) * M_PI;
            if (sensorRotation >= 2 * M_PI) sensorRotation = std::fmod(sensorRotation, 2 * M_PI);
        } else if (brush.autoBrush && brush.autoBrush->tiltDirectionRotation) {
            const auto &response = *brush.autoBrush->tiltDirectionRotation;
            const qreal additive = 2 * responseCurveValue(response, tiltDirection(point)) - 1;
            const qreal normalizedBaseAngle = -point.canvasRotation / 360.;
            const qreal value = wrap(2 * normalizedBaseAngle + response.strength * additive, -1, 1);
            sensorRotation = (1 - value) * M_PI;
            if (sensorRotation >= 2 * M_PI) sensorRotation = std::fmod(sensorRotation, 2 * M_PI);
        } else if (brush.autoBrush && brush.autoBrush->drawingAngleRotation) {
            const auto &settings = *brush.autoBrush;
            qreal offset = .5 + drawingAngle / (2 * M_PI) + settings.drawingAngleOffset / 360.;
            if (offset > 1) offset -= 1;
            if (settings.drawingAngleRotation->kind == PressureResponse::Kind::Curve)
                offset = wrap(responseCurveValue(*settings.drawingAngleRotation, wrap(offset + .5, 0, 1)) + .5, 0, 1);
            if (point.canvasMirroredH != point.canvasMirroredV) offset = .5 - offset;
            sensorRotation = (1 - wrap(2 * offset, -1, 1)) * M_PI;
            if (sensorRotation >= 2 * M_PI) sensorRotation = std::fmod(sensorRotation, 2 * M_PI);
        }
        const qreal angle = brush.autoBrush ? brush.autoBrush->angle + sensorRotation : 0;
        const auto metric = tipMetric(scale, angle);
        QPointF position = point.position;
        if (brush.autoBrush && brush.autoBrush->scatter && (brush.autoBrush->scatterAxisX || brush.autoBrush->scatterAxisY)) {
            const auto &settings = *brush.autoBrush;
            const auto side = [](qreal value) {
                if (std::abs(value - qRound(value)) < 1e-6) value = qRound(value);
                return std::max(1, int(std::ceil(value)));
            };
            const qreal extent = std::max(side(metric.width()), side(metric.height()));
            const qreal amount = std::clamp(settings.scatter->strength * responseCurveValue(*settings.scatter, point.pressure), qreal(0), qreal(5));
            const auto jitter = [&] { return (2. * (qreal(random()) / random.max()) - 1.) * extent * amount; };
            const qreal x = jitter();
            if (settings.scatterAxisX && settings.scatterAxisY) position += QPointF(x, jitter());
            else {
                QVector2D movement(std::cos(drawingAngle), std::sin(drawingAngle));
                if (settings.scatterAxisY) movement = QVector2D(-movement.y(), movement.x());
                movement *= x;
                position += movement.toPointF();
            }
        }
        quint32 imageIndex = 0;
        if (brush.autoBrush && brush.autoBrush->imageTips && brush.autoBrush->imageTips->randomSelection) {
            boost::uniform_smallint<int> select(0, brush.autoBrush->imageTips->tipCount - 1);
            imageIndex = select(random);
        }
        if (!visit({position, point.pressure, scale, metric, angle, point.canvasMirroredH, point.canvasMirroredV, dabSequence, false, imageIndex})) return false;
        ++count;
        const auto distance = [&](qreal side) {
            return brush.autoSpacing ? brush.autoSpacingCoefficient * (side < 1 ? side : std::sqrt(side)) : side * brush.spacing;
        };
        const bool isotropic = brush.autoBrush && brush.autoBrush->isotropicSpacing;
        const auto spacingMetric = brush.autoBrush ?
            tipMetric(scale, brush.autoBrush->angle) : metric;
        const qreal width = isotropic ? std::max(spacingMetric.width(), spacingMetric.height()) : spacingMetric.width();
        const qreal height = isotropic ? width : spacingMetric.height();
        spacing = {distance(width), distance(height)};
        spacingRotation = isotropic || !brush.autoBrush ? 0 :
            point.canvasMirroredH != point.canvasMirroredV ? 2 * M_PI - sensorRotation : sensorRotation;
        return true;
    };
    const auto paintLineDab = [&](const InputSample &point) {
        if (brush.autoBrush && brush.autoBrush->fanCornersStep > 0 && haveLastPaintInformation) {
            const qreal step = brush.autoBrush->fanCornersStep;
            const auto distance = [](qreal first, qreal last) {
                qreal value = std::fmod(std::abs(first - last), 2 * M_PI);
                return value > M_PI ? 2 * M_PI - value : value;
            };
            const qreal finalAngle = direction(lastDabPosition, point.position, lastDrawingAngle);
            const qreal fullDistance = distance(lastDrawingAngle, finalAngle);
            qreal angle = lastDrawingAngle;
            int index = 0;
            while (distance(angle, finalAngle) > step) {
                angle = distance(angle + step, finalAngle) < distance(angle - step, finalAngle) ? angle + step : angle - step;
                const qreal fraction = step * index++ / fullDistance;
                auto fan = lastPaintInformation;
                fan.position = fan.position + fraction * (point.position - fan.position);
                fan.pressure = (1 - fraction) * fan.pressure + fraction * point.pressure;
                fan.speed = (1 - fraction) * fan.speed + fraction * point.speed;
                fan.xTilt = (1 - fraction) * fan.xTilt + fraction * point.xTilt;
                fan.yTilt = (1 - fraction) * fan.yTilt + fraction * point.yTilt;
                if (!append(fan, false, angle)) return false;
            }
        }
        return append(point);
    };
    const auto line = [&](InputSample first, const InputSample &last) {
        if (directLines) {
            if (first.position == last.position && operation.shape.kind == ShapeKind::Freehand) return true;
            int x = int(std::floor(first.position.x())), y = int(std::floor(first.position.y()));
            const int endX = int(std::floor(last.position.x())), endY = int(std::floor(last.position.y()));
            const int dx = endX - x, dy = endY - y;
            float slope = 0;
            bool locked = true;
            if (dx == 0) slope = 2;
            else if (dy != 0) { slope = float(dy) / float(dx); locked = false; }
            float fx = x, fy = y;
            const auto pixel = [&] {
                if (count == PaintGeometryElementLimit) return false;
                // Native sharp one-pixel lines use DDA and bypass the tip,
                // pressure, paper and distance state of ordinary dabs.
                if (!visit({QPointF(x, y), 1, 1, QSizeF(1, 1), 0, false, false, sequence, true})) return false;
                ++count;
                return true;
            };
            if (!pixel()) return false;
            if (std::abs(slope) > 1) {
                const int step = dy > 0 ? 1 : -1;
                slope = (locked ? 0.f : 1.f / slope) * step;
                while (y != endY) {
                    y += step; fx += slope; x = qRound(fx);
                    if (!pixel()) return false;
                }
            } else {
                const int step = dx > 0 ? 1 : -1;
                slope *= step;
                while (x != endX) {
                    x += step; fy += slope; y = qRound(fy);
                    if (!pixel()) return false;
                }
            }
            return true;
        }
        while (first.position != last.position) {
            qreal fraction;
            auto delta = last.position - first.position;
            if (spacing.x() == spacing.y()) {
                const qreal length = QVector2D(delta).length();
                const qreal next = std::max(qreal(.5), spacing.x()) - accumulated.x();
                if (next > length) { accumulated.rx() += length; return true; }
                fraction = next <= 0 ? 0 : next / length;
            } else {
                const qreal a = 1. / std::max(qreal(.5), spacing.x());
                const qreal b = 1. / std::max(qreal(.5), spacing.y());
                const qreal x = accumulated.x(), y = accumulated.y();
                const qreal gamma = (x * a) * (x * a) + (y * b) * (y * b) - 1;
                if (gamma >= 0) fraction = 0;
                else {
                    if (spacingRotation > 2e-3) delta = QTransform().rotateRadians(spacingRotation).map(delta);
                    const qreal dx = std::abs(delta.x()), dy = std::abs(delta.y());
                    const qreal alpha = (dx * a) * (dx * a) + (dy * b) * (dy * b);
                    const qreal beta = x * dx * a * a + y * dy * b * b;
                    const qreal discriminant = beta * beta - alpha * gamma;
                    if (discriminant < 0) {
                        qWarning() << "GPU brush elliptical spacing has no solution" << first.position << last.position;
                        return false;
                    }
                    fraction = (-beta + std::sqrt(discriminant)) / alpha;
                    if (fraction < 0 || fraction > 1) { accumulated += QPointF(dx, dy); return true; }
                }
            }
            first.position = (1 - fraction) * first.position + fraction * last.position;
            first.pressure = (1 - fraction) * first.pressure + fraction * last.pressure;
            first.speed = (1 - fraction) * first.speed + fraction * last.speed;
            first.xTilt = (1 - fraction) * first.xTilt + fraction * last.xTilt;
            first.yTilt = (1 - fraction) * first.yTilt + fraction * last.yTilt;
            accumulated = {};
            // Native spacing can emit an unchanged rounded position while
            // resetting its distance. The dab limit bounds these iterations.
            if (!paintLineDab(first)) return false;
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
            middle.speed = .5 * curve.first.speed + .5 * curve.last.speed;
            middle.xTilt = .5 * curve.first.xTilt + .5 * curve.last.xTilt;
            middle.yTilt = .5 * curve.first.yTilt + .5 * curve.last.yTilt;
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
    if (operation.shape.kind == ShapeKind::Path || !operation.shape.pathCommands.isEmpty()) {
        qsizetype index = 0, call = 0;
        QPointF lineSpacing = spacing, lineAccumulated = accumulated;
        qreal lineRotation = spacingRotation;
        qsizetype lineSequence = sequence;
        QPointF lineLastPosition;
        qreal lineLastAngle = 0;
        InputSample lineLastInformation;
        bool lineHaveLastInformation = false;
        InputSample previous;
        for (const auto command : operation.shape.pathCommands) {
            // A painter-path call owns its distance, while opacity averaging
            // belongs to the whole brush stroke, including intervening lines.
            if (call < operation.shape.pathCalls.size() && index == operation.shape.pathCalls[call].first) {
                lineSpacing = spacing;
                lineAccumulated = accumulated;
                lineRotation = spacingRotation;
                lineSequence = sequence;
                lineLastPosition = lastDabPosition;
                lineLastAngle = lastDrawingAngle;
                lineLastInformation = lastPaintInformation;
                lineHaveLastInformation = haveLastPaintInformation;
                spacing = {.5, .5};
                accumulated = {};
                spacingRotation = 0;
                sequence = 0;
                lastDabPosition = {};
                lastDrawingAngle = 0;
                haveLastPaintInformation = false;
            }
            if (command == PathCommand::CubicTo) {
                if (!cubic(previous, input[index].position, input[index + 1].position, input[index + 2])) return false;
                previous = input[index + 2];
                index += 3;
            } else {
                const auto point = input[index++];
                if (command == PathCommand::Point) {
                    if (!append(point, true)) return false;
                    accumulated = {};
                }
                if (command == PathCommand::LineTo && !line(previous, point)) return false;
                previous = point;
            }
            if (call < operation.shape.pathCalls.size() && index == operation.shape.pathCalls[call].second) {
                spacing = lineSpacing;
                accumulated = lineAccumulated;
                spacingRotation = lineRotation;
                sequence = lineSequence;
                lastDabPosition = lineLastPosition;
                lastDrawingAngle = lineLastAngle;
                lastPaintInformation = lineLastInformation;
                haveLastPaintInformation = lineHaveLastInformation;
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
