/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_manager.h"
#include "KisPopupWidgetAction.h"

#include <type_traits>
#include <utility>

#include <QTest>

namespace
{

#define ASSERT_INPUT_MANAGER_MEMBER(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisInputManager::method)), signature>)

} // namespace

class KisInputManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inputManagerTypeLifetimeAndConstructionSchemaRemainStable();
    void inputManagerCanvasTrackingSignaturesRemainStable();
    void inputManagerCanvasAndToolContextSignaturesRemainStable();
    void inputManagerEventFilterSignaturesRemainStable();
    void inputManagerPopupConfigurationAndDiagnosticsSignaturesRemainStable();
    void popupWidgetActionTypeAndLifecycleSchemaRemainStable();
};

void KisInputManagerSchemaContractTest::inputManagerTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Manager = KisInputManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager, QObject *>);
    static_assert(std::has_virtual_destructor_v<Manager>);

    QVERIFY(true);
}

void KisInputManagerSchemaContractTest::inputManagerCanvasTrackingSignaturesRemainStable()
{
    ASSERT_INPUT_MANAGER_MEMBER(addTrackedCanvas, void (KisInputManager::*)(KoCanvasBase *));
    ASSERT_INPUT_MANAGER_MEMBER(removeTrackedCanvas, void (KisInputManager::*)(KoCanvasBase *));

    QVERIFY(true);
}

void KisInputManagerSchemaContractTest::inputManagerCanvasAndToolContextSignaturesRemainStable()
{
    ASSERT_INPUT_MANAGER_MEMBER(canvas, KoCanvasBase * (KisInputManager::*)() const);
    ASSERT_INPUT_MANAGER_MEMBER(toolCanvas, KisToolCanvas * (KisInputManager::*)() const);
    ASSERT_INPUT_MANAGER_MEMBER(toolProxy, QPointer<KisToolProxy> (KisInputManager::*)() const);

    QVERIFY(true);
}

void KisInputManagerSchemaContractTest::inputManagerEventFilterSignaturesRemainStable()
{
    using Manager = KisInputManager;

    ASSERT_INPUT_MANAGER_MEMBER(attachPriorityEventFilter, void (Manager::*)(QObject *, int));
    ASSERT_INPUT_MANAGER_MEMBER(detachPriorityEventFilter, void (Manager::*)(QObject *));
    ASSERT_INPUT_MANAGER_MEMBER(eventFilter, bool (Manager::*)(QObject *, QEvent *));
    ASSERT_INPUT_MANAGER_MEMBER(setupAsEventFilter, void (Manager::*)(QObject *));
    static_assert(
        std::is_same_v<decltype(std::declval<Manager &>().attachPriorityEventFilter(std::declval<QObject *>())), void>);

    QVERIFY(true);
}

void KisInputManagerSchemaContractTest::inputManagerPopupConfigurationAndDiagnosticsSignaturesRemainStable()
{
    using Manager = KisInputManager;

    ASSERT_INPUT_MANAGER_MEMBER(registerPopupWidget, void (Manager::*)(KisPopupWidgetInterface *));
    ASSERT_INPUT_MANAGER_MEMBER(reloadSettings, void (Manager::*)());
    ASSERT_INPUT_MANAGER_MEMBER(toggleTabletLogger, void (Manager::*)());

    QVERIFY(true);
}

void KisInputManagerSchemaContractTest::popupWidgetActionTypeAndLifecycleSchemaRemainStable()
{
    using Action = KisPopupWidgetAction;

    static_assert(std::is_base_of_v<QObject, Action>);
    static_assert(std::is_base_of_v<KisAbstractInputAction, Action>);
    static_assert(std::is_default_constructible_v<Action>);
    static_assert(std::has_virtual_destructor_v<Action>);
    static_assert(std::is_same_v<decltype(&Action::priority), int (Action::*)() const>);
    static_assert(std::is_same_v<decltype(&Action::end), void (Action::*)(QEvent *)>);

    QVERIFY(true);
}

#undef ASSERT_INPUT_MANAGER_MEMBER

QTEST_APPLESS_MAIN(KisInputManagerSchemaContractTest)

#include "KisInputManagerSchemaContractTest.moc"
