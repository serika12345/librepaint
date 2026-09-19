/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lazybrush/kis_lazy_fill_tools.h"

#include <QTest>

class KisLazyFillToolsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filteringOptionsValuesAndEqualityRemainStable();
};

void KisLazyFillToolsContractTest::filteringOptionsValuesAndEqualityRemainStable()
{
    using FilteringOptions = KisLazyFillTools::FilteringOptions;

    const FilteringOptions defaults;
    QCOMPARE(defaults.useEdgeDetection, false);
    QCOMPARE(defaults.edgeDetectionSize, 4.0);
    QCOMPARE(defaults.fuzzyRadius, 0.0);
    QCOMPARE(defaults.cleanUpAmount, 0.0);

    const FilteringOptions configured(true, 8.0, 2.0, 1.0);
    const FilteringOptions same(true, 8.0, 2.0, 1.0);
    QCOMPARE(configured.useEdgeDetection, true);
    QCOMPARE(configured.edgeDetectionSize, 8.0);
    QCOMPARE(configured.fuzzyRadius, 2.0);
    QCOMPARE(configured.cleanUpAmount, 1.0);
    QVERIFY(configured == same);

    FilteringOptions changed = same;
    changed.useEdgeDetection = false;
    QVERIFY(!(configured == changed));
    changed = same;
    changed.edgeDetectionSize = 9.0;
    QVERIFY(!(configured == changed));
    changed = same;
    changed.fuzzyRadius = 3.0;
    QVERIFY(!(configured == changed));
    changed = same;
    changed.cleanUpAmount = 2.0;
    QVERIFY(!(configured == changed));
}

QTEST_APPLESS_MAIN(KisLazyFillToolsContractTest)

#include "KisLazyFillToolsContractTest.moc"
