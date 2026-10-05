/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_CANVAS_PERFORMANCE_MEASUREMENT_P_H
#define KIS_CANVAS_PERFORMANCE_MEASUREMENT_P_H

#include <QObject>
#include <QVariantMap>
#include <chrono>

namespace Krita::Canvas::Performance
{
inline constexpr char enabledProperty[] = "librepaintCanvasPerformanceEnabled";
inline constexpr char inputProperty[] = "librepaintCanvasPerformanceInput";
inline constexpr char samplesProperty[] = "librepaintCanvasPerformanceSamples";

inline qint64 nowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

/** GUI-thread diagnostic counters, enabled by the comparison consumer on its
 * own widget/window. Stage duration measures CPU work including API waits;
 * completion records CPU return, independently of GPU/OS display completion.
 */
inline void record(QObject &owner, const QString &stage, qint64 durationNs,
                   qint64 input, qint64 finishNs)
{
    if (!owner.property(enabledProperty).toBool()) return;
    QVariantMap samples = owner.property(samplesProperty).toMap();
    QVariantMap sample = samples.value(stage).toMap();
    sample[QStringLiteral("count")] = sample.value(QStringLiteral("count")).toLongLong() + 1;
    sample[QStringLiteral("total_ns")] = sample.value(QStringLiteral("total_ns")).toLongLong() + durationNs;
    sample[QStringLiteral("last_ns")] = durationNs;
    sample[QStringLiteral("input")] = input;
    sample[QStringLiteral("finish_ns")] = finishNs;
    samples[stage] = sample;
    owner.setProperty(samplesProperty, samples);
}

class Measurement
{
public:
    Measurement(QObject &owner, const char *stage)
        : m_owner(owner.property(enabledProperty).toBool() ? &owner : nullptr), m_stage(stage)
    {
        if (m_owner) {
            m_input = m_owner->property(inputProperty).toLongLong();
            m_start = nowNs();
        }
    }
    ~Measurement()
    {
        if (m_owner) {
            const qint64 finish = nowNs();
            record(*m_owner, QString::fromLatin1(m_stage), finish - m_start, m_input, finish);
        }
    }
    Measurement(const Measurement &) = delete;
    Measurement &operator=(const Measurement &) = delete;
private:
    QObject *m_owner;
    const char *m_stage;
    qint64 m_start = 0;
    qint64 m_input = 0;
};
}
#endif
