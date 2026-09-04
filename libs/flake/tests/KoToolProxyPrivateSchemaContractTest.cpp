/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QKeySequence>

#include <KoToolProxy.h>
#include <KoToolProxy_p.h>

#include <QTest>

#include <type_traits>

#define ASSERT_TOOL_PROXY_PRIVATE_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolProxyPrivate::method)), signature>)

// Remove this temporary compatibility contract after all non-flake includes of
// KoToolProxy_p.h and direct access through KoToolProxy::priv() have been removed.
class KoToolProxyPrivateSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolProxyPrivateTypeLifetimeAndSelectionStateSchemaRemainStable();
    void toolProxyPrivateCanvasBindingSignaturesRemainStable();
    void toolProxyPrivateAutoScrollSignaturesRemainStable();
    void toolProxyPrivatePressAndMultiClickStateSchemaRemainStable();
    void toolProxyPrivateInputEventMetadataSchemaRemainStable();
};

void KoToolProxyPrivateSchemaContractTest::toolProxyPrivateTypeLifetimeAndSelectionStateSchemaRemainStable()
{
    using Private = KoToolProxyPrivate;

    static_assert(std::is_class_v<Private>);
    static_assert(std::is_same_v<decltype(Private::activeTool), KoToolBase *>);
    static_assert(std::is_same_v<decltype(Private::hasSelection), bool>);
    static_assert(std::is_same_v<decltype(Private::parent), KoToolProxy *>);
    static_assert(std::is_constructible_v<Private, KoToolProxy *>);
    ASSERT_TOOL_PROXY_PRIVATE_SIGNATURE(selectionChanged, void (Private::*)(bool));
}

void KoToolProxyPrivateSchemaContractTest::toolProxyPrivateCanvasBindingSignaturesRemainStable()
{
    using Private = KoToolProxyPrivate;

    static_assert(std::is_same_v<decltype(Private::controller), KoCanvasController *>);
    ASSERT_TOOL_PROXY_PRIVATE_SIGNATURE(isActiveLayerEditable, bool (Private::*)());
    ASSERT_TOOL_PROXY_PRIVATE_SIGNATURE(setCanvasController, void (Private::*)(KoCanvasController *));
}

void KoToolProxyPrivateSchemaContractTest::toolProxyPrivateAutoScrollSignaturesRemainStable()
{
    using Private = KoToolProxyPrivate;

    static_assert(std::is_same_v<decltype(Private::scrollTimer), QTimer>);
    static_assert(std::is_same_v<decltype(Private::widgetScrollPointDoc), QPointF>);
    ASSERT_TOOL_PROXY_PRIVATE_SIGNATURE(checkAutoScroll, void (Private::*)(const KoPointerEvent &));
    ASSERT_TOOL_PROXY_PRIVATE_SIGNATURE(timeout, void (Private::*)());
}

void KoToolProxyPrivateSchemaContractTest::toolProxyPrivatePressAndMultiClickStateSchemaRemainStable()
{
    using Private = KoToolProxyPrivate;

    static_assert(std::is_same_v<decltype(Private::isToolPressed), bool>);
    static_assert(std::is_same_v<decltype(Private::mouseDownPoint), QPoint>);
    static_assert(std::is_same_v<decltype(Private::mouseLeaveWorkaround), bool>);
    static_assert(std::is_same_v<decltype(Private::multiClickCount), int>);
    static_assert(std::is_same_v<decltype(Private::multiClickGlobalPoint), QPointF>);
}

void KoToolProxyPrivateSchemaContractTest::toolProxyPrivateInputEventMetadataSchemaRemainStable()
{
    using Private = KoToolProxyPrivate;

    static_assert(std::is_same_v<decltype(Private::lastPointerEvent), boost::optional<KoPointerEventWrapper>>);
    static_assert(std::is_same_v<decltype(Private::multiClickSource), QEvent::Type>);
    static_assert(std::is_same_v<decltype(Private::multiClickTimeStamp), QElapsedTimer>);
    static_assert(std::is_same_v<decltype(Private::toolPriorityShortcuts), QVector<QKeySequence>>);
}

QTEST_GUILESS_MAIN(KoToolProxyPrivateSchemaContractTest)

#include "KoToolProxyPrivateSchemaContractTest.moc"
