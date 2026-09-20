/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/KisStrokeSpeedMeasurer.h"

#include <QPointF>
#include <QTest>
#include <QVector>

namespace
{

bool safeAssertTriggered = false;

void compareReal(qreal actual, qreal expected, qreal tolerance = 1e-12)
{
    QVERIFY2(qAbs(actual - expected) <= tolerance,
             qPrintable(QStringLiteral("actual=%1 expected=%2 tolerance=%3")
                            .arg(actual, 0, 'g', 16)
                            .arg(expected, 0, 'g', 16)
                            .arg(tolerance, 0, 'g', 16)));
}

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    safeAssertTriggered = true;
}

class KisStrokeSpeedMeasurerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialAndSingleSampleAreStationary();
    void singleAndBatchSamplesReportDistancePerMillisecond();
    void smoothingWindowPreservesPeakAndTracksRecentMotion();
    void resetClearsAllReportedSpeeds();
};

void KisStrokeSpeedMeasurerContractTest::initialAndSingleSampleAreStationary()
{
    safeAssertTriggered = false;

    {
        KisStrokeSpeedMeasurer measurer(100);
        QCOMPARE(measurer.averageSpeed(), 0.0);
        QCOMPARE(measurer.currentSpeed(), 0.0);
        QCOMPARE(measurer.maxSpeed(), 0.0);

        measurer.addSample(QPointF(12.0, -8.0), 25);
        QCOMPARE(measurer.averageSpeed(), 0.0);
        QCOMPARE(measurer.currentSpeed(), 0.0);
        QCOMPARE(measurer.maxSpeed(), 0.0);
    }

    QVERIFY(!safeAssertTriggered);
}

void KisStrokeSpeedMeasurerContractTest::singleAndBatchSamplesReportDistancePerMillisecond()
{
    safeAssertTriggered = false;

    KisStrokeSpeedMeasurer individual(1000);
    individual.addSample(QPointF(0.0, 0.0), 0);
    individual.addSample(QPointF(3.0, 4.0), 10);
    individual.addSample(QPointF(6.0, 8.0), 20);

    compareReal(individual.averageSpeed(), 0.5);
    compareReal(individual.currentSpeed(), 0.5);
    QCOMPARE(individual.maxSpeed(), 0.0);

    KisStrokeSpeedMeasurer batch(1000);
    batch.addSample(QPointF(0.0, 0.0), 0);
    batch.addSamples({QPointF(3.0, 4.0), QPointF(6.0, 8.0)}, 20);

    compareReal(batch.averageSpeed(), individual.averageSpeed());
    compareReal(batch.currentSpeed(), individual.currentSpeed());
    QCOMPARE(batch.maxSpeed(), individual.maxSpeed());
    QVERIFY(!safeAssertTriggered);
}

void KisStrokeSpeedMeasurerContractTest::smoothingWindowPreservesPeakAndTracksRecentMotion()
{
    safeAssertTriggered = false;

    KisStrokeSpeedMeasurer measurer(10);
    measurer.addSample(QPointF(0.0, 0.0), 0);
    measurer.addSample(QPointF(3.0, 4.0), 10);

    compareReal(measurer.currentSpeed(), 0.5);
    compareReal(measurer.maxSpeed(), 0.5);

    measurer.addSample(QPointF(3.0, 4.0), 20);
    compareReal(measurer.currentSpeed(), 0.25);
    compareReal(measurer.maxSpeed(), 0.5);

    measurer.addSample(QPointF(3.0, 4.0), 30);
    compareReal(measurer.currentSpeed(), 0.0);
    compareReal(measurer.maxSpeed(), 0.5);
    QVERIFY(!safeAssertTriggered);
}

void KisStrokeSpeedMeasurerContractTest::resetClearsAllReportedSpeeds()
{
    safeAssertTriggered = false;

    KisStrokeSpeedMeasurer measurer(10);
    measurer.addSample(QPointF(0.0, 0.0), 0);
    measurer.addSample(QPointF(3.0, 4.0), 10);
    QVERIFY(measurer.averageSpeed() > 0.0);
    QVERIFY(measurer.currentSpeed() > 0.0);
    QVERIFY(measurer.maxSpeed() > 0.0);

    measurer.reset();

    QCOMPARE(measurer.averageSpeed(), 0.0);
    QCOMPARE(measurer.currentSpeed(), 0.0);
    QCOMPARE(measurer.maxSpeed(), 0.0);

    measurer.addSample(QPointF(100.0, 100.0), 200);
    QCOMPARE(measurer.averageSpeed(), 0.0);
    QCOMPARE(measurer.currentSpeed(), 0.0);
    QCOMPARE(measurer.maxSpeed(), 0.0);
    QVERIFY(!safeAssertTriggered);
}

QTEST_GUILESS_MAIN(KisStrokeSpeedMeasurerContractTest)

#include "KisStrokeSpeedMeasurerContractTest.moc"
