/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTimedSignalThreshold.h>

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

class KisTimedSignalThresholdContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesQObjectLifetime();
    void forceDoneEmitsAndResetsThePeriod();
    void stableStartsEmitAfterTheDelay();
    void stopCancelsTheCurrentPeriod();
    void disabledThresholdIgnoresStarts();
    void updatedThresholdsTakeEffectForTheCurrentPeriod();
    void longGapCancelsTheCurrentPeriod();
};

void KisTimedSignalThresholdContractTest::preservesQObjectLifetime()
{
    QObject parent;
    auto *threshold = new KisTimedSignalThreshold(10, 20, &parent);
    QPointer<KisTimedSignalThreshold> guard(threshold);

    QCOMPARE(threshold->parent(), &parent);

    delete threshold;

    QVERIFY(guard.isNull());
}

void KisTimedSignalThresholdContractTest::forceDoneEmitsAndResetsThePeriod()
{
    KisTimedSignalThreshold threshold(60000);
    QSignalSpy timeoutSpy(&threshold, &KisTimedSignalThreshold::timeout);

    threshold.start();
    threshold.forceDone();

    QCOMPARE(timeoutSpy.count(), 1);

    threshold.start();
    QCOMPARE(timeoutSpy.count(), 1);

    threshold.forceDone();
    QCOMPARE(timeoutSpy.count(), 2);
}

void KisTimedSignalThresholdContractTest::stableStartsEmitAfterTheDelay()
{
    KisTimedSignalThreshold threshold(1, 60000);
    QSignalSpy timeoutSpy(&threshold, &KisTimedSignalThreshold::timeout);

    threshold.start();
    QTest::qWait(5);
    threshold.start();

    QCOMPARE(timeoutSpy.count(), 1);

    threshold.start();
    QCOMPARE(timeoutSpy.count(), 1);
}

void KisTimedSignalThresholdContractTest::stopCancelsTheCurrentPeriod()
{
    KisTimedSignalThreshold threshold(1, 60000);
    QSignalSpy timeoutSpy(&threshold, &KisTimedSignalThreshold::timeout);

    threshold.start();
    QTest::qWait(5);
    threshold.stop();
    threshold.start();

    QCOMPARE(timeoutSpy.count(), 0);

    QTest::qWait(5);
    threshold.start();
    QCOMPARE(timeoutSpy.count(), 1);
}

void KisTimedSignalThresholdContractTest::disabledThresholdIgnoresStarts()
{
    KisTimedSignalThreshold threshold(1, 60000);
    QSignalSpy timeoutSpy(&threshold, &KisTimedSignalThreshold::timeout);

    threshold.start();
    QTest::qWait(5);
    threshold.setEnabled(false);
    threshold.start();

    QCOMPARE(timeoutSpy.count(), 0);

    threshold.setEnabled(true);
    threshold.start();
    QTest::qWait(5);
    threshold.start();

    QCOMPARE(timeoutSpy.count(), 1);
}

void KisTimedSignalThresholdContractTest::updatedThresholdsTakeEffectForTheCurrentPeriod()
{
    KisTimedSignalThreshold threshold(60000, 60000);
    QSignalSpy timeoutSpy(&threshold, &KisTimedSignalThreshold::timeout);

    threshold.start();
    QTest::qWait(5);
    threshold.setDelayThreshold(1, 60000);
    threshold.start();

    QCOMPARE(timeoutSpy.count(), 1);
}

void KisTimedSignalThresholdContractTest::longGapCancelsTheCurrentPeriod()
{
    KisTimedSignalThreshold threshold(60000, 1);
    QSignalSpy timeoutSpy(&threshold, &KisTimedSignalThreshold::timeout);

    threshold.start();
    QTest::qWait(5);
    threshold.start();

    QCOMPARE(timeoutSpy.count(), 0);

    threshold.start();
    QCOMPARE(timeoutSpy.count(), 0);
}

QTEST_GUILESS_MAIN(KisTimedSignalThresholdContractTest)

#include "KisTimedSignalThresholdContractTest.moc"
