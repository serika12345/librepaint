/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoProgressUpdater.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PROGRESS_UPDATER_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoProgressUpdater::method)), signature>)
} // namespace

class KoProgressUpdaterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void progressUpdaterTypeAndModeSchemaRemainStable();
    void progressUpdaterConstructionAndLifetimeSchemaRemainStable();
    void progressUpdaterExecutionAndCancellationSignaturesRemainStable();
    void progressUpdaterConfigurationAndStartSignaturesRemainStable();
    void progressUpdaterSubtaskSignaturesRemainStable();
};

void KoProgressUpdaterSchemaContractTest::progressUpdaterTypeAndModeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoProgressUpdater>);
    static_assert(std::is_enum_v<KoProgressUpdater::Mode>);
    static_assert(KoProgressUpdater::Mode::Threaded == 0);
    static_assert(KoProgressUpdater::Mode::Unthreaded == 1);
}

void KoProgressUpdaterSchemaContractTest::progressUpdaterConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_base_of_v<QObject, KoProgressUpdater>);
    static_assert(std::is_constructible_v<KoProgressUpdater, KoProgressProxy *, KoProgressUpdater::Mode>);
    static_assert(std::is_constructible_v<KoProgressUpdater, QPointer<KoUpdater>>);
    static_assert(std::has_virtual_destructor_v<KoProgressUpdater>);
    static_assert(std::is_same_v<decltype(KoProgressUpdater(std::declval<KoProgressProxy *>())), KoProgressUpdater>);
}

void KoProgressUpdaterSchemaContractTest::progressUpdaterExecutionAndCancellationSignaturesRemainStable()
{
    ASSERT_PROGRESS_UPDATER_SIGNATURE(interrupted, bool (KoProgressUpdater::*)() const);
    ASSERT_PROGRESS_UPDATER_SIGNATURE(cancel, void (KoProgressUpdater::*)());
    ASSERT_PROGRESS_UPDATER_SIGNATURE(triggerUpdateAsynchronously, void (KoProgressUpdater::*)());
}

void KoProgressUpdaterSchemaContractTest::progressUpdaterConfigurationAndStartSignaturesRemainStable()
{
    ASSERT_PROGRESS_UPDATER_SIGNATURE(autoNestNames, bool (KoProgressUpdater::*)() const);
    ASSERT_PROGRESS_UPDATER_SIGNATURE(setAutoNestNames, void (KoProgressUpdater::*)(bool));
    ASSERT_PROGRESS_UPDATER_SIGNATURE(setUpdateInterval, void (KoProgressUpdater::*)(int));
    ASSERT_PROGRESS_UPDATER_SIGNATURE(updateInterval, int (KoProgressUpdater::*)() const);
    ASSERT_PROGRESS_UPDATER_SIGNATURE(start, void (KoProgressUpdater::*)(int, const QString &));
    static_assert(std::is_same_v<decltype(std::declval<KoProgressUpdater &>().start()), void>);
}

void KoProgressUpdaterSchemaContractTest::progressUpdaterSubtaskSignaturesRemainStable()
{
    ASSERT_PROGRESS_UPDATER_SIGNATURE(removePersistentSubtask, void (KoProgressUpdater::*)(QPointer<KoUpdater>));
    ASSERT_PROGRESS_UPDATER_SIGNATURE(startSubtask,
                                      QPointer<KoUpdater> (KoProgressUpdater::*)(int, const QString &, bool));
    static_assert(std::is_same_v<decltype(std::declval<KoProgressUpdater &>().startSubtask()), QPointer<KoUpdater>>);
}

QTEST_GUILESS_MAIN(KoProgressUpdaterSchemaContractTest)

#include "KoProgressUpdaterSchemaContractTest.moc"
