/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoColorSpaceConstants.h"

#include <QTest>

class KoColorSpaceConstantsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void integerOpacityUsesByteEndpoints();
    void floatingOpacityUsesUnitInterval();
};

void KoColorSpaceConstantsContractTest::integerOpacityUsesByteEndpoints()
{
    QCOMPARE(OPACITY_TRANSPARENT_U8, quint8(0));
    QCOMPARE(OPACITY_OPAQUE_U8, quint8(255));
}

void KoColorSpaceConstantsContractTest::floatingOpacityUsesUnitInterval()
{
    QCOMPARE(OPACITY_TRANSPARENT_F, qreal(0.0));
    QCOMPARE(OPACITY_OPAQUE_F, qreal(1.0));
}

QTEST_GUILESS_MAIN(KoColorSpaceConstantsContractTest)

#include "KoColorSpaceConstantsContractTest.moc"
