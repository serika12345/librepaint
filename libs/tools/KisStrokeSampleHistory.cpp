/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokeSampleHistory.h"

#include <algorithm>

namespace
{
constexpr int MaximumSampleCount = 64;
}

KisStrokeSampleHistory::KisStrokeSampleHistory(qreal keepMs, qreal anchorMs, qreal gapResetMs)
    : m_keepMs(std::max({keepMs, anchorMs, gapResetMs}))
    , m_anchorMs(anchorMs)
    , m_gapResetMs(gapResetMs)
{
}

bool KisStrokeSampleHistory::add(const QPointF &pos, qreal timeMs, qreal pressure)
{
    bool historyDropped = false;

    if (!m_samples.isEmpty()) {
        const Sample &last = m_samples.constLast();

        if (timeMs < last.time || timeMs - last.time >= m_gapResetMs) {
            m_samples.clear();
            historyDropped = true;
        } else if (timeMs == last.time) {
            m_samples.last().position = pos;
            m_samples.last().pressure = pressure;
            return false;
        }
    }

    Sample sample;
    sample.position = pos;
    sample.time = timeMs;
    sample.pressure = pressure;
    m_samples.append(sample);

    trim();

    return historyDropped;
}

void KisStrokeSampleHistory::reset()
{
    m_samples.clear();
}

int KisStrokeSampleHistory::size() const
{
    return m_samples.size();
}

const KisStrokeSampleHistory::Sample &KisStrokeSampleHistory::latest() const
{
    return m_samples.constLast();
}

QVector<QPointF> KisStrokeSampleHistory::anchorPolyline() const
{
    QVector<QPointF> polyline;
    if (m_samples.isEmpty()) return polyline;

    const qreal latestTime = m_samples.constLast().time;

    int anchorStart = m_samples.size() - 1;
    for (int i = 0; i < m_samples.size(); ++i) {
        if (latestTime - m_samples[i].time <= m_anchorMs) {
            anchorStart = i;
            break;
        }
    }
    if (anchorStart == m_samples.size() - 1 && m_samples.size() > 1) {
        anchorStart = m_samples.size() - 2;
    }

    polyline.reserve(m_samples.size() - anchorStart);
    for (int i = anchorStart; i < m_samples.size(); ++i) {
        polyline.append(m_samples[i].position);
    }

    return polyline;
}

void KisStrokeSampleHistory::trim()
{
    if (m_samples.isEmpty()) return;

    const qreal latestTime = m_samples.constLast().time;
    const qreal oldestKept = latestTime - m_keepMs;

    while (m_samples.size() > 1 && m_samples.constFirst().time < oldestKept) {
        m_samples.removeFirst();
    }
    while (m_samples.size() > MaximumSampleCount) {
        m_samples.removeFirst();
    }
}
