/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePrediction.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int MaximumSampleCount = 64;

qreal clampValue(qreal value, qreal minimum, qreal maximum)
{
    return std::min(std::max(value, minimum), maximum);
}

qreal smoothStep(qreal normalized)
{
    const qreal c = clampValue(normalized, 0.0, 1.0);
    return c * c * (3.0 - 2.0 * c);
}
}

struct KisStrokePrediction::Private
{
    struct Sample {
        QPointF position;
        qreal time = 0.0;
        qreal pressure = 0.0;
    };

    /// Keep the time span the window and the anchor need and the point cap.
    /// The previous point survives the time span, because a longer gap
    /// resets the history in addSample().
    void trim()
    {
        if (samples.isEmpty()) return;

        const qreal latestTime = samples.constLast().time;
        const qreal keepMs =
            std::max({settings.anchorMs, settings.velocityWindowMs, settings.gapResetMs});
        const qreal oldestKept = latestTime - keepMs;

        while (samples.size() > 1 && samples.constFirst().time < oldestKept) {
            samples.removeFirst();
        }
        while (samples.size() > MaximumSampleCount) {
            samples.removeFirst();
        }
    }

    KisStrokePredictionSettings settings;
    QVector<Sample> samples;
};

KisStrokePrediction::KisStrokePrediction()
    : m_d(new Private)
{
}

KisStrokePrediction::~KisStrokePrediction() = default;

void KisStrokePrediction::setSettings(const KisStrokePredictionSettings &settings)
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

    m_d->settings = clamped;
}

KisStrokePredictionSettings KisStrokePrediction::settings() const
{
    return m_d->settings;
}

void KisStrokePrediction::reset()
{
    m_d->samples.clear();
}

bool KisStrokePrediction::addSample(const QPointF &pos, qreal timeMs, qreal pressure)
{
    bool historyDropped = false;

    if (!m_d->samples.isEmpty()) {
        const Private::Sample &last = m_d->samples.constLast();

        if (timeMs < last.time || timeMs - last.time >= m_d->settings.gapResetMs) {
            m_d->samples.clear();
            historyDropped = true;
        } else if (timeMs == last.time) {
            m_d->samples.last().position = pos;
            m_d->samples.last().pressure = pressure;
            return false;
        }
    }

    Private::Sample sample;
    sample.position = pos;
    sample.time = timeMs;
    sample.pressure = pressure;
    m_d->samples.append(sample);

    m_d->trim();

    return historyDropped;
}

KisStrokePredictionResult KisStrokePrediction::predict() const
{
    KisStrokePredictionResult result;

    if (m_d->samples.size() < 2) return result;

    const qreal latestTime = m_d->samples.constLast().time;

    int windowStart = m_d->samples.size() - 1;
    for (int i = 0; i < m_d->samples.size(); ++i) {
        if (latestTime - m_d->samples[i].time <= m_d->settings.velocityWindowMs) {
            windowStart = i;
            break;
        }
    }
    if (windowStart == m_d->samples.size() - 1) {
        windowStart = m_d->samples.size() - 2;
    }

    const qreal windowDuration = latestTime - m_d->samples[windowStart].time;
    if (windowDuration <= 0.0) return result;

    const QPointF latestPosition = m_d->samples.constLast().position;

    result.velocity =
        (latestPosition - m_d->samples[windowStart].position) / windowDuration;
    result.speed = std::hypot(result.velocity.x(), result.velocity.y());

    const qreal speedFactor = smoothStep(
        (result.speed - m_d->settings.speedZeroPxPerMs) /
        (m_d->settings.speedFullPxPerMs - m_d->settings.speedZeroPxPerMs));
    const qreal pressureFactor = smoothStep(
        (m_d->samples.constLast().pressure - m_d->settings.pressureZero) /
        (m_d->settings.pressureFull - m_d->settings.pressureZero));
    result.confidence = speedFactor * pressureFactor;

    const QPointF moveVector =
        result.velocity * result.confidence * m_d->settings.horizonMs;
    result.moveLength = std::hypot(moveVector.x(), moveVector.y());

    if (result.moveLength < m_d->settings.minMovePx) {
        return result;
    }

    int anchorStart = m_d->samples.size() - 1;
    for (int i = 0; i < m_d->samples.size(); ++i) {
        if (latestTime - m_d->samples[i].time <= m_d->settings.anchorMs) {
            anchorStart = i;
            break;
        }
    }
    if (anchorStart == m_d->samples.size() - 1) {
        anchorStart = m_d->samples.size() - 2;
    }

    result.polyline.reserve(m_d->samples.size() - anchorStart + 1);
    for (int i = anchorStart; i < m_d->samples.size(); ++i) {
        result.polyline.append(m_d->samples[i].position);
    }
    result.latestIndex = result.polyline.size() - 1;
    result.polyline.append(latestPosition + moveVector);
    result.valid = true;

    return result;
}

int KisStrokePrediction::sampleCount() const
{
    return m_d->samples.size();
}
