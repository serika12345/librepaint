/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <kis_action_registry.h>

#include <type_traits>
#include <utility>

#define ASSERT_ACTION_REGISTRY_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisActionRegistry::method)), signature>)

class KisActionRegistrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionRegistryCategorySchemaRemainsStable();
    void actionRegistryIdentityAndLifetimeSchemaRemainsStable();
    void actionRegistryLookupAndFactorySchemaRemainsStable();
    void actionRegistryShortcutConfigurationSchemaRemainsStable();
    void actionRegistryMaintenanceAndNotificationSchemaRemainsStable();
};

void KisActionRegistrySchemaContractTest::actionRegistryCategorySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisActionRegistry>);
    static_assert(std::is_class_v<KisActionRegistry::ActionCategory>);
    static_assert(std::is_default_constructible_v<KisActionRegistry::ActionCategory>);
    static_assert(std::is_constructible_v<KisActionRegistry::ActionCategory, const QString &, const QString &>);
    static_assert(std::is_same_v<decltype(KisActionRegistry::ActionCategory::componentName), QString>);
    static_assert(std::is_same_v<decltype(KisActionRegistry::ActionCategory::categoryName), QString>);
    static_assert(std::is_same_v<decltype(&KisActionRegistry::ActionCategory::isValid),
                                 bool (KisActionRegistry::ActionCategory::*)() const>);
}

void KisActionRegistrySchemaContractTest::actionRegistryIdentityAndLifetimeSchemaRemainsStable()
{
    static_assert(std::is_default_constructible_v<KisActionRegistry>);
    static_assert(std::has_virtual_destructor_v<KisActionRegistry>);
    ASSERT_ACTION_REGISTRY_SIGNATURE(instance, KisActionRegistry * (*)());
}

void KisActionRegistrySchemaContractTest::actionRegistryLookupAndFactorySchemaRemainsStable()
{
    ASSERT_ACTION_REGISTRY_SIGNATURE(hasAction, bool (KisActionRegistry::*)(const QString &) const);
    ASSERT_ACTION_REGISTRY_SIGNATURE(getActionProperty,
                                     QString (KisActionRegistry::*)(const QString &, const QString &));
    ASSERT_ACTION_REGISTRY_SIGNATURE(makeQAction, QAction * (KisActionRegistry::*)(const QString &, QObject *));
    static_assert(
        std::is_same_v<decltype(std::declval<KisActionRegistry &>().makeQAction(std::declval<const QString &>())),
                       QAction *>);
    ASSERT_ACTION_REGISTRY_SIGNATURE(propertizeAction, bool (KisActionRegistry::*)(const QString &, QAction *));
    ASSERT_ACTION_REGISTRY_SIGNATURE(fetchActionCategory,
                                     KisActionRegistry::ActionCategory (KisActionRegistry::*)(const QString &) const);
    ASSERT_ACTION_REGISTRY_SIGNATURE(registeredShortcutIds, QList<QString> (KisActionRegistry::*)() const);
}

void KisActionRegistrySchemaContractTest::actionRegistryShortcutConfigurationSchemaRemainsStable()
{
    ASSERT_ACTION_REGISTRY_SIGNATURE(settingsPageSaved, void (KisActionRegistry::*)());
    ASSERT_ACTION_REGISTRY_SIGNATURE(loadCustomShortcuts, void (KisActionRegistry::*)());
    ASSERT_ACTION_REGISTRY_SIGNATURE(notifySettingsUpdated, void (KisActionRegistry::*)());
    ASSERT_ACTION_REGISTRY_SIGNATURE(applyShortcutScheme, void (KisActionRegistry::*)(const KConfigBase *));
    static_assert(std::is_same_v<decltype(std::declval<KisActionRegistry &>().applyShortcutScheme()), void>);
    ASSERT_ACTION_REGISTRY_SIGNATURE(loadShortcutScheme, void (KisActionRegistry::*)(const QString &));
}

void KisActionRegistrySchemaContractTest::actionRegistryMaintenanceAndNotificationSchemaRemainsStable()
{
    ASSERT_ACTION_REGISTRY_SIGNATURE(updateShortcut, void (KisActionRegistry::*)(const QString &, QAction *));
    ASSERT_ACTION_REGISTRY_SIGNATURE(sanityCheckPropertized, bool (KisActionRegistry::*)(const QString &));
    ASSERT_ACTION_REGISTRY_SIGNATURE(shortcutsUpdated, void (KisActionRegistry::*)());
}

QTEST_GUILESS_MAIN(KisActionRegistrySchemaContractTest)
#include "KisActionRegistrySchemaContractTest.moc"
