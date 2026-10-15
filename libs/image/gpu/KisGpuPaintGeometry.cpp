/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintGeometry_p.h"
#include <algorithm>
#include <cmath>

namespace LibrePaint::Image::Gpu::Detail {
namespace {
quint32 integerRoot(quint64 value)
{
    quint64 root = 0, bit = quint64(1) << 62;
    while (bit > value) bit >>= 2;
    while (bit != 0) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1) + bit;
        } else root >>= 1;
        bit >>= 2;
    }
    return quint32(root);
}
qint32 fixed(qreal value) { return qint32(std::llround(value * 16)); }
quint32 pressure(qreal value) { return quint32(std::llround(value * 65535)); }
}

QList<PaintSegment> paintGeometry(const PaintOperation &operation, quint32 step)
{
    const auto input = operation.inputSamples();
    QList<InputSample> points;
    switch (operation.shape.kind) {
    case ShapeKind::Freehand:
        points = input;
        break;
    case ShapeKind::Line: {
        points = input.size() == 1 ? QList<InputSample>{input.front(), input.front()} : input;
        const auto inputDelta = input.back().position - input.front().position;
        const qreal lengthSquared = QPointF::dotProduct(inputDelta, inputDelta);
        const auto lineDelta = operation.shape.second - operation.shape.first;
        for (qsizetype i = 1; i + 1 < points.size(); ++i) {
            const qreal fraction = lengthSquared > 0 ?
                std::clamp(QPointF::dotProduct(points[i].position - input.front().position, inputDelta) / lengthSquared, qreal(0), qreal(1)) :
                qreal(i) / (points.size() - 1);
            points[i].position = operation.shape.first + lineDelta * fraction;
        }
        points.front().position = operation.shape.first;
        points.back().position = operation.shape.second;
        break;
    }
    case ShapeKind::Rectangle:
    case ShapeKind::RoundedRectangle: {
        const auto &shape = operation.shape;
        const auto center = (shape.first + shape.second) / 2;
        const qreal cosine = std::cos(shape.rotation), sine = std::sin(shape.rotation);
        const QRectF rect = QRectF(shape.first, shape.second).normalized();
        const qreal rx = std::min(shape.cornerRadii.width(), rect.width() / 2);
        const qreal ry = std::min(shape.cornerRadii.height(), rect.height() / 2);
        QList<QPointF> outline;
        if (shape.kind == ShapeKind::RoundedRectangle && rx > 0 && ry > 0) {
            constexpr qreal pi = 3.14159265358979323846;
            const int count = std::clamp(int(std::ceil(pi * std::sqrt(std::max(rx, ry) / 0.125) / 4)), 4, 16384);
            const QList<QPointF> centers{{rect.right() - rx, rect.top() + ry},
                                         {rect.right() - rx, rect.bottom() - ry},
                                         {rect.left() + rx, rect.bottom() - ry},
                                         {rect.left() + rx, rect.top() + ry}};
            outline.reserve(4 * (count + 1));
            for (int corner = 0; corner < 4; ++corner) {
                for (int i = 0; i <= count; ++i) {
                    const qreal angle = (corner - 1 + qreal(i) / count) * pi / 2;
                    outline.append(centers[corner] + QPointF(rx * std::cos(angle), ry * std::sin(angle)));
                }
            }
        } else {
            outline = {shape.first, QPointF(shape.second.x(), shape.first.y()),
                       shape.second, QPointF(shape.first.x(), shape.second.y())};
        }
        for (const auto &corner : outline) {
            auto point = input.front();
            const auto delta = corner - center;
            point.position = center + QPointF(delta.x() * cosine - delta.y() * sine,
                                               delta.x() * sine + delta.y() * cosine);
            points.append(point);
        }
        points.append(points.front());
        break;
    }
    case ShapeKind::Ellipse: {
        const auto &shape = operation.shape;
        const auto center = (shape.first + shape.second) / 2;
        const qreal rx = std::abs(shape.second.x() - shape.first.x()) / 2;
        const qreal ry = std::abs(shape.second.y() - shape.first.y()) / 2;
        constexpr qreal pi = 3.14159265358979323846;
        const int count = std::clamp(int(std::ceil(pi * std::sqrt(std::max(rx, ry) / 0.125))), 16, 65536);
        const qreal cosine = std::cos(shape.rotation), sine = std::sin(shape.rotation);
        points.reserve(count + 1);
        for (int i = 0; i < count; ++i) {
            const qreal angle = qreal(i) * 2 * pi / count;
            const qreal x = rx * std::cos(angle), y = ry * std::sin(angle);
            auto point = input.front();
            point.position = center + QPointF(x * cosine - y * sine, x * sine + y * cosine);
            points.append(point);
        }
        points.append(points.front());
        break;
    }
    }
    QList<PaintSegment> result;
    result.reserve(points.size());
    quint32 phase = 0;
    for (qsizetype i = 0; i < points.size(); ++i) {
        if (i == 0 && points.size() > 1) continue;
        const auto &a = points[i == 0 ? 0 : i - 1];
        const auto &b = points[i];
        const qint32 ax = fixed(a.position.x()), ay = fixed(a.position.y());
        const qint32 bx = fixed(b.position.x()), by = fixed(b.position.y());
        const qint64 dx = qint64(bx) - ax, dy = qint64(by) - ay;
        const quint32 length = integerRoot(quint64(dx * dx) + quint64(dy * dy));
        result.append({ax, ay, bx, by, pressure(a.pressure), pressure(b.pressure), phase, length});
        phase = (quint64(phase) + length) % step;
    }
    return result;
}


std::optional<QList<quint64>> paintSegmentTiles(const PaintSegment &segment, quint32 diameter,
                                               QRect region, QSize canvas, qsizetype maximumCoordinates, int tileSize)
{
    QList<quint64> result;
    if (region.isEmpty()) return result;
    if (tileSize <= 0) return std::nullopt;
    const int size = tileSize;
    const qint64 radius = (diameter + 31) / 32;
    const int left = int(std::max<qint64>(region.left() / size * size, std::min(segment.ax, segment.bx) / 16 - radius));
    const int top = int(std::max<qint64>(region.top() / size * size, std::min(segment.ay, segment.by) / 16 - radius));
    const int right = int(std::min<qint64>(qMin(canvas.width() - 1, (region.right() / size + 1) * size - 1),
                                          std::max(segment.ax, segment.bx) / 16 + radius));
    const int bottom = int(std::min<qint64>(qMin(canvas.height() - 1, (region.bottom() / size + 1) * size - 1),
                                           std::max(segment.ay, segment.by) / 16 + radius));
    if (right < left || bottom < top) return result;
    for (int y = top / size; y <= bottom / size; ++y) {
        qreal first = 0, last = 1;
        if (segment.ay != segment.by) {
            const qreal lower = qreal(y * size - radius - 1) * 16;
            const qreal upper = qreal((y + 1) * size + radius + 1) * 16;
            const qreal dy = qreal(segment.by) - segment.ay;
            const qreal a = (lower - segment.ay) / dy, b = (upper - segment.ay) / dy;
            first = std::max<qreal>(0, std::min(a, b));
            last = std::min<qreal>(1, std::max(a, b));
            if (first > last) continue;
        }
        const qreal a = (segment.ax + (qreal(segment.bx) - segment.ax) * first) / 16;
        const qreal b = (segment.ax + (qreal(segment.bx) - segment.ax) * last) / 16;
        const int rowLeft = std::max(left, int(std::floor(std::min(a, b) - radius - 1)));
        const int rowRight = std::min(right, int(std::ceil(std::max(a, b) + radius + 1)));
        for (int x = rowLeft / size; x <= rowRight / size; ++x) {
            if (result.size() >= maximumCoordinates) return std::nullopt;
            result.append(key(x, y));
        }
    }
    return result;
}

}
