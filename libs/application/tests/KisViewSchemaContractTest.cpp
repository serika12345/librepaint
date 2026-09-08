/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/kis_zoom_manager.h>
#include <ui/workspace/KisView.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_VIEW_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisView::method)), signature>)
#define ASSERT_ZOOM_MANAGER_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisZoomManager::method)), signature>)
} // namespace

class KisViewSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeSharedOwnershipAndViewManagerSchemaRemainStable();
    void documentWindowAndStatusSignaturesRemainStable();
    void canvasInputAndImageSignaturesRemainStable();
    void currentContentAndProofingSignaturesRemainStable();
    void floatingPresentationAndViewStateSignaturesRemainStable();
    void presentationAndLoadingSlotSignaturesRemainStable();
    void notificationSignaturesRemainStable();
    void zoomManagerTypeAndLifetimeSchemaRemainStable();
    void zoomManagerSetupAndImageSynchronizationSignaturesRemainStable();
    void zoomManagerPresentationObjectSignaturesRemainStable();
    void zoomManagerRulerControlSignaturesRemainStable();
    void zoomManagerActionSlotSignaturesRemainStable();
};

void KisViewSchemaContractTest::typeSharedOwnershipAndViewManagerSchemaRemainStable()
{
    static_assert(std::is_class_v<KisView>);
    static_assert(std::is_same_v<KisImageWSP, KisWeakSharedPtr<KisImage>>);
    static_assert(std::is_same_v<KisLayerSP, KisSharedPtr<KisLayer>>);
    static_assert(std::is_same_v<KisMaskSP, KisSharedPtr<KisMask>>);
    static_assert(std::is_same_v<KisNodeSP, KisSharedPtr<KisNode>>);
    static_assert(std::is_same_v<KisSelectionSP, KisSharedPtr<KisSelection>>);
    static_assert(std::is_constructible_v<KisView, KisDocument *, KisViewManager *, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisView>);
    ASSERT_VIEW_SIGNATURE(setViewManager, void (KisView::*)(KisViewManager *));
    ASSERT_VIEW_SIGNATURE(viewManager, KisViewManager * (KisView::*)() const);
}

void KisViewSchemaContractTest::documentWindowAndStatusSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(document, KisDocument * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(replaceBy, KisView * (KisView::*)(KisDocument *));
    ASSERT_VIEW_SIGNATURE(mainWindow, KisMainWindow * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(setSubWindow, void (KisView::*)(QMdiSubWindow *));
    ASSERT_VIEW_SIGNATURE(statusBar, QStatusBar * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(addStatusBarItem, void (KisView::*)(QWidget *, int, bool));
    ASSERT_VIEW_SIGNATURE(removeStatusBarItem, void (KisView::*)(QWidget *));
    ASSERT_VIEW_SIGNATURE(createChangeUnitActions, QList<QAction *> (KisView::*)(bool));
    ASSERT_VIEW_SIGNATURE(closeView, void (KisView::*)());
    ASSERT_VIEW_SIGNATURE(currentScreen, QScreen * (KisView::*)() const);
}

void KisViewSchemaContractTest::canvasInputAndImageSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(zoomManager, KisZoomManager * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(canvasController, KisCanvasController * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(resourceProvider, KisCanvasResourceProvider * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(globalInputManager, KisInputManager * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(canvasBase, KisCanvas2 * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(image, KisImageWSP (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(viewConverter, KisCoordinatesConverter * (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(canvasIsMirrored, bool (KisView::*)() const);
}

void KisViewSchemaContractTest::currentContentAndProofingSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(setCurrentNode, void (KisView::*)(KisNodeSP));
    ASSERT_VIEW_SIGNATURE(currentNode, KisNodeSP (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(currentLayer, KisLayerSP (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(currentMask, KisMaskSP (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(softProofing, bool (KisView::*)());
    ASSERT_VIEW_SIGNATURE(gamutCheck, bool (KisView::*)());
    ASSERT_VIEW_SIGNATURE(selection, KisSelectionSP (KisView::*)());
    ASSERT_VIEW_SIGNATURE(notifyCurrentStateChanged, void (KisView::*)(bool));
    ASSERT_VIEW_SIGNATURE(isCurrent, bool (KisView::*)() const);
    ASSERT_VIEW_SIGNATURE(syncLastActiveNodeToDocument, void (KisView::*)());
}

void KisViewSchemaContractTest::floatingPresentationAndViewStateSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(setShowFloatingMessage, void (KisView::*)(bool));
    ASSERT_VIEW_SIGNATURE(showFloatingMessage,
                          void (KisView::*)(const QString &, const QIcon &, int, KisFloatingMessage::Priority, int));
    ASSERT_VIEW_SIGNATURE(saveViewState, void (KisView::*)(KisPropertiesConfiguration &) const);
    ASSERT_VIEW_SIGNATURE(restoreViewState, void (KisView::*)(const KisPropertiesConfiguration &));
}

void KisViewSchemaContractTest::presentationAndLoadingSlotSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(slotSavingStatusMessage, void (KisView::*)(const QString &, int, bool));
    ASSERT_VIEW_SIGNATURE(slotClearStatusText, void (KisView::*)());
    ASSERT_VIEW_SIGNATURE(slotSoftProofing, void (KisView::*)(bool));
    ASSERT_VIEW_SIGNATURE(slotGamutCheck, void (KisView::*)(bool));
    ASSERT_VIEW_SIGNATURE(queryClose, bool (KisView::*)());
    ASSERT_VIEW_SIGNATURE(slotMigratedToScreen, void (KisView::*)(QScreen *));
    ASSERT_VIEW_SIGNATURE(slotScreenOrResolutionChanged, void (KisView::*)());
    ASSERT_VIEW_SIGNATURE(slotThemeChanged, void (KisView::*)(QPalette));
    ASSERT_VIEW_SIGNATURE(slotUpdateDocumentTitle, void (KisView::*)());
    ASSERT_VIEW_SIGNATURE(slotLoadingFinished, void (KisView::*)());
    ASSERT_VIEW_SIGNATURE(slotImageResolutionChanged, void (KisView::*)());
    ASSERT_VIEW_SIGNATURE(slotImageSizeChanged, void (KisView::*)(const QPointF &, const QPointF &));
}

void KisViewSchemaContractTest::notificationSignaturesRemainStable()
{
    ASSERT_VIEW_SIGNATURE(sigSizeChanged, void (KisView::*)(const QPointF &, const QPointF &));
    ASSERT_VIEW_SIGNATURE(sigProfileChanged, void (KisView::*)(const KoColorProfile *));
    ASSERT_VIEW_SIGNATURE(sigColorSpaceChanged, void (KisView::*)(const KoColorSpace *));
}

void KisViewSchemaContractTest::zoomManagerTypeAndLifetimeSchemaRemainStable()
{
    using Manager = KisZoomManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_constructible_v<Manager, QPointer<KisView>, KoZoomHandler *, KoCanvasController *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
}

void KisViewSchemaContractTest::zoomManagerSetupAndImageSynchronizationSignaturesRemainStable()
{
    ASSERT_ZOOM_MANAGER_SIGNATURE(setup, void (KisZoomManager::*)(KisKActionCollection *));
    ASSERT_ZOOM_MANAGER_SIGNATURE(updateImageBoundsSnapping, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(syncOnImageResolutionChange, void (KisZoomManager::*)());
}

void KisViewSchemaContractTest::zoomManagerPresentationObjectSignaturesRemainStable()
{
    ASSERT_ZOOM_MANAGER_SIGNATURE(zoomAction, KoZoomAction * (KisZoomManager::*)() const);
    ASSERT_ZOOM_MANAGER_SIGNATURE(zoomActionWidget, QWidget * (KisZoomManager::*)() const);
    ASSERT_ZOOM_MANAGER_SIGNATURE(horizontalRuler, KoRuler * (KisZoomManager::*)() const);
    ASSERT_ZOOM_MANAGER_SIGNATURE(verticalRuler, KoRuler * (KisZoomManager::*)() const);
}

void KisViewSchemaContractTest::zoomManagerRulerControlSignaturesRemainStable()
{
    ASSERT_ZOOM_MANAGER_SIGNATURE(setShowRulers, void (KisZoomManager::*)(bool));
    ASSERT_ZOOM_MANAGER_SIGNATURE(setRulersTrackMouse, void (KisZoomManager::*)(bool));
    ASSERT_ZOOM_MANAGER_SIGNATURE(mousePositionChanged, void (KisZoomManager::*)(const QPoint &));
    ASSERT_ZOOM_MANAGER_SIGNATURE(applyRulersUnit, void (KisZoomManager::*)(const KoUnit &));
    ASSERT_ZOOM_MANAGER_SIGNATURE(setRulersPixelMultiple2, void (KisZoomManager::*)(bool));
}

void KisViewSchemaContractTest::zoomManagerActionSlotSignaturesRemainStable()
{
    ASSERT_ZOOM_MANAGER_SIGNATURE(zoomTo100, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(slotZoomIn, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(slotZoomOut, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(slotZoomToFit, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(slotZoomToFitWidth, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(slotZoomToFitHeight, void (KisZoomManager::*)());
    ASSERT_ZOOM_MANAGER_SIGNATURE(slotToggleZoomToFit, void (KisZoomManager::*)());
}

#undef ASSERT_VIEW_SIGNATURE
#undef ASSERT_ZOOM_MANAGER_SIGNATURE

QTEST_GUILESS_MAIN(KisViewSchemaContractTest)

#include "KisViewSchemaContractTest.moc"
