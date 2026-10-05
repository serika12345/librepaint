/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QTest>
#include "../canvas/kis_canvas_performance_measurement_p.h"
#include "CanvasFrameReturnObservation.h"

using namespace Krita::Canvas::Performance;

class KisCanvasPerformanceMeasurementTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void endpointUsesFirstFrameContainingTheFinalProjection()
    {
        QObject owner;
        owner.setProperty(enabledProperty, true);
        CanvasFrameReturnObservation observed(owner, "submit");
        observed.reset(7);
        record(owner, QStringLiteral("submit"), 3, 7, 120);
        record(owner, QStringLiteral("projection"), 4, 7, 150);
        record(owner, QStringLiteral("submit"), 5, 7, 170);
        record(owner, QStringLiteral("submit"), 6, 7, 190);
        QCOMPARE(observed.firstReturnAtOrAfter(150), 170);
        QCOMPARE(observed.firstReturnAtOrAfter(200), 0);
        // A frame submitted after projection but before the resulting upload
        // still contains the previous image and cannot be the GPU endpoint.
        CanvasFrameReturnObservation uploads(owner, "upload");
        uploads.reset(7);
        record(owner, QStringLiteral("upload"), 2, 7, 180);
        QCOMPARE(observed.firstReturnAtOrAfter(uploads.firstReturnAtOrAfter(150)), 190);
        observed.reset(8);
        record(owner, QStringLiteral("submit"), 7, 7, 210);
        QCOMPARE(observed.firstReturnAtOrAfter(200), 0);
        record(owner, QStringLiteral("submit"), 8, 8, 230);
        QCOMPARE(observed.firstReturnAtOrAfter(200), 230);
    }
    void aggregatesStagesForTheInputWithoutMixingOwners()
    {
        QObject first, second;
        first.setProperty(enabledProperty, true);
        second.setProperty(enabledProperty, true);
        record(first, QStringLiteral("compose"), 12, 7, 100);
        record(first, QStringLiteral("upload"), 4, 7, 105);
        record(first, QStringLiteral("compose"), 20, 8, 130);
        record(second, QStringLiteral("compose"), 3, 1, 200);
        const auto samples = first.property(samplesProperty).toMap();
        const auto compose = samples.value(QStringLiteral("compose")).toMap();
        QCOMPARE(compose.value(QStringLiteral("count")).toLongLong(), 2);
        QCOMPARE(compose.value(QStringLiteral("total_ns")).toLongLong(), 32);
        QCOMPARE(compose.value(QStringLiteral("last_ns")).toLongLong(), 20);
        QCOMPARE(compose.value(QStringLiteral("input")).toLongLong(), 8);
        QCOMPARE(compose.value(QStringLiteral("finish_ns")).toLongLong(), 130);
        QCOMPARE(samples.value(QStringLiteral("upload")).toMap().value(QStringLiteral("total_ns")).toLongLong(), 4);
        QCOMPARE(second.property(samplesProperty).toMap().value(QStringLiteral("compose")).toMap().value(QStringLiteral("total_ns")).toLongLong(), 3);
    }
    void ordinaryCanvasProducesNoMeasurement()
    {
        QObject owner;
        record(owner, QStringLiteral("compose"), 12, 7, 100);
        { Measurement measure(owner, "compose"); }
        QVERIFY(!owner.property(samplesProperty).isValid());
    }
    void scopeRecordsCpuReturnForTheCurrentInput()
    {
        QObject owner;
        owner.setProperty(enabledProperty, true);
        owner.setProperty(inputProperty, 9);
        const auto before = nowNs();
        { Measurement measure(owner, "submit"); }
        const auto sample = owner.property(samplesProperty).toMap().value(QStringLiteral("submit")).toMap();
        QCOMPARE(sample.value(QStringLiteral("input")).toLongLong(), 9);
        QVERIFY(sample.value(QStringLiteral("finish_ns")).toLongLong() >= before);
        QVERIFY(sample.value(QStringLiteral("finish_ns")).toLongLong() <= nowNs());
    }
};
QTEST_GUILESS_MAIN(KisCanvasPerformanceMeasurementTest)
#include "KisCanvasPerformanceMeasurementTest.moc"
