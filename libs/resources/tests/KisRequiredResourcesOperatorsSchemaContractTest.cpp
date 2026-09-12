/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisRequiredResourcesOperators.h>

#include <QTest>

#include <type_traits>

namespace
{
struct SnapshotProbe;
} // namespace

class KisRequiredResourcesOperatorsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void localStorageDetailSignaturesRemainStable();
    void resourceLoadAndGuiThreadDetailSignaturesRemainStable();
    void snapshotTemplateSignaturesRemainStable();
};

void KisRequiredResourcesOperatorsSchemaContractTest::localStorageDetailSignaturesRemainStable()
{
    using namespace KisRequiredResourcesOperators;

    static_assert(std::is_same_v<decltype(&detail::isLocalResourcesStorage), bool (*)(KisResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&detail::createLocalResourcesStorage),
                                 KisResourcesInterfaceSP (*)(const QList<KoResourceSP> &)>);

    QVERIFY(true);
}

void KisRequiredResourcesOperatorsSchemaContractTest::resourceLoadAndGuiThreadDetailSignaturesRemainStable()
{
    using namespace KisRequiredResourcesOperators;

    static_assert(std::is_same_v<decltype(&detail::assertInGuiThread), void (*)()>);
    static_assert(std::is_same_v<decltype(&detail::addResourceOrWarnIfNotLoaded),
                                 void (*)(KoResourceLoadResult, QList<KoResourceSP> *, KisResourcesInterfaceSP)>);

    QVERIFY(true);
}

void KisRequiredResourcesOperatorsSchemaContractTest::snapshotTemplateSignaturesRemainStable()
{
    using namespace KisRequiredResourcesOperators;

    static_assert(std::is_same_v<decltype(&hasLocalResourcesSnapshot<SnapshotProbe>), bool (*)(const SnapshotProbe *)>);
    static_assert(std::is_same_v<decltype(&createLocalResourcesSnapshot<SnapshotProbe>),
                                 void (*)(SnapshotProbe *, KisResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&cloneWithResourcesSnapshot<KoResourceSP>),
                                 KoResourceSP (*)(const KoResource *, KisResourcesInterfaceSP)>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KisRequiredResourcesOperatorsSchemaContractTest)

#include "KisRequiredResourcesOperatorsSchemaContractTest.moc"
