/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_timing_information.h"

#include <QTest>

class KisTimingInformationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultTimingDisablesTimedSpacing();
    void intervalTimingPreservesRequestedDelay();
};

void KisTimingInformationContractTest::defaultTimingDisablesTimedSpacing()
{
    const KisTimingInformation timing;

    QVERIFY(!timing.isTimedSpacingEnabled());
    QCOMPARE(timing.timedSpacingInterval(), LONG_TIME);
    QCOMPARE(LONG_TIME, 320000000000.0);
}

void KisTimingInformationContractTest::intervalTimingPreservesRequestedDelay()
{
    const KisTimingInformation timing(12.5);

    QVERIFY(timing.isTimedSpacingEnabled());
    QCOMPARE(timing.timedSpacingInterval(), 12.5);
}

QTEST_GUILESS_MAIN(KisTimingInformationContractTest)

#include "KisTimingInformationContractTest.moc"
