/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_types.h"

#include <QTest>

#include <type_traits>

class KisLockedPropertiesProxySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schemaRemainsStable();
};

void KisLockedPropertiesProxySchemaContractTest::schemaRemainsStable()
{
    static_assert(std::is_same_v<KisLockedPropertiesProxySP, KisPinnedSharedPtr<KisLockedPropertiesProxy>>);
    static_assert(std::is_same_v<KisLockedPropertiesProxyWSP, KisWeakSharedPtr<KisLockedPropertiesProxy>>);
}

QTEST_APPLESS_MAIN(KisLockedPropertiesProxySchemaContractTest)

#include "KisLockedPropertiesProxySchemaContractTest.moc"
