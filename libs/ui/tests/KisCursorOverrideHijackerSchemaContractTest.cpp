/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "events/kis_cursor_override_hijacker.h"

#include <QTest>

#include <type_traits>

class KisCursorOverrideHijackerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cursorOverrideHijackerTypeConstructionAndLifetimeSchemaRemainStable();
};

void KisCursorOverrideHijackerSchemaContractTest::cursorOverrideHijackerTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Hijacker = KisCursorOverrideHijacker;

    static_assert(std::is_class_v<Hijacker>);
    static_assert(std::is_default_constructible_v<Hijacker>);
    static_assert(std::is_destructible_v<Hijacker>);
}

QTEST_APPLESS_MAIN(KisCursorOverrideHijackerSchemaContractTest)

#include "KisCursorOverrideHijackerSchemaContractTest.moc"
