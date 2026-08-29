/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "WidgetsDebug.h"

#include <QTest>

class WidgetsDebugContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void categoryHasStableIdentityAndDefaultSeverity();
};

void WidgetsDebugContractTest::categoryHasStableIdentityAndDefaultSeverity()
{
    QLoggingCategory::setFilterRules(QString());

    const QLoggingCategory &first = WIDGETS_LOG();
    const QLoggingCategory &second = WIDGETS_LOG();

    QCOMPARE(&first, &second);
    QCOMPARE(QString::fromLatin1(first.categoryName()), QStringLiteral("krita.lib.widgets"));
    QVERIFY(!first.isDebugEnabled());
    QVERIFY(first.isInfoEnabled());
    QVERIFY(first.isWarningEnabled());
    QVERIFY(first.isCriticalEnabled());
}

QTEST_GUILESS_MAIN(WidgetsDebugContractTest)

#include "WidgetsDebugContractTest.moc"
