/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKalmanStrokePredictor.h"

#include "KisStrokeSampleHistory.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr int StateSize = 4;
constexpr int MaximumIntervalCount = 20;
constexpr qreal ProcessNoiseScale = 0.01;
constexpr qreal ObservationNoise = 1.0;
constexpr qreal JankZero = 0.1;
constexpr qreal JankFull = 0.2;
constexpr qreal JankPositionStep = 0.1;
constexpr qreal JankVelocityStep = 0.5;

using Vector4 = std::array<qreal, StateSize>;
using Matrix4 = std::array<std::array<qreal, StateSize>, StateSize>;

qreal clampValue(qreal value, qreal minimum, qreal maximum)
{
    return std::min(std::max(value, minimum), maximum);
}

qreal smoothStep(qreal normalized)
{
    const qreal c = clampValue(normalized, 0.0, 1.0);
    return c * c * (3.0 - 2.0 * c);
}

Matrix4 identityMatrix()
{
    Matrix4 matrix{};
    for (int i = 0; i < StateSize; ++i) {
        matrix[i][i] = 1.0;
    }
    return matrix;
}

const Matrix4 &transitionMatrix()
{
    // Position, velocity, acceleration, jank with dt = 1.
    static const Matrix4 matrix = {{
        {{1.0, 1.0, 0.5, 1.0 / 6.0}},
        {{0.0, 1.0, 1.0, 0.5}},
        {{0.0, 0.0, 1.0, 1.0}},
        {{0.0, 0.0, 0.0, 1.0}},
    }};
    return matrix;
}

/// Q = 0.01 * G * G^T, G = {1/6, 1/2, 1, 1}.
const Matrix4 &processNoiseMatrix()
{
    static const Matrix4 matrix = [] {
        const Vector4 g = {{1.0 / 6.0, 0.5, 1.0, 1.0}};
        Matrix4 noise{};
        for (int i = 0; i < StateSize; ++i) {
            for (int j = 0; j < StateSize; ++j) {
                noise[i][j] = ProcessNoiseScale * g[i] * g[j];
            }
        }
        return noise;
    }();
    return matrix;
}

Vector4 transition(const Vector4 &state)
{
    const Matrix4 &f = transitionMatrix();
    Vector4 next{};
    for (int i = 0; i < StateSize; ++i) {
        for (int j = 0; j < StateSize; ++j) {
            next[i] += f[i][j] * state[j];
        }
    }
    return next;
}

Matrix4 transitionCovariance(const Matrix4 &covariance)
{
    const Matrix4 &f = transitionMatrix();
    const Matrix4 &q = processNoiseMatrix();

    Matrix4 intermediate{};
    for (int i = 0; i < StateSize; ++i) {
        for (int j = 0; j < StateSize; ++j) {
            for (int k = 0; k < StateSize; ++k) {
                intermediate[i][j] += f[i][k] * covariance[k][j];
            }
        }
    }

    Matrix4 next{};
    for (int i = 0; i < StateSize; ++i) {
        for (int j = 0; j < StateSize; ++j) {
            next[i][j] = q[i][j];
            for (int k = 0; k < StateSize; ++k) {
                next[i][j] += intermediate[i][k] * f[j][k];
            }
        }
    }
    return next;
}

/// H = {1, 0, 0, 0} and R = 1: the measurement updates only through the
/// first column of the covariance.
void updateAxis(Vector4 *state, Matrix4 *covariance, qreal observation)
{
    const qreal residual = observation - (*state)[0];
    const qreal innovation = (*covariance)[0][0] + ObservationNoise;

    Vector4 gain{};
    for (int i = 0; i < StateSize; ++i) {
        gain[i] = (*covariance)[i][0] / innovation;
    }
    for (int i = 0; i < StateSize; ++i) {
        (*state)[i] += gain[i] * residual;
    }

    Matrix4 next = *covariance;
    for (int i = 0; i < StateSize; ++i) {
        for (int j = 0; j < StateSize; ++j) {
            next[i][j] = (*covariance)[i][j] - gain[i] * (*covariance)[0][j];
        }
    }
    *covariance = next;
}

KisStrokePredictionSettings clampSettings(const KisStrokePredictionSettings &settings)
{
    KisStrokePredictionSettings clamped = settings;

    clamped.horizonMs = clampValue(settings.horizonMs, 8.0, 25.0);
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

struct AxisState {
    Vector4 state{};
    Matrix4 covariance = identityMatrix();
    int updates = 0;
};
} // namespace

struct KisKalmanStrokePredictor::Private
{
    Private(const KisStrokePredictionSettings &input)
        : settings(clampSettings(input))
        , history(std::max(settings.velocityWindowMs, settings.decelLongWindowMs),
                  settings.anchorMs,
                  settings.gapResetMs)
    {
    }

    void resetFilter()
    {
        axes[0] = AxisState();
        axes[1] = AxisState();
        intervalCount = 0;
        intervalSum = 0.0;
    }

    KisStrokePredictionSettings settings;
    KisStrokeSampleHistory history;
    AxisState axes[2];
    int intervalCount = 0;
    qreal intervalSum = 0.0;
};

KisKalmanStrokePredictor::KisKalmanStrokePredictor(const KisStrokePredictionSettings &settings)
    : m_d(new Private(settings))
{
}

KisKalmanStrokePredictor::~KisKalmanStrokePredictor() = default;

void KisKalmanStrokePredictor::reset()
{
    m_d->history.reset();
    m_d->resetFilter();
}

void KisKalmanStrokePredictor::addSample(const KisStrokePredictionSample &sample)
{
    Private &d = *m_d;

    const bool hadSamples = d.history.size() > 0;
    const bool sameTime = hadSamples && d.history.latest().time == sample.timeMs;
    const qreal previousTime = hadSamples ? d.history.latest().time : 0.0;

    const bool historyDropped = d.history.add(sample.documentPos, sample.timeMs, sample.pressure);
    if (historyDropped) {
        d.resetFilter();
    }
    if (sameTime) return;

    if (!historyDropped && hadSamples) {
        const qreal interval = sample.timeMs - previousTime;
        if (interval > 0.0 && d.intervalCount < MaximumIntervalCount) {
            d.intervalSum += interval;
            ++d.intervalCount;
        }
    }

    if (d.axes[0].updates == 0) {
        d.axes[0].state[0] = sample.documentPos.x();
        d.axes[1].state[0] = sample.documentPos.y();
        d.axes[0].updates = 1;
        d.axes[1].updates = 1;
        return;
    }

    const qreal observations[2] = {sample.documentPos.x(), sample.documentPos.y()};
    for (int axis = 0; axis < 2; ++axis) {
        AxisState &filter = d.axes[axis];
        filter.state = transition(filter.state);
        filter.covariance = transitionCovariance(filter.covariance);
        updateAxis(&filter.state, &filter.covariance, observations[axis]);
        ++filter.updates;
    }
}

KisStrokePredictionResult KisKalmanStrokePredictor::predict(qreal horizonMs) const
{
    const Private &d = *m_d;
    KisStrokePredictionResult result;

    if (d.axes[0].updates < StateSize) return result;
    if (d.intervalCount <= 0) return result;

    const qreal reportMs = d.intervalSum / d.intervalCount;
    if (reportMs <= 0.0) return result;

    Vector4 x = d.axes[0].state;
    Vector4 y = d.axes[1].state;

    result.velocity = QPointF(x[1] / reportMs, y[1] / reportMs);
    result.speed = std::hypot(result.velocity.x(), result.velocity.y());

    const qreal speedFactor = smoothStep(
        (result.speed - d.settings.speedZeroPxPerMs) /
        (d.settings.speedFullPxPerMs - d.settings.speedZeroPxPerMs));
    const qreal pressureFactor = smoothStep(
        (d.history.latest().pressure - d.settings.pressureZero) /
        (d.settings.pressureFull - d.settings.pressureZero));

    const qreal jank = std::hypot(x[3], y[3]);
    const qreal jankFactor = 1.0 - clampValue((jank - JankZero) / (JankFull - JankZero), 0.0, 1.0);

    result.confidence = speedFactor * pressureFactor * jankFactor;

    const qreal horizon = clampValue(horizonMs, 8.0, 25.0);
    const qreal advance = horizon / reportMs * result.confidence;
    if (advance <= 0.0) return result;

    QVector<QPointF> predicted;

    const int fullSteps = int(std::floor(advance));
    for (int step = 0; step < fullSteps; ++step) {
        x[2] += x[3] * JankPositionStep;
        x[1] += x[2] * JankVelocityStep;
        x[0] += x[1];

        y[2] += y[3] * JankPositionStep;
        y[1] += y[2] * JankVelocityStep;
        y[0] += y[1];

        predicted.append(QPointF(x[0], y[0]));
    }

    const qreal fraction = advance - fullSteps;
    if (fraction > 0.0) {
        predicted.append(QPointF(x[0] + x[1] * fraction, y[0] + y[1] * fraction));
    }

    const QVector<QPointF> anchor = d.history.anchorPolyline();

    QPointF from = anchor.constLast();
    result.moveLength = 0.0;
    for (const QPointF &to : predicted) {
        result.moveLength += std::hypot(to.x() - from.x(), to.y() - from.y());
        from = to;
    }

    if (result.moveLength < d.settings.minMovePx) return result;

    result.polyline = anchor;
    result.latestIndex = result.polyline.size() - 1;
    for (const QPointF &point : predicted) {
        result.polyline.append(point);
    }
    result.valid = true;
    result.source = KisStrokePredictionSource::Kalman;

    return result;
}

KisStrokePredictionSource KisKalmanStrokePredictor::source() const
{
    return KisStrokePredictionSource::Kalman;
}
