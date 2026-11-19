/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStrokePrediction.h"
#include "KisStrokePredictionCap.h"

#include <QPointF>
#include <QTest>
#include <QVector>

#include <cmath>

namespace
{
constexpr qreal Tolerance = 1e-9;

void verifyClose(qreal actual, qreal expected)
{
    QVERIFY2(std::abs(actual - expected) <= Tolerance,
             qPrintable(QStringLiteral("%1 is not %2").arg(actual).arg(expected)));
}

void verifySame(const KisStrokePredictionResult &actual, const KisStrokePredictionResult &expected)
{
    QCOMPARE(actual.valid, expected.valid);
    QCOMPARE(actual.latestIndex, expected.latestIndex);
    QCOMPARE(actual.polyline.size(), expected.polyline.size());
    for (int i = 0; i < expected.polyline.size(); ++i) {
        verifyClose(actual.polyline.at(i).x(), expected.polyline.at(i).x());
        verifyClose(actual.polyline.at(i).y(), expected.polyline.at(i).y());
    }
    verifyClose(actual.velocity.x(), expected.velocity.x());
    verifyClose(actual.velocity.y(), expected.velocity.y());
    verifyClose(actual.speed, expected.speed);
    verifyClose(actual.confidence, expected.confidence);
    verifyClose(actual.moveLength, expected.moveLength);
}

KisStrokePredictionResult threePredictions()
{
    KisStrokePredictionResult result;
    result.valid = true;
    result.polyline = {QPointF(0.0, 0.0), QPointF(0.0, 0.0), QPointF(10.0, 0.0),
                       QPointF(10.0, 10.0), QPointF(10.0, 40.0)};
    result.latestIndex = 1;
    result.moveLength = 50.0;
    return result;
}

void addFastStroke(KisStrokePrediction &predictor)
{
    for (int i = 0; i < 8; ++i) {
        const qreal time = 4.0 * i;
        predictor.addSample(QPointF(4.0 * time, 0.0), time, 1.0);
    }
}
} // namespace

class KisStrokePredictionCapContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void singlePredictionMatchesTheOldCap();
    void threePredictionsAreCutAtTheCap();
    void aCapAtOrBelowZeroChangesNothing();
    void anInvalidResultChangesNothing();
    void aMoveBelowTheCapChangesNothing();
};

void KisStrokePredictionCapContractTest::singlePredictionMatchesTheOldCap()
{
    KisStrokePredictionSettings settings;
    settings.maxMovePx = 30.0;

    KisStrokePrediction capped;
    capped.setSettings(settings);
    addFastStroke(capped);

    const KisStrokePredictionResult expected = capped.predict();
    QVERIFY(expected.valid);
    verifyClose(expected.moveLength, 30.0);

    settings.maxMovePx = 0.0;
    KisStrokePrediction uncapped;
    uncapped.setSettings(settings);
    addFastStroke(uncapped);

    KisStrokePredictionResult actual = uncapped.predict();
    QVERIFY(actual.valid);
    verifyClose(actual.moveLength, 4.0 * 16.7);
    KisStrokePredictionCap::apply(actual, 30.0);

    verifySame(actual, expected);
}

void KisStrokePredictionCapContractTest::threePredictionsAreCutAtTheCap()
{
    KisStrokePredictionResult result = threePredictions();

    KisStrokePredictionCap::apply(result, 25.0);

    QVERIFY(result.valid);
    QCOMPARE(result.latestIndex, 1);
    QCOMPARE(result.polyline.size(), 5);
    verifyClose(result.moveLength, 25.0);
    verifyClose(result.polyline.at(2).x(), 10.0);
    verifyClose(result.polyline.at(2).y(), 0.0);
    verifyClose(result.polyline.at(3).x(), 10.0);
    verifyClose(result.polyline.at(3).y(), 10.0);
    verifyClose(result.polyline.constLast().x(), 10.0);
    verifyClose(result.polyline.constLast().y(), 15.0);
}

void KisStrokePredictionCapContractTest::aCapAtOrBelowZeroChangesNothing()
{
    const KisStrokePredictionResult original = threePredictions();

    KisStrokePredictionResult zero = original;
    KisStrokePredictionCap::apply(zero, 0.0);
    verifySame(zero, original);

    KisStrokePredictionResult negative = original;
    KisStrokePredictionCap::apply(negative, -5.0);
    verifySame(negative, original);
}

void KisStrokePredictionCapContractTest::anInvalidResultChangesNothing()
{
    KisStrokePredictionResult original = threePredictions();
    original.valid = false;

    KisStrokePredictionResult result = original;
    KisStrokePredictionCap::apply(result, 5.0);

    verifySame(result, original);
}

void KisStrokePredictionCapContractTest::aMoveBelowTheCapChangesNothing()
{
    const KisStrokePredictionResult original = threePredictions();

    KisStrokePredictionResult result = original;
    KisStrokePredictionCap::apply(result, 50.0);
    verifySame(result, original);

    KisStrokePredictionResult longer = original;
    KisStrokePredictionCap::apply(longer, 80.0);
    verifySame(longer, original);
}

QTEST_GUILESS_MAIN(KisStrokePredictionCapContractTest)

#include "KisStrokePredictionCapContractTest.moc"
