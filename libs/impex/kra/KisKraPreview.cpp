/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisKraPreview_p.h"

#include <array>

namespace LibrePaint::ImportExport::Kra::Detail {
namespace {
constexpr int PreviewExtent = 256;

struct PreviewSpan
{
    int first = 0;
    int end = 1;
    quint64 total = 1;
    quint64 beginPosition = 0;
    quint64 endPosition = 0;
    quint64 scale = 1;
    quint64 leading = 1;
    bool linear = true;

    quint64 weight(int index) const
    {
        if (linear) return index == first ? leading : total - leading;
        return qMin(endPosition, quint64(index + 1) * scale) -
               qMax(beginPosition, quint64(index) * scale);
    }
};

PreviewSpan previewSpan(int index, int source, int destination)
{
    PreviewSpan span;
    if (source >= destination) {
        span.linear = false;
        span.scale = destination;
        span.beginPosition = quint64(index) * source;
        span.endPosition = quint64(index + 1) * source;
        span.first = int(span.beginPosition / destination);
        span.end = int((span.endPosition + destination - 1) / destination);
        span.total = source;
    } else {
        // Pixel centres use a rational coordinate with denominator 2 * destination.
        const qint64 centre = (2 * qint64(index) + 1) * source - destination;
        const qint64 denominator = 2 * destination;
        if (centre >= qint64(source - 1) * denominator) {
            span.first = source - 1;
            span.end = source;
        } else if (centre > 0) {
            span.first = int(centre / denominator);
            span.end = span.first + 2;
            span.total = denominator;
            span.leading = denominator - centre % denominator;
        }
    }
    return span;
}
}

QImage makePreview(const QImage &projection)
{
    if (projection.isNull()) return {};
    const auto source = projection.convertToFormat(QImage::Format_RGBA8888);
    if (source.isNull()) return {};
    auto size = source.size().scaled(QSize(PreviewExtent, PreviewExtent), Qt::KeepAspectRatio);
    size.setWidth(qMax(1, size.width()));
    size.setHeight(qMax(1, size.height()));
    QImage preview(size, QImage::Format_RGBA8888);
    if (preview.isNull()) return {};
    std::array<PreviewSpan, PreviewExtent> horizontal;
    std::array<PreviewSpan, PreviewExtent> vertical;
    for (int x = 0; x < size.width(); ++x) horizontal[x] = previewSpan(x, source.width(), size.width());
    for (int y = 0; y < size.height(); ++y) vertical[y] = previewSpan(y, source.height(), size.height());
    for (int y = 0; y < size.height(); ++y) {
        auto *output = preview.scanLine(y);
        const auto &rows = vertical[y];
        for (int x = 0; x < size.width(); ++x) {
            const auto &columns = horizontal[x];
            std::array<quint64, 4> weighted{};
            for (int sy = rows.first; sy < rows.end; ++sy) {
                const auto *row = source.constScanLine(sy);
                const quint64 rowWeight = rows.weight(sy);
                for (int sx = columns.first; sx < columns.end; ++sx) {
                    const auto *pixel = row + sx * 4;
                    const quint64 alpha = rowWeight * columns.weight(sx) * pixel[3];
                    weighted[3] += alpha;
                    for (int channel = 0; channel < 3; ++channel) weighted[channel] += alpha * pixel[channel];
                }
            }
            const quint64 total = rows.total * columns.total;
            output[x * 4 + 3] = quint8((weighted[3] + total / 2) / total);
            for (int channel = 0; channel < 3; ++channel)
                output[x * 4 + channel] = output[x * 4 + 3]
                    ? quint8((weighted[channel] + weighted[3] / 2) / weighted[3]) : 0;
        }
    }
    return preview;
}
}
