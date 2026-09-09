/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_progress_proxy.h"
#include "kis_progress_updater.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_SIGNATURE(type, method, ...)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)

} // namespace

class KisImageProgressSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void progressInterfaceTypeAndLifetimeSchemaRemainStable();
    void progressInterfaceAttachmentSignaturesRemainStable();
    void progressUpdaterTypeConstructionAndLifetimeSchemaRemainStable();
    void nodeProgressTypeAndQuerySchemaRemainStable();
    void nodeProgressMutationAndNotificationSignaturesRemainStable();
};

void KisImageProgressSchemaContractTest::progressInterfaceTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisProgressInterface>);
    static_assert(std::has_virtual_destructor_v<KisProgressInterface>);

    QVERIFY(true);
}

void KisImageProgressSchemaContractTest::progressInterfaceAttachmentSignaturesRemainStable()
{
    using Interface = KisProgressInterface;

    ASSERT_SIGNATURE(Interface, attachUpdater, void (Interface::*)(KoProgressUpdater *));
    ASSERT_SIGNATURE(Interface, detachUpdater, void (Interface::*)(KoProgressUpdater *));
}

void KisImageProgressSchemaContractTest::progressUpdaterTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Updater = KisProgressUpdater;

    static_assert(std::is_class_v<Updater>);
    static_assert(std::is_base_of_v<KoProgressUpdater, Updater>);
    static_assert(std::is_constructible_v<Updater, KisProgressInterface *, KoProgressProxy *>);
    static_assert(std::is_constructible_v<Updater, KisProgressInterface *, KoProgressProxy *, KoProgressUpdater::Mode>);
    static_assert(std::has_virtual_destructor_v<Updater>);

    QVERIFY(true);
}

void KisImageProgressSchemaContractTest::nodeProgressTypeAndQuerySchemaRemainStable()
{
    using Proxy = KisNodeProgressProxy;

    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<QObject, Proxy>);
    static_assert(std::is_base_of_v<KoProgressProxy, Proxy>);
    ASSERT_SIGNATURE(Proxy, maximum, int (Proxy::*)() const);
    ASSERT_SIGNATURE(Proxy, percentage, int (Proxy::*)() const);
}

void KisImageProgressSchemaContractTest::nodeProgressMutationAndNotificationSignaturesRemainStable()
{
    using Proxy = KisNodeProgressProxy;

    ASSERT_SIGNATURE(Proxy, setValue, void (Proxy::*)(int));
    ASSERT_SIGNATURE(Proxy, setRange, void (Proxy::*)(int, int));
    ASSERT_SIGNATURE(Proxy, setFormat, void (Proxy::*)(const QString &));
    ASSERT_SIGNATURE(Proxy, percentageChanged, void (Proxy::*)(int, const KisNodeSP &));
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisImageProgressSchemaContractTest)

#include "KisImageProgressSchemaContractTest.moc"
