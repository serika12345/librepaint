/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePredictionReplay.h"

#include <QJsonObject>
#include <QPointF>
#include <QTest>

namespace
{
bool closeTo(qreal actual, qreal expected, qreal tolerance)
{
    return qAbs(actual - expected) <= tolerance;
}

#define VERIFY_CLOSE(actual, expected, tolerance)                              \
    QVERIFY2(closeTo((actual), (expected), (tolerance)),                       \
             qPrintable(QStringLiteral("%1 is not %2").arg(qreal(actual)).arg(qreal(expected))))

QVector<KisStrokePredictionReplayStroke> constantStroke(qreal speedPerMs,
                                                        qreal intervalMs,
                                                        int count)
{
    KisStrokePredictionReplayStroke stroke;
    for (int i = 0; i < count; ++i) {
        KisStrokePredictionReplaySample sample;
        sample.arrivalMs = intervalMs * i;
        sample.eventMs = intervalMs * i;
        sample.position = QPointF(speedPerMs * sample.eventMs, 0.0);
        sample.pressure = 1.0;
        stroke.samples.append(sample);
    }
    return {stroke};
}

/// Four events every 16.7 ms, one millisecond apart inside a burst.
QVector<KisStrokePredictionReplayStroke> burstStroke()
{
    KisStrokePredictionReplayStroke stroke;
    qreal arrivalMs = 0.0;
    for (int burst = 0; burst < 4; ++burst) {
        for (int i = 0; i < 4; ++i) {
            KisStrokePredictionReplaySample sample;
            sample.arrivalMs = arrivalMs;
            sample.eventMs = arrivalMs;
            sample.position = QPointF(5.0 * arrivalMs, 0.0);
            sample.pressure = 1.0;
            stroke.samples.append(sample);
            arrivalMs += 1.0;
        }
        arrivalMs += 13.7;
    }
    return {stroke};
}
} // namespace

class KisStrokePredictionReplayContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stopOvershootFollowsTheCap();
    void fixedExpiryDoesNotExpireMidStroke();
    void shorterExpiryShortensTheVisibleStop();
    void expiryBelowTheArrivalGapExpiresMidStroke();
    void gapExpiryExpiresBetweenBursts();
    void decelerationFactorKeepsSteadyAhead();
    void cappedPredictionErrorIsTheLeadMinusTheCap();
    void validPredictionsAreCounted();
};

void KisStrokePredictionReplayContractTest::stopOvershootFollowsTheCap()
{
    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(5.0, 4.17, 8);

    KisStrokePredictionReplaySettings settings;
    settings.capDiameters = 0.0;

    KisStrokePredictionReplayReport withoutCap =
        runKisStrokePredictionReplay(strokes, settings);
    QCOMPARE(withoutCap.stopOvershootPx.n, 1);
    VERIFY_CLOSE(withoutCap.stopOvershootPx.max, 5.0 * 16.7, 2.0);

    settings.capDiameters = 3.0;
    settings.diameter = 10.0;

    KisStrokePredictionReplayReport withCap =
        runKisStrokePredictionReplay(strokes, settings);
    QCOMPARE(withCap.stopOvershootPx.n, 1);
    VERIFY_CLOSE(withCap.stopOvershootPx.max, 30.0, 1.0);
}

void KisStrokePredictionReplayContractTest::fixedExpiryDoesNotExpireMidStroke()
{
    KisStrokePredictionReplaySettings settings;
    settings.gapExpiry = false;

    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(5.0, 4.17, 8);

    const KisStrokePredictionReplayReport report =
        runKisStrokePredictionReplay(strokes, settings);

    QCOMPARE(report.midStrokeExpiry.n, 1);
    VERIFY_CLOSE(report.midStrokeExpiry.max, 0.0, 0.0);
}

void KisStrokePredictionReplayContractTest::shorterExpiryShortensTheVisibleStop()
{
    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(5.0, 4.0, 8);

    KisStrokePredictionReplaySettings longExpiry;
    longExpiry.expiryMs = 20.0;

    KisStrokePredictionReplaySettings shortExpiry;
    shortExpiry.expiryMs = 5.0;

    const KisStrokePredictionReplayReport longReport =
        runKisStrokePredictionReplay(strokes, longExpiry);
    const KisStrokePredictionReplayReport shortReport =
        runKisStrokePredictionReplay(strokes, shortExpiry);

    QCOMPARE(longReport.stopVisibleMs.n, 1);
    QCOMPARE(shortReport.stopVisibleMs.n, 1);
    QVERIFY(shortReport.stopVisibleMs.max < longReport.stopVisibleMs.max);

    const QJsonObject json =
        kisStrokePredictionReplayReportJson(shortReport, shortExpiry);
    QCOMPARE(json.value(QStringLiteral("replay.stop_visible_ms"))
                 .toObject()
                 .value(QStringLiteral("max"))
                 .toDouble(),
             double(shortReport.stopVisibleMs.max));
    QVERIFY(json.contains(QStringLiteral("replay.stop_excess_area_px_ms")));
}

void KisStrokePredictionReplayContractTest::expiryBelowTheArrivalGapExpiresMidStroke()
{
    // One sample per frame: the idle gap is the frame interval.
    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(5.0, 16.7, 8);

    KisStrokePredictionReplaySettings below;
    below.expiryMs = 16.0;

    const KisStrokePredictionReplayReport expired =
        runKisStrokePredictionReplay(strokes, below);
    QCOMPARE(expired.midStrokeExpiry.n, 1);
    QVERIFY(expired.midStrokeExpiry.sum >= 1.0);

    KisStrokePredictionReplaySettings above;
    above.expiryMs = 17.0;

    const KisStrokePredictionReplayReport kept =
        runKisStrokePredictionReplay(strokes, above);
    VERIFY_CLOSE(kept.midStrokeExpiry.sum, 0.0, 0.0);
}

void KisStrokePredictionReplayContractTest::gapExpiryExpiresBetweenBursts()
{
    const QVector<KisStrokePredictionReplayStroke> strokes = burstStroke();

    KisStrokePredictionReplaySettings gapSettings;
    gapSettings.gapExpiry = true;
    const KisStrokePredictionReplayReport gap =
        runKisStrokePredictionReplay(strokes, gapSettings);
    QCOMPARE(gap.midStrokeExpiry.n, 1);
    QVERIFY(gap.midStrokeExpiry.max >= 1.0);

    KisStrokePredictionReplaySettings fixedSettings;
    fixedSettings.gapExpiry = false;
    const KisStrokePredictionReplayReport fixed =
        runKisStrokePredictionReplay(strokes, fixedSettings);
    QCOMPARE(fixed.midStrokeExpiry.n, 1);
    VERIFY_CLOSE(fixed.midStrokeExpiry.max, 0.0, 0.0);
}

void KisStrokePredictionReplayContractTest::decelerationFactorKeepsSteadyAhead()
{
    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(5.0, 4.0, 10);

    KisStrokePredictionReplaySettings withDecel;
    withDecel.decel = true;

    KisStrokePredictionReplaySettings withoutDecel;
    withoutDecel.decel = false;

    const KisStrokePredictionReplayReport on =
        runKisStrokePredictionReplay(strokes, withDecel);
    const KisStrokePredictionReplayReport off =
        runKisStrokePredictionReplay(strokes, withoutDecel);

    QVERIFY(on.aheadPx.n > 0);
    QCOMPARE(on.aheadPx.n, off.aheadPx.n);
    VERIFY_CLOSE(on.aheadPx.p50, off.aheadPx.p50, 1e-9);
}

/// A steady stroke whose lead over one horizon exceeds the cap: the error to
/// the real point one horizon later is the lead minus the cap.
void KisStrokePredictionReplayContractTest::cappedPredictionErrorIsTheLeadMinusTheCap()
{
    const qreal speed = 5.0;
    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(speed, 4.17, 20);

    KisStrokePredictionReplaySettings settings;
    settings.capDiameters = 3.0;
    settings.diameter = 10.0;

    const qreal lead = speed * settings.horizonMs;
    const qreal cap = settings.capDiameters * settings.diameter;
    QVERIFY(lead > cap);

    const KisStrokePredictionReplayReport report =
        runKisStrokePredictionReplay(strokes, settings);

    QVERIFY(report.predictionErrorPx.n > 0);
    VERIFY_CLOSE(report.predictionErrorPx.p50, lead - cap, 1.0);
}

void KisStrokePredictionReplayContractTest::validPredictionsAreCounted()
{
    const QVector<KisStrokePredictionReplayStroke> strokes =
        constantStroke(5.0, 4.17, 8);

    KisStrokePredictionReplaySettings settings;

    const KisStrokePredictionReplayReport report =
        runKisStrokePredictionReplay(strokes, settings);

    QCOMPARE(report.predictionSamples, 8);
    QVERIFY(report.validPredictions > 0);
    QVERIFY(report.validPredictions <= report.predictionSamples);

    const QJsonObject json =
        kisStrokePredictionReplayReportJson(report, settings);
    const QJsonObject counted =
        json.value(QStringLiteral("replay.valid_predictions")).toObject();
    QCOMPARE(counted.value(QStringLiteral("valid")).toInt(),
             report.validPredictions);
    QCOMPARE(counted.value(QStringLiteral("samples")).toInt(),
             report.predictionSamples);

    const QVector<KisStrokePredictionReplayStroke> single =
        constantStroke(5.0, 4.17, 1);
    const KisStrokePredictionReplayReport one =
        runKisStrokePredictionReplay(single, settings);
    QCOMPARE(one.predictionSamples, 1);
    QCOMPARE(one.validPredictions, 0);
}

QTEST_GUILESS_MAIN(KisStrokePredictionReplayContractTest)

#include "KisStrokePredictionReplayContractTest.moc"
