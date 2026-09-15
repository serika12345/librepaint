/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <nodes/kis_mask_manager.h>

#include <QTest>

#include <type_traits>

class KisMaskManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maskManagerPublicSchemaRemainsStable();
};

void KisMaskManagerSchemaContractTest::maskManagerPublicSchemaRemainsStable()
{
    using Manager = KisMaskManager;

    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(&Manager::setView), void (Manager::*)(QPointer<KisView>)>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KisMaskManagerSchemaContractTest)

#include "KisMaskManagerSchemaContractTest.moc"
