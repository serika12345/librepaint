/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePrediction.h"

#include <QPointF>
#include <QTest>

#include <QtGlobal>

namespace
{

bool closeTo(qreal actual, qreal expected, qreal tolerance)
{
    return qAbs(actual - expected) <= tolerance;
}

#define VERIFY_CLOSE(actual, expected, tolerance)                              \
    QVERIFY2(closeTo((actual), (expected), (tolerance)),                       \
             qPrintable(QStringLiteral("%1 is not %2").arg(qreal(actual)).arg(qreal(expected))))

void addConstantLine(KisStrokePrediction &predictor, qreal speed,
                     qreal pressure, int count = 6, qreal intervalMs = 10.0)
{
    for (int i = 0; i < count; ++i) {
        const qreal time = intervalMs * i;
        predictor.addSample(QPointF(speed * time, 0.0), time, pressure);
    }
}

} // namespace

class KisStrokePredictionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constantSpeedReachesTheHorizon();
    void singlePointIsNotValid();
    void confidenceFollowsSpeed();
    void confidenceFollowsPressure();
    void gapsAndClockRestartsResetTheHistory();
    void sameTimeReplacesTheLatestPoint();
    void quantizedTimesKeepSpeedWithinTolerance();
    void settingsAreClampedAndKeepTheHistory();
    void resetDropsTheHistory();
    void smallMoveIsNotValidButReportsLength();
    void historyIsBounded();
    void strongDecelerationRemovesThePrediction();
    void stoppedEventsRemoveThePrediction();
    void mildDecelerationLowersTheConfidence();
    void decelerationSettingsAreClamped();
};

void KisStrokePredictionContractTest::constantSpeedReachesTheHorizon()
{
    KisStrokePrediction predictor;
    for (int i = 0; i < 8; ++i) {
        const qreal time = 4.0 * i;
        QVERIFY(!predictor.addSample(QPointF(2.0 * time, 0.0), time, 1.0));
    }

    const KisStrokePredictionResult result = predictor.predict();

    QVERIFY(result.valid);
    VERIFY_CLOSE(result.speed, 2.0, 1e-9);
    VERIFY_CLOSE(result.confidence, 1.0, 1e-9);
    VERIFY_CLOSE(result.moveLength, 2.0 * 16.7, 1e-9);

    // The anchor starts at 24 ms before the latest sample (t = 4 ms), so the
    // polyline holds 7 real points and the predicted one.
    QCOMPARE(result.latestIndex, 6);
    QCOMPARE(result.polyline.size(), 8);
    VERIFY_CLOSE(result.polyline.at(0).x(), 8.0, 1e-9);
    VERIFY_CLOSE(result.polyline.at(result.latestIndex).x(), 56.0, 1e-9);
    VERIFY_CLOSE(result.polyline.constLast().x(), 56.0 + 2.0 * 16.7, 1e-9);
    VERIFY_CLOSE(result.polyline.constLast().y(), 0.0, 1e-9);
}

void KisStrokePredictionContractTest::singlePointIsNotValid()
{
    KisStrokePrediction predictor;
    predictor.addSample(QPointF(10.0, 20.0), 5.0, 1.0);

    const KisStrokePredictionResult result = predictor.predict();

    QVERIFY(!result.valid);
    QCOMPARE(result.latestIndex, -1);
    QVERIFY(result.polyline.isEmpty());
}

void KisStrokePredictionContractTest::confidenceFollowsSpeed()
{
    KisStrokePrediction slow;
    addConstantLine(slow, 0.1, 1.0);
    const KisStrokePredictionResult slowResult = slow.predict();
    QVERIFY(!slowResult.valid);
    VERIFY_CLOSE(slowResult.confidence, 0.0, 1e-9);

    // 0.4 px/ms is halfway between speedZero (0.2) and speedFull (0.6), and
    // smooth(0.5) is 0.5.
    KisStrokePrediction mid;
    addConstantLine(mid, 0.4, 1.0);
    const KisStrokePredictionResult midResult = mid.predict();
    QVERIFY(midResult.valid);
    QVERIFY(midResult.confidence > 0.0);
    QVERIFY(midResult.confidence < 1.0);
    VERIFY_CLOSE(midResult.confidence, 0.5, 1e-9);

    qreal previousConfidence = 0.0;
    for (const qreal speed : {0.25, 0.4, 0.55}) {
        KisStrokePrediction predictor;
        addConstantLine(predictor, speed, 1.0);
        const qreal confidence = predictor.predict().confidence;
        QVERIFY(confidence > previousConfidence);
        previousConfidence = confidence;
    }
}

void KisStrokePredictionContractTest::confidenceFollowsPressure()
{
    KisStrokePrediction zero;
    addConstantLine(zero, 2.0, 0.05);
    const KisStrokePredictionResult zeroResult = zero.predict();
    QVERIFY(!zeroResult.valid);
    VERIFY_CLOSE(zeroResult.confidence, 0.0, 1e-9);

    KisStrokePrediction mid;
    addConstantLine(mid, 2.0, 0.10);
    const KisStrokePredictionResult midResult = mid.predict();
    QVERIFY(midResult.confidence > 0.0);
    QVERIFY(midResult.confidence < 1.0);

    for (const qreal pressure : {0.15, 0.5}) {
        KisStrokePrediction full;
        addConstantLine(full, 2.0, pressure);
        VERIFY_CLOSE(full.predict().confidence, 1.0, 1e-9);
    }
}

void KisStrokePredictionContractTest::gapsAndClockRestartsResetTheHistory()
{
    KisStrokePrediction predictor;
    QVERIFY(!predictor.addSample(QPointF(0.0, 0.0), 0.0, 1.0));
    QVERIFY(!predictor.addSample(QPointF(8.0, 0.0), 4.0, 1.0));
    QCOMPARE(predictor.sampleCount(), 2);

    // A gap shorter than gapResetMs only appends.
    QVERIFY(!predictor.addSample(QPointF(46.0, 0.0), 23.0, 1.0));
    QCOMPARE(predictor.sampleCount(), 3);

    // A gap of gapResetMs starts a new stroke.
    QVERIFY(predictor.addSample(QPointF(86.0, 0.0), 43.0, 1.0));
    QCOMPARE(predictor.sampleCount(), 1);

    // A clock going backwards starts a new stroke as well.
    QVERIFY(predictor.addSample(QPointF(80.0, 0.0), 40.0, 1.0));
    QCOMPARE(predictor.sampleCount(), 1);
}

void KisStrokePredictionContractTest::sameTimeReplacesTheLatestPoint()
{
    KisStrokePrediction predictor;
    predictor.addSample(QPointF(0.0, 0.0), 0.0, 1.0);
    predictor.addSample(QPointF(4.0, 0.0), 4.0, 0.5);
    QCOMPARE(predictor.sampleCount(), 2);

    QVERIFY(!predictor.addSample(QPointF(8.0, 0.0), 4.0, 1.0));
    QCOMPARE(predictor.sampleCount(), 2);

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(result.valid);
    QCOMPARE(result.polyline.at(result.latestIndex), QPointF(8.0, 0.0));
    VERIFY_CLOSE(result.speed, 2.0, 1e-9);
    VERIFY_CLOSE(result.confidence, 1.0, 1e-9);
}

void KisStrokePredictionContractTest::quantizedTimesKeepSpeedWithinTolerance()
{
    const qreal times[] = {0.0, 4.0, 7.0, 12.0, 15.0, 20.0, 24.0};

    KisStrokePrediction predictor;
    for (const qreal time : times) {
        predictor.addSample(QPointF(2.0 * time, 0.0), time, 1.0);
    }

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(result.valid);
    QVERIFY(qAbs(result.speed - 2.0) <= 0.3);
}

void KisStrokePredictionContractTest::settingsAreClampedAndKeepTheHistory()
{
    KisStrokePrediction predictor;
    predictor.addSample(QPointF(0.0, 0.0), 0.0, 1.0);
    predictor.addSample(QPointF(8.0, 0.0), 4.0, 1.0);

    KisStrokePredictionSettings settings;
    settings.horizonMs = 7.0;
    settings.anchorMs = 150.0;
    settings.velocityWindowMs = 1.0;
    settings.gapResetMs = 2.0;
    settings.speedZeroPxPerMs = 0.5;
    settings.speedFullPxPerMs = 0.4;
    settings.pressureZero = 0.2;
    settings.pressureFull = 0.1;
    predictor.setSettings(settings);

    const KisStrokePredictionSettings clamped = predictor.settings();
    VERIFY_CLOSE(clamped.horizonMs, 8.0, 1e-12);
    VERIFY_CLOSE(clamped.anchorMs, 100.0, 1e-12);
    VERIFY_CLOSE(clamped.velocityWindowMs, 4.0, 1e-12);
    VERIFY_CLOSE(clamped.gapResetMs, 5.0, 1e-12);
    VERIFY_CLOSE(clamped.speedFullPxPerMs, 0.51, 1e-12);
    VERIFY_CLOSE(clamped.pressureFull, 0.21, 1e-12);
    QCOMPARE(predictor.sampleCount(), 2);

    settings.horizonMs = 30.0;
    predictor.setSettings(settings);
    VERIFY_CLOSE(predictor.settings().horizonMs, 25.0, 1e-12);
}

void KisStrokePredictionContractTest::resetDropsTheHistory()
{
    KisStrokePrediction predictor;
    addConstantLine(predictor, 2.0, 1.0);
    QVERIFY(predictor.predict().valid);

    predictor.reset();

    QCOMPARE(predictor.sampleCount(), 0);
    QVERIFY(!predictor.predict().valid);
}

void KisStrokePredictionContractTest::smallMoveIsNotValidButReportsLength()
{
    KisStrokePrediction predictor;
    KisStrokePredictionSettings settings;
    settings.speedZeroPxPerMs = 0.01;
    settings.speedFullPxPerMs = 0.02;
    settings.horizonMs = 8.0;
    predictor.setSettings(settings);

    addConstantLine(predictor, 0.05, 1.0);

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(!result.valid);
    VERIFY_CLOSE(result.confidence, 1.0, 1e-9);
    QVERIFY(qAbs(result.moveLength - 0.4) <= 0.01);
}

// 4 ms apart; positions advance by the given step per sample (px).
void addSteps(KisStrokePrediction &predictor, const QVector<qreal> &steps)
{
    qreal x = 0.0;
    qreal time = 0.0;
    predictor.addSample(QPointF(x, 0.0), time, 1.0);
    for (const qreal step : steps) {
        x += step;
        time += 4.0;
        predictor.addSample(QPointF(x, 0.0), time, 1.0);
    }
}

void KisStrokePredictionContractTest::strongDecelerationRemovesThePrediction()
{
    // 3 px/ms for 28 ms, then 0.3 px/ms for 12 ms: the speed over 8 ms is
    // 0.3 and over 20 ms 1.38, a ratio of 0.22 (below the 0.4 of confidence 0).
    KisStrokePrediction predictor;
    addSteps(predictor, {12, 12, 12, 12, 12, 12, 12, 1.2, 1.2, 1.2});

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(!result.valid);
    VERIFY_CLOSE(result.confidence, 0.0, 1e-9);
}

void KisStrokePredictionContractTest::stoppedEventsRemoveThePrediction()
{
    // The pointer stops while events keep coming at the same position.
    KisStrokePrediction predictor;
    addSteps(predictor, {12, 12, 12, 12, 12, 12, 0, 0, 0});

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(!result.valid);
    VERIFY_CLOSE(result.confidence, 0.0, 1e-9);
}

void KisStrokePredictionContractTest::mildDecelerationLowersTheConfidence()
{
    // 3 px/ms for 28 ms, then 1 px/ms for 12 ms: ratio 0.71 between the windows.
    KisStrokePrediction predictor;
    addSteps(predictor, {12, 12, 12, 12, 12, 12, 12, 4, 4, 4});

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(result.valid);
    QVERIFY(result.confidence > 0.0);
    QVERIFY(result.confidence < 1.0);

    KisStrokePrediction steady;
    addSteps(steady, {4, 4, 4, 4, 4, 4, 4, 4, 4, 4});
    VERIFY_CLOSE(steady.predict().confidence, 1.0, 1e-9);
}

void KisStrokePredictionContractTest::decelerationSettingsAreClamped()
{
    KisStrokePrediction predictor;
    KisStrokePredictionSettings settings;
    settings.decelShortWindowMs = 1.0;
    settings.decelLongWindowMs = 1.0;
    settings.decelRatioZero = 0.9;
    settings.decelRatioFull = 0.5;
    predictor.setSettings(settings);

    const KisStrokePredictionSettings kept = predictor.settings();
    VERIFY_CLOSE(kept.decelShortWindowMs, 2.0, 1e-9);
    VERIFY_CLOSE(kept.decelLongWindowMs, 4.0, 1e-9);
    VERIFY_CLOSE(kept.decelRatioZero, 0.9, 1e-9);
    VERIFY_CLOSE(kept.decelRatioFull, 0.91, 1e-9);
}

void KisStrokePredictionContractTest::historyIsBounded()
{
    KisStrokePrediction predictor;
    for (int i = 0; i < 200; ++i) {
        predictor.addSample(QPointF(qreal(i), 0.0), qreal(i), 1.0);
    }

    QVERIFY(predictor.sampleCount() <= 64);
    QVERIFY(predictor.sampleCount() >= 2);

    const KisStrokePredictionResult result = predictor.predict();
    QVERIFY(result.valid);
    // The anchor keeps 24 ms of samples, and the polyline adds the predicted
    // point.
    QVERIFY(result.polyline.size() <= 26);
    QVERIFY(result.polyline.constFirst().x() >= 199.0 - 24.0);
}

QTEST_GUILESS_MAIN(KisStrokePredictionContractTest)

#include "KisStrokePredictionContractTest.moc"
