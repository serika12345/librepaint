/*
 *  SPDX-FileCopyrightText: 2018 The Chromium Authors
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "KisKalmanStrokePredictor.h"

#include "KisKalmanFilter.h"
#include "KisStrokeSampleHistory.h"

#include <algorithm>
#include <cmath>

namespace
{
/// Chromium: a gap longer than this makes the input discontinuous, so all the
/// filters start again.
constexpr qreal MaximumTimeDeltaMs = 20.0;

/// AndroidX: the change of the estimated acceleration, in px per sample, at
/// which the confidence is left alone, and the one at which it reaches zero.
constexpr qreal JerkZeroPx = 0.1;
constexpr qreal JerkFullPx = 0.2;

/// The prediction is displayed through the same path as the confirmed line,
/// so the upper limit is 60 ms to try a length close to the whole delay from
/// input to display.
constexpr qreal kMaxHorizonMs = 60.0;

qreal clampValue(qreal value, qreal minimum, qreal maximum)
{
    return std::min(std::max(value, minimum), maximum);
}

qreal smoothStep(qreal normalized)
{
    const qreal c = clampValue(normalized, 0.0, 1.0);
    return c * c * (3.0 - 2.0 * c);
}

/// 1 while the acceleration is steady, falling to 0 at a change of JerkFullPx
/// or more.
qreal jerkFactor(qreal jerkPx)
{
    return 1.0 - clampValue((jerkPx - JerkZeroPx) / (JerkFullPx - JerkZeroPx), 0.0, 1.0);
}

KisStrokePredictionSettings clampSettings(const KisStrokePredictionSettings &settings)
{
    KisStrokePredictionSettings clamped = settings;

    clamped.horizonMs = clampValue(settings.horizonMs, 8.0, kMaxHorizonMs);
    clamped.anchorMs = clampValue(settings.anchorMs, 0.0, 100.0);
    clamped.velocityWindowMs = clampValue(settings.velocityWindowMs, 4.0, 50.0);
    clamped.gapResetMs = clampValue(settings.gapResetMs, 5.0, 100.0);
    clamped.speedFullPxPerMs = settings.speedFullPxPerMs <= settings.speedZeroPxPerMs
        ? settings.speedZeroPxPerMs + 0.01
        : settings.speedFullPxPerMs;
    clamped.pressureFull = settings.pressureFull <= settings.pressureZero
        ? settings.pressureZero + 0.01
        : settings.pressureFull;
    clamped.maxMovePx = std::max(0.0, settings.maxMovePx);
    clamped.decelShortWindowMs = clampValue(settings.decelShortWindowMs, 2.0, 20.0);
    clamped.decelLongWindowMs = clampValue(settings.decelLongWindowMs,
                                           clamped.decelShortWindowMs + 2.0, 50.0);
    clamped.decelRatioZero = clampValue(settings.decelRatioZero, 0.0, 1.5);
    clamped.decelRatioFull = settings.decelRatioFull <= clamped.decelRatioZero
        ? clamped.decelRatioZero + 0.01
        : settings.decelRatioFull;

    return clamped;
}
} // namespace

struct KisKalmanStrokePredictor::Private
{
    Private(const KisStrokePredictionSettings &input, bool inputJerkEnabled)
        : settings(clampSettings(input))
        , jerkEnabled(inputJerkEnabled)
        , history(std::max(settings.velocityWindowMs, settings.decelLongWindowMs),
                  settings.anchorMs,
                  settings.gapResetMs)
    {
    }

    void resetFilters()
    {
        xAxis.reset();
        yAxis.reset();
        intervals.reset();
        jerkPx = 0.0;
        hasAcceleration = false;
    }

    KisStrokePredictionSettings settings;
    bool jerkEnabled = false;
    KisStrokeSampleHistory history;
    KisKalmanFilter xAxis;
    KisKalmanFilter yAxis;
    KisKalmanFilter intervals;
    qreal jerkPx = 0.0;           // acceleration change of the latest update
    qreal lastAccelerationX = 0.0;
    qreal lastAccelerationY = 0.0;
    bool hasAcceleration = false;
};

KisKalmanStrokePredictor::KisKalmanStrokePredictor(const KisStrokePredictionSettings &settings,
                                                   bool jerkEnabled)
    : m_d(new Private(settings, jerkEnabled))
{
}

KisKalmanStrokePredictor::~KisKalmanStrokePredictor() = default;

void KisKalmanStrokePredictor::reset()
{
    m_d->history.reset();
    m_d->resetFilters();
}

void KisKalmanStrokePredictor::addSample(const KisStrokePredictionSample &sample)
{
    Private &d = *m_d;

    const bool hadSamples = d.history.size() > 0;

    if (hadSamples && d.history.latest().time == sample.timeMs) {
        // A sample at the same time only replaces the previous one.
        d.history.add(sample.documentPos, sample.timeMs, sample.pressure);
        return;
    }

    if (hadSamples) {
        const qreal interval = sample.timeMs - d.history.latest().time;
        if (interval > MaximumTimeDeltaMs) {
            d.resetFilters();
        } else if (interval >= 0.0) {
            d.intervals.update(interval, 0.0);
        }
    }

    if (d.history.add(sample.documentPos, sample.timeMs, sample.pressure)) {
        d.resetFilters();
    }

    const qreal dtMs = d.intervals.position();
    d.xAxis.update(sample.documentPos.x(), dtMs);
    d.yAxis.update(sample.documentPos.y(), dtMs);

    if (d.jerkEnabled) {
        const qreal accelerationX = d.xAxis.acceleration();
        const qreal accelerationY = d.yAxis.acceleration();
        if (d.hasAcceleration) {
            d.jerkPx = std::hypot(accelerationX - d.lastAccelerationX,
                                  accelerationY - d.lastAccelerationY)
                * dtMs * dtMs;
        } else {
            d.jerkPx = 0.0;
        }
        d.lastAccelerationX = accelerationX;
        d.lastAccelerationY = accelerationY;
        d.hasAcceleration = true;
    }
}

KisStrokePredictionResult KisKalmanStrokePredictor::predict(qreal horizonMs) const
{
    const Private &d = *m_d;
    KisStrokePredictionResult result;

    if (!d.xAxis.stable() || !d.yAxis.stable()) return result;

    result.velocity = QPointF(d.xAxis.velocity(), d.yAxis.velocity());
    result.speed = std::hypot(result.velocity.x(), result.velocity.y());

    const qreal speedFactor = smoothStep(
        (result.speed - d.settings.speedZeroPxPerMs) /
        (d.settings.speedFullPxPerMs - d.settings.speedZeroPxPerMs));
    const qreal pressureFactor = smoothStep(
        (d.history.latest().pressure - d.settings.pressureZero) /
        (d.settings.pressureFull - d.settings.pressureZero));

    result.confidence = speedFactor * pressureFactor;
    if (d.jerkEnabled) {
        result.confidence *= jerkFactor(d.jerkPx);
    }

    const qreal predDt = clampValue(horizonMs, 8.0, kMaxHorizonMs) * result.confidence;
    const QPointF acceleration(d.xAxis.acceleration(), d.yAxis.acceleration());
    const QPointF latest = d.history.latest().position;
    const QPointF predicted =
        latest + result.velocity * predDt + acceleration * (0.5 * predDt * predDt);

    result.moveLength = std::hypot(predicted.x() - latest.x(), predicted.y() - latest.y());
    if (result.moveLength < d.settings.minMovePx) return result;

    result.polyline = d.history.anchorPolyline();
    result.latestIndex = result.polyline.size() - 1;
    result.polyline.append(predicted);
    result.valid = true;
    result.source = KisStrokePredictionSource::Kalman;

    return result;
}

KisStrokePredictionSource KisKalmanStrokePredictor::source() const
{
    return m_d->jerkEnabled ? KisStrokePredictionSource::KalmanJerk
                            : KisStrokePredictionSource::Kalman;
}
