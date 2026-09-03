/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include <kactioncategory.h>
#include <kedittoolbar.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KActionCategoryEditToolBarSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionCategoryTypeAndLifetimeSchemaRemainStable();
    void actionCategoryCollectionAndIdentitySignaturesRemainStable();
    void actionCategoryActionCreationSignaturesRemainStable();
    void editToolBarTypeLifetimeAndConfigurationSchemaRemainStable();
    void editToolBarNotificationSignaturesRemainStable();
};

void KActionCategoryEditToolBarSchemaContractTest::actionCategoryTypeAndLifetimeSchemaRemainStable()
{
    using Category = KisKActionCategory;

    static_assert(std::is_class_v<Category>);
    static_assert(std::is_base_of_v<QObject, Category>);
    static_assert(std::is_constructible_v<Category, const QString &>);
    static_assert(std::is_constructible_v<Category, const QString &, KisKActionCollection *>);
    static_assert(std::has_virtual_destructor_v<Category>);
}

void KActionCategoryEditToolBarSchemaContractTest::actionCategoryCollectionAndIdentitySignaturesRemainStable()
{
    using Category = KisKActionCategory;

    static_assert(std::is_same_v<decltype(&Category::actions), const QList<QAction *> (Category::*)() const>);
    static_assert(std::is_same_v<decltype(&Category::collection), KisKActionCollection *(Category::*)() const>);
    static_assert(std::is_same_v<decltype(&Category::text), QString (Category::*)() const>);
    static_assert(std::is_same_v<decltype(&Category::setText), void (Category::*)(const QString &)>);
}

void KActionCategoryEditToolBarSchemaContractTest::actionCategoryActionCreationSignaturesRemainStable()
{
    using Category = KisKActionCategory;
    using StandardAction = KStandardAction::StandardAction;
    using TemplateAdd = QAction *(Category::*)(const QString &, const QObject *, const char *);
    using StandardAdd = QAction *(Category::*)(StandardAction, const QObject *, const char *);
    using NamedStandardAdd = QAction *(Category::*)(StandardAction, const QString &, const QObject *, const char *);
    using ActionAdd = QAction *(Category::*)(const QString &, QAction *);
    using ReceiverAdd = QAction *(Category::*)(const QString &, const QObject *, const char *);

    static_assert(std::is_same_v<decltype(static_cast<TemplateAdd>(&Category::add<QAction>)), TemplateAdd>);
    static_assert(std::is_same_v<decltype(static_cast<StandardAdd>(&Category::addAction)), StandardAdd>);
    static_assert(std::is_same_v<decltype(static_cast<NamedStandardAdd>(&Category::addAction)), NamedStandardAdd>);
    static_assert(std::is_same_v<decltype(static_cast<ActionAdd>(&Category::addAction)), ActionAdd>);
    static_assert(std::is_same_v<decltype(static_cast<ReceiverAdd>(&Category::addAction)), ReceiverAdd>);

    static_assert(
        std::is_same_v<decltype(std::declval<Category &>().add<QAction>(std::declval<const QString &>())), QAction *>);
    static_assert(
        std::is_same_v<decltype(std::declval<Category &>().addAction(std::declval<StandardAction>())), QAction *>);
    static_assert(std::is_same_v<decltype(std::declval<Category &>().addAction(std::declval<StandardAction>(),
                                                                               std::declval<const QString &>())),
                                 QAction *>);
    static_assert(
        std::is_same_v<decltype(std::declval<Category &>().addAction(std::declval<const QString &>())), QAction *>);
}

void KActionCategoryEditToolBarSchemaContractTest::editToolBarTypeLifetimeAndConfigurationSchemaRemainStable()
{
    using ToolBarEditor = KisKEditToolBar;

    static_assert(std::is_class_v<ToolBarEditor>);
    static_assert(std::is_base_of_v<QDialog, ToolBarEditor>);
    static_assert(std::is_constructible_v<ToolBarEditor, KisKXMLGUIFactory *>);
    static_assert(std::is_constructible_v<ToolBarEditor, KisKXMLGUIFactory *, QWidget *>);
    static_assert(std::has_virtual_destructor_v<ToolBarEditor>);
    static_assert(
        std::is_same_v<decltype(&ToolBarEditor::setDefaultToolBar), void (ToolBarEditor::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&ToolBarEditor::setGlobalDefaultToolBar), void (*)(const char *)>);
    static_assert(
        std::is_same_v<decltype(&ToolBarEditor::setResourceFile), void (ToolBarEditor::*)(const QString &, bool)>);
    static_assert(
        std::is_same_v<decltype(std::declval<ToolBarEditor &>().setResourceFile(std::declval<const QString &>())),
                       void>);
}

void KActionCategoryEditToolBarSchemaContractTest::editToolBarNotificationSignaturesRemainStable()
{
    using ToolBarEditor = KisKEditToolBar;

    static_assert(std::is_same_v<decltype(&ToolBarEditor::newToolBarConfig), void (ToolBarEditor::*)()>);
    static_assert(std::is_same_v<decltype(&ToolBarEditor::newToolbarConfig), void (ToolBarEditor::*)()>);
}

QTEST_APPLESS_MAIN(KActionCategoryEditToolBarSchemaContractTest)

#include "KActionCategoryEditToolBarSchemaContractTest.moc"
