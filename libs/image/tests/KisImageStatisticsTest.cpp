/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_histogram.h>
#include <kis_memory_statistics_server.h>

#include <QTest>

class KisImageStatisticsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyHistogramHasNoSamples();
    void initialMemoryUsageIsZero();
    void initialSwapUsageAndLimitsAreZero();
};

void KisImageStatisticsTest::emptyHistogramHasNoSamples()
{
    KisHistogram::Calculations calculations;
    QCOMPARE(calculations.getMax(), 0.0);
    QCOMPARE(calculations.getMin(), 0.0);
    QCOMPARE(calculations.getMean(), 0.0);
    QCOMPARE(calculations.getTotal(), 0.0);
    QCOMPARE(calculations.getHighest(), quint32(0));
    QCOMPARE(calculations.getLowest(), quint32(0));
    QCOMPARE(calculations.getCount(), quint32(0));
}

void KisImageStatisticsTest::initialMemoryUsageIsZero()
{
    const KisMemoryStatisticsServer::Statistics statistics;
    QCOMPARE(statistics.imageSize, qint64(0));
    QCOMPARE(statistics.layersSize, qint64(0));
    QCOMPARE(statistics.projectionsSize, qint64(0));
    QCOMPARE(statistics.lodSize, qint64(0));
    QCOMPARE(statistics.totalMemorySize, qint64(0));
    QCOMPARE(statistics.realMemorySize, qint64(0));
    QCOMPARE(statistics.historicalMemorySize, qint64(0));
    QCOMPARE(statistics.poolSize, qint64(0));
}

void KisImageStatisticsTest::initialSwapUsageAndLimitsAreZero()
{
    const KisMemoryStatisticsServer::Statistics statistics;
    QCOMPARE(statistics.swapSize, qint64(0));
    QCOMPARE(statistics.totalMemoryLimit, qint64(0));
    QCOMPARE(statistics.tilesHardLimit, qint64(0));
    QCOMPARE(statistics.tilesSoftLimit, qint64(0));
    QCOMPARE(statistics.tilesPoolLimit, qint64(0));
}

QTEST_GUILESS_MAIN(KisImageStatisticsTest)

#include "KisImageStatisticsTest.moc"
