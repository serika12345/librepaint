/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <kstandardaction.h>

#include <QTest>

#include <array>
#include <type_traits>

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

#define ASSERT_STANDARD_ACTION_SIGNATURE(functionName, signatureType)                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signatureType>(&KStandardAction::functionName)), signatureType>)

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

#undef ASSERT_STANDARD_ACTION_SIGNATURE

QTEST_GUILESS_MAIN(KStandardActionEnumContractTest)

#include "KStandardActionEnumContractTest.moc"
