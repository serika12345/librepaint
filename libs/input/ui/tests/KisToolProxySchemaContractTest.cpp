/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool_proxy.h>

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

QTEST_APPLESS_MAIN(KisToolProxySchemaContractTest)

#include "KisToolProxySchemaContractTest.moc"
