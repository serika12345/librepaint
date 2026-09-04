/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_profile_manager.h"

#include <type_traits>

#include <QTest>

namespace
{

#define ASSERT_INPUT_PROFILE_MANAGER_MEMBER(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisInputProfileManager::method)), signature>)

} // namespace

class KisInputProfileManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inputProfileManagerTypeLifetimeAndSingletonSchemaRemainStable();
    void inputProfileManagerActionSignaturesRemainStable();
    void inputProfileManagerProfileQueryAndSelectionSignaturesRemainStable();
    void inputProfileManagerProfileMutationSignaturesRemainStable();
    void inputProfileManagerPersistenceAndNotificationSignaturesRemainStable();
};

void KisInputProfileManagerSchemaContractTest::inputProfileManagerTypeLifetimeAndSingletonSchemaRemainStable()
{
    using Manager = KisInputProfileManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_default_constructible_v<Manager>);
    static_assert(std::is_constructible_v<Manager, QObject *>);
    static_assert(!std::is_copy_constructible_v<Manager>);
    static_assert(!std::is_copy_assignable_v<Manager>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(static_cast<Manager *(*)()>(&Manager::instance)), Manager *(*)()>);

    QVERIFY(true);
}

void KisInputProfileManagerSchemaContractTest::inputProfileManagerActionSignaturesRemainStable()
{
    using Action = KisAbstractInputAction;
    using Actions = QList<Action *>;

    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(action, Action * (KisInputProfileManager::*)(const QString &) const);
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(actions, Actions (KisInputProfileManager::*)());
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(setActions, void (KisInputProfileManager::*)(const Actions &));

    QVERIFY(true);
}

void KisInputProfileManagerSchemaContractTest::inputProfileManagerProfileQueryAndSelectionSignaturesRemainStable()
{
    using Profile = KisInputProfile;
    using Profiles = QList<Profile *>;
    using Shortcuts = QList<KisShortcutConfiguration *>;

    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(currentProfile, Profile * (KisInputProfileManager::*)() const);
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(currentProfileChanged, void (KisInputProfileManager::*)());
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(getConflictingShortcuts, Shortcuts (KisInputProfileManager::*)(Profile *));
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(profileNames, QStringList (KisInputProfileManager::*)() const);
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(profiles, Profiles (KisInputProfileManager::*)() const);
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(setCurrentProfile, void (KisInputProfileManager::*)(Profile *));

    QVERIFY(true);
}

void KisInputProfileManagerSchemaContractTest::inputProfileManagerProfileMutationSignaturesRemainStable()
{
    using Manager = KisInputProfileManager;

    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(addProfile, KisInputProfile * (Manager::*)(const QString &));
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(duplicateProfile, void (Manager::*)(const QString &, const QString &));
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(removeProfile, void (Manager::*)(const QString &));
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(renameProfile, bool (Manager::*)(const QString &, const QString &));

    QVERIFY(true);
}

void KisInputProfileManagerSchemaContractTest::inputProfileManagerPersistenceAndNotificationSignaturesRemainStable()
{
    using Manager = KisInputProfileManager;

    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(loadProfiles, void (Manager::*)());
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(profilesChanged, void (Manager::*)());
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(resetAll, void (Manager::*)());
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(saveProfile, void (Manager::*)(KisInputProfile *, QString));
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(saveProfiles, void (Manager::*)());
    ASSERT_INPUT_PROFILE_MANAGER_MEMBER(setProfileLocations, void (Manager::*)(const QStringList &, const QString &));

    QVERIFY(true);
}

#undef ASSERT_INPUT_PROFILE_MANAGER_MEMBER

QTEST_APPLESS_MAIN(KisInputProfileManagerSchemaContractTest)

#include "KisInputProfileManagerSchemaContractTest.moc"
