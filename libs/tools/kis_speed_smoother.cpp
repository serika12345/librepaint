/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_speed_smoother.h>

#include <boost/circular_buffer.hpp>
#include <QElapsedTimer>
#include <QPointF>

#include <KisFilteredRollingMean.h>
#include <kis_algebra_2d.h>

namespace {
constexpr int MaximumSmoothHistory = 512;
constexpr int DefaultSmoothingSamples = 3;
constexpr int MinimumTrackingDistance = 5;
}

struct KisSpeedSmoother::Private
{
    Private(int historySize)
        : distances(historySize)
        , timeDiffsMean(200, 0.8)
    {
        timer.start();
    }

    struct DistancePoint {
        DistancePoint() = default;

        DistancePoint(qreal newDistance, qreal newTime)
            : distance(newDistance)
            , time(newTime)
        {
        }

        qreal distance {0.0};
        qreal time {0.0};
    };

    using DistanceBuffer = boost::circular_buffer<DistancePoint>;
    DistanceBuffer distances;

    KisFilteredRollingMean timeDiffsMean;

    QPointF lastPoint;
    QElapsedTimer timer;
    qreal lastTime {0.0};
    qreal lastSpeed {0.0};

    bool useEventTimestamps {false};
    int smoothingSamples {DefaultSmoothingSamples};
};

KisSpeedSmoother::KisSpeedSmoother()
    : m_d(new Private(MaximumSmoothHistory))
{
}

KisSpeedSmoother::~KisSpeedSmoother() = default;

qreal KisSpeedSmoother::lastSpeed() const
{
    return m_d->lastSpeed;
}

qreal KisSpeedSmoother::getNextSpeed(const QPointF &point, ulong timestamp)
{
    const qreal time = m_d->useEventTimestamps
        ? qreal(timestamp)
        : qreal(m_d->timer.nsecsElapsed()) / 1000000;

    return getNextSpeedImpl(point, time);
}

void KisSpeedSmoother::clear()
{
    m_d->timer.restart();
    m_d->distances.clear();
    m_d->distances.push_back(Private::DistancePoint(0.0, 0.0));
    m_d->lastPoint = QPointF();
    m_d->lastSpeed = 0.0;
}

void KisSpeedSmoother::setSettings(bool useEventTimestamps,
                                   int smoothingSamples)
{
    m_d->useEventTimestamps = useEventTimestamps;
    m_d->smoothingSamples = smoothingSamples;
}

qreal KisSpeedSmoother::getNextSpeedImpl(const QPointF &point, qreal time)
{
    if (m_d->lastPoint.isNull()) {
        m_d->lastPoint = point;
        m_d->lastTime = time;
        m_d->lastSpeed = 0.0;
        return 0.0;
    }

    const qreal distance = kisDistance(point, m_d->lastPoint);
    // Getting the exact same position is bogus, it is probably just a previous
    // point reported again. On Android, this happens all the time. Skip them.
    if (qFuzzyIsNull(distance)) {
        return m_d->lastSpeed;
    }

    const qreal timeDifference = time - m_d->lastTime;

    m_d->timeDiffsMean.addValue(timeDifference);
    const qreal averageTimeDifference = m_d->timeDiffsMean.filteredMean();

    m_d->lastPoint = point;
    m_d->lastTime = time;

    m_d->distances.push_back(Private::DistancePoint(distance, time));

    Private::DistanceBuffer::const_reverse_iterator it =
        m_d->distances.rbegin();
    const Private::DistanceBuffer::const_reverse_iterator end =
        m_d->distances.rend();

    qreal totalDistance = 0.0;
    qreal totalTime = 0.0;
    int itemsSearched = 0;

    for (; it != end; ++it) {
        ++itemsSearched;
        totalDistance += it->distance;

        /**
         * The specific tablet event timestamps are not reliable. The filtered
         * mean estimates the tablet sample rate under the assumption that a
         * device generates events at a fixed rate.
         */
        totalTime += averageTimeDifference;

        if (itemsSearched > m_d->smoothingSamples
            && totalDistance > MinimumTrackingDistance) {
            break;
        }
    }

    if (totalTime > 0.0 && totalDistance > MinimumTrackingDistance) {
        m_d->lastSpeed = totalDistance / totalTime;
    }

    return m_d->lastSpeed;
}
