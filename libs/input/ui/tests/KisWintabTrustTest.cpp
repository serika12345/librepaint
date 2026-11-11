/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <QPointF>

#include <input/ui/wintab/KisWintabTrust.h>

class KisWintabTrustTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testUndecidedAllowsCorrection();
    void testNearPositionTrusts();
    void testFarPositionDistrusts();
    void testBoundaryIsTrusted();
    void testProximityChangeResets();
    void testFirstTabletEventStartsProximity();
};

void KisWintabTrustTest::testUndecidedAllowsCorrection()
{
    KisWintabTrust trust;

    QCOMPARE(trust.state(), KisWintabTrust::State::Undecided);
    QVERIFY(trust.correctionAllowed());
}

void KisWintabTrustTest::testNearPositionTrusts()
{
    KisWintabTrust trust;
    trust.beginProximity();

    // The driver's range maps 1.5 px left of the cursor.
    QCOMPARE(trust.decide(QPointF(100.0, 100.0), QPointF(101.5, 99.0)),
             KisWintabTrust::State::Trusted);
    QCOMPARE(trust.differencePixels(), 1.5);
    QVERIFY(trust.correctionAllowed());

    // The first press of a proximity decides once.
    QCOMPARE(trust.decide(QPointF(200.0, 200.0), QPointF(240.0, 240.0)),
             KisWintabTrust::State::Trusted);
    QCOMPARE(trust.differencePixels(), 1.5);
}

void KisWintabTrustTest::testFarPositionDistrusts()
{
    KisWintabTrust trust;
    trust.beginProximity();

    QCOMPARE(trust.decide(QPointF(100.0, 100.0), QPointF(100.0, 102.5)),
             KisWintabTrust::State::Distrusted);
    QCOMPARE(trust.differencePixels(), 2.5);
    QVERIFY(!trust.correctionAllowed());
}

void KisWintabTrustTest::testBoundaryIsTrusted()
{
    KisWintabTrust trust;
    trust.beginProximity();

    // 2.0 px and below is trusted; only a larger difference distrusts.
    QCOMPARE(trust.decide(QPointF(10.0, 10.0), QPointF(12.0, 8.0)),
             KisWintabTrust::State::Trusted);
    QVERIFY(trust.correctionAllowed());
}

void KisWintabTrustTest::testProximityChangeResets()
{
    KisWintabTrust trust;
    trust.beginProximity();
    QCOMPARE(trust.decide(QPointF(0.0, 0.0), QPointF(10.0, 0.0)),
             KisWintabTrust::State::Distrusted);

    trust.beginProximity();
    QCOMPARE(trust.state(), KisWintabTrust::State::Undecided);
    QVERIFY(trust.correctionAllowed());

    trust.endProximity();
    QCOMPARE(trust.state(), KisWintabTrust::State::Undecided);
    QVERIFY(trust.correctionAllowed());
}

void KisWintabTrustTest::testFirstTabletEventStartsProximity()
{
    KisWintabTrust trust;

    // Qt can miss the enter event: the first tablet event that arrives
    // without one starts the proximity and clears an old decision.
    trust.endProximity();
    QCOMPARE(trust.decide(QPointF(0.0, 0.0), QPointF(0.0, 10.0)),
             KisWintabTrust::State::Distrusted);

    trust.noteTabletEvent();
    QCOMPARE(trust.state(), KisWintabTrust::State::Undecided);
    QVERIFY(trust.correctionAllowed());
}

QTEST_GUILESS_MAIN(KisWintabTrustTest)

#include "KisWintabTrustTest.moc"
