/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWraparoundAxis.h"

#include <QTest>

class KisWraparoundAxisContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void axisValuesPreserveStoredSettings();
};

void KisWraparoundAxisContractTest::axisValuesPreserveStoredSettings()
{
    QCOMPARE(int(WRAPAROUND_BOTH), 0);
    QCOMPARE(int(WRAPAROUND_HORIZONTAL), 1);
    QCOMPARE(int(WRAPAROUND_VERTICAL), 2);
}

QTEST_GUILESS_MAIN(KisWraparoundAxisContractTest)

#include "KisWraparoundAxisContractTest.moc"
