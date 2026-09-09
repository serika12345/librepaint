/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "widgets/kis_cmb_idlist.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisCmbIDListSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionLifetimeAndAutoIdSchemaRemainStable();
    void listAndAutomaticOptionSignaturesRemainStable();
    void currentItemSignaturesRemainStable();
    void selectionNotificationSignaturesRemainStable();
};

void KisCmbIDListSchemaContractTest::typeConstructionLifetimeAndAutoIdSchemaRemainStable()
{
    using Selector = KisCmbIDList;

    static_assert(std::is_class_v<Selector>);
    static_assert(std::is_base_of_v<QComboBox, Selector>);
    static_assert(std::is_default_constructible_v<Selector>);
    static_assert(std::is_constructible_v<Selector, QWidget *, const char *>);
    static_assert(std::has_virtual_destructor_v<Selector>);
    static_assert(std::is_same_v<decltype(Selector::AutoOptionID), const KoID>);
}

void KisCmbIDListSchemaContractTest::listAndAutomaticOptionSignaturesRemainStable()
{
    using Selector = KisCmbIDList;

    static_assert(std::is_same_v<decltype(&Selector::setIDList), void (Selector::*)(const QList<KoID> &, bool)>);
    static_assert(std::is_same_v<decltype(&Selector::allowAuto), void (Selector::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Selector::setAutoHint), void (Selector::*)(const QString &)>);
    static_assert(
        std::is_same_v<decltype(std::declval<Selector &>().setIDList(std::declval<const QList<KoID> &>())), void>);
    static_assert(std::is_same_v<decltype(std::declval<Selector &>().allowAuto()), void>);
}

void KisCmbIDListSchemaContractTest::currentItemSignaturesRemainStable()
{
    using Selector = KisCmbIDList;
    using SetCurrentId = void (Selector::*)(KoID);
    using SetCurrentText = void (Selector::*)(const QString &);

    static_assert(std::is_same_v<decltype(static_cast<SetCurrentId>(&Selector::setCurrent)), SetCurrentId>);
    static_assert(std::is_same_v<decltype(static_cast<SetCurrentText>(&Selector::setCurrent)), SetCurrentText>);
    static_assert(std::is_same_v<decltype(&Selector::currentItem), KoID (Selector::*)() const>);
}

void KisCmbIDListSchemaContractTest::selectionNotificationSignaturesRemainStable()
{
    using Selector = KisCmbIDList;

    static_assert(std::is_same_v<decltype(&Selector::activated), void (Selector::*)(const KoID &)>);
    static_assert(std::is_same_v<decltype(&Selector::highlighted), void (Selector::*)(const KoID &)>);
}

QTEST_GUILESS_MAIN(KisCmbIDListSchemaContractTest)

#include "KisCmbIDListSchemaContractTest.moc"
