/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DebugPigment.h"

#include <QTest>

class DebugPigmentContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void categoryHasStableIdentityNameAndSeverity();
};

void DebugPigmentContractTest::categoryHasStableIdentityNameAndSeverity()
{
    QLoggingCategory::setFilterRules(QString());

    const QLoggingCategory &first = PIGMENT_log();
    const QLoggingCategory &second = PIGMENT_log();

    QCOMPARE(&first, &second);
    QCOMPARE(QString::fromLatin1(first.categoryName()), QStringLiteral("krita.lib.pigment"));
    QVERIFY(!first.isDebugEnabled());
    QVERIFY(first.isInfoEnabled());
    QVERIFY(first.isWarningEnabled());
    QVERIFY(first.isCriticalEnabled());
}

QTEST_GUILESS_MAIN(DebugPigmentContractTest)

#include "DebugPigmentContractTest.moc"
