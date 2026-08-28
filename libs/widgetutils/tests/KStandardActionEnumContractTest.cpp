/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <kstandardaction.h>

#include <QTest>

#include <array>

class KStandardActionEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void standardActionIdsRemainStable();
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

QTEST_GUILESS_MAIN(KStandardActionEnumContractTest)

#include "KStandardActionEnumContractTest.moc"
