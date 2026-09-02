/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <khelpmenu.h>

#include <type_traits>

#define ASSERT_HELP_MENU_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisKHelpMenu::method)), signature>)

class KHelpMenuSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void helpMenuTypeAndIdentifierSchemaRemainsStable();
    void helpMenuLifetimeSchemaRemainsStable();
    void helpMenuAccessSchemaRemainsStable();
    void helpMenuRoutingSignaturesRemainStable();
    void helpMenuNotificationSignatureRemainsStable();
};

void KHelpMenuSchemaContractTest::helpMenuTypeAndIdentifierSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisKHelpMenu>);
    static_assert(std::is_enum_v<KisKHelpMenu::MenuId>);
    static_assert(KisKHelpMenu::menuHelpContents == 0);
    static_assert(KisKHelpMenu::menuWhatsThis == 1);
    static_assert(KisKHelpMenu::menuAboutApp == 2);
    static_assert(KisKHelpMenu::menuAboutKDE == 3);
    static_assert(KisKHelpMenu::menuReportBug == 4);
    static_assert(KisKHelpMenu::menuSwitchLanguage == 5);
}

void KHelpMenuSchemaContractTest::helpMenuLifetimeSchemaRemainsStable()
{
    static_assert(std::is_constructible_v<KisKHelpMenu, QWidget *, const QString &, bool>);
    static_assert(std::is_constructible_v<KisKHelpMenu>);
    static_assert(std::is_constructible_v<KisKHelpMenu, QWidget *, const KAboutData &, bool>);
    static_assert(std::is_constructible_v<KisKHelpMenu, QWidget *, const KAboutData &>);
    static_assert(std::has_virtual_destructor_v<KisKHelpMenu>);
}

void KHelpMenuSchemaContractTest::helpMenuAccessSchemaRemainsStable()
{
    ASSERT_HELP_MENU_SIGNATURE(menu, QMenu * (KisKHelpMenu::*)());
    ASSERT_HELP_MENU_SIGNATURE(action, QAction * (KisKHelpMenu::*)(KisKHelpMenu::MenuId) const);
}

void KHelpMenuSchemaContractTest::helpMenuRoutingSignaturesRemainStable()
{
    ASSERT_HELP_MENU_SIGNATURE(appHelpActivated, void (KisKHelpMenu::*)());
    ASSERT_HELP_MENU_SIGNATURE(contextHelpActivated, void (KisKHelpMenu::*)());
    ASSERT_HELP_MENU_SIGNATURE(aboutApplication, void (KisKHelpMenu::*)());
    ASSERT_HELP_MENU_SIGNATURE(aboutKDE, void (KisKHelpMenu::*)());
    ASSERT_HELP_MENU_SIGNATURE(reportBug, void (KisKHelpMenu::*)());
    ASSERT_HELP_MENU_SIGNATURE(switchApplicationLanguage, void (KisKHelpMenu::*)());
}

void KHelpMenuSchemaContractTest::helpMenuNotificationSignatureRemainsStable()
{
    ASSERT_HELP_MENU_SIGNATURE(showAboutApplication, void (KisKHelpMenu::*)());
}

QTEST_GUILESS_MAIN(KHelpMenuSchemaContractTest)
#include "KHelpMenuSchemaContractTest.moc"
