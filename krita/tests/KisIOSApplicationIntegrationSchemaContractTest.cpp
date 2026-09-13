/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisIOSLifecycleHandler.h"
#include "KisIOSMemoryWarningHandler.h"
#include "KisIOSPencilInteraction.h"

#include <QTest>

#include <type_traits>

class KisIOSApplicationIntegrationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lifecycleTypeAndEntryPointSignaturesRemainStable();
    void memoryWarningHandlerSignatureRemainsStable();
    void pencilTapTypeAndInstallationSignatureRemainStable();
};

void KisIOSApplicationIntegrationSchemaContractTest::lifecycleTypeAndEntryPointSignaturesRemainStable()
{
    using LifecycleHandler = void (*)(KisIOSLifecycleEvent);
    using LifecycleInstallation = void (*)(KisIOSLifecycleHandler);
    using BackgroundTaskCompletion = void (*)();

    static_assert(std::is_enum_v<KisIOSLifecycleEvent>);
    static_assert(std::is_same_v<decltype(KisIOSLifecycleEvent::WillResignActive), KisIOSLifecycleEvent>);
    static_assert(std::is_same_v<decltype(KisIOSLifecycleEvent::DidEnterBackground), KisIOSLifecycleEvent>);
    static_assert(std::is_same_v<decltype(KisIOSLifecycleEvent::WillEnterForeground), KisIOSLifecycleEvent>);
    static_assert(std::is_same_v<decltype(KisIOSLifecycleEvent::DidBecomeActive), KisIOSLifecycleEvent>);
    static_assert(std::is_same_v<decltype(KisIOSLifecycleEvent::BackgroundTaskExpired), KisIOSLifecycleEvent>);
    static_assert(std::is_same_v<KisIOSLifecycleHandler, LifecycleHandler>);
    static_assert(std::is_same_v<decltype(&installKisIOSLifecycleHandler), LifecycleInstallation>);
    static_assert(std::is_same_v<decltype(&finishKisIOSBackgroundTask), BackgroundTaskCompletion>);
}

void KisIOSApplicationIntegrationSchemaContractTest::memoryWarningHandlerSignatureRemainsStable()
{
    using MemoryWarningHandler = void (*)();

    static_assert(std::is_same_v<decltype(&installKisIOSMemoryWarningHandler), MemoryWarningHandler>);
}

void KisIOSApplicationIntegrationSchemaContractTest::pencilTapTypeAndInstallationSignatureRemainStable()
{
    using PencilTapHandler = void (*)(KisIOSPencilTapAction);
    using PencilInstallation = void (*)(void *, KisIOSPencilTapHandler);

    static_assert(std::is_enum_v<KisIOSPencilTapAction>);
    static_assert(std::is_same_v<decltype(KisIOSPencilTapAction::SwitchEraser), KisIOSPencilTapAction>);
    static_assert(std::is_same_v<decltype(KisIOSPencilTapAction::SwitchPrevious), KisIOSPencilTapAction>);
    static_assert(std::is_same_v<KisIOSPencilTapHandler, PencilTapHandler>);
    static_assert(std::is_same_v<decltype(&installKisIOSPencilInteraction), PencilInstallation>);
}

QTEST_GUILESS_MAIN(KisIOSApplicationIntegrationSchemaContractTest)

#include "KisIOSApplicationIntegrationSchemaContractTest.moc"
