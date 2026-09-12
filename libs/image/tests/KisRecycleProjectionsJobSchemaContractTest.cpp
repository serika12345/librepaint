/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRecycleProjectionsJob.h"

#include <QTest>

#include <type_traits>

class KisRecycleProjectionsJobSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void projectionStoreTypeAndConstructionSchemaRemainStable();
    void executionAndIdentitySignaturesRemainStable();
    void jobOverrideSignatureRemainsStable();
};

void KisRecycleProjectionsJobSchemaContractTest::projectionStoreTypeAndConstructionSchemaRemainStable()
{
    using Job = KisRecycleProjectionsJob;
    using ProjectionStore = KisSafeNodeProjectionStoreBaseWSP;

    static_assert(std::is_same_v<ProjectionStore, KisWeakSharedPtr<KisSafeNodeProjectionStoreBase>>);
    static_assert(std::is_class_v<Job>);
    static_assert(std::is_constructible_v<Job, ProjectionStore>);
}

void KisRecycleProjectionsJobSchemaContractTest::executionAndIdentitySignaturesRemainStable()
{
    using Job = KisRecycleProjectionsJob;

    static_assert(std::is_same_v<decltype(&Job::run), void (Job::*)()>);
    static_assert(std::is_same_v<decltype(&Job::levelOfDetail), int (Job::*)() const>);
    static_assert(std::is_same_v<decltype(&Job::debugName), QString (Job::*)() const>);
}

void KisRecycleProjectionsJobSchemaContractTest::jobOverrideSignatureRemainsStable()
{
    using Job = KisRecycleProjectionsJob;

    static_assert(std::is_same_v<decltype(&Job::overrides), bool (Job::*)(const KisSpontaneousJob *)>);
}

QTEST_GUILESS_MAIN(KisRecycleProjectionsJobSchemaContractTest)

#include "KisRecycleProjectionsJobSchemaContractTest.moc"
