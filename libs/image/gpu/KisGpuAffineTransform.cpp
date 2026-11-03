/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuAffineTransform.h"
#include "KisGpuPixelData.h"
#include <QtMath>
#include <cmath>
#include <limits>
#include <algorithm>

namespace LibrePaint::Image::Gpu {
namespace {
bool regionValid(QRect rect)
{
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    return rect.width() >= 0 && rect.height() >= 0 && rect.x() >= -limit && rect.y() >= -limit &&
        qint64(rect.x()) + rect.width() <= limit && qint64(rect.y()) + rect.height() <= limit;
}
std::optional<QPair<int, int>> lineRange(const AffinePass &pass, int line)
{
    const bool horizontal = pass.kind == AffinePass::Horizontal;
    const int first = horizontal ? pass.source.x() : pass.source.y();
    const int end = first + (horizontal ? pass.source.width() : pass.source.height());
    const double support = pass.shear == 0 ? 0 : pass.kernel->support;
    const auto map = [&](int value) { return value * pass.scale + pass.offset + line * pass.shear; };
    const double left = map(pass.scale >= 0 ? first : end) - support;
    const double right = map(pass.scale >= 0 ? end : first) + support;
    constexpr int limit = RgbaTile::CoordinateLimit;
    if (!std::isfinite(left) || !std::isfinite(right) || left < -limit || right > limit) return {};
    const int start = qRound(left), stop = qRound(right);
    if (stop < start || (stop == start && stop == limit)) return {};
    return qMakePair(start, stop == start ? stop + 1 : stop);
}
QRect remapped(QRect rect, const std::array<qint32, 6> &map, bool inverse)
{
    if (rect.isEmpty()) return {};
    auto point = [&](QPoint p) {
        if (!inverse) return QPoint(map[0] * p.x() + map[1] * p.y() + map[4], map[2] * p.x() + map[3] * p.y() + map[5]);
        p -= QPoint(map[4], map[5]);
        return QPoint(map[0] * p.x() + map[2] * p.y(), map[1] * p.x() + map[3] * p.y());
    };
    const auto a = point(rect.topLeft()), b = point(rect.topRight()), c = point(rect.bottomLeft()), d = point(rect.bottomRight());
    return QRect(QPoint(std::min({a.x(), b.x(), c.x(), d.x()}), std::min({a.y(), b.y(), c.y(), d.y()})),
                 QPoint(std::max({a.x(), b.x(), c.x(), d.x()}), std::max({a.y(), b.y(), c.y(), d.y()})));
}
}
std::optional<AffinePass> affineResamplePass(AffinePass::Kind kind, QRect source, double scale,
                                           double shear, double offset, const AffineKernelSP &kernel)
{
    if ((kind != AffinePass::Horizontal && kind != AffinePass::Vertical) || !kernel || source.isEmpty() ||
        !regionValid(source) || !std::isfinite(scale) || !std::isfinite(shear) || !std::isfinite(offset) ||
        std::abs(scale) > std::numeric_limits<qint32>::max() / 256. || std::abs(scale) < 1. / 256.) return {};
    AffinePass pass; pass.kind = kind; pass.source = source;
    pass.scale = scale; pass.shear = shear; pass.offset = offset; pass.kernel = kernel;
    const bool horizontal = kind == AffinePass::Horizontal;
    const int first = horizontal ? source.top() : source.left(), last = horizontal ? source.bottom() : source.right();
    const auto start = lineRange(pass, first), end = lineRange(pass, last);
    if (!start || !end) return {};
    const int minimum = std::min(start->first, end->first), maximum = std::max(start->second, end->second);
    pass.destination = horizontal ? QRect(minimum, first, maximum - minimum, last - first + 1) :
                                    QRect(first, minimum, last - first + 1, maximum - minimum);
    if (!regionValid(pass.destination)) return {};
    return pass;
}
bool validAffinePlan(const AffinePlan &plan)
{
    if (plan.passes.size() > 16 || plan.filterId.size() > 1024) return false;
    for (const auto &pass : plan.passes) {
        if (!regionValid(pass.source) || !regionValid(pass.destination)) return false;
        if (pass.kind == AffinePass::Remap) {
            const auto &m = pass.map;
            if (m[0] < -1 || m[0] > 1 || m[1] < -1 || m[1] > 1 || m[2] < -1 || m[2] > 1 || m[3] < -1 || m[3] > 1 ||
                qint64(m[4]) < -2LL * RgbaTile::CoordinateLimit || qint64(m[4]) > 2LL * RgbaTile::CoordinateLimit ||
                qint64(m[5]) < -2LL * RgbaTile::CoordinateLimit || qint64(m[5]) > 2LL * RgbaTile::CoordinateLimit ||
                m[0] * m[0] + m[2] * m[2] != 1 || m[1] * m[1] + m[3] * m[3] != 1 ||
                m[0] * m[1] + m[2] * m[3] != 0 || (pass.replaceLayer && pass.mapWholeLayer)) return false;
            if (!pass.mapWholeLayer && remapped(pass.source, m, false) != pass.destination) return false;
            continue;
        }
        if (pass.kind != AffinePass::Horizontal && pass.kind != AffinePass::Vertical) return false;
        const auto &kernel = pass.kernel;
        if (!kernel || kernel->positionScale < 1 || kernel->positionScale > 256 ||
            !std::isfinite(kernel->support) || kernel->support < 0 || kernel->support > 16 ||
            kernel->weights.size() > 1024 * 1024) return false;
        for (const auto &phase : kernel->phases) {
            if (phase.offset < 0 || phase.count <= 0 || phase.count > 8192 || phase.center < 0 || phase.center > phase.count ||
                qint64(phase.offset) + phase.count > kernel->weights.size()) return false;
            qint64 sum = 0, absolute = 0;
            for (int i = 0; i < phase.count; ++i) {
                const auto value = kernel->weights[phase.offset + i];
                if (value < -32768 || value > 32767) return false;
                sum += value; absolute += std::abs(value);
            }
            if (sum != 255 || absolute > std::numeric_limits<qint64>::max() / (65535LL * 65535)) return false;
        }
        const auto expected = affineResamplePass(pass.kind, pass.source, pass.scale, pass.shear, pass.offset, kernel);
        if (!expected || expected->destination != pass.destination) return false;
    }
    return true;
}
QRect affineNeededRegion(const AffinePass &pass, QRect output)
{
    if (pass.kind == AffinePass::Remap) {
        if (pass.mapWholeLayer) return remapped(output, pass.map, true);
        const auto needed = remapped(output.intersected(pass.destination), pass.map, true).intersected(pass.source);
        return pass.replaceLayer ? needed : output.united(needed);
    }
    return output.intersects(pass.destination) ? output.united(pass.source) : output;
}
QRect affineOutputRegion(const AffinePass &pass, QRect input)
{
    if (pass.kind == AffinePass::Remap) {
        if (pass.mapWholeLayer) return remapped(input, pass.map, false);
        const auto moved = remapped(input.intersected(pass.source), pass.map, false);
        return pass.replaceLayer ? moved : input.united(moved);
    }
    return input.intersects(pass.source) ? input.united(pass.destination) : input;
}
}
