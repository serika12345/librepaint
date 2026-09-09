/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_collapsible_button_group.h"

#include <QTest>

#include <type_traits>

class KisCollapsibleButtonGroupSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void autoRaiseAndIconSizeSignaturesRemainStable();
    void collapseStateSignaturesRemainStable();
    void sizingAndActionSignaturesRemainStable();
};

void KisCollapsibleButtonGroupSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_class_v<Group>);
    static_assert(std::is_base_of_v<QWidget, Group>);
    static_assert(std::is_constructible_v<Group, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Group>);
}

void KisCollapsibleButtonGroupSchemaContractTest::autoRaiseAndIconSizeSignaturesRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_same_v<decltype(&Group::setAutoRaise), void (Group::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Group::autoRaise), bool (Group::*)() const>);
    static_assert(std::is_same_v<decltype(&Group::setIconSize), void (Group::*)(const QSize &)>);
    static_assert(std::is_same_v<decltype(&Group::iconSize), QSize (Group::*)() const>);
}

void KisCollapsibleButtonGroupSchemaContractTest::collapseStateSignaturesRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_same_v<decltype(&Group::setAutoCollapse), void (Group::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Group::setCollapsed), void (Group::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Group::collapsed), bool (Group::*)() const>);
}

void KisCollapsibleButtonGroupSchemaContractTest::sizingAndActionSignaturesRemainStable()
{
    using Group = KisCollapsibleButtonGroup;

    static_assert(std::is_same_v<decltype(&Group::sizeHint), QSize (Group::*)() const>);
    static_assert(std::is_same_v<decltype(&Group::minimumSizeHint), QSize (Group::*)() const>);
    static_assert(std::is_same_v<decltype(&Group::addAction), QToolButton *(Group::*)(QAction *)>);
}

QTEST_GUILESS_MAIN(KisCollapsibleButtonGroupSchemaContractTest)

#include "KisCollapsibleButtonGroupSchemaContractTest.moc"
