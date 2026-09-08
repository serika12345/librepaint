/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ui/workspace/KisMainWindow.h>
#include <ui/workspace/KisTemplateGroup.h>
#include <ui/workspace/KisWindowLayoutManager.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_MAIN_WINDOW_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisMainWindow::method)), signature>)
#define ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisWindowLayoutManager::method)), signature>)
#define ASSERT_TEMPLATE_GROUP_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTemplateGroup::method)), signature>)
} // namespace

class KisMainWindowSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeOpenFlagsAndLifetimeSchemaRemainStable();
    void identityViewAndCanvasSignaturesRemainStable();
    void documentOpenAndSaveSignaturesRemainStable();
    void dockerWorkspaceAndResourceSignaturesRemainStable();
    void presentationAndSessionActionSignaturesRemainStable();
    void viewLifecycleActionSignaturesRemainStable();
    void availabilityAndSynchronizationSignaturesRemainStable();
    void notificationSignaturesRemainStable();
    void windowLayoutManagerTypeAndLifetimeSchemaRemainStable();
    void windowLayoutDisplaySchemaRemainsStable();
    void windowLayoutDisplayLayoutSchemaRemainsStable();
    void primaryWorkspaceSignaturesRemainStable();
    void synchronizedDocumentAndLayoutSignaturesRemainStable();
    void templateGroupTypeAndLifetimeSchemaRemainStable();
    void templateGroupIdentityAndDirectorySignaturesRemainStable();
    void templateGroupSortingSignaturesRemainStable();
    void templateGroupVisibilitySignaturesRemainStable();
    void templateGroupCollectionSignaturesRemainStable();
};

void KisMainWindowSchemaContractTest::typeOpenFlagsAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMainWindow>);
    static_assert(std::is_same_v<KisMainWindow::OpenFlags, QFlags<KisMainWindow::OpenFlag>>);
    static_assert(std::is_enum_v<KisMainWindow::OpenFlag>);
    static_assert(KisMainWindow::None == 0);
    static_assert(KisMainWindow::Import == 0x1);
    static_assert(KisMainWindow::BatchMode == 0x2);
    static_assert(KisMainWindow::RecoveryFile == 0x4);
    static_assert(std::is_constructible_v<KisMainWindow, QUuid>);
    static_assert(std::has_virtual_destructor_v<KisMainWindow>);
}

void KisMainWindowSchemaContractTest::identityViewAndCanvasSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(createPopupMenu, QMenu * (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(id, QUuid (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(showView, void (KisMainWindow::*)(KisView *, QMdiSubWindow *));
    ASSERT_MAIN_WINDOW_SIGNATURE(activeView, KisView * (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(canvasObservers, QList<KoCanvasObserverBase *> (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(resourceManager, KoCanvasResourceProvider * (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(viewCount, int (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(viewManager, KisViewManager * (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(canvasWindow, QWidget * (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(canvasDetached, bool (KisMainWindow::*)() const);
}

void KisMainWindowSchemaContractTest::documentOpenAndSaveSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(openDocument, bool (KisMainWindow::*)(const QString &, KisMainWindow::OpenFlags));
    ASSERT_MAIN_WINDOW_SIGNATURE(showDocument, void (KisMainWindow::*)(KisDocument *));
    ASSERT_MAIN_WINDOW_SIGNATURE(saveDocument, bool (KisMainWindow::*)(KisDocument *, bool, bool, bool));
    ASSERT_MAIN_WINDOW_SIGNATURE(addViewAndNotifyLoadingCompleted,
                                 KisView * (KisMainWindow::*)(KisDocument *, QMdiSubWindow *));
    ASSERT_MAIN_WINDOW_SIGNATURE(showOpenFileDialog, QStringList (KisMainWindow::*)(bool));
    ASSERT_MAIN_WINDOW_SIGNATURE(hackIsSaving, bool (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(installBundle, bool (KisMainWindow::*)(const QString &) const);
    ASSERT_MAIN_WINDOW_SIGNATURE(layoutThumbnail, QImage (KisMainWindow::*)());
}

void KisMainWindowSchemaContractTest::dockerWorkspaceAndResourceSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(dockWidgets, QList<QDockWidget *> (KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(dockWidget, QDockWidget * (KisMainWindow::*)(const QString &));
    ASSERT_MAIN_WINDOW_SIGNATURE(saveWindowState, void (KisMainWindow::*)(bool));
    ASSERT_MAIN_WINDOW_SIGNATURE(windowStateConfig, const KConfigGroup &(KisMainWindow::*)() const);
    ASSERT_MAIN_WINDOW_SIGNATURE(restoreWorkspace, bool (KisMainWindow::*)(KoResourceSP));
    ASSERT_MAIN_WINDOW_SIGNATURE(restoreWorkspaceState, bool (KisMainWindow::*)(const QByteArray &));
    ASSERT_MAIN_WINDOW_SIGNATURE(swapWorkspaces, void (*)(KisMainWindow *, KisMainWindow *));
}

void KisMainWindowSchemaContractTest::presentationAndSessionActionSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(showWelcomeScreen, void (KisMainWindow::*)(bool));
    ASSERT_MAIN_WINDOW_SIGNATURE(clearRecentFiles, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(removeRecentFile, void (KisMainWindow::*)(QString));
    ASSERT_MAIN_WINDOW_SIGNATURE(slotFileNew, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(slotFileOpen, void (KisMainWindow::*)(bool));
    ASSERT_MAIN_WINDOW_SIGNATURE(slotFileOpenRecent, void (KisMainWindow::*)(const QUrl &));
    ASSERT_MAIN_WINDOW_SIGNATURE(slotPreferences, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(slotFileSave, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(slotChangeInterfaceScale, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(slotShowSessionManager, void (KisMainWindow::*)());
#ifdef Q_OS_ANDROID
    ASSERT_MAIN_WINDOW_SIGNATURE(slotFlashWindowHack, void (KisMainWindow::*)());
#endif
    ASSERT_MAIN_WINDOW_SIGNATURE(setCanvasDetached, void (KisMainWindow::*)(bool));
    ASSERT_MAIN_WINDOW_SIGNATURE(viewFullscreen, void (KisMainWindow::*)(bool));
}

void KisMainWindowSchemaContractTest::viewLifecycleActionSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(newOptionWidgets,
                                 void (KisMainWindow::*)(KoCanvasController *, const QList<QPointer<QWidget>> &));
    ASSERT_MAIN_WINDOW_SIGNATURE(newView, KisView * (KisMainWindow::*)(QObject *, QMdiSubWindow *));
    ASSERT_MAIN_WINDOW_SIGNATURE(notifyChildViewDestroyed, void (KisMainWindow::*)(KisView *));
    ASSERT_MAIN_WINDOW_SIGNATURE(setActiveView, void (KisMainWindow::*)(KisView *));
    ASSERT_MAIN_WINDOW_SIGNATURE(unsetActiveView, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(subWindowActivated, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(windowFocused, void (KisMainWindow::*)());
}

void KisMainWindowSchemaContractTest::availabilityAndSynchronizationSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(checkActiveBundlesAvailable, bool (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(checkPaintOpAvailable, bool (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(synchronizeDynamicActions, void (KisMainWindow::*)());
}

void KisMainWindowSchemaContractTest::notificationSignaturesRemainStable()
{
    ASSERT_MAIN_WINDOW_SIGNATURE(documentSaved, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(loadCompleted, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(restoringDone, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(themeChanged, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(keyBindingsChanged, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(guiLoadingFinished, void (KisMainWindow::*)());
    ASSERT_MAIN_WINDOW_SIGNATURE(activeViewChanged, void (KisMainWindow::*)());
}

void KisMainWindowSchemaContractTest::windowLayoutManagerTypeAndLifetimeSchemaRemainStable()
{
    using Manager = KisWindowLayoutManager;
    using InstanceSignature = Manager *(*)();

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_default_constructible_v<Manager>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(static_cast<InstanceSignature>(&Manager::instance)), InstanceSignature>);
}

void KisMainWindowSchemaContractTest::windowLayoutDisplaySchemaRemainsStable()
{
    using Display = KisWindowLayoutManager::Display;

    static_assert(std::is_class_v<Display>);
    static_assert(std::is_same_v<decltype(&Display::resolution), QSize Display::*>);
    static_assert(std::is_same_v<decltype(&Display::matches), bool (Display::*)(QScreen *) const>);
}

void KisMainWindowSchemaContractTest::windowLayoutDisplayLayoutSchemaRemainsStable()
{
    using Display = KisWindowLayoutManager::Display;
    using DisplayLayout = KisWindowLayoutManager::DisplayLayout;

    static_assert(std::is_class_v<DisplayLayout>);
    static_assert(std::is_same_v<decltype(&DisplayLayout::name), QString DisplayLayout::*>);
    static_assert(std::is_same_v<decltype(&DisplayLayout::displays), QVector<Display> DisplayLayout::*>);
    static_assert(std::is_same_v<decltype(&DisplayLayout::preferredWindowLayout), QString DisplayLayout::*>);
    static_assert(std::is_same_v<decltype(&DisplayLayout::matches), bool (DisplayLayout::*)(QList<QScreen *>) const>);
}

void KisMainWindowSchemaContractTest::primaryWorkspaceSignaturesRemainStable()
{
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(primaryWorkspaceFollowsFocus, bool (KisWindowLayoutManager::*)() const);
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(setPrimaryWorkspaceFollowsFocus,
                                           void (KisWindowLayoutManager::*)(bool, QUuid));
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(primaryWindowId, QUuid (KisWindowLayoutManager::*)() const);
}

void KisMainWindowSchemaContractTest::synchronizedDocumentAndLayoutSignaturesRemainStable()
{
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(isShowImageInAllWindowsEnabled, bool (KisWindowLayoutManager::*)() const);
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(setShowImageInAllWindowsEnabled, void (KisWindowLayoutManager::*)(bool));
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(activeDocumentChanged, void (KisWindowLayoutManager::*)(KisDocument *));
    ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE(setLastUsedLayout,
                                           void (KisWindowLayoutManager::*)(KisWindowLayoutResource *));
}

void KisMainWindowSchemaContractTest::templateGroupTypeAndLifetimeSchemaRemainStable()
{
    using Group = KisTemplateGroup;

    static_assert(std::is_class_v<Group>);
    static_assert(std::is_constructible_v<Group, const QString &, const QString &, int, bool>);
    static_assert(std::is_destructible_v<Group>);
}

void KisMainWindowSchemaContractTest::templateGroupIdentityAndDirectorySignaturesRemainStable()
{
    using Group = KisTemplateGroup;

    ASSERT_TEMPLATE_GROUP_SIGNATURE(name, QString (Group::*)() const);
    ASSERT_TEMPLATE_GROUP_SIGNATURE(dirs, QStringList (Group::*)() const);
    ASSERT_TEMPLATE_GROUP_SIGNATURE(addDir, void (Group::*)(const QString &));
    ASSERT_TEMPLATE_GROUP_SIGNATURE(touched, bool (Group::*)() const);
}

void KisMainWindowSchemaContractTest::templateGroupSortingSignaturesRemainStable()
{
    using Group = KisTemplateGroup;

    ASSERT_TEMPLATE_GROUP_SIGNATURE(sortingWeight, int (Group::*)() const);
    ASSERT_TEMPLATE_GROUP_SIGNATURE(setSortingWeight, void (Group::*)(int));
}

void KisMainWindowSchemaContractTest::templateGroupVisibilitySignaturesRemainStable()
{
    using Group = KisTemplateGroup;

    ASSERT_TEMPLATE_GROUP_SIGNATURE(isHidden, bool (Group::*)() const);
    ASSERT_TEMPLATE_GROUP_SIGNATURE(setHidden, void (Group::*)(bool) const);
}

void KisMainWindowSchemaContractTest::templateGroupCollectionSignaturesRemainStable()
{
    using Group = KisTemplateGroup;

    ASSERT_TEMPLATE_GROUP_SIGNATURE(templates, QList<KisTemplate *> (Group::*)() const);
    ASSERT_TEMPLATE_GROUP_SIGNATURE(add, bool (Group::*)(KisTemplate *, bool, bool));
    ASSERT_TEMPLATE_GROUP_SIGNATURE(find, KisTemplate * (Group::*)(const QString &) const);
}

#undef ASSERT_MAIN_WINDOW_SIGNATURE
#undef ASSERT_WINDOW_LAYOUT_MANAGER_SIGNATURE
#undef ASSERT_TEMPLATE_GROUP_SIGNATURE

QTEST_GUILESS_MAIN(KisMainWindowSchemaContractTest)

#include "KisMainWindowSchemaContractTest.moc"
