/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_latency_tracker.h"

#include <QTest>

#include <stdexcept>

namespace
{

class ScalarTrackerProbe : public KisScalarTracker<qint64>
{
public:
    ScalarTrackerProbe(const QString &name, int windowSize)
        : KisScalarTracker<qint64>(name, windowSize)
    {
    }

    qint64 lastMean = -1;
    qint64 lastVariance = -1;
    qint64 lastMaximum = -1;
    QString lastMessage;
    int printCount = 0;

protected:
    QString format(qint64 mean, qint64 variance, qint64 maximum) override
    {
        lastMean = mean;
        lastVariance = variance;
        lastMaximum = maximum;
        return QStringLiteral("formatted");
    }

    void print(const QString &message) override
    {
        lastMessage = message;
        ++printCount;
    }
};

class DestructionProbe : public KisScalarTracker<int>
{
public:
    explicit DestructionProbe(bool *destroyed)
        : KisScalarTracker<int>(QStringLiteral("destruction"), 2)
        , m_destroyed(destroyed)
    {
    }

    ~DestructionProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class LatencyTrackerProbe : public KisLatencyTracker
{
public:
    explicit LatencyTrackerProbe(qint64 now)
        : KisLatencyTracker(1)
        , m_now(now)
    {
    }

    qint64 lastMean = -1;
    qint64 lastVariance = -1;
    qint64 lastMaximum = -1;
    QString lastMessage;
    int printCount = 0;

protected:
    qint64 currentTimestamp() const override
    {
        return m_now;
    }

    QString format(qint64 mean, qint64 variance, qint64 maximum) override
    {
        lastMean = mean;
        lastVariance = variance;
        lastMaximum = maximum;
        return QStringLiteral("latency");
    }

    void print(const QString &message) override
    {
        lastMessage = message;
        ++printCount;
    }

private:
    qint64 m_now;
};

} // namespace

class KisLatencyTrackerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rollingMaximumReportsEmptyStateAndCurrentWindow();
    void scalarTrackerAggregatesAtTheWindowBoundaryAndDestroysVirtually();
    void latencyTrackerPushesTheDifferenceFromCurrentTime();
};

void KisLatencyTrackerContractTest::rollingMaximumReportsEmptyStateAndCurrentWindow()
{
    KisRollingMax<int> maximum(2);
    bool threwForEmptyMaximum = false;
    try {
        (void)maximum.max();
    } catch (const std::runtime_error &) {
        threwForEmptyMaximum = true;
    }
    QVERIFY(threwForEmptyMaximum);

    maximum.push(5);
    maximum.push(4);
    QCOMPARE(maximum.max(), 5);

    maximum.push(3);
    QCOMPARE(maximum.max(), 5);

    maximum.push(2);
    QCOMPARE(maximum.max(), 4);
}

void KisLatencyTrackerContractTest::scalarTrackerAggregatesAtTheWindowBoundaryAndDestroysVirtually()
{
    ScalarTrackerProbe tracker(QStringLiteral("scalar"), 3);
    tracker.push(2);
    tracker.push(4);
    QCOMPARE(tracker.printCount, 0);

    tracker.push(6);
    QCOMPARE(tracker.printCount, 1);
    QCOMPARE(tracker.lastMean, 4);
    QCOMPARE(tracker.lastMaximum, 6);
    QCOMPARE(tracker.lastMessage, QStringLiteral("formatted"));
    QVERIFY(tracker.lastVariance >= 0);

    bool destroyed = false;
    KisScalarTracker<int> *base = new DestructionProbe(&destroyed);
    delete base;
    QVERIFY(destroyed);
}

void KisLatencyTrackerContractTest::latencyTrackerPushesTheDifferenceFromCurrentTime()
{
    LatencyTrackerProbe tracker(125);
    tracker.push(100);

    QCOMPARE(tracker.printCount, 1);
    QCOMPARE(tracker.lastMean, 25);
    QCOMPARE(tracker.lastMaximum, 25);
    QCOMPARE(tracker.lastMessage, QStringLiteral("latency"));
    QVERIFY(tracker.lastVariance >= 0);
}

QTEST_GUILESS_MAIN(KisLatencyTrackerContractTest)

#include "KisLatencyTrackerContractTest.moc"
