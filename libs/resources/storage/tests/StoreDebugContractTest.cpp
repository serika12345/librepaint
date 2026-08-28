/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <StoreDebug.h>

#include <QTest>

class StoreDebugContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesCategoryAndDefaultSeverity();
};

void StoreDebugContractTest::preservesCategoryAndDefaultSeverity()
{
    QLoggingCategory::setFilterRules(QString());

    const QLoggingCategory &first = STORE_LOG();
    const QLoggingCategory &second = STORE_LOG();
    QCOMPARE(&first, &second);
    QCOMPARE(QString::fromLatin1(first.categoryName()), QStringLiteral("krita.lib.store"));
    QVERIFY(!first.isDebugEnabled());
    QVERIFY(first.isInfoEnabled());
    QVERIFY(first.isWarningEnabled());
    QVERIFY(first.isCriticalEnabled());
}

QTEST_GUILESS_MAIN(StoreDebugContractTest)

#include "StoreDebugContractTest.moc"
