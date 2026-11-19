/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKalmanFilter.h"
#include "KisKalmanStrokePredictor.h"
#include "KisLinearStrokePredictor.h"
#include "KisStrokePrediction.h"

#include <QPointF>
#include <QTest>
#include <QVector>

#include <cmath>

namespace
{
constexpr qreal IntervalMs = 4.17;
constexpr qreal SpeedPxPerMs = 5.0;
constexpr qreal HorizonMs = 16.7;

struct Sample {
    QPointF position;
    qreal timeMs = 0.0;
    qreal pressure = 1.0;
};

KisStrokePredictionSample predictorSample(const Sample &sample)
{
    KisStrokePredictionSample result;
    result.documentPos = sample.position;
    result.timeMs = sample.timeMs;
    result.pressure = sample.pressure;
    return result;
}

QVector<Sample> constantSpeed(qreal speed, qreal intervalMs, int count)
{
    QVector<Sample> samples;
    for (int i = 0; i < count; ++i) {
        const qreal time = intervalMs * i;
        samples.append({QPointF(speed * time, 0.0), time, 1.0});
    }
    return samples;
}

/// Constant speed for steadyMs, then a smooth fall to rest over stopMs. The
/// position follows u - u^3 + u^4 / 2 with u the normalised stop time.
QVector<Sample> smoothStop(qreal speed, qreal intervalMs, qreal steadyMs, qreal stopMs)
{
    QVector<Sample> samples;

    for (qreal time = 0.0; time <= steadyMs; time += intervalMs) {
        samples.append({QPointF(speed * time, 0.0), time, 1.0});
    }

    const qreal startX = speed * steadyMs;
    for (qreal time = steadyMs + intervalMs; time < steadyMs + stopMs; time += intervalMs) {
        const qreal u = (time - steadyMs) / stopMs;
        const qreal x = startX + speed * stopMs * (u - u * u * u + u * u * u * u / 2.0);
        samples.append({QPointF(x, 0.0), time, 1.0});
    }
    samples.append({QPointF(startX + speed * stopMs / 2.0, 0.0), steadyMs + stopMs, 1.0});

    return samples;
}
} // namespace

class KisKalmanStrokePredictorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constantSpeedReachesTheHorizon();
    void fewerThanFourSamplesAreInvalid();
    void aSmoothStopMovesLessThanTheLinearPredictor();
    void aGapOfTwentyMillisecondsIsInvalid();
    void sourceIsKalman();
    void twoUpdatesFollowTheChromiumFormulas();
};

void KisKalmanStrokePredictorContractTest::constantSpeedReachesTheHorizon()
{
    KisStrokePredictionSettings settings;
    KisKalmanStrokePredictor predictor(settings);

    const QVector<Sample> samples = constantSpeed(SpeedPxPerMs, IntervalMs, 30);
    for (int i = 0; i < samples.size(); ++i) {
        predictor.addSample(predictorSample(samples.at(i)));

        if (i < 19) continue;

        const KisStrokePredictionResult result = predictor.predict(HorizonMs);
        QVERIFY2(result.valid, qPrintable(QStringLiteral("sample %1 is not valid").arg(i)));

        const QPointF latest = samples.at(i).position;
        const QPointF expected(latest.x() + SpeedPxPerMs * HorizonMs, latest.y());
        const qreal distance = std::hypot(result.polyline.constLast().x() - expected.x(),
                                          result.polyline.constLast().y() - expected.y());
        QVERIFY2(distance <= 2.0,
                 qPrintable(QStringLiteral("sample %1: the last point is %2 px from the horizon")
                                .arg(i).arg(distance)));
    }
}

void KisKalmanStrokePredictorContractTest::fewerThanFourSamplesAreInvalid()
{
    KisStrokePredictionSettings settings;
    KisKalmanStrokePredictor predictor(settings);

    const QVector<Sample> samples = constantSpeed(SpeedPxPerMs, IntervalMs, 4);
    for (int i = 0; i < 3; ++i) {
        predictor.addSample(predictorSample(samples.at(i)));
        QVERIFY(!predictor.predict(HorizonMs).valid);
    }
}

void KisKalmanStrokePredictorContractTest::aSmoothStopMovesLessThanTheLinearPredictor()
{
    KisStrokePredictionSettings settings;

    const QVector<Sample> samples = smoothStop(50.0, 15.0, 150.0, 90.0);
    QVERIFY(samples.size() > 5);

    KisKalmanStrokePredictor kalman(settings);
    KisLinearStrokePredictor linear(settings);
    for (const Sample &sample : samples) {
        kalman.addSample(predictorSample(sample));
        linear.addSample(predictorSample(sample));
    }

    const KisStrokePredictionResult kalmanResult = kalman.predict(HorizonMs);
    const KisStrokePredictionResult linearResult = linear.predict(HorizonMs);

    QVERIFY2(kalmanResult.moveLength < linearResult.moveLength,
             qPrintable(QStringLiteral("Kalman moves %1 px, linear moves %2 px")
                            .arg(kalmanResult.moveLength).arg(linearResult.moveLength)));
}

void KisKalmanStrokePredictorContractTest::aGapOfTwentyMillisecondsIsInvalid()
{
    KisStrokePredictionSettings settings;
    KisKalmanStrokePredictor predictor(settings);

    const QVector<Sample> samples = constantSpeed(SpeedPxPerMs, IntervalMs, 8);
    for (const Sample &sample : samples) {
        predictor.addSample(predictorSample(sample));
    }

    Sample afterGap = samples.constLast();
    afterGap.position += QPointF(5.0 * 20.0, 0.0);
    afterGap.timeMs += 20.0;
    predictor.addSample(predictorSample(afterGap));

    QVERIFY(!predictor.predict(HorizonMs).valid);
}

void KisKalmanStrokePredictorContractTest::sourceIsKalman()
{
    KisStrokePredictionSettings settings;
    KisKalmanStrokePredictor predictor(settings);

    QCOMPARE(predictor.source(), KisStrokePredictionSource::Kalman);

    for (const Sample &sample : constantSpeed(SpeedPxPerMs, IntervalMs, 30)) {
        predictor.addSample(predictorSample(sample));
    }

    const KisStrokePredictionResult result = predictor.predict(HorizonMs);
    QVERIFY(result.valid);
    QCOMPARE(result.source, KisStrokePredictionSource::Kalman);
}

void KisKalmanStrokePredictorContractTest::twoUpdatesFollowTheChromiumFormulas()
{
    // Chromium's KalmanFilter::Update() with the observations 100 and 120 at
    // 4.17 ms apart. The expected values come from the equations and the
    // constants of ui/base/prediction/kalman_filter.cc.
    KisKalmanFilter filter;
    filter.update(100.0, 0.0);
    filter.update(120.0, 4.17);

    const KisKalmanFilter::Vector3 state = filter.state();
    QVERIFY(std::abs(state.x - 119.791097178885) <= 1e-9);
    QVERIFY(std::abs(state.y - 8.520814975888097) <= 1e-9);
    QVERIFY(std::abs(state.z - 1.8344580843737515) <= 1e-9);

    const qreal expected[3][3] = {
        {0.98955485894425, 0.4260407487944049, 0.09172290421868756},
        {0.4260407487944049, 1.1852621035704118, 0.4704681117609309},
        {0.09172290421868758, 0.4704681117609308, 0.20454499336999016},
    };

    const KisKalmanFilter::Matrix3 covariance = filter.covariance();
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const qreal difference = std::abs(covariance.values[row][column]
                                              - expected[row][column]);
            QVERIFY2(difference <= 1e-9,
                     qPrintable(QStringLiteral("covariance %1,%2 differs by %3")
                                    .arg(row).arg(column).arg(difference)));
        }
    }
}

QTEST_GUILESS_MAIN(KisKalmanStrokePredictorContractTest)

#include "KisKalmanStrokePredictorContractTest.moc"
