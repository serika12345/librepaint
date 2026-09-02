/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceManager_p.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_RESOURCE_MANAGER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoResourceManager::method)), signature>)
} // namespace

class KoResourceManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceManagerTypeAndLifetimeSchemaRemainsStable();
    void resourceManagerTypedValueSignaturesRemainStable();
    void resourceManagerDerivedAndMediatorSignaturesRemainStable();
    void resourceManagerDependencyAndAbstractSignaturesRemainStable();
    void resourceManagerNotificationSignaturesRemainStable();
};

void KoResourceManagerSchemaContractTest::resourceManagerTypeAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoResourceManager>);
    static_assert(std::is_base_of_v<QObject, KoResourceManager>);
    static_assert(std::is_default_constructible_v<KoResourceManager>);
}

void KoResourceManagerSchemaContractTest::resourceManagerTypedValueSignaturesRemainStable()
{
    ASSERT_RESOURCE_MANAGER_SIGNATURE(setResource, void (KoResourceManager::*)(int, const QVariant &));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(setResource, void (KoResourceManager::*)(int, const KoColor &));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(setResource, void (KoResourceManager::*)(int, KoShape *));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(setResource, void (KoResourceManager::*)(int, const KoUnit &));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(resource, QVariant (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(boolResource, bool (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(intResource, int (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(koColorResource, KoColor (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(koShapeResource, KoShape * (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(stringResource, QString (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(sizeResource, QSizeF (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(unitResource, KoUnit (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(hasResource, bool (KoResourceManager::*)(int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(clearResource, void (KoResourceManager::*)(int));
}

void KoResourceManagerSchemaContractTest::resourceManagerDerivedAndMediatorSignaturesRemainStable()
{
    ASSERT_RESOURCE_MANAGER_SIGNATURE(addDerivedResourceConverter,
                                      void (KoResourceManager::*)(KoDerivedResourceConverterSP));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(hasDerivedResourceConverter, bool (KoResourceManager::*)(int));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(removeDerivedResourceConverter, void (KoResourceManager::*)(int));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(addResourceUpdateMediator,
                                      void (KoResourceManager::*)(KoResourceUpdateMediatorSP));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(hasResourceUpdateMediator, bool (KoResourceManager::*)(int));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(removeResourceUpdateMediator, void (KoResourceManager::*)(int));
}

void KoResourceManagerSchemaContractTest::resourceManagerDependencyAndAbstractSignaturesRemainStable()
{
    ASSERT_RESOURCE_MANAGER_SIGNATURE(addActiveCanvasResourceDependency,
                                      void (KoResourceManager::*)(KoActiveCanvasResourceDependencySP));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(hasActiveCanvasResourceDependency, bool (KoResourceManager::*)(int, int) const);
    ASSERT_RESOURCE_MANAGER_SIGNATURE(removeActiveCanvasResourceDependency, void (KoResourceManager::*)(int, int));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(hasAbstractResource, bool (KoResourceManager::*)(int));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(removeAbstractResource, void (KoResourceManager::*)(int));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(setAbstractResource,
                                      void (KoResourceManager::*)(KoAbstractCanvasResourceInterfaceSP));
}

void KoResourceManagerSchemaContractTest::resourceManagerNotificationSignaturesRemainStable()
{
    ASSERT_RESOURCE_MANAGER_SIGNATURE(resourceChanged, void (KoResourceManager::*)(int, const QVariant &));
    ASSERT_RESOURCE_MANAGER_SIGNATURE(resourceChangeAttempted, void (KoResourceManager::*)(int, const QVariant &));
}

QTEST_GUILESS_MAIN(KoResourceManagerSchemaContractTest)

#include "KoResourceManagerSchemaContractTest.moc"
