/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSelectionTags.h"

#include <QTest>

class KisSelectionTagsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void selectionModesPreserveValues();
    void selectionActionsPreserveValues();
};

void KisSelectionTagsContractTest::selectionModesPreserveValues()
{
    QCOMPARE(static_cast<int>(PIXEL_SELECTION), 0);
    QCOMPARE(static_cast<int>(SHAPE_PROTECTION), 1);
}

void KisSelectionTagsContractTest::selectionActionsPreserveValues()
{
    QCOMPARE(static_cast<int>(SELECTION_REPLACE), 0);
    QCOMPARE(static_cast<int>(SELECTION_ADD), 1);
    QCOMPARE(static_cast<int>(SELECTION_SUBTRACT), 2);
    QCOMPARE(static_cast<int>(SELECTION_INTERSECT), 3);
    QCOMPARE(static_cast<int>(SELECTION_SYMMETRICDIFFERENCE), 4);
    QCOMPARE(static_cast<int>(SELECTION_DEFAULT), 5);
}

QTEST_GUILESS_MAIN(KisSelectionTagsContractTest)

#include "KisSelectionTagsContractTest.moc"
