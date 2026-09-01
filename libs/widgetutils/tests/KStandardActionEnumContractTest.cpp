/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <kstandardaction.h>

#include "../xmlgui/kactioncollection.h"

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

namespace
{

using StandardAction = KStandardAction::StandardAction;
using ReceiverActionSignature = QAction *(*)(const QObject *, const char *, QObject *);
using ParentActionSignature = QAction *(*)(QObject *);
using CreateSignature = QAction *(*)(StandardAction, const QObject *, const char *, QObject *);
using NameSignature = const char *(*)(StandardAction);
using NamesSignature = QStringList (*)();
using ActionIdsSignature = QList<StandardAction> (*)();
using ShortcutSignature = KStandardShortcut::StandardShortcut (*)(StandardAction);
using RecentActionSignature = KRecentFilesAction *(*)(const QObject *, const char *, QObject *);
using ToggleActionSignature = KToggleAction *(*)(const QObject *, const char *, QObject *);
using FullScreenActionSignature = KToggleFullScreenAction *(*)(const QObject *, const char *, QWidget *, QObject *);

using ActionCollection = KisKActionCollection;
using ActionCollectionAllCollectionsSignature = const QList<ActionCollection *> &(*)();
using ActionCollectionVoidSignature = void (ActionCollection::*)();
using ActionCollectionStringSignature = QString (ActionCollection::*)() const;
using ActionCollectionSetStringSignature = void (ActionCollection::*)(const QString &);
using ActionCollectionParentClientSignature = const KisKXMLGUIClient *(ActionCollection::*)() const;
using ActionCollectionWriteSettingsSignature = void (ActionCollection::*)(KConfigGroup *, bool, QAction *) const;
using ActionCollectionCountSignature = int (ActionCollection::*)() const;
using ActionCollectionEmptySignature = bool (ActionCollection::*)() const;
using ActionCollectionActionIndexSignature = QAction *(ActionCollection::*)(int) const;
using ActionCollectionActionNameSignature = QAction *(ActionCollection::*)(const QString &) const;
using ActionCollectionActionsSignature = QList<QAction *> (ActionCollection::*)() const;
using ActionCollectionUngroupedActionsSignature = const QList<QAction *> (ActionCollection::*)() const;
using ActionCollectionActionGroupsSignature = const QList<QActionGroup *> (ActionCollection::*)() const;
using ActionCollectionCategoriesSignature = QList<KisKActionCategory *> (ActionCollection::*)() const;
using ActionCollectionCategorySignature = KisKActionCategory *(ActionCollection::*)(const QString &);
using ActionCollectionTemplateAddSignature = QAction *(ActionCollection::*)(const QString &,
                                                                            const QObject *,
                                                                            const char *);
using ActionCollectionStandardAddSignature = QAction *(ActionCollection::*)(StandardAction,
                                                                            const QObject *,
                                                                            const char *);
using ActionCollectionNamedStandardAddSignature = QAction *(ActionCollection::*)(StandardAction,
                                                                                 const QString &,
                                                                                 const QObject *,
                                                                                 const char *);
using ActionCollectionActionAddSignature = QAction *(ActionCollection::*)(const QString &, QAction *);
using ActionCollectionReceiverAddSignature = QAction *(ActionCollection::*)(const QString &,
                                                                            const QObject *,
                                                                            const char *);
using ActionCollectionActionsAddSignature = void (ActionCollection::*)(const QList<QAction *> &);
using ActionCollectionCategorizedAddSignature = QAction *(ActionCollection::*)(const QString &,
                                                                               QAction *,
                                                                               const QString &);
using ActionCollectionActionMutationSignature = void (ActionCollection::*)(QAction *);
using ActionCollectionTakeActionSignature = QAction *(ActionCollection::*)(QAction *);
using ActionCollectionDefaultShortcutSignature = QKeySequence (ActionCollection::*)(QAction *) const;
using ActionCollectionDefaultShortcutsSignature = QList<QKeySequence> (ActionCollection::*)(QAction *) const;
using ActionCollectionSetDefaultShortcutSignature = void (ActionCollection::*)(QAction *, const QKeySequence &);
using ActionCollectionSetDefaultShortcutsSignature = void (ActionCollection::*)(QAction *, const QList<QKeySequence> &);
using ActionCollectionShortcutConfigurableSignature = bool (ActionCollection::*)(QAction *) const;
using ActionCollectionSetShortcutConfigurableSignature = void (ActionCollection::*)(QAction *, bool);
using ActionCollectionAssociateWidgetSignature = void (ActionCollection::*)(QWidget *) const;
using ActionCollectionMutateWidgetSignature = void (ActionCollection::*)(QWidget *);
using ActionCollectionAssociatedWidgetsSignature = QList<QWidget *> (ActionCollection::*)() const;
using ActionCollectionNotificationSignature = void (ActionCollection::*)(QAction *);

#define ASSERT_STANDARD_ACTION_SIGNATURE(functionName, signatureType)                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signatureType>(&KStandardAction::functionName)), signatureType>)

#define ASSERT_ACTION_COLLECTION_SIGNATURE(functionName, signatureType)                                                \
    static_assert(std::is_same_v<decltype(static_cast<signatureType>(&ActionCollection::functionName)), signatureType>)

} // namespace

class KStandardActionEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void standardActionIdsRemainStable();
    void standardActionIdentityAndFileSignaturesRemainStable();
    void standardActionEditSignaturesRemainStable();
    void standardActionViewNavigationSignaturesRemainStable();
    void standardActionToolAndSettingsSignaturesRemainStable();
    void standardActionHelpSignaturesRemainStable();
    void actionCollectionIdentityAndConfigurationSignaturesRemainStable();
    void actionCollectionLookupAndClassificationSignaturesRemainStable();
    void actionCollectionMembershipMutationSignaturesRemainStable();
    void actionCollectionShortcutSignaturesRemainStable();
    void actionCollectionAssociationAndNotificationSignaturesRemainStable();
};

void KStandardActionEnumContractTest::standardActionIdsRemainStable()
{
    using namespace KStandardAction;
    const std::array<StandardAction, 67> actions {{
        ActionNone,
        New, Open, OpenRecent, Save, SaveAs, Revert, Close,
        Print, PrintPreview, Mail, Quit,
        Undo, Redo, Cut, Copy, Paste, SelectAll, Deselect, Find, FindNext, FindPrev,
        Replace,
        ActualSize, FitToPage, FitToWidth, FitToHeight, ZoomIn, ZoomOut,
        Zoom, Redisplay,
        Up, Back, Forward, Home, Prior, Next, Goto, GotoPage, GotoLine,
        FirstPage, LastPage, DocumentBack, DocumentForward,
        AddBookmark, EditBookmarks,
        Spelling,
        ShowMenubar, ShowToolbar, ShowStatusbar, SaveOptions, KeyBindings,
        Preferences, ConfigureToolbars, ResetConfigurations,
        Help, HelpContents, WhatsThis, ReportBug, AboutApp, AboutKDE, TipofDay,
        ConfigureNotifications, FullScreen, Clear, PasteText,
        SwitchApplicationLanguage,
    }};

    for (std::size_t index = 0; index < actions.size(); ++index) {
        QCOMPARE(int(actions[index]), int(index));
    }
}

void KStandardActionEnumContractTest::standardActionIdentityAndFileSignaturesRemainStable()
{
    ASSERT_STANDARD_ACTION_SIGNATURE(actionIds, ActionIdsSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(create, CreateSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(name, NameSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(shortcutForActionId, ShortcutSignature);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    ASSERT_STANDARD_ACTION_SIGNATURE(stdName, NameSignature);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    ASSERT_STANDARD_ACTION_SIGNATURE(stdNames, NamesSignature);

    ASSERT_STANDARD_ACTION_SIGNATURE(openNew, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(open, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(openRecent, RecentActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(save, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(saveAs, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(revert, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(close, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(print, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(printPreview, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(mail, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(quit, ReceiverActionSignature);
}

void KStandardActionEnumContractTest::standardActionEditSignaturesRemainStable()
{
    ASSERT_STANDARD_ACTION_SIGNATURE(undo, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(redo, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(cut, ParentActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(cut, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(copy, ParentActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(copy, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(paste, ParentActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(paste, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(pasteText, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(clear, ParentActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(clear, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(selectAll, ParentActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(selectAll, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(deselect, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(find, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(findNext, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(findPrev, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(replace, ReceiverActionSignature);
}

void KStandardActionEnumContractTest::standardActionViewNavigationSignaturesRemainStable()
{
    ASSERT_STANDARD_ACTION_SIGNATURE(actualSize, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(fitToPage, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(fitToWidth, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(fitToHeight, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(zoomIn, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(zoomOut, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(zoom, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(redisplay, ReceiverActionSignature);

    ASSERT_STANDARD_ACTION_SIGNATURE(up, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(back, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(forward, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(home, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(prior, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(next, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(goTo, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(gotoPage, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(gotoLine, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(firstPage, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(lastPage, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(documentBack, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(documentForward, ReceiverActionSignature);

    ASSERT_STANDARD_ACTION_SIGNATURE(addBookmark, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(editBookmarks, ReceiverActionSignature);
}

void KStandardActionEnumContractTest::standardActionToolAndSettingsSignaturesRemainStable()
{
    ASSERT_STANDARD_ACTION_SIGNATURE(spelling, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(showMenubar, ToggleActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(showStatusbar, ToggleActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(fullScreen, FullScreenActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(saveOptions, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(keyBindings, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(preferences, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(configureToolbars, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(configureNotifications, ReceiverActionSignature);
}

void KStandardActionEnumContractTest::standardActionHelpSignaturesRemainStable()
{
    ASSERT_STANDARD_ACTION_SIGNATURE(help, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(helpContents, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(whatsThis, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(tipOfDay, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(reportBug, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(aboutApp, ReceiverActionSignature);
    ASSERT_STANDARD_ACTION_SIGNATURE(aboutKDE, ReceiverActionSignature);
}

void KStandardActionEnumContractTest::actionCollectionIdentityAndConfigurationSignaturesRemainStable()
{
    static_assert(std::is_class_v<ActionCollection>);
    static_assert(std::is_base_of_v<QObject, ActionCollection>);
    static_assert(std::is_constructible_v<ActionCollection, QObject *>);
    static_assert(std::is_constructible_v<ActionCollection, QObject *, const QString &>);
    static_assert(!std::is_convertible_v<QObject *, ActionCollection>);
    static_assert(std::is_destructible_v<ActionCollection>);
    static_assert(std::has_virtual_destructor_v<ActionCollection>);

    ASSERT_ACTION_COLLECTION_SIGNATURE(allCollections, ActionCollectionAllCollectionsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(componentName, ActionCollectionStringSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(setComponentName, ActionCollectionSetStringSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(componentDisplayName, ActionCollectionStringSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(setComponentDisplayName, ActionCollectionSetStringSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(parentGUIClient, ActionCollectionParentClientSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(configGroup, ActionCollectionStringSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(setConfigGroup, ActionCollectionSetStringSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(readSettings, ActionCollectionVoidSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(writeSettings, ActionCollectionWriteSettingsSignature);
    static_assert(std::is_same_v<decltype(std::declval<const ActionCollection &>().writeSettings()), void>);
}

void KStandardActionEnumContractTest::actionCollectionLookupAndClassificationSignaturesRemainStable()
{
    ASSERT_ACTION_COLLECTION_SIGNATURE(count, ActionCollectionCountSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(isEmpty, ActionCollectionEmptySignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(action, ActionCollectionActionIndexSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(action, ActionCollectionActionNameSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(actions, ActionCollectionActionsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(actionsWithoutGroup, ActionCollectionUngroupedActionsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(actionGroups, ActionCollectionActionGroupsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(categories, ActionCollectionCategoriesSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(getCategory, ActionCollectionCategorySignature);
}

void KStandardActionEnumContractTest::actionCollectionMembershipMutationSignaturesRemainStable()
{
    ASSERT_ACTION_COLLECTION_SIGNATURE(clear, ActionCollectionVoidSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(add<QAction>, ActionCollectionTemplateAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addAction, ActionCollectionStandardAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addAction, ActionCollectionNamedStandardAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addAction, ActionCollectionActionAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addAction, ActionCollectionReceiverAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addActions, ActionCollectionActionsAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addCategorizedAction, ActionCollectionCategorizedAddSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(removeAction, ActionCollectionActionMutationSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(takeAction, ActionCollectionTakeActionSignature);

    static_assert(
        std::is_same_v<decltype(std::declval<ActionCollection &>().add<QAction>(std::declval<const QString &>())),
                       QAction *>);
    static_assert(
        std::is_same_v<decltype(std::declval<ActionCollection &>().addAction(StandardAction::ActionNone)), QAction *>);
    static_assert(
        std::is_same_v<decltype(std::declval<ActionCollection &>().addAction(StandardAction::ActionNone,
                                                                             std::declval<const QString &>())),
                       QAction *>);
    static_assert(
        std::is_same_v<decltype(std::declval<ActionCollection &>().addAction(std::declval<const QString &>(),
                                                                             static_cast<QAction *>(nullptr))),
                       QAction *>);
    static_assert(
        std::is_same_v<decltype(std::declval<ActionCollection &>().addAction(std::declval<const QString &>())),
                       QAction *>);
}

void KStandardActionEnumContractTest::actionCollectionShortcutSignaturesRemainStable()
{
    ASSERT_ACTION_COLLECTION_SIGNATURE(defaultShortcut, ActionCollectionDefaultShortcutSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(defaultShortcuts, ActionCollectionDefaultShortcutsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(setDefaultShortcut, ActionCollectionSetDefaultShortcutSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(setDefaultShortcuts, ActionCollectionSetDefaultShortcutsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(isShortcutsConfigurable, ActionCollectionShortcutConfigurableSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(setShortcutsConfigurable, ActionCollectionSetShortcutConfigurableSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(updateShortcuts, ActionCollectionVoidSignature);
}

void KStandardActionEnumContractTest::actionCollectionAssociationAndNotificationSignaturesRemainStable()
{
    ASSERT_ACTION_COLLECTION_SIGNATURE(associateWidget, ActionCollectionAssociateWidgetSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(addAssociatedWidget, ActionCollectionMutateWidgetSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(removeAssociatedWidget, ActionCollectionMutateWidgetSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(associatedWidgets, ActionCollectionAssociatedWidgetsSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(clearAssociatedWidgets, ActionCollectionVoidSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(inserted, ActionCollectionNotificationSignature);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    ASSERT_ACTION_COLLECTION_SIGNATURE(actionHighlighted, ActionCollectionNotificationSignature);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    ASSERT_ACTION_COLLECTION_SIGNATURE(actionHovered, ActionCollectionNotificationSignature);
    ASSERT_ACTION_COLLECTION_SIGNATURE(actionTriggered, ActionCollectionNotificationSignature);
}

#undef ASSERT_ACTION_COLLECTION_SIGNATURE
#undef ASSERT_STANDARD_ACTION_SIGNATURE

QTEST_GUILESS_MAIN(KStandardActionEnumContractTest)

#include "KStandardActionEnumContractTest.moc"
