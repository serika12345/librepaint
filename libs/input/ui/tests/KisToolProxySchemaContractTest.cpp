/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool_proxy.h>

#include "kis_change_primary_setting_action.h"

#include <QTest>

#include <type_traits>

namespace
{
using Proxy = KisToolProxy;

#define ASSERT_TOOL_PROXY_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Proxy::method)), signature>)
} // namespace

class KisToolProxySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolProxyTypeLifetimeAndConstructionSchemaRemainStable();
    void toolProxyActionStateOrdinalsRemainStable();
    void toolProxyImageAndActiveToolSignaturesRemainStable();
    void toolProxyEventForwardingAndCapabilitySignaturesRemainStable();
    void toolProxyActionLifecycleAndNotificationSignaturesRemainStable();
    void primarySettingActionTypeShortcutAndLifetimeSchemaRemainStable();
    void primarySettingActionShortcutOrdinalsRemainStable();
    void primarySettingActionLifecycleSignaturesRemainStable();
    void primarySettingActionEventAndPolicySignaturesRemainStable();
};

void KisToolProxySchemaContractTest::toolProxyTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<KoToolProxy, Proxy>);
    static_assert(std::is_constructible_v<Proxy, KoCanvasBase *>);
    static_assert(std::is_constructible_v<Proxy, KoCanvasBase *, QObject *>);
}

void KisToolProxySchemaContractTest::toolProxyActionStateOrdinalsRemainStable()
{
    static_assert(std::is_enum_v<Proxy::ActionState>);
    static_assert(Proxy::BEGIN == 0);
    static_assert(Proxy::CONTINUE == 1);
    static_assert(Proxy::END == 2);
}

void KisToolProxySchemaContractTest::toolProxyImageAndActiveToolSignaturesRemainStable()
{
    ASSERT_TOOL_PROXY_SIGNATURE(initializeImage, void (Proxy::*)(KisImageSP));
    ASSERT_TOOL_PROXY_SIGNATURE(setActiveTool, void (Proxy::*)(KoToolBase *));
}

void KisToolProxySchemaContractTest::toolProxyEventForwardingAndCapabilitySignaturesRemainStable()
{
    ASSERT_TOOL_PROXY_SIGNATURE(forwardEvent,
                                bool (Proxy::*)(Proxy::ActionState, KisTool::ToolAction, QEvent *, QEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(forwardHoverEvent, void (Proxy::*)(QEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(primaryActionSupportsHiResEvents, bool (Proxy::*)() const);
    ASSERT_TOOL_PROXY_SIGNATURE(alternateActionSupportsHiResEvents, bool (Proxy::*)(KisTool::AlternateAction) const);
    ASSERT_TOOL_PROXY_SIGNATURE(supportsPaintingAssistants, bool (Proxy::*)() const);
}

void KisToolProxySchemaContractTest::toolProxyActionLifecycleAndNotificationSignaturesRemainStable()
{
    ASSERT_TOOL_PROXY_SIGNATURE(activateToolAction, void (Proxy::*)(KisTool::ToolAction));
    ASSERT_TOOL_PROXY_SIGNATURE(deactivateToolAction, void (Proxy::*)(KisTool::ToolAction));
    ASSERT_TOOL_PROXY_SIGNATURE(toolPrimaryActionActivated, void (Proxy::*)(bool));
}

void KisToolProxySchemaContractTest::primarySettingActionTypeShortcutAndLifetimeSchemaRemainStable()
{
    using Action = KisChangePrimarySettingAction;

    static_assert(std::is_class_v<Action>);
    static_assert(std::is_base_of_v<KisAbstractInputAction, Action>);
    static_assert(std::is_enum_v<Action::Shortcut>);
    static_assert(std::is_default_constructible_v<Action>);
    static_assert(std::has_virtual_destructor_v<Action>);
}

void KisToolProxySchemaContractTest::primarySettingActionShortcutOrdinalsRemainStable()
{
    using Shortcut = KisChangePrimarySettingAction::Shortcut;

    static_assert(Shortcut::PrimaryAlternateChangeSizeShortcut == 0);
    static_assert(Shortcut::SecondaryAlternateChangeSizeShortcut == 1);
}

void KisToolProxySchemaContractTest::primarySettingActionLifecycleSignaturesRemainStable()
{
    using Action = KisChangePrimarySettingAction;
    using ShortcutOperation = void (Action::*)(int);
    using BeginOperation = void (Action::*)(int, QEvent *);
    using EventOperation = void (Action::*)(QEvent *);

    static_assert(std::is_same_v<decltype(static_cast<ShortcutOperation>(&Action::activate)), ShortcutOperation>);
    static_assert(std::is_same_v<decltype(static_cast<ShortcutOperation>(&Action::deactivate)), ShortcutOperation>);
    static_assert(std::is_same_v<decltype(static_cast<BeginOperation>(&Action::begin)), BeginOperation>);
    static_assert(std::is_same_v<decltype(static_cast<EventOperation>(&Action::end)), EventOperation>);
}

void KisToolProxySchemaContractTest::primarySettingActionEventAndPolicySignaturesRemainStable()
{
    using Action = KisChangePrimarySettingAction;
    using EventOperation = void (Action::*)(QEvent *);
    using PriorityQuery = int (Action::*)() const;
    using ShortcutBooleanQuery = bool (Action::*)(int) const;

    static_assert(std::is_same_v<decltype(static_cast<EventOperation>(&Action::inputEvent)), EventOperation>);
    static_assert(std::is_same_v<decltype(static_cast<PriorityQuery>(&Action::priority)), PriorityQuery>);
    static_assert(std::is_same_v<decltype(static_cast<ShortcutBooleanQuery>(&Action::supportsHiResInputEvents)),
                                 ShortcutBooleanQuery>);
}

QTEST_APPLESS_MAIN(KisToolProxySchemaContractTest)

#include "KisToolProxySchemaContractTest.moc"
