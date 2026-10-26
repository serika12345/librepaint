/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintGeometry_p.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <boost/multiprecision/cpp_int.hpp>
#include <QSet>
#include <QLineF>
#include <QVector2D>
#include <QPainterPath>
#include <QTransform>

namespace LibrePaint::Image::Gpu {
namespace {
bool closedSegmentsHaveUniqueIntersection(QPointF a, QPointF b, QPointF c, QPointF d)
{
    static_assert(sizeof(qreal) == sizeof(quint64) && std::numeric_limits<qreal>::is_iec559 &&
                  std::numeric_limits<qreal>::radix == 2 && std::numeric_limits<qreal>::digits == 53 &&
                  std::numeric_limits<qreal>::min_exponent == -1021 &&
                  std::numeric_limits<qreal>::max_exponent == 1024);
    const std::array<qreal, 8> values{a.x(), a.y(), b.x(), b.y(), c.x(), c.y(), d.x(), d.y()};
    std::array<quint64, 8> bits, significands;
    std::array<int, 8> exponents;
    int smallestExponent = std::numeric_limits<int>::max();
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::memcpy(&bits[i], &values[i], sizeof(qreal));
        const auto exponent = (bits[i] >> 52) & 0x7ff;
        if (exponent == 0x7ff) return false;
        significands[i] = bits[i] & ((quint64(1) << 52) - 1);
        exponents[i] = exponent ? int(exponent) - 1075 : -1074;
        if (exponent) significands[i] |= quint64(1) << 52;
        if (significands[i]) smallestExponent = std::min(smallestExponent, exponents[i]);
    }
    // Keep the represented endpoints exact before taking differences and cross products.
    using boost::multiprecision::cpp_int;
    std::array<cpp_int, 8> coordinates;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!significands[i]) continue;
        coordinates[i] = cpp_int(significands[i]) << (exponents[i] - smallestExponent);
        if (bits[i] >> 63) coordinates[i] = -coordinates[i];
    }
    const cpp_int rx = coordinates[2] - coordinates[0], ry = coordinates[3] - coordinates[1];
    const cpp_int sx = coordinates[6] - coordinates[4], sy = coordinates[7] - coordinates[5];
    const cpp_int wx = coordinates[4] - coordinates[0], wy = coordinates[5] - coordinates[1];
    cpp_int denominator = rx * sy - ry * sx;
    if (denominator == 0) return false;
    cpp_int firstNumerator = wx * sy - wy * sx;
    cpp_int secondNumerator = wx * ry - wy * rx;
    if (denominator < 0) {
        denominator = -denominator;
        firstNumerator = -firstNumerator;
        secondNumerator = -secondNumerator;
    }
    return firstNumerator >= 0 && firstNumerator <= denominator &&
           secondNumerator >= 0 && secondNumerator <= denominator;
}
}

std::optional<std::array<QPointF, 2>> basicCurveControls(QPointF first, QPointF last,
                                                      QPointF tangent1, QPointF tangent2)
{
    if (tangent1.isNull() || tangent2.isNull()) return std::nullopt;
    QLineF line1(first, first + tangent1);
    QLineF line2(last, last - tangent2);
    QLineF controls(line1.p2(), line2.p2());
    QLineF chord(first, last);
    QPointF intersection, target1, target2;
    if (closedSegmentsHaveUniqueIntersection(controls.p1(), controls.p2(), chord.p1(), chord.p2())) {
        const auto length = chord.length() / 2;
        line1.setLength(length);
        line2.setLength(length);
        target1 = line1.p2();
        target2 = line2.p2();
    } else {
        const auto type = line1.intersects(line2, &intersection);
        if (type == QLineF::NoIntersection || intersection.manhattanLength() > 1e6)
            intersection = (first + last) / 2;
        target1 = target2 = intersection;
    }
    auto velocity1 = QLineF(QPointF(), tangent1).length();
    auto velocity2 = QLineF(QPointF(), tangent2).length();
    if (velocity1 == 0 || velocity2 == 0) velocity1 = velocity2 = 1e-6;
    const auto similarity = std::max(qreal(0.5), std::min(velocity1 / velocity2, velocity2 / velocity1));
    qreal coefficient = qreal(0.8) * (1 - std::max(qreal(0), similarity - qreal(0.8)));
    QPointF control1, control2;
    if (velocity1 > velocity2) {
        control1 = first * (1 - coefficient) + coefficient * target1;
        coefficient *= similarity;
        control2 = last * (1 - coefficient) + coefficient * target2;
    } else {
        control2 = last * (1 - coefficient) + coefficient * target2;
        coefficient *= similarity;
        control1 = first * (1 - coefficient) + coefficient * target1;
    }
    return std::array<QPointF, 2>{control1, control2};
}

namespace Detail {

std::optional<QList<InputSample>> weightedInput(const QList<InputSample> &input, const WeightedSmoothing &settings)
{
    auto points = input;
    if (settings.distanceMin == 0 && settings.distanceMax == 0) return points;
    QList<qreal> distances;
    distances.reserve(input.size());
    distances.append(0);
    // Bound replay even for stationary input whose Gaussian cutoff never advances.
    constexpr qsizetype maximumWeights = 16 * 1024 * 1024;
    qsizetype weights = 0;
    for (qsizetype index = 1; index < points.size(); ++index) {
        distances.append(QVector2D(points[index].position - points[index - 1].position).length());
        if (index < 4) continue;
        const auto speed = points[index - 1].speed;
        const qreal sigma = ((1 - speed) * settings.distanceMax + speed * settings.distanceMin) / 3;
        const qreal variance = sigma * sigma;
        const qreal gaussian = sigma > 0 ? 1 / (std::sqrt(2 * M_PI) * sigma) : 0;
        qreal distanceSum = 0, scaleSum = 0, x = 0, y = 0, pressureSum = 0, baseRate = 0;
        for (qsizetype i = index; i >= 1; --i) {
            if (++weights > maximumWeights) return std::nullopt;
            const auto &point = points[i];
            qreal distance = distances[i];
            if (i < index) {
                auto gradient = point.pressure - points[i + 1].pressure;
                if (gradient > 0) {
                    gradient *= (40 * settings.tailAggressiveness) * (1 - point.pressure);
                    distance += gradient * 3 * sigma;
                }
            }
            qreal rate = 0;
            if (variance != 0) {
                distanceSum += distance;
                rate = gaussian * std::exp(-distanceSum * distanceSum / (2 * variance));
            }
            if (i == index) baseRate = rate;
            else if (baseRate / rate > 100) break;
            scaleSum += rate;
            x += rate * point.position.x();
            y += rate * point.position.y();
            if (settings.smoothPressure) pressureSum += rate * point.pressure;
        }
        if (scaleSum != 0) {
            x /= scaleSum;
            y /= scaleSum;
            if (settings.smoothPressure) pressureSum /= scaleSum;
        }
        if ((x != 0 && y != 0) || (x == points[index].position.x() && y == points[index].position.y())) {
            points[index].position = {x, y};
            if (settings.smoothPressure) points[index].pressure = pressureSum;
        }
    }
    return points;
}


QList<InputSample> pixelPerfectInput(const QList<InputSample> &input)
{
    if (input.size() < 2) return input;
    QList<InputSample> points;
    points.reserve(input.size() - 1);
    std::optional<InputSample> tentative;
    QPoint lastPixel;
    const auto adjacent = [&](QPoint pixel) {
        return std::abs(pixel.x() - lastPixel.x()) <= 1 && std::abs(pixel.y() - lastPixel.y()) <= 1;
    };
    for (qsizetype index = 1; index < input.size(); ++index) {
        auto point = input[index];
        const QPoint pixel(int(std::floor(point.position.x())), int(std::floor(point.position.y())));
        point.position = QPointF(pixel) + QPointF(0.5, 0.5);
        if (points.isEmpty()) {
            points.append(point);
            lastPixel = pixel;
            continue;
        }
        if (tentative && !adjacent(pixel)) {
            points.append(*tentative);
            lastPixel = QPoint(int(std::floor(tentative->position.x())), int(std::floor(tentative->position.y())));
            tentative.reset();
        }
        if (adjacent(pixel)) tentative = point;
        else {
            points.append(point);
            lastPixel = pixel;
        }
    }
    return points;
}


std::optional<QList<FigureFillEdge>> figureFillEdges(const PaintOperation &operation)
{
    QList<FigureFillEdge> result;
    if (!hasFigureFill(operation.figureStyle)) return result;
    const auto &shape = operation.shape;
    const auto input = operation.inputSamples();
    QPainterPath path;
    const QRectF bounds = QRectF(shape.first, shape.second).normalized();
    if (shape.kind == ShapeKind::Ellipse) path.addEllipse(bounds);
    else if (shape.kind == ShapeKind::Rectangle) path.addRect(bounds);
    else if (shape.kind == ShapeKind::RoundedRectangle)
        path.addRoundedRect(bounds, shape.cornerRadii.width(), shape.cornerRadii.height(), Qt::AbsoluteSize);
    else if (shape.kind == ShapeKind::Path) {
        qsizetype i = 0;
        for (const auto command : shape.pathCommands) {
            if (command == PathCommand::MoveTo) path.moveTo(input[i++].position);
            else if (command == PathCommand::LineTo) path.lineTo(input[i++].position);
            else {
                path.cubicTo(input[i].position, input[i + 1].position, input[i + 2].position);
                i += 3;
            }
        }
    } else if (!input.isEmpty()) {
        path.moveTo(input.first().position);
        for (qsizetype i = 1; i < input.size(); ++i) path.lineTo(input[i].position);
    }
    if (shape.rotation != 0 && (shape.kind == ShapeKind::Ellipse || shape.kind == ShapeKind::Rectangle ||
                                shape.kind == ShapeKind::RoundedRectangle)) {
        const auto center = bounds.center();
        QTransform rotation;
        rotation.translate(center.x(), center.y()).rotateRadians(shape.rotation).translate(-center.x(), -center.y());
        path = rotation.map(path);
    }
    // Qt's outline mapper flattens at a quarter pixel before 26.6 quantization.
    // Keep geometry here; integer cell coverage and compositing run on the GPU.
    // Rules: qtbase v6.11.1 qoutlinemapper.cpp and qbezier.cpp (GPL-2.0).
    QPointF first, previous;
    bool subpath = false;
    const auto mirroredRounding = [](qreal raw, qint32 rounded, int axis) {
        return qint32(qRound64((qreal(2) * axis - raw) * 64) - (qint64(128) * axis - rounded));
    };
    const auto append = [&](QPointF point) {
        const qint32 ax = qRound(previous.x() * 64), ay = qRound(previous.y() * 64);
        const qint32 bx = qRound(point.x() * 64), by = qRound(point.y() * 64);
        const auto center = operation.brush.mirrorCenter;
        const std::array<qint32, 4> rounding{mirroredRounding(previous.x(), ax, center.x()),
                                            mirroredRounding(previous.y(), ay, center.y()),
                                            mirroredRounding(point.x(), bx, center.x()),
                                            mirroredRounding(point.y(), by, center.y())};
        if (ax != bx || ay != by || rounding[0] != rounding[2] || rounding[1] != rounding[3])
            result.append({ax, ay, bx, by, rounding});
        previous = point;
        return result.size() <= PaintGeometryElementLimit;
    };
    const auto flatten = [&](const auto &self, const std::array<QPointF, 4> &p, int depth) -> bool {
        const auto chord = p[3] - p[0], firstControl = p[0] - p[1], secondControl = p[0] - p[2];
        const qreal length = qAbs(chord.x()) + qAbs(chord.y());
        const qreal deviation = length > 1 ?
            qAbs(chord.x() * firstControl.y() - chord.y() * firstControl.x()) +
                qAbs(chord.x() * secondControl.y() - chord.y() * secondControl.x()) :
            qAbs(firstControl.x()) + qAbs(firstControl.y()) + qAbs(secondControl.x()) + qAbs(secondControl.y());
        if (depth == 0 || deviation < .25 * std::max(length, qreal(1))) return append(p[3]);
        const auto a = (p[0] + p[1]) * .5, b = (p[1] + p[2]) * .5, c = (p[2] + p[3]) * .5;
        const auto d = (a + b) * .5, e = (b + c) * .5, middle = (d + e) * .5;
        return self(self, {p[0], a, d, middle}, depth - 1) && self(self, {middle, e, c, p[3]}, depth - 1);
    };
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto point = path.elementAt(i);
        if (point.isMoveTo()) {
            if (subpath && !append(first)) return std::nullopt;
            first = previous = point;
            subpath = true;
        } else if (point.isCurveTo()) {
            if (!flatten(flatten, {previous, point, path.elementAt(i + 1), path.elementAt(i + 2)}, 9)) return std::nullopt;
            i += 2;
        } else if (!append(point)) return std::nullopt;
    }
    if (subpath && !append(first)) return std::nullopt;
    return result;
}


std::optional<QList<quint64>> figureFillTiles(const QList<FigureFillEdge> &edges, const Brush &brush,
                                             QRect region, qsizetype maximumCoordinates, int tileSize)
{
    if (tileSize <= 0) return std::nullopt;
    if (edges.isEmpty()) return QList<quint64>{};
    QSet<quint64> coordinates;
    const int mirrorMode = int(brush.mirrorHorizontal) | (int(brush.mirrorVertical) << 1);
    for (int reflection = 0; reflection < 4; ++reflection) {
        if ((reflection & mirrorMode) != reflection) continue;
        const auto mapped = [&](qint32 x, qint32 y, qint32 dx, qint32 dy) {
            return std::array<qint64, 2>{reflection & 1 ? qint64(128) * brush.mirrorCenter.x() - x + dx : x,
                                         reflection & 2 ? qint64(128) * brush.mirrorCenter.y() - y + dy : y};
        };
        const auto &edge = edges.first();
        const auto origin = mapped(edge.ax, edge.ay, edge.mirroredRounding[0], edge.mirroredRounding[1]);
        qint64 left = origin[0], right = left, top = origin[1], bottom = top;
        qint64 directionX = 0, directionY = 0;
        bool hasArea = false;
        for (const auto &edge : edges) {
            const auto &rounding = edge.mirroredRounding;
            for (const auto &point : {mapped(edge.ax, edge.ay, rounding[0], rounding[1]),
                                      mapped(edge.bx, edge.by, rounding[2], rounding[3])}) {
                left = std::min(left, point[0]);
                right = std::max(right, point[0]);
                top = std::min(top, point[1]);
                bottom = std::max(bottom, point[1]);
                const qint64 dx = point[0] - origin[0], dy = point[1] - origin[1];
                if (directionX == 0 && directionY == 0) { directionX = dx; directionY = dy; }
                else if (directionX * dy != directionY * dx) hasArea = true;
            }
        }
        if (!hasArea) continue;
        const QRect bounds(QPoint(int(std::floor(qreal(left) / 64)), int(std::floor(qreal(top) / 64))),
                           QPoint(int(std::ceil(qreal(right) / 64)) - 1, int(std::ceil(qreal(bottom) / 64)) - 1));
        const QRect clipped = bounds.intersected(region);
        if (clipped.isEmpty()) continue;
        const int x0 = pixelTileIndex(clipped.left(), tileSize), x1 = pixelTileIndex(clipped.right(), tileSize);
        const int y0 = pixelTileIndex(clipped.top(), tileSize), y1 = pixelTileIndex(clipped.bottom(), tileSize);
        if (qint64(x1 - x0 + 1) * (y1 - y0 + 1) > maximumCoordinates) return std::nullopt;
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                coordinates.insert(key(x, y));
                if (coordinates.size() > maximumCoordinates) return std::nullopt;
            }
        }
    }
    return coordinates.values();
}


}
}
