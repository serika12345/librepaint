/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ResourceListViewModes.h>

#include <QTest>

class ResourceListViewModesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valuesHaveStableIdentityAndOrder();
};

void ResourceListViewModesContractTest::valuesHaveStableIdentityAndOrder()
{
    QCOMPARE(static_cast<int>(ListViewMode::IconGrid), 0);
    QCOMPARE(static_cast<int>(ListViewMode::IconStripHorizontal), 1);
    QCOMPARE(static_cast<int>(ListViewMode::Detail), 2);

    QVERIFY(ListViewMode::IconGrid != ListViewMode::IconStripHorizontal);
    QVERIFY(ListViewMode::IconStripHorizontal != ListViewMode::Detail);
    QVERIFY(ListViewMode::IconGrid != ListViewMode::Detail);
}

QTEST_GUILESS_MAIN(ResourceListViewModesContractTest)

#include "ResourceListViewModesContractTest.moc"
