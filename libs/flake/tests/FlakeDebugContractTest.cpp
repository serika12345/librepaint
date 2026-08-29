/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FlakeDebug.h"

#include <QTest>

class FlakeDebugContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void categoryHasStableIdentityAndDefaultSeverity();
};

void FlakeDebugContractTest::categoryHasStableIdentityAndDefaultSeverity()
{
    QLoggingCategory::setFilterRules(QString());

    const QLoggingCategory &first = FLAKE_LOG();
    const QLoggingCategory &second = FLAKE_LOG();

    QCOMPARE(&first, &second);
    QCOMPARE(QString::fromLatin1(first.categoryName()), QStringLiteral("krita.lib.flake"));
    QVERIFY(!first.isDebugEnabled());
    QVERIFY(first.isInfoEnabled());
    QVERIFY(first.isWarningEnabled());
    QVERIFY(first.isCriticalEnabled());
}

QTEST_GUILESS_MAIN(FlakeDebugContractTest)

#include "FlakeDebugContractTest.moc"
