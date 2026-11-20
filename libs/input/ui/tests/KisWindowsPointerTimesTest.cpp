/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisWindowsPointerTimes.h>

#include <QTest>

namespace
{

KisWindowsPointerTimes::Sample sample(quint32 messageTime, qreal x, qreal y,
                                      quint64 performanceCount)
{
    KisWindowsPointerTimes::Sample entry;
    entry.messageTime = messageTime;
    entry.logicalPosition = QPointF(x, y);
    entry.performanceCount = performanceCount;
    return entry;
}

} // namespace

/**
 * Contract of the pointer-sample times that replace the 15.6 ms steps of
 * msg.time. The class is pure computation, so the tests run on every platform.
 */
class KisWindowsPointerTimesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mapsEachSampleOfOneMessageByPosition();
    void rejectsPositionsFartherThanOneAndAHalfPixels();
    void dropsSamplesOlderThanTheMatch();
    void ignoresSamplesWithoutAPerformanceCount();
    void keepsTheNewestFiveHundredTwelveSamples();
    void retakesTheOffsetAfterALargeDeviation();
};

void KisWindowsPointerTimesTest::mapsEachSampleOfOneMessageByPosition()
{
    KisWindowsPointerTimes times;
    // 10 MHz: 100000 counts are 10 ms.
    times.setFrequency(10000000);

    QVector<KisWindowsPointerTimes::Sample> samples;
    for (int i = 0; i < 4; ++i) {
        samples.append(sample(1000, 10.0 * i, 0.0,
                              100000000 + quint64(i) * 100000));
    }
    times.addSamples(samples);

    // The four samples of one message share msg.time, so the position picks
    // the sample and the counter gives the time inside the message.
    for (int i = 0; i < 4; ++i) {
        quint64 timestampMs = 0;
        QVERIFY(times.take(1000, QPointF(10.0 * i, 0.0), &timestampMs));
        QCOMPARE(timestampMs, quint64(1000 + 10 * i));
    }
}

void KisWindowsPointerTimesTest::rejectsPositionsFartherThanOneAndAHalfPixels()
{
    const KisWindowsPointerTimes::Sample entry = sample(2000, 10.0, 10.0, 3000000);

    {
        KisWindowsPointerTimes times;
        times.setFrequency(1000000);
        times.addSamples({entry});

        quint64 timestampMs = 0;
        QVERIFY(times.take(2000, QPointF(11.5, 10.0), &timestampMs));
    }

    {
        KisWindowsPointerTimes times;
        times.setFrequency(1000000);
        times.addSamples({entry});

        quint64 timestampMs = 0;
        QVERIFY(!times.take(2000, QPointF(11.51, 10.0), &timestampMs));

        // A miss keeps the samples, so the exact position still matches.
        QVERIFY(times.take(2000, QPointF(10.0, 10.0), &timestampMs));
    }
}

void KisWindowsPointerTimesTest::dropsSamplesOlderThanTheMatch()
{
    KisWindowsPointerTimes times;
    times.setFrequency(1000000);
    times.addSamples({sample(3000, 0.0, 0.0, 3000000),
                      sample(3000, 10.0, 0.0, 3001000),
                      sample(3000, 20.0, 0.0, 3002000)});

    quint64 timestampMs = 0;
    QVERIFY(times.take(3000, QPointF(10.0, 0.0), &timestampMs));

    // The matched sample and the older one belong to this event already.
    QVERIFY(!times.take(3000, QPointF(0.0, 0.0), &timestampMs));
    QVERIFY(times.take(3000, QPointF(20.0, 0.0), &timestampMs));
}

void KisWindowsPointerTimesTest::ignoresSamplesWithoutAPerformanceCount()
{
    KisWindowsPointerTimes times;
    times.setFrequency(1000000);
    times.addSamples({sample(4000, 0.0, 0.0, 0),
                      sample(4000, 10.0, 0.0, 4000000)});

    quint64 timestampMs = 0;
    QVERIFY(!times.take(4000, QPointF(0.0, 0.0), &timestampMs));
    QVERIFY(times.take(4000, QPointF(10.0, 0.0), &timestampMs));
}

void KisWindowsPointerTimesTest::keepsTheNewestFiveHundredTwelveSamples()
{
    KisWindowsPointerTimes times;
    times.setFrequency(1000000);

    QVector<KisWindowsPointerTimes::Sample> samples;
    for (int i = 0; i < 600; ++i) {
        // One message time and 1 ms of counter per sample keep the offset
        // stable over the whole run.
        samples.append(sample(1000 + quint32(i), 0.0, 0.0,
                              10000000 + quint64(i) * 1000));
    }
    times.addSamples(samples);

    quint64 timestampMs = 0;
    // Samples 0 to 87 fell out; 88 is the oldest one kept.
    QVERIFY(!times.take(1000, QPointF(0.0, 0.0), &timestampMs));
    QVERIFY(times.take(1088, QPointF(0.0, 0.0), &timestampMs));
    QVERIFY(times.take(1599, QPointF(0.0, 0.0), &timestampMs));
}

void KisWindowsPointerTimesTest::retakesTheOffsetAfterALargeDeviation()
{
    KisWindowsPointerTimes times;
    times.setFrequency(1000);

    // The first sample fixes the offset between the counter and msg.time.
    times.addSamples({sample(5000, 0.0, 0.0, 10000000)});

    quint64 timestampMs = 0;
    QVERIFY(times.take(5000, QPointF(0.0, 0.0), &timestampMs));
    QCOMPARE(timestampMs, quint64(5000));

    // The counter jumps 10 s while msg.time advances 1 ms, so the stored
    // offset would be off by far more than 1000 ms.
    times.addSamples({sample(5001, 0.0, 0.0, 20000000)});
    QVERIFY(times.take(5001, QPointF(0.0, 0.0), &timestampMs));
    QCOMPARE(timestampMs, quint64(5001));

    // The new offset holds for the following samples.
    times.addSamples({sample(5006, 0.0, 0.0, 20005000)});
    QVERIFY(times.take(5006, QPointF(0.0, 0.0), &timestampMs));
    QCOMPARE(timestampMs, quint64(5006));
}

QTEST_GUILESS_MAIN(KisWindowsPointerTimesTest)

#include "KisWindowsPointerTimesTest.moc"
