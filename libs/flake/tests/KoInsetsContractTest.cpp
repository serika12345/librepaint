/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoInsets.h"

#include <QTest>

class KoInsetsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultAndClearPreserveZeroSides();
    void constructionPreservesAllSides();
    void debugStreamPreservesSideOrder();
};

void KoInsetsContractTest::defaultAndClearPreserveZeroSides()
{
    KoInsets insets;
    QCOMPARE(insets.top, 0.0);
    QCOMPARE(insets.left, 0.0);
    QCOMPARE(insets.bottom, 0.0);
    QCOMPARE(insets.right, 0.0);

    insets.top = 1.0;
    insets.left = 2.0;
    insets.bottom = 3.0;
    insets.right = 4.0;
    insets.clear();

    QCOMPARE(insets.top, 0.0);
    QCOMPARE(insets.left, 0.0);
    QCOMPARE(insets.bottom, 0.0);
    QCOMPARE(insets.right, 0.0);
}

void KoInsetsContractTest::constructionPreservesAllSides()
{
    const KoInsets insets(1.25, -2.5, 3.75, 4.5);

    QCOMPARE(insets.top, 1.25);
    QCOMPARE(insets.left, -2.5);
    QCOMPARE(insets.bottom, 3.75);
    QCOMPARE(insets.right, 4.5);
}

void KoInsetsContractTest::debugStreamPreservesSideOrder()
{
    QString output;
    {
        QDebug debug(&output);
        debug.nospace() << KoInsets(1.25, -2.5, 3.75, 4.5);
    }

#ifndef NDEBUG
    QCOMPARE(output, QStringLiteral("KoInsets [top=1.25, left=-2.5, bottom=3.75, right=4.5]"));
#else
    QCOMPARE(output, QStringLiteral(" "));
#endif
}

QTEST_GUILESS_MAIN(KoInsetsContractTest)

#include "KoInsetsContractTest.moc"
