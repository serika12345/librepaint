/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisColord.h>
#include <kis_color_manager.h>

#include <QTest>

#include <type_traits>

class KisColorLifecycleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorManagerLifetimeAndNotificationSchemaRemainStable();
    void colordLifetimeAndNotificationSchemaRemainStable();
};

void KisColorLifecycleSchemaContractTest::colorManagerLifetimeAndNotificationSchemaRemainStable()
{
    using ChangedSignature = void (KisColorManager::*)(QString);

    static_assert(std::has_virtual_destructor_v<KisColorManager>);
    static_assert(std::is_same_v<decltype(static_cast<ChangedSignature>(&KisColorManager::changed)), ChangedSignature>);
}

void KisColorLifecycleSchemaContractTest::colordLifetimeAndNotificationSchemaRemainStable()
{
    using ChangedSignature = void (KisColord::*)();

    static_assert(std::has_virtual_destructor_v<KisColord>);
    static_assert(std::is_same_v<decltype(static_cast<ChangedSignature>(&KisColord::changed)), ChangedSignature>);
}

QTEST_GUILESS_MAIN(KisColorLifecycleSchemaContractTest)

#include "KisColorLifecycleSchemaContractTest.moc"
