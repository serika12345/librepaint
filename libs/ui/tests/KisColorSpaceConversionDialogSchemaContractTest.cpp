/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/KisColorSpaceConversionDialog.h"

#include <QTest>

#include <type_traits>

class KisColorSpaceConversionDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void generatedPageTypeAndConstructionSchemaRemainStable();
    void dialogTypeConstructionAndLifetimeSchemaRemainStable();
    void publicPageAndButtonGroupMembersRemainStable();
    void colorSpaceAndConversionSignaturesRemainStable();
    void interactionResponseSignaturesRemainStable();
};

void KisColorSpaceConversionDialogSchemaContractTest::generatedPageTypeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<WdgConvertColorSpace>);
    static_assert(std::is_base_of_v<QWidget, WdgConvertColorSpace>);
    static_assert(std::is_constructible_v<WdgConvertColorSpace, QWidget *>);
}

void KisColorSpaceConversionDialogSchemaContractTest::dialogTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Dialog = KisColorSpaceConversionDialog;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<KoDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, QWidget *, const char *>);
    static_assert(std::has_virtual_destructor_v<Dialog>);
}

void KisColorSpaceConversionDialogSchemaContractTest::publicPageAndButtonGroupMembersRemainStable()
{
    using Dialog = KisColorSpaceConversionDialog;

    static_assert(std::is_same_v<decltype(&Dialog::m_page), WdgConvertColorSpace * Dialog::*>);
    static_assert(std::is_same_v<decltype(&Dialog::m_intentButtonGroup), QButtonGroup Dialog::*>);
}

void KisColorSpaceConversionDialogSchemaContractTest::colorSpaceAndConversionSignaturesRemainStable()
{
    using Dialog = KisColorSpaceConversionDialog;

    static_assert(
        std::is_same_v<decltype(&Dialog::setInitialColorSpace), void (Dialog::*)(const KoColorSpace *, KisImageSP)>);
    static_assert(std::is_same_v<decltype(&Dialog::colorSpace), const KoColorSpace *(Dialog::*)() const>);
    static_assert(std::is_same_v<decltype(&Dialog::conversionIntent),
                                 KoColorConversionTransformation::Intent (Dialog::*)() const>);
    static_assert(std::is_same_v<decltype(&Dialog::conversionFlags),
                                 KoColorConversionTransformation::ConversionFlags (Dialog::*)() const>);
}

void KisColorSpaceConversionDialogSchemaContractTest::interactionResponseSignaturesRemainStable()
{
    using Dialog = KisColorSpaceConversionDialog;

    static_assert(std::is_same_v<decltype(&Dialog::selectionChanged), void (Dialog::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Dialog::okClicked), void (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::slotColorSpaceChanged), void (Dialog::*)(const KoColorSpace *)>);
}

QTEST_APPLESS_MAIN(KisColorSpaceConversionDialogSchemaContractTest)

#include "KisColorSpaceConversionDialogSchemaContractTest.moc"
