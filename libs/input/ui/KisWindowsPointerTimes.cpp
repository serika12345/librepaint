/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWindowsPointerTimes.h"

#include <cmath>

KisWindowsPointerTimes *KisWindowsPointerTimes::s_instance = nullptr;

namespace
{

// The newest pointer messages carry the samples that a coalesced tablet
// event covers; older ones are not needed any more.
constexpr int MaxSamples = 512;

// Two samples of one message must sit on the same logical pixel, up to the
// rounding of the native to logical conversion.
constexpr qreal MaxDistanceSquared = 1.5 * 1.5;

// A sample whose computed time is further than this from msg.time means the
// performance counter and the message clock drifted apart.
constexpr qint32 MaxDeviationMs = 1000;

/**
 * The performance counter in milliseconds without an overflow: the integer
 * part and the fraction are scaled apart, so the fraction keeps sub-ms
 * resolution even after hours of uptime.
 */
double performanceCountMs(quint64 performanceCount, quint64 frequency)
{
    const quint64 whole = performanceCount / frequency;
    const quint64 remainder = performanceCount % frequency;
    return double(whole) * 1000.0
        + double(remainder) * 1000.0 / double(frequency);
}

} // namespace

KisWindowsPointerTimes *KisWindowsPointerTimes::instance()
{
    return s_instance;
}

void KisWindowsPointerTimes::install(KisWindowsPointerTimes *times)
{
    s_instance = times;
}

void KisWindowsPointerTimes::setFrequency(quint64 countsPerSecond)
{
    m_frequency = countsPerSecond;
}

void KisWindowsPointerTimes::addSamples(const QVector<Sample> &samples)
{
    // Without the counter frequency a performance count means nothing.
    if (m_frequency == 0) return;

    for (const Sample &sample : samples) {
        // Zero is what Windows reports for a sample without a counter value.
        if (sample.performanceCount == 0) continue;

        const double qpcMs = performanceCountMs(sample.performanceCount, m_frequency);
        const qint64 wholeMs = qint64(std::floor(qpcMs));

        if (!m_haveOffset) {
            m_offset = qint64(sample.messageTime) - wholeMs;
            m_haveOffset = true;
        }

        quint32 timestampMs = quint32(wholeMs + m_offset);

        // The message clock and the counter run from different origins, so a
        // large difference means the stored offset no longer fits.
        const qint32 deviation = qint32(timestampMs - sample.messageTime);
        if (deviation > MaxDeviationMs || deviation < -MaxDeviationMs) {
            m_offset = qint64(sample.messageTime) - wholeMs;
            timestampMs = quint32(wholeMs + m_offset);
        }

        Entry entry;
        entry.messageTime = sample.messageTime;
        entry.timestampMs = timestampMs;
        entry.logicalPosition = sample.logicalPosition;
        m_entries.append(entry);
    }

    if (m_entries.size() > MaxSamples) {
        m_entries.remove(0, m_entries.size() - MaxSamples);
    }
}

bool KisWindowsPointerTimes::take(quint64 eventTimestamp, const QPointF &logicalPosition,
                                  quint64 *timestampMs)
{
    const quint32 messageTime = quint32(eventTimestamp);

    int match = -1;
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry &entry = m_entries.at(i);
        if (entry.messageTime != messageTime) continue;

        const QPointF delta = entry.logicalPosition - logicalPosition;
        if (delta.x() * delta.x() + delta.y() * delta.y() > MaxDistanceSquared) continue;

        match = i;
        break;
    }

    if (match < 0) return false;

    if (timestampMs) {
        *timestampMs = m_entries.at(match).timestampMs;
    }

    // The matched sample and every older one belong to this or an earlier
    // event, so a repeated lookup must not find them again.
    m_entries.remove(0, match + 1);

    return true;
}
