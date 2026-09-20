/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisNodeAdditionFlags.h>

#include <QTest>

class KisNodeAdditionFlagsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valuesAndCombination();
};

void KisNodeAdditionFlagsTest::valuesAndCombination()
{
    QCOMPARE(static_cast<int>(KisNodeAdditionFlag::None), 0x0);
    QCOMPARE(static_cast<int>(KisNodeAdditionFlag::DontActivateNode), 0x1);

    KisNodeAdditionFlags flags;
    QVERIFY(!flags.testFlag(KisNodeAdditionFlag::DontActivateNode));

    flags |= KisNodeAdditionFlag::DontActivateNode;
    QVERIFY(flags.testFlag(KisNodeAdditionFlag::DontActivateNode));
    QCOMPARE(flags, KisNodeAdditionFlags(KisNodeAdditionFlag::DontActivateNode));
}

QTEST_GUILESS_MAIN(KisNodeAdditionFlagsTest)

#include "KisNodeAdditionFlagsTest.moc"
