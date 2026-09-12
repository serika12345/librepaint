/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/KisGrabKeyboardFocusRecoveryWorkaround.h"

#include <QTest>

#include <type_traits>

class KisGrabKeyboardFocusRecoveryWorkaroundSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusRecoverySingletonSchemaRemainsStable();
    void focusRecoveryOperationSignatureRemainsStable();
};

void KisGrabKeyboardFocusRecoveryWorkaroundSchemaContractTest::focusRecoverySingletonSchemaRemainsStable()
{
    using Workaround = KisGrabKeyboardFocusRecoveryWorkaround;

    static_assert(std::is_class_v<Workaround>);
    static_assert(std::is_same_v<decltype(&Workaround::instance), Workaround *(*)()>);
}

void KisGrabKeyboardFocusRecoveryWorkaroundSchemaContractTest::focusRecoveryOperationSignatureRemainsStable()
{
    using Workaround = KisGrabKeyboardFocusRecoveryWorkaround;

    static_assert(std::is_same_v<decltype(&Workaround::recoverFocus), void (Workaround::*)()>);
}

QTEST_APPLESS_MAIN(KisGrabKeyboardFocusRecoveryWorkaroundSchemaContractTest)

#include "KisGrabKeyboardFocusRecoveryWorkaroundSchemaContractTest.moc"
