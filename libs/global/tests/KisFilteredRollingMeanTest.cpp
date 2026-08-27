/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisFilteredRollingMean.h>
#include <kis_assert.h>

#include <QTest>

namespace
{

int safeAssertCount = 0;

}

void kis_safe_assert_recoverable(const char *, const char *, int)
{
    safeAssertCount++;
}

class KisFilteredRollingMeanTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void emptyStateAndUnfilteredMean();
    void filtersExtremesAndRollsWindow();
};

void KisFilteredRollingMeanTest::init()
{
    safeAssertCount = 0;
}

void KisFilteredRollingMeanTest::emptyStateAndUnfilteredMean()
{
    KisFilteredRollingMean mean(4, 1.0);
    QVERIFY(mean.isEmpty());

    mean.addValue(2.0);
    mean.addValue(4.0);

    QVERIFY(!mean.isEmpty());
    QCOMPARE(mean.filteredMean(), 3.0);
    QCOMPARE(safeAssertCount, 0);
}

void KisFilteredRollingMeanTest::filtersExtremesAndRollsWindow()
{
    KisFilteredRollingMean mean(5, 0.6);
    mean.addValue(1.0);
    mean.addValue(2.0);
    mean.addValue(3.0);
    mean.addValue(4.0);
    mean.addValue(100.0);

    QCOMPARE(mean.filteredMean(), 3.0);

    mean.addValue(5.0);

    QCOMPARE(mean.filteredMean(), 4.0);
    QCOMPARE(safeAssertCount, 0);
}

QTEST_GUILESS_MAIN(KisFilteredRollingMeanTest)

#include "KisFilteredRollingMeanTest.moc"
