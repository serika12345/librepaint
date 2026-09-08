/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/kis_statusbar.h>
#include <ui/workspace/KisViewManager.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_VIEW_MANAGER_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisViewManager::method)), signature>)
#define ASSERT_STATUS_BAR_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisStatusBar::method)), signature>)
} // namespace

class KisViewManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndViewConnectionSchemaRemainStable();
    void canvasImageAndCurrentContentSignaturesRemainStable();
    void managerAccessSignaturesRemainStable();
    void progressOperationAndControlSignaturesRemainStable();
    void windowResourceAndToolSignaturesRemainStable();
    void floatingPresentationAndGuiUpdateSignaturesRemainStable();
    void viewAuthorAndToolActionSignaturesRemainStable();
    void notificationSignaturesRemainStable();
    void statusBarTypeAndLifetimeSchemaRemainStable();
    void statusBarSetupAndPresentationSignaturesRemainStable();
    void statusBarProgressAndExtraWidgetSignaturesRemainStable();
    void statusBarImageSelectionAndProfileSignaturesRemainStable();
    void statusBarHelpUpdateAndNotificationSignaturesRemainStable();
};

void KisViewManagerSchemaContractTest::typeConstructionAndViewConnectionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisViewManager>);
    static_assert(std::is_same_v<KisPaintDeviceSP, KisSharedPtr<KisPaintDevice>>);
    static_assert(std::is_constructible_v<KisViewManager, QWidget *, KisKActionCollection *>);
    static_assert(std::has_virtual_destructor_v<KisViewManager>);
    ASSERT_VIEW_MANAGER_SIGNATURE(actionCollection, KisKActionCollection * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(setCurrentView, void (KisViewManager::*)(KisView *));
    ASSERT_VIEW_MANAGER_SIGNATURE(viewCount, int (KisViewManager::*)() const);
}

void KisViewManagerSchemaContractTest::canvasImageAndCurrentContentSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(image, KisImageWSP (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(canvasResourceProvider, KisCanvasResourceProvider * (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(canvasBase, KisCanvas2 * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(canvas, QWidget * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(statusBar, KisStatusBar * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(paintOpBox, KisPaintopBox * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(activeNode, KisNodeSP (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(activeNodeIsAnimated, bool (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(activeLayer, KisLayerSP (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(activeDevice, KisPaintDeviceSP (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(selection, KisSelectionSP (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(selectionEditable, bool (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(document, KisDocument * (KisViewManager::*)() const);
}

void KisViewManagerSchemaContractTest::managerAccessSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(selectionManager, KisSelectionManager * (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(nodeManager, KisNodeManager * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(actionManager, KisActionManager * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(filterManager, KisFilterManager * (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(imageManager, KisImageManager * (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(inputManager, KisInputManager * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(idleTasksManager, KisIdleTasksManager * (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(textPropertyManager, KisTextPropertiesManager * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(undoAdapter, KisUndoAdapter * (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(gridManager, KisGridManager * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(guidesManager, KisGuidesManager * (KisViewManager::*)() const);
}

void KisViewManagerSchemaContractTest::progressOperationAndControlSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(createUnthreadedUpdater, QPointer<KoUpdater> (KisViewManager::*)(const QString &));
    ASSERT_VIEW_MANAGER_SIGNATURE(createThreadedUpdater, QPointer<KoUpdater> (KisViewManager::*)(const QString &));
    ASSERT_VIEW_MANAGER_SIGNATURE(blockUntilOperationsFinished, bool (KisViewManager::*)(KisImageSP));
    ASSERT_VIEW_MANAGER_SIGNATURE(blockUntilOperationsFinishedForced, void (KisViewManager::*)(KisImageSP));
    ASSERT_VIEW_MANAGER_SIGNATURE(notifyWorkspaceLoaded, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(enableControls, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(disableControls, void (KisViewManager::*)());
}

void KisViewManagerSchemaContractTest::windowResourceAndToolSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(mainWindow, KisMainWindow * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(mainWindowAsQWidget, QWidget * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(qtMainWindow, QMainWindow * (KisViewManager::*)() const);
    ASSERT_VIEW_MANAGER_SIGNATURE(setQtMainWindow, void (KisViewManager::*)(QMainWindow *));
    ASSERT_VIEW_MANAGER_SIGNATURE(initializeResourceManager, void (*)(KoCanvasResourceProvider *));
    ASSERT_VIEW_MANAGER_SIGNATURE(testingInitializeOpacityToPresetResourceConverter,
                                  void (*)(KoCanvasResourceProvider *));
    ASSERT_VIEW_MANAGER_SIGNATURE(activateTransformToolWithExternalSource, void (KisViewManager::*)(KisPaintDeviceSP));
}

void KisViewManagerSchemaContractTest::floatingPresentationAndGuiUpdateSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(
        showFloatingMessage,
        void (KisViewManager::*)(const QString &, const QIcon &, int, KisFloatingMessage::Priority, int));
    ASSERT_VIEW_MANAGER_SIGNATURE(showFloatingZoomMessage, void (KisViewManager::*)(const QString &));
    ASSERT_VIEW_MANAGER_SIGNATURE(showFloatingRotationMessage, void (KisViewManager::*)(const QString &));
    ASSERT_VIEW_MANAGER_SIGNATURE(setShowFloatingMessage, void (KisViewManager::*)(bool));
    ASSERT_VIEW_MANAGER_SIGNATURE(switchCanvasOnly, void (KisViewManager::*)(bool));
    ASSERT_VIEW_MANAGER_SIGNATURE(showHideScrollbars, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(updateGUI, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(updateIcons, void (KisViewManager::*)());
}

void KisViewManagerSchemaContractTest::viewAuthorAndToolActionSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(slotViewAdded, void (KisViewManager::*)(KisView *));
    ASSERT_VIEW_MANAGER_SIGNATURE(slotViewRemoved, void (KisViewManager::*)(KisView *));
    ASSERT_VIEW_MANAGER_SIGNATURE(slotActivateTransformTool, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(changeAuthorProfile, void (KisViewManager::*)(const QString &));
    ASSERT_VIEW_MANAGER_SIGNATURE(slotUpdateAuthorProfileActions, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(slotToggleFgBg, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(slotToggleBrushOutline, void (KisViewManager::*)());
}

void KisViewManagerSchemaContractTest::notificationSignaturesRemainStable()
{
    ASSERT_VIEW_MANAGER_SIGNATURE(floatingMessageRequested, void (KisViewManager::*)(const QString &, const QString &));
    ASSERT_VIEW_MANAGER_SIGNATURE(viewChanged, void (KisViewManager::*)());
    ASSERT_VIEW_MANAGER_SIGNATURE(brushOutlineToggled, void (KisViewManager::*)());
}

void KisViewManagerSchemaContractTest::statusBarTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisStatusBar>);
    static_assert(std::is_constructible_v<KisStatusBar, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<KisStatusBar>);
}

void KisViewManagerSchemaContractTest::statusBarSetupAndPresentationSignaturesRemainStable()
{
    ASSERT_STATUS_BAR_SIGNATURE(setup, void (KisStatusBar::*)());
    ASSERT_STATUS_BAR_SIGNATURE(setView, void (KisStatusBar::*)(QPointer<KisView>));
    ASSERT_STATUS_BAR_SIGNATURE(hideAllStatusBarItems, void (KisStatusBar::*)());
    ASSERT_STATUS_BAR_SIGNATURE(showAllStatusBarItems, void (KisStatusBar::*)());
}

void KisViewManagerSchemaContractTest::statusBarProgressAndExtraWidgetSignaturesRemainStable()
{
    ASSERT_STATUS_BAR_SIGNATURE(progressUpdater, KoProgressUpdater * (KisStatusBar::*)());
    ASSERT_STATUS_BAR_SIGNATURE(addExtraWidget, void (KisStatusBar::*)(QWidget *));
    ASSERT_STATUS_BAR_SIGNATURE(removeExtraWidget, void (KisStatusBar::*)(QWidget *));
}

void KisViewManagerSchemaContractTest::statusBarImageSelectionAndProfileSignaturesRemainStable()
{
    ASSERT_STATUS_BAR_SIGNATURE(imageSizeChanged, void (KisStatusBar::*)());
    ASSERT_STATUS_BAR_SIGNATURE(setSelection, void (KisStatusBar::*)(KisImageWSP));
    ASSERT_STATUS_BAR_SIGNATURE(setProfile, void (KisStatusBar::*)(KisImageWSP));
}

void KisViewManagerSchemaContractTest::statusBarHelpUpdateAndNotificationSignaturesRemainStable()
{
    ASSERT_STATUS_BAR_SIGNATURE(setHelp, void (KisStatusBar::*)(const QString &));
    ASSERT_STATUS_BAR_SIGNATURE(updateStatusBarProfileLabel, void (KisStatusBar::*)());
    ASSERT_STATUS_BAR_SIGNATURE(updateSelectionToolTip, void (KisStatusBar::*)());
    ASSERT_STATUS_BAR_SIGNATURE(sigCancellationRequested, void (KisStatusBar::*)());
}

#undef ASSERT_VIEW_MANAGER_SIGNATURE
#undef ASSERT_STATUS_BAR_SIGNATURE

QTEST_GUILESS_MAIN(KisViewManagerSchemaContractTest)

#include "KisViewManagerSchemaContractTest.moc"
