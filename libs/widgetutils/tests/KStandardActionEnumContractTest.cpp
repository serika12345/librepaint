/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <kstandardaction.h>

#include "../xmlgui/kactioncollection.h"

#include <QTest>

#include <array>
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



} // namespace

class KStandardActionEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void standardActionIdsRemainStable();
    void standardActionIdentityAndFileSignaturesRemainStable();
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
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

}

void KStandardActionEnumContractTest::actionCollectionAssociationAndNotificationSignaturesRemainStable()
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

QTEST_GUILESS_MAIN(KStandardActionEnumContractTest)

#include "KStandardActionEnumContractTest.moc"
