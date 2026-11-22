/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionCap.h"

#include <cmath>

void KisStrokePredictionCap::apply(KisStrokePredictionResult &result, qreal maxMovePx)
{
    if (!result.valid || maxMovePx <= 0.0) return;
    if (result.latestIndex < 0 || result.latestIndex + 1 >= result.polyline.size()) return;

    qreal moveLength = 0.0;
    for (int i = result.latestIndex; i + 1 < result.polyline.size(); ++i) {
        const QPointF delta = result.polyline.at(i + 1) - result.polyline.at(i);
        moveLength += std::hypot(delta.x(), delta.y());
    }

    if (moveLength <= maxMovePx) return;

    qreal walked = 0.0;
    for (int i = result.latestIndex; i + 1 < result.polyline.size(); ++i) {
        const QPointF from = result.polyline.at(i);
        const QPointF delta = result.polyline.at(i + 1) - from;
        const qreal segmentLength = std::hypot(delta.x(), delta.y());

        if (walked + segmentLength >= maxMovePx) {
            const qreal remaining = maxMovePx - walked;
            const QPointF cut = segmentLength > 0.0
                ? from + delta * (remaining / segmentLength)
                : from;
            result.polyline.resize(i + 1);
            result.polyline.append(cut);
            result.moveLength = maxMovePx;
            return;
        }

        walked += segmentLength;
    }
}

void KisStrokePredictionCap::mapToPixels(KisStrokePredictionResult &result, const QTransform &documentToPixel)
{
    if (!result.valid) return;

    for (int i = 0; i < result.polyline.size(); ++i) {
        result.polyline[i] = documentToPixel.map(result.polyline.at(i));
    }

    const QTransform linear(documentToPixel.m11(), documentToPixel.m12(),
                            documentToPixel.m21(), documentToPixel.m22(),
                            0.0, 0.0);
    result.velocity = linear.map(result.velocity);
    result.speed = std::hypot(result.velocity.x(), result.velocity.y());

    result.moveLength = 0.0;
    for (int i = result.latestIndex; i >= 0 && i + 1 < result.polyline.size(); ++i) {
        const QPointF delta = result.polyline.at(i + 1) - result.polyline.at(i);
        result.moveLength += std::hypot(delta.x(), delta.y());
    }
}
