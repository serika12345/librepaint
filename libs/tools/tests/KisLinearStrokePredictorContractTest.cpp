/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisLinearStrokePredictor.h"
#include "KisStrokePrediction.h"
#include "KisStrokePredictionCap.h"

#include <QPointF>
#include <QString>
#include <QTest>
#include <QVector>

#include <cmath>

namespace
{
constexpr qreal Tolerance = 1e-9;

struct Sample {
    QPointF position;
    qreal timeMs = 0.0;
    qreal pressure = 1.0;
};

bool verifyClose(const QString &caseName, int index, const char *field,
                 qreal actual, qreal expected, QString *error)
{
    if (std::abs(actual - expected) <= Tolerance) return true;

    *error = QStringLiteral("%1: sample %2: %3 is %4, the reference gives %5")
                 .arg(caseName).arg(index).arg(QString::fromLatin1(field))
                 .arg(actual).arg(expected);
    return false;
}

/// Feed the same samples to the old predictor with the cap in its settings and
/// to the adapter followed by the cap, and compare every result.
bool sequenceMatches(const QString &caseName, const QVector<Sample> &samples,
                     qreal maxMovePx, qreal horizonMs, QString *error)
{
    KisStrokePredictionSettings settings;
    settings.horizonMs = horizonMs;
    settings.maxMovePx = maxMovePx;

    KisStrokePrediction reference;
    reference.setSettings(settings);

    KisLinearStrokePredictor adapter(settings);

    for (int i = 0; i < samples.size(); ++i) {
        const Sample &sample = samples.at(i);

        reference.addSample(sample.position, sample.timeMs, sample.pressure);

        KisStrokePredictionSample adapterSample;
        adapterSample.documentPos = sample.position;
        adapterSample.timeMs = sample.timeMs;
        adapterSample.pressure = sample.pressure;
        adapter.addSample(adapterSample);

        const KisStrokePredictionResult expected = reference.predict();
        KisStrokePredictionResult actual = adapter.predict(horizonMs);
        KisStrokePredictionCap::apply(actual, maxMovePx);

        if (actual.valid != expected.valid) {
            *error = QStringLiteral("%1: sample %2: valid is %3, the reference gives %4")
                         .arg(caseName).arg(i).arg(actual.valid).arg(expected.valid);
            return false;
        }
        if (actual.latestIndex != expected.latestIndex) {
            *error = QStringLiteral("%1: sample %2: latestIndex is %3, the reference gives %4")
                         .arg(caseName).arg(i).arg(actual.latestIndex).arg(expected.latestIndex);
            return false;
        }
        if (actual.polyline.size() != expected.polyline.size()) {
            *error = QStringLiteral("%1: sample %2: polyline has %3 points, the reference gives %4")
                         .arg(caseName).arg(i).arg(actual.polyline.size()).arg(expected.polyline.size());
            return false;
        }
        for (int point = 0; point < expected.polyline.size(); ++point) {
            if (!verifyClose(caseName, i, "polyline x",
                             actual.polyline.at(point).x(), expected.polyline.at(point).x(), error)
                || !verifyClose(caseName, i, "polyline y",
                                actual.polyline.at(point).y(), expected.polyline.at(point).y(), error)) {
                return false;
            }
        }
        if (!verifyClose(caseName, i, "confidence", actual.confidence, expected.confidence, error)
            || !verifyClose(caseName, i, "moveLength", actual.moveLength, expected.moveLength, error)) {
            return false;
        }
    }

    return true;
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

/// 4 ms apart; positions advance by the given step per sample.
QVector<Sample> stepped(const QVector<qreal> &steps)
{
    QVector<Sample> samples;
    qreal x = 0.0;
    qreal time = 0.0;
    samples.append({QPointF(x, 0.0), time, 1.0});
    for (const qreal step : steps) {
        x += step;
        time += 4.0;
        samples.append({QPointF(x, 0.0), time, 1.0});
    }
    return samples;
}
} // namespace

class KisLinearStrokePredictorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constantSpeedMatchesTheOldPredictor();
    void decelerationMatchesTheOldPredictor();
    void suddenStopMatchesTheOldPredictor();
    void historyResetByAGapMatchesTheOldPredictor();
    void cappedMoveMatchesTheOldPredictor();
    void uncappedMoveMatchesTheOldPredictor();
    void horizonOf25msMatchesTheOldPredictor();
    void sourceIsLinear();

private:
    void check(const QString &name, const QVector<Sample> &samples,
               qreal maxMovePx, qreal horizonMs);
};

void KisLinearStrokePredictorContractTest::check(const QString &name,
                                                 const QVector<Sample> &samples,
                                                 qreal maxMovePx, qreal horizonMs)
{
    QString error;
    QVERIFY2(sequenceMatches(name, samples, maxMovePx, horizonMs, &error), qPrintable(error));
}

void KisLinearStrokePredictorContractTest::constantSpeedMatchesTheOldPredictor()
{
    check(QStringLiteral("constant speed"), constantSpeed(2.0, 4.0, 8), 30.0, 16.7);
}

void KisLinearStrokePredictorContractTest::decelerationMatchesTheOldPredictor()
{
    check(QStringLiteral("deceleration"), stepped({12, 12, 12, 12, 12, 12, 12, 1.2, 1.2, 1.2}),
          30.0, 16.7);
}

void KisLinearStrokePredictorContractTest::suddenStopMatchesTheOldPredictor()
{
    check(QStringLiteral("sudden stop"), stepped({12, 12, 12, 12, 12, 12, 0, 0, 0}), 30.0, 16.7);
}

void KisLinearStrokePredictorContractTest::historyResetByAGapMatchesTheOldPredictor()
{
    QVector<Sample> samples = constantSpeed(2.0, 4.0, 3);

    // A gap of 52 ms is above gapResetMs, so the history starts again here.
    for (int i = 0; i < 3; ++i) {
        const qreal time = 60.0 + 4.0 * i;
        samples.append({QPointF(2.0 * time, 0.0), time, 1.0});
    }

    check(QStringLiteral("gap reset"), samples, 30.0, 16.7);
}

void KisLinearStrokePredictorContractTest::cappedMoveMatchesTheOldPredictor()
{
    check(QStringLiteral("capped move"), constantSpeed(4.0, 4.0, 8), 30.0, 16.7);
}

void KisLinearStrokePredictorContractTest::uncappedMoveMatchesTheOldPredictor()
{
    check(QStringLiteral("uncapped move"), constantSpeed(0.8, 4.0, 8), 30.0, 16.7);
}

void KisLinearStrokePredictorContractTest::horizonOf25msMatchesTheOldPredictor()
{
    check(QStringLiteral("horizon 25 ms"), constantSpeed(1.0, 4.0, 8), 30.0, 25.0);
}

void KisLinearStrokePredictorContractTest::sourceIsLinear()
{
    KisStrokePredictionSettings settings;
    KisLinearStrokePredictor predictor(settings);

    QCOMPARE(predictor.source(), KisStrokePredictionSource::Linear);

    for (const Sample &sample : constantSpeed(2.0, 4.0, 8)) {
        KisStrokePredictionSample adapterSample;
        adapterSample.documentPos = sample.position;
        adapterSample.timeMs = sample.timeMs;
        adapterSample.pressure = sample.pressure;
        predictor.addSample(adapterSample);
    }

    const KisStrokePredictionResult result = predictor.predict(16.7);
    QVERIFY(result.valid);
    QCOMPARE(result.source, KisStrokePredictionSource::Linear);
}

QTEST_GUILESS_MAIN(KisLinearStrokePredictorContractTest)

#include "KisLinearStrokePredictorContractTest.moc"
