/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_speed_smoother.h"

#include <QPointF>
#include <QTest>

namespace
{

bool safeAssertTriggered = false;

} // namespace

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    safeAssertTriggered = true;
}

class KisSpeedSmootherContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void eventTimestampsProduceDeterministicSpeeds();
    void clearResetsReportedSpeed();
};

void KisSpeedSmootherContractTest::eventTimestampsProduceDeterministicSpeeds()
{
    safeAssertTriggered = false;
    KisSpeedSmoother smoother;
    smoother.setSettings(true, 1);
    smoother.clear();

    QCOMPARE(smoother.lastSpeed(), 0.0);
    QCOMPARE(smoother.getNextSpeed(QPointF(10.0, 10.0), 100), 0.0);
    QCOMPARE(smoother.getNextSpeed(QPointF(20.0, 10.0), 110), 0.5);
    QCOMPARE(smoother.lastSpeed(), 0.5);
    QCOMPARE(smoother.getNextSpeed(QPointF(20.0, 10.0), 120), 0.5);
    QCOMPARE(smoother.lastSpeed(), 0.5);
    QCOMPARE(smoother.getNextSpeed(QPointF(30.0, 10.0), 120), 1.0);
    QCOMPARE(smoother.lastSpeed(), 1.0);
    QVERIFY(!safeAssertTriggered);
}

void KisSpeedSmootherContractTest::clearResetsReportedSpeed()
{
    safeAssertTriggered = false;
    KisSpeedSmoother smoother;
    smoother.setSettings(true, 1);
    smoother.clear();
    smoother.getNextSpeed(QPointF(10.0, 10.0), 100);
    smoother.getNextSpeed(QPointF(20.0, 10.0), 110);
    QCOMPARE(smoother.lastSpeed(), 0.5);

    smoother.clear();

    QCOMPARE(smoother.lastSpeed(), 0.0);
    QCOMPARE(smoother.getNextSpeed(QPointF(30.0, 40.0), 200), 0.0);
    QVERIFY(!safeAssertTriggered);
}

QTEST_GUILESS_MAIN(KisSpeedSmootherContractTest)

#include "KisSpeedSmootherContractTest.moc"
