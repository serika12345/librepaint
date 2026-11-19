/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePrediction.h"
#include "KisStrokeSampleHistory.h"

#include <QPointF>
#include <QString>
#include <QTest>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace
{
constexpr qreal Tolerance = 1e-9;

struct Sample {
    QPointF position;
    qreal timeMs = 0.0;
    qreal pressure = 1.0;
};

qreal historyKeepMs(const KisStrokePredictionSettings &settings)
{
    return std::max(settings.velocityWindowMs, settings.decelLongWindowMs);
}

/// Feed the same samples to the history and to KisStrokePrediction, and
/// compare the anchor polyline with the part of the prediction line from its
/// anchor to the latest real point.
bool anchorsMatch(const QString &caseName, const QVector<Sample> &samples,
                  const KisStrokePredictionSettings &settings, QString *error)
{
    KisStrokeSampleHistory history(historyKeepMs(settings), settings.anchorMs, settings.gapResetMs);

    KisStrokePrediction reference;
    reference.setSettings(settings);

    for (int i = 0; i < samples.size(); ++i) {
        const Sample &sample = samples.at(i);

        const bool dropped = history.add(sample.position, sample.timeMs, sample.pressure);
        const bool referenceDropped = reference.addSample(sample.position, sample.timeMs,
                                                          sample.pressure);
        if (dropped != referenceDropped) {
            *error = QStringLiteral("%1: sample %2: dropped is %3, the reference gives %4")
                         .arg(caseName).arg(i).arg(dropped).arg(referenceDropped);
            return false;
        }

        const KisStrokePredictionResult result = reference.predict();
        if (!result.valid) continue;

        const QVector<QPointF> actual = history.anchorPolyline();
        const QVector<QPointF> expected = result.polyline.mid(0, result.latestIndex + 1);

        if (actual.size() != expected.size()) {
            *error = QStringLiteral("%1: sample %2: the anchor has %3 points, the reference gives %4")
                         .arg(caseName).arg(i).arg(actual.size()).arg(expected.size());
            return false;
        }
        for (int point = 0; point < expected.size(); ++point) {
            if (std::abs(actual.at(point).x() - expected.at(point).x()) > Tolerance
                || std::abs(actual.at(point).y() - expected.at(point).y()) > Tolerance) {
                *error = QStringLiteral("%1: sample %2: point %3 differs")
                             .arg(caseName).arg(i).arg(point);
                return false;
            }
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
} // namespace

class KisStrokeSampleHistoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void anchorMatchesThePredictionForConstantSpeed();
    void anchorMatchesThePredictionWithAShortAnchor();
    void anchorMatchesThePredictionAfterATrim();
    void anchorMatchesThePredictionAfterAGap();
    void sameTimeReplacesThePreviousSample();
    void aGapRestartsTheHistory();
    void aTimeGoingBackwardsRestartsTheHistory();
    void oldPointsAreTrimmed();
    void thePointCountIsCappedAt64();
};

void KisStrokeSampleHistoryContractTest::anchorMatchesThePredictionForConstantSpeed()
{
    const QVector<Sample> samples = constantSpeed(2.0, 4.0, 12);

    QString error;
    QVERIFY2(anchorsMatch(QStringLiteral("constant speed"), samples,
                          KisStrokePredictionSettings(), &error),
             qPrintable(error));
}

void KisStrokeSampleHistoryContractTest::anchorMatchesThePredictionWithAShortAnchor()
{
    KisStrokePredictionSettings settings;
    settings.anchorMs = 10.0;

    QString error;
    QVERIFY2(anchorsMatch(QStringLiteral("short anchor"), constantSpeed(2.0, 4.0, 12),
                          settings, &error),
             qPrintable(error));
}

void KisStrokeSampleHistoryContractTest::anchorMatchesThePredictionAfterATrim()
{
    QString error;
    QVERIFY2(anchorsMatch(QStringLiteral("trim"), constantSpeed(1.0, 4.0, 40),
                          KisStrokePredictionSettings(), &error),
             qPrintable(error));
}

void KisStrokeSampleHistoryContractTest::anchorMatchesThePredictionAfterAGap()
{
    QVector<Sample> samples = constantSpeed(2.0, 4.0, 5);
    samples.append({QPointF(10.0, 0.0), 60.0, 1.0});
    samples.append({QPointF(20.0, 0.0), 64.0, 1.0});
    samples.append({QPointF(30.0, 0.0), 68.0, 1.0});

    QString error;
    QVERIFY2(anchorsMatch(QStringLiteral("gap"), samples,
                          KisStrokePredictionSettings(), &error),
             qPrintable(error));
}

void KisStrokeSampleHistoryContractTest::sameTimeReplacesThePreviousSample()
{
    KisStrokeSampleHistory history(12.0, 24.0, 20.0);

    QVERIFY(!history.add(QPointF(0.0, 0.0), 0.0, 1.0));
    QVERIFY(!history.add(QPointF(10.0, 0.0), 10.0, 1.0));

    QCOMPARE(history.size(), 2);
    QVERIFY(!history.add(QPointF(12.0, 3.0), 10.0, 0.5));

    QCOMPARE(history.size(), 2);
    QCOMPARE(history.latest().position, QPointF(12.0, 3.0));
    QCOMPARE(history.latest().time, 10.0);
    QCOMPARE(history.latest().pressure, 0.5);
}

void KisStrokeSampleHistoryContractTest::aGapRestartsTheHistory()
{
    KisStrokeSampleHistory history(12.0, 24.0, 20.0);

    history.add(QPointF(0.0, 0.0), 0.0, 1.0);
    QVERIFY(!history.add(QPointF(10.0, 0.0), 10.0, 1.0));
    QVERIFY(history.add(QPointF(20.0, 0.0), 30.0, 1.0));

    QCOMPARE(history.size(), 1);
    QCOMPARE(history.latest().position, QPointF(20.0, 0.0));
}

void KisStrokeSampleHistoryContractTest::aTimeGoingBackwardsRestartsTheHistory()
{
    KisStrokeSampleHistory history(12.0, 24.0, 20.0);

    history.add(QPointF(0.0, 0.0), 10.0, 1.0);
    QVERIFY(history.add(QPointF(20.0, 0.0), 0.0, 1.0));

    QCOMPARE(history.size(), 1);
    QCOMPARE(history.latest().time, 0.0);
}

void KisStrokeSampleHistoryContractTest::oldPointsAreTrimmed()
{
    // The kept span is the largest of keepMs, anchorMs and gapResetMs, so the
    // 24 ms of the default anchor leaves the points from 176 ms to 196 ms.
    KisStrokeSampleHistory history(12.0, 24.0, 20.0);
    for (int i = 0; i <= 49; ++i) {
        history.add(QPointF(4.0 * i, 0.0), 4.0 * i, 1.0);
    }

    QCOMPARE(history.size(), 7);
}

void KisStrokeSampleHistoryContractTest::thePointCountIsCappedAt64()
{
    KisStrokeSampleHistory history(500.0, 24.0, 500.0);
    for (int i = 0; i < 100; ++i) {
        history.add(QPointF(i, 0.0), i, 1.0);
    }

    QCOMPARE(history.size(), 64);
}

QTEST_GUILESS_MAIN(KisStrokeSampleHistoryContractTest)

#include "KisStrokeSampleHistoryContractTest.moc"
