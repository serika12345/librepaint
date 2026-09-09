/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ui/workspace/KisOpenPane.h"
#include "ui/workspace/KisTemplateTree.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_OPEN_PANE_SIGNATURE(method, ...)                                                                        \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisOpenPane::method)), __VA_ARGS__>)
#define ASSERT_TEMPLATE_TREE_SIGNATURE(method, ...)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisTemplateTree::method)), __VA_ARGS__>)

} // namespace

class KisOpenPaneSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void paneAdditionAndSelectionSignaturesRemainStable();
    void documentTemplateAndLayoutNotificationSignaturesRemainStable();
    void templateTreeTypeConstructionAndLifetimeSchemaRemainStable();
    void templateTreeResourcePathAndPersistenceSignaturesRemainStable();
    void templateTreeGroupMembershipSignaturesRemainStable();
};

void KisOpenPaneSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisOpenPane>);
    static_assert(std::is_base_of_v<QDialog, KisOpenPane>);
    static_assert(std::is_constructible_v<KisOpenPane, QWidget *, const QStringList &>);
    static_assert(std::has_virtual_destructor_v<KisOpenPane>);

    QVERIFY(true);
}

void KisOpenPaneSchemaContractTest::paneAdditionAndSelectionSignaturesRemainStable()
{
    ASSERT_OPEN_PANE_SIGNATURE(
        addPane,
        QTreeWidgetItem * (KisOpenPane::*)(const QString &, const QString &, const QString &, QWidget *, int));
    ASSERT_OPEN_PANE_SIGNATURE(
        addPane,
        QTreeWidgetItem * (KisOpenPane::*)(const QString &, const QString &, const QPixmap &, QWidget *, int));
    ASSERT_OPEN_PANE_SIGNATURE(addCustomDocumentWidget,
                               void (KisOpenPane::*)(QWidget *, const QString &, const QString &, const QString &));
    ASSERT_OPEN_PANE_SIGNATURE(selectItem, bool (KisOpenPane::*)(const QString &));
}

void KisOpenPaneSchemaContractTest::documentTemplateAndLayoutNotificationSignaturesRemainStable()
{
    ASSERT_OPEN_PANE_SIGNATURE(documentSelected, void (KisOpenPane::*)(KisDocument *));
    ASSERT_OPEN_PANE_SIGNATURE(openExistingFile, void (KisOpenPane::*)(const QUrl &));
    ASSERT_OPEN_PANE_SIGNATURE(openTemplate, void (KisOpenPane::*)(const QUrl &));
    ASSERT_OPEN_PANE_SIGNATURE(alwaysUseChanged, void (KisOpenPane::*)(KisTemplatesPane *, const QString &));
    ASSERT_OPEN_PANE_SIGNATURE(splitterResized, void (KisOpenPane::*)(KisDetailsPane *, const QList<int> &));
    ASSERT_OPEN_PANE_SIGNATURE(cancelButton, void (KisOpenPane::*)());
}

void KisOpenPaneSchemaContractTest::templateTreeTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTemplateTree>);
    static_assert(std::is_constructible_v<KisTemplateTree, const QString &, bool>);
    static_assert(std::is_constructible_v<KisTemplateTree, const QString &>);
    static_assert(std::is_destructible_v<KisTemplateTree>);

    QVERIFY(true);
}

void KisOpenPaneSchemaContractTest::templateTreeResourcePathAndPersistenceSignaturesRemainStable()
{
    ASSERT_TEMPLATE_TREE_SIGNATURE(templatesResourcePath, QString (KisTemplateTree::*)() const);
    ASSERT_TEMPLATE_TREE_SIGNATURE(readTemplateTree, void (KisTemplateTree::*)());
    ASSERT_TEMPLATE_TREE_SIGNATURE(writeTemplateTree, void (KisTemplateTree::*)());
}

void KisOpenPaneSchemaContractTest::templateTreeGroupMembershipSignaturesRemainStable()
{
    ASSERT_TEMPLATE_TREE_SIGNATURE(add, bool (KisTemplateTree::*)(KisTemplateGroup *));
    ASSERT_TEMPLATE_TREE_SIGNATURE(find, KisTemplateGroup * (KisTemplateTree::*)(const QString &) const);
    ASSERT_TEMPLATE_TREE_SIGNATURE(defaultGroup, KisTemplateGroup * (KisTemplateTree::*)() const);
    ASSERT_TEMPLATE_TREE_SIGNATURE(defaultTemplate, KisTemplate * (KisTemplateTree::*)() const);
    ASSERT_TEMPLATE_TREE_SIGNATURE(groups, QList<KisTemplateGroup *> (KisTemplateTree::*)() const);
}

#undef ASSERT_TEMPLATE_TREE_SIGNATURE
#undef ASSERT_OPEN_PANE_SIGNATURE

QTEST_APPLESS_MAIN(KisOpenPaneSchemaContractTest)

#include "KisOpenPaneSchemaContractTest.moc"
