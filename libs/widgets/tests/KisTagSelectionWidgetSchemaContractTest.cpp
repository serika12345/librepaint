/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <KisTagSelectionWidget.h>
#include <QTest>

#include <type_traits>

#define ASSERT_TAG_SELECTION_SIGNATURE(type, method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class KisTagSelectionWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tagSelectionWidgetTypeAndLifetimeSchemaRemainStable();
    void tagSelectionWidgetListAndNotificationSignaturesRemainStable();
    void addTagButtonTypeLifetimeAndAvailabilitySchemaRemainStable();
    void addTagButtonActionAndNotificationSignaturesRemainStable();
    void closeableTagLabelTypeLifetimeAndNotificationSchemaRemainStable();
};

void KisTagSelectionWidgetSchemaContractTest::tagSelectionWidgetTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTagSelectionWidget>);
    static_assert(std::is_base_of_v<QWidget, KisTagSelectionWidget>);
    static_assert(std::is_default_constructible_v<KisTagSelectionWidget>);
    static_assert(std::is_constructible_v<KisTagSelectionWidget, QWidget *>);
    static_assert(std::is_constructible_v<KisTagSelectionWidget, QWidget *, bool>);
    static_assert(std::has_virtual_destructor_v<KisTagSelectionWidget>);
}

void KisTagSelectionWidgetSchemaContractTest::tagSelectionWidgetListAndNotificationSignaturesRemainStable()
{
    ASSERT_TAG_SELECTION_SIGNATURE(KisTagSelectionWidget,
                                   setTagList,
                                   void (KisTagSelectionWidget::*)(bool, QList<KoID> &, QList<KoID> &));
    ASSERT_TAG_SELECTION_SIGNATURE(KisTagSelectionWidget,
                                   setTagList,
                                   void (KisTagSelectionWidget::*)(bool, QList<KoID> &, QList<KoID> &, QList<KoID> &));
    ASSERT_TAG_SELECTION_SIGNATURE(KisTagSelectionWidget, sigAddTagToSelection, void (KisTagSelectionWidget::*)(KoID));
    ASSERT_TAG_SELECTION_SIGNATURE(KisTagSelectionWidget, sigCreateNewTag, void (KisTagSelectionWidget::*)(QString));
    ASSERT_TAG_SELECTION_SIGNATURE(KisTagSelectionWidget,
                                   sigRemoveTagFromSelection,
                                   void (KisTagSelectionWidget::*)(KoID));
}

void KisTagSelectionWidgetSchemaContractTest::addTagButtonTypeLifetimeAndAvailabilitySchemaRemainStable()
{
    static_assert(std::is_class_v<WdgAddTagButton>);
    static_assert(std::is_base_of_v<QToolButton, WdgAddTagButton>);
    static_assert(std::is_constructible_v<WdgAddTagButton, QWidget *>);
    static_assert(std::is_constructible_v<WdgAddTagButton, QWidget *, bool>);
    static_assert(std::has_virtual_destructor_v<WdgAddTagButton>);
    ASSERT_TAG_SELECTION_SIGNATURE(WdgAddTagButton, setAvailableTagsList, void (WdgAddTagButton::*)(QList<KoID> &));
    ASSERT_TAG_SELECTION_SIGNATURE(WdgAddTagButton, setAddNewTagAvailable, void (WdgAddTagButton::*)(bool));
}

void KisTagSelectionWidgetSchemaContractTest::addTagButtonActionAndNotificationSignaturesRemainStable()
{
    ASSERT_TAG_SELECTION_SIGNATURE(WdgAddTagButton, sigCreateNewTag, void (WdgAddTagButton::*)(QString));
    ASSERT_TAG_SELECTION_SIGNATURE(WdgAddTagButton, sigAddNewTag, void (WdgAddTagButton::*)(KoID));
    ASSERT_TAG_SELECTION_SIGNATURE(WdgAddTagButton, slotFinishLastAction, void (WdgAddTagButton::*)());
}

void KisTagSelectionWidgetSchemaContractTest::closeableTagLabelTypeLifetimeAndNotificationSchemaRemainStable()
{
    static_assert(std::is_class_v<WdgCloseableLabel>);
    static_assert(std::is_base_of_v<QWidget, WdgCloseableLabel>);
    static_assert(std::is_constructible_v<WdgCloseableLabel, KoID, bool>);
    static_assert(std::is_constructible_v<WdgCloseableLabel, KoID, bool, bool>);
    static_assert(std::is_constructible_v<WdgCloseableLabel, KoID, bool, bool, QWidget *>);
    static_assert(std::has_virtual_destructor_v<WdgCloseableLabel>);
    ASSERT_TAG_SELECTION_SIGNATURE(WdgCloseableLabel, sigRemoveTagFromSelection, void (WdgCloseableLabel::*)(KoID));
}

QTEST_GUILESS_MAIN(KisTagSelectionWidgetSchemaContractTest)
#include "KisTagSelectionWidgetSchemaContractTest.moc"
