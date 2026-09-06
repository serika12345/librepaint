/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMacosEntitlements.h"
#include "KisMacosSecurityBookmarkManager.h"
#include "KisMacosSystemProber.h"

#include <QTest>

#include <type_traits>

class KisMacosValuesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void entitlementValuesRemainStable();
    void securityBookmarkTypeValuesRemainStable();
    void entitlementsTypeLifetimeAndConstructionSchemaRemainStable();
    void entitlementsQuerySchemaRemainsStable();
    void securityBookmarkTypeLifetimeAndConstructionSchemaRemainStable();
    void securityBookmarkAccessSchemaRemainsStable();
    void systemProberSchemaRemainsStable();
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

void KisMacosValuesContractTest::entitlementsTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMacosEntitlements>);
    static_assert(std::is_default_constructible_v<KisMacosEntitlements>);
    static_assert(std::has_virtual_destructor_v<KisMacosEntitlements>);
}

void KisMacosValuesContractTest::entitlementsQuerySchemaRemainsStable()
{
    using LoadSignature = void (KisMacosEntitlements::*)();
    using HasEntitlementSignature = bool (KisMacosEntitlements::*)(KisMacosEntitlements::Entitlements);
    using SandboxSignature = bool (KisMacosEntitlements::*)();

    static_assert(std::is_same_v<decltype(&KisMacosEntitlements::loadAvailableEntitlements), LoadSignature>);
    static_assert(std::is_same_v<decltype(&KisMacosEntitlements::hasEntitlement), HasEntitlementSignature>);
    static_assert(std::is_same_v<decltype(&KisMacosEntitlements::sandbox), SandboxSignature>);
}

void KisMacosValuesContractTest::securityBookmarkTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMacosSecurityBookmarkManager>);
    static_assert(std::is_default_constructible_v<KisMacosSecurityBookmarkManager>);
    static_assert(std::has_virtual_destructor_v<KisMacosSecurityBookmarkManager>);
}

void KisMacosValuesContractTest::securityBookmarkAccessSchemaRemainsStable()
{
    using PathQuerySignature = bool (KisMacosSecurityBookmarkManager::*)(const QString &);
    using PathActionSignature = void (KisMacosSecurityBookmarkManager::*)(const QString &);
    using UrlActionSignature = void (KisMacosSecurityBookmarkManager::*)(const QUrl &);
    using CreateBookmarkSignature =
        void (KisMacosSecurityBookmarkManager::*)(const QString &,
                                                  const QString &,
                                                  KisMacosSecurityBookmarkManager::SecurityBookmarkType);
    using ActionSignature = void (KisMacosSecurityBookmarkManager::*)();
    using SandboxQuerySignature = bool (KisMacosSecurityBookmarkManager::*)();

    static_assert(
        std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::parentDirHasPermissions), PathQuerySignature>);
    static_assert(std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::requestAccessToDir), PathQuerySignature>);
    static_assert(std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::slotCreateBookmark), PathActionSignature>);
    static_assert(
        std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::addBookmarkAndCheckParentDir), UrlActionSignature>);
    static_assert(
        std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::createBookmarkFromPath), CreateBookmarkSignature>);
    static_assert(std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::startAccessingSecurityScopedResources),
                                 ActionSignature>);
    static_assert(std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::stopAccessingSecurityScopedResources),
                                 ActionSignature>);
    static_assert(
        std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::loadSecurityScopedResources), ActionSignature>);
    static_assert(std::is_same_v<decltype(&KisMacosSecurityBookmarkManager::isSandboxed), SandboxQuerySignature>);
}

void KisMacosValuesContractTest::systemProberSchemaRemainsStable()
{
    using SystemProberSignature = bool (*)();

    static_assert(std::is_same_v<decltype(&iskritaRunningActivate), SystemProberSignature>);
}

QTEST_GUILESS_MAIN(KisMacosValuesContractTest)

#include "KisMacosValuesContractTest.moc"
