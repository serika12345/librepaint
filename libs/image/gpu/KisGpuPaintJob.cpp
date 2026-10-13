/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintJob_p.h"
#include <algorithm>
#include <cmath>

namespace Krita::Image::Gpu::Detail {
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
    case ShapeKind::Line:
        points = {input.front(), input.back()};
        points.front().position = operation.shape.first;
        points.back().position = operation.shape.second;
        break;
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
                                               QRect region, QSize canvas, qsizetype maximumCoordinates)
{
    QList<quint64> result;
    if (region.isEmpty()) return result;
    constexpr int size = TileRenderer::TileSize;
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

quint32 packedPixel(const std::array<quint8, 4> &rgba)
{
    return quint32(rgba[0]) | quint32(rgba[1]) << 8 | quint32(rgba[2]) << 16 | quint32(rgba[3]) << 24;
}

bool preparePaintJob(QRhi &rhi, PaintJob &job, QRhiBuffer *zero)
{
    if (!job.output) return false;
    if (!job.geometry.isEmpty()) {
        job.samples = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::StorageBuffer,
                                            quint32(job.geometry.size() * sizeof(PaintSegment))));
        if (!job.samples->create()) return false;
    }
    job.parameters = own(rhi, rhi.newBuffer(QRhiBuffer::Immutable, QRhiBuffer::UniformBuffer, sizeof(PaintParameters)));
    job.bindings = own(rhi, rhi.newShaderResourceBindings());
    if (!job.parameters->create()) return false;
    job.bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::ComputeStage, job.parameters.get()),
        QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::ComputeStage, job.samples ? job.samples.get() : zero),
        QRhiShaderResourceBinding::bufferLoad(2, QRhiShaderResourceBinding::ComputeStage, job.input ? job.input->pixels.get() : zero),
        QRhiShaderResourceBinding::bufferStore(3, QRhiShaderResourceBinding::ComputeStage, job.output->pixels.get())
    });
    return job.bindings->create();
}
}
