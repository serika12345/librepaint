/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCanvasController.h>
#include <KoToolProxy.h>

#include <QTest>

#include <type_traits>

#define ASSERT_CANVAS_CONTROLLER_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCanvasController::method)), signature>)
#define ASSERT_TOOL_PROXY_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoToolProxy::method)), signature>)

namespace
{
class CanvasControllerConstructorProbe : public KoCanvasController
{
public:
    explicit CanvasControllerConstructorProbe(KisKActionCollection *actionCollection)
        : KoCanvasController(actionCollection)
    {
    }
};

class ToolProxyConstructorProbe : public KoToolProxy
{
public:
    using KoToolProxy::KoToolProxy;

protected:
    QPointF widgetToDocument(const QPointF &widgetPoint) const override;
    QPointF documentToWidget(const QPointF &documentPoint) const override;
};
} // namespace

class KoCanvasControllerProxySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolProxyTypeLifetimeAndActiveToolSignaturesRemainStable();
    void toolProxyPointerEventForwardingSignaturesRemainStable();
    void toolProxyKeyboardFocusAndDropSignaturesRemainStable();
    void toolProxyPaintingAndEditingSignaturesRemainStable();
    void toolProxyNotificationSignaturesRemainStable();
    void canvasControllerTypeLifetimeAndCanvasSignaturesRemainStable();
    void canvasControllerVisibilityAndPositionSignaturesRemainStable();
    void canvasControllerZoomSignaturesRemainStable();
    void canvasControllerPanSignaturesRemainStable();
    void canvasControllerScrollBarSignaturesRemainStable();
    void canvasControllerProxyIdentityAndAccessSignaturesRemainStable();
    void canvasControllerProxyLifecycleNotificationsRemainStable();
    void canvasControllerProxyPositionAndOffsetNotificationsRemainStable();
    void canvasControllerProxyViewGeometryNotificationsRemainStable();
    void canvasControllerProxyZoomAndMirrorNotificationsRemainStable();
};

void KoCanvasControllerProxySchemaContractTest::toolProxyTypeLifetimeAndActiveToolSignaturesRemainStable()
{
    using Proxy = KoToolProxy;
    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<QObject, Proxy>);
    static_assert(std::is_abstract_v<Proxy>);
    static_assert(std::has_virtual_destructor_v<Proxy>);
    static_assert(std::is_constructible_v<ToolProxyConstructorProbe, KoCanvasBase *>);
    static_assert(std::is_constructible_v<ToolProxyConstructorProbe, KoCanvasBase *, QObject *>);
    ASSERT_TOOL_PROXY_SIGNATURE(setActiveTool, void (Proxy::*)(KoToolBase *));
    ASSERT_TOOL_PROXY_SIGNATURE(lastDeliveredPointerEvent, KoPointerEvent * (Proxy::*)() const);
    ASSERT_TOOL_PROXY_SIGNATURE(toolPriorityShortcuts, QVector<QKeySequence> (Proxy::*)() const);
    ASSERT_TOOL_PROXY_SIGNATURE(priv, KoToolProxyPrivate * (Proxy::*)());
}

void KoCanvasControllerProxySchemaContractTest::toolProxyPointerEventForwardingSignaturesRemainStable()
{
    using Proxy = KoToolProxy;
    ASSERT_TOOL_PROXY_SIGNATURE(tabletEvent, void (Proxy::*)(QTabletEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(touchEvent, void (Proxy::*)(QTouchEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(mousePressEvent, void (Proxy::*)(QMouseEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(mousePressEvent, void (Proxy::*)(KoPointerEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(mouseDoubleClickEvent, void (Proxy::*)(QMouseEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(mouseDoubleClickEvent, void (Proxy::*)(KoPointerEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(mouseMoveEvent, void (Proxy::*)(QMouseEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(mouseMoveEvent, void (Proxy::*)(KoPointerEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(mouseReleaseEvent, void (Proxy::*)(QMouseEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(mouseReleaseEvent, void (Proxy::*)(KoPointerEvent *));
}

void KoCanvasControllerProxySchemaContractTest::toolProxyKeyboardFocusAndDropSignaturesRemainStable()
{
    using Proxy = KoToolProxy;
    ASSERT_TOOL_PROXY_SIGNATURE(keyPressEvent, void (Proxy::*)(QKeyEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(keyReleaseEvent, void (Proxy::*)(QKeyEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(inputMethodQuery, QVariant (Proxy::*)(Qt::InputMethodQuery) const);
    ASSERT_TOOL_PROXY_SIGNATURE(inputMethodEvent, void (Proxy::*)(QInputMethodEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(focusInEvent, void (Proxy::*)(QFocusEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(focusOutEvent, void (Proxy::*)(QFocusEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(dragMoveEvent, void (Proxy::*)(QDragMoveEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(dragLeaveEvent, void (Proxy::*)(QDragLeaveEvent *));
    ASSERT_TOOL_PROXY_SIGNATURE(dropEvent, void (Proxy::*)(QDropEvent *, const QPointF &));
    ASSERT_TOOL_PROXY_SIGNATURE(processEvent, void (Proxy::*)(QEvent *) const);
}

void KoCanvasControllerProxySchemaContractTest::toolProxyPaintingAndEditingSignaturesRemainStable()
{
    using Proxy = KoToolProxy;
    ASSERT_TOOL_PROXY_SIGNATURE(paint, void (Proxy::*)(QPainter &, const KoViewConverter &));
    ASSERT_TOOL_PROXY_SIGNATURE(repaintDecorations, void (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(explicitUserStrokeEndRequest, void (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(popupActionsMenu, QMenu * (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(popupWidget, KisPopupWidgetInterface * (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(deleteSelection, void (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(hasSelection, bool (Proxy::*)() const);
    ASSERT_TOOL_PROXY_SIGNATURE(cut, void (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(copy, void (Proxy::*)() const);
    ASSERT_TOOL_PROXY_SIGNATURE(paste, bool (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(selectAll, bool (Proxy::*)());
    ASSERT_TOOL_PROXY_SIGNATURE(deselect, void (Proxy::*)());
}

void KoCanvasControllerProxySchemaContractTest::toolProxyNotificationSignaturesRemainStable()
{
    using Proxy = KoToolProxy;
    ASSERT_TOOL_PROXY_SIGNATURE(selectionChanged, void (Proxy::*)(bool));
    ASSERT_TOOL_PROXY_SIGNATURE(toolChanged, void (Proxy::*)(const QString &));
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerTypeLifetimeAndCanvasSignaturesRemainStable()
{
    using Controller = KoCanvasController;
    static_assert(std::is_class_v<Controller>);
    static_assert(std::is_abstract_v<Controller>);
    static_assert(std::has_virtual_destructor_v<Controller>);
    static_assert(std::is_abstract_v<CanvasControllerConstructorProbe>);
    static_assert(std::is_same_v<decltype(Controller::proxyObject), QPointer<KoCanvasControllerProxyObject>>);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(actionCollection, KisKActionCollection * (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(canvas, KoCanvasBase * (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setCanvas, void (Controller::*)(KoCanvasBase *));
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerVisibilityAndPositionSignaturesRemainStable()
{
    using Controller = KoCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(currentCursorPosition, QPointF (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(ensureVisibleDoc, void (Controller::*)(const QRectF &, bool));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(preferredCenter, QPointF (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setPreferredCenter, void (Controller::*)(const QPointF &));
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerZoomSignaturesRemainStable()
{
    using Controller = KoCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setZoom, void (Controller::*)(KoZoomMode::Mode, qreal));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomIn, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomIn, void (Controller::*)(const KoViewTransformStillPoint &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomOut, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomOut, void (Controller::*)(const KoViewTransformStillPoint &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomState, KoZoomState (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(zoomTo, void (Controller::*)(const QRect &));
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerPanSignaturesRemainStable()
{
    using Controller = KoCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(pan, void (Controller::*)(const QPoint &));
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(panDown, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(panLeft, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(panRight, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(panUp, void (Controller::*)());
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerScrollBarSignaturesRemainStable()
{
    using Controller = KoCanvasController;
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(resetScrollBars, void (Controller::*)());
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(scrollBarValue, QPoint (Controller::*)() const);
    ASSERT_CANVAS_CONTROLLER_SIGNATURE(setScrollBarValue, void (Controller::*)(const QPoint &));
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerProxyIdentityAndAccessSignaturesRemainStable()
{
    using Proxy = KoCanvasControllerProxyObject;
    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<QObject, Proxy>);
    static_assert(std::is_constructible_v<Proxy, KoCanvasController *>);
    static_assert(std::is_constructible_v<Proxy, KoCanvasController *, QObject *>);
    static_assert(
        std::is_same_v<decltype(static_cast<KoCanvasController *(Proxy::*)() const>(&Proxy::canvasController)),
                       KoCanvasController *(Proxy::*)() const>);
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerProxyLifecycleNotificationsRemainStable()
{
    using Proxy = KoCanvasControllerProxyObject;
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(KoCanvasController *)>(&Proxy::emitCanvasRemoved)),
                       void (Proxy::*)(KoCanvasController *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(KoCanvasController *)>(&Proxy::emitCanvasSet)),
                                 void (Proxy::*)(KoCanvasController *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)()>(&Proxy::emitCanvasStateChanged)), void (Proxy::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(KoCanvasController *)>(&Proxy::canvasRemoved)),
                                 void (Proxy::*)(KoCanvasController *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(KoCanvasController *)>(&Proxy::canvasSet)),
                                 void (Proxy::*)(KoCanvasController *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)()>(&Proxy::canvasStateChanged)), void (Proxy::*)()>);
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerProxyPositionAndOffsetNotificationsRemainStable()
{
    using Proxy = KoCanvasControllerProxyObject;
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(const QPoint &)>(&Proxy::emitCanvasMousePositionChanged)),
                       void (Proxy::*)(const QPoint &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)()>(&Proxy::emitCanvasOffsetChanged)), void (Proxy::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QPointF &)>(
                                     &Proxy::emitDocumentMousePositionChanged)),
                                 void (Proxy::*)(const QPointF &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QPointF &, const QPointF &)>(
                                     &Proxy::emitMoveDocumentOffset)),
                                 void (Proxy::*)(const QPointF &, const QPointF &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(const QPoint &)>(&Proxy::canvasMousePositionChanged)),
                       void (Proxy::*)(const QPoint &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)()>(&Proxy::canvasOffsetChanged)), void (Proxy::*)()>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(const QPointF &)>(&Proxy::documentMousePositionChanged)),
                       void (Proxy::*)(const QPointF &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QPointF &, const QPointF &)>(
                                     &Proxy::moveDocumentOffset)),
                                 void (Proxy::*)(const QPointF &, const QPointF &)>);
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerProxyViewGeometryNotificationsRemainStable()
{
    using Proxy = KoCanvasControllerProxyObject;
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QSize &)>(&Proxy::emitSizeChanged)),
                                 void (Proxy::*)(const QSize &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QRectF &)>(
                                     &Proxy::emitDocumentRectInWidgetPixelsChanged)),
                                 void (Proxy::*)(const QRectF &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(qreal)>(&Proxy::emitDocumentRotationChanged)),
                                 void (Proxy::*)(qreal)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QSize &)>(&Proxy::sizeChanged)),
                                 void (Proxy::*)(const QSize &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const QRectF &)>(
                                     &Proxy::documentRectInWidgetPixelsChanged)),
                                 void (Proxy::*)(const QRectF &)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(qreal)>(&Proxy::documentRotationChanged)),
                                 void (Proxy::*)(qreal)>);
}

void KoCanvasControllerProxySchemaContractTest::canvasControllerProxyZoomAndMirrorNotificationsRemainStable()
{
    using Proxy = KoCanvasControllerProxyObject;
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(qreal)>(&Proxy::emitEffectiveZoomChanged)),
                                 void (Proxy::*)(qreal)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(const KoZoomState &)>(&Proxy::emitZoomStateChanged)),
                       void (Proxy::*)(const KoZoomState &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(bool, bool)>(&Proxy::emitDocumentMirrorStatusChanged)),
                       void (Proxy::*)(bool, bool)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(qreal)>(&Proxy::effectiveZoomChanged)),
                                 void (Proxy::*)(qreal)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Proxy::*)(const KoZoomState &)>(&Proxy::zoomStateChanged)),
                                 void (Proxy::*)(const KoZoomState &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Proxy::*)(bool, bool)>(&Proxy::documentMirrorStatusChanged)),
                       void (Proxy::*)(bool, bool)>);
}

QTEST_APPLESS_MAIN(KoCanvasControllerProxySchemaContractTest)

#include "KoCanvasControllerProxySchemaContractTest.moc"
