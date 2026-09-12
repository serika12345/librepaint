/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoConfigAuthorPage.h"

#include <QTest>

#include <type_traits>

class KoConfigAuthorPageSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void authorPageTypeLifecycleAndApplySchemaRemainStable();
    void contactInfoDelegateTypeLifecycleAndStateSchemaRemainStable();
    void contactInfoDelegateEditorSignatureRemainsStable();
};

void KoConfigAuthorPageSchemaContractTest::authorPageTypeLifecycleAndApplySchemaRemainStable()
{
    static_assert(std::is_class_v<KoConfigAuthorPage>);
    static_assert(std::is_constructible_v<KoConfigAuthorPage>);
    static_assert(std::has_virtual_destructor_v<KoConfigAuthorPage>);
    static_assert(std::is_same_v<decltype(&KoConfigAuthorPage::apply), void (KoConfigAuthorPage::*)()>);

    QVERIFY(true);
}

void KoConfigAuthorPageSchemaContractTest::contactInfoDelegateTypeLifecycleAndStateSchemaRemainStable()
{
    static_assert(std::is_class_v<KoContactInfoDelegate>);
    static_assert(std::is_constructible_v<KoContactInfoDelegate, QWidget *, QStringList>);
    static_assert(std::has_virtual_destructor_v<KoContactInfoDelegate>);
    static_assert(
        std::is_same_v<decltype(&KoContactInfoDelegate::m_contactModes), QStringList KoContactInfoDelegate::*>);

    QVERIFY(true);
}

void KoConfigAuthorPageSchemaContractTest::contactInfoDelegateEditorSignatureRemainsStable()
{
    using CreateEditor =
        QWidget *(KoContactInfoDelegate::*)(QWidget *, const QStyleOptionViewItem &, const QModelIndex &) const;
    static_assert(std::is_same_v<decltype(&KoContactInfoDelegate::createEditor), CreateEditor>);

    QVERIFY(true);
}

QTEST_APPLESS_MAIN(KoConfigAuthorPageSchemaContractTest)

#include "KoConfigAuthorPageSchemaContractTest.moc"
