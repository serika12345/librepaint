/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "application/ui/workspace/KisAndroidSplash.h"
#include "events/kis_cursor_override_hijacker.h"

#include <QTest>

#include <type_traits>

class KisCursorOverrideHijackerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cursorOverrideHijackerTypeConstructionAndLifetimeSchemaRemainStable();
    void androidSplashTypeAndStaticApiSchemaRemainStable();
};

void KisCursorOverrideHijackerSchemaContractTest::cursorOverrideHijackerTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Hijacker = KisCursorOverrideHijacker;

    static_assert(std::is_class_v<Hijacker>);
    static_assert(std::is_default_constructible_v<Hijacker>);
    static_assert(std::is_destructible_v<Hijacker>);
}

void KisCursorOverrideHijackerSchemaContractTest::androidSplashTypeAndStaticApiSchemaRemainStable()
{
    using Splash = KisAndroidSplash;

    static_assert(std::is_class_v<Splash>);
    static_assert(std::is_base_of_v<QObject, Splash>);
    static_assert(std::is_same_v<decltype(&Splash::instance), Splash *(*)()>);
    static_assert(std::is_same_v<decltype(&Splash::show), void (*)()>);
    static_assert(std::is_same_v<decltype(&Splash::setLoaded), void (*)(bool)>);
    static_assert(std::is_same_v<decltype(&Splash::setLoadingText), void (*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Splash::sigSplashDialogDismissed), void (Splash::*)()>);
}

QTEST_APPLESS_MAIN(KisCursorOverrideHijackerSchemaContractTest)

#include "KisCursorOverrideHijackerSchemaContractTest.moc"
