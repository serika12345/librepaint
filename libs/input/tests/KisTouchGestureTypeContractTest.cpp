/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisTouchGestureType.h>

#include <QTest>

class KisTouchGestureTypeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valuesRemainStable();
};

void KisTouchGestureTypeContractTest::valuesRemainStable()
{
    QCOMPARE(static_cast<int>(KisTouchGestureType::Unsupported), 0);
    QCOMPARE(static_cast<int>(KisTouchGestureType::Tap), 1);
    QCOMPARE(static_cast<int>(KisTouchGestureType::Drag), 2);
    QCOMPARE(static_cast<int>(KisTouchGestureType::Hold), 3);
}

QTEST_GUILESS_MAIN(KisTouchGestureTypeContractTest)

#include "KisTouchGestureTypeContractTest.moc"
