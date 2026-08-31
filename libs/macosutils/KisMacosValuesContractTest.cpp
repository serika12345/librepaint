/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMacosEntitlements.h"
#include "KisMacosSecurityBookmarkManager.h"

#include <QTest>

class KisMacosValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void entitlementValuesRemainStable();
    void securityBookmarkTypeValuesRemainStable();
};

void KisMacosValuesContractTest::entitlementValuesRemainStable()
{
    QCOMPARE(int(KisMacosEntitlements::Sandbox), 0);
    QCOMPARE(int(KisMacosEntitlements::BookmarkScopeApp), 1);
    QCOMPARE(int(KisMacosEntitlements::BookmarkScopeDocument), 2);
    QCOMPARE(int(KisMacosEntitlements::Other), 99);
}

void KisMacosValuesContractTest::securityBookmarkTypeValuesRemainStable()
{
    QCOMPARE(int(KisMacosSecurityBookmarkManager::File), 0);
    QCOMPARE(int(KisMacosSecurityBookmarkManager::Directory), 1);
    QVERIFY(KisMacosSecurityBookmarkManager::File != KisMacosSecurityBookmarkManager::Directory);
}

QTEST_GUILESS_MAIN(KisMacosValuesContractTest)

#include "KisMacosValuesContractTest.moc"
