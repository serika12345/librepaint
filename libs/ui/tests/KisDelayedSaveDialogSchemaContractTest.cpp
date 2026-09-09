/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/kis_delayed_save_dialog.h"

#include <QTest>

#include <type_traits>
#include <utility>

class KisDelayedSaveDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dialogTypeConstructionAndLifetimeSchemaRemainStable();
    void resultTypeAndOrdinalsRemainStable();
    void presentationTypeAndOrdinalsRemainStable();
    void busyWaitControlSignaturesRemainStable();
};

void KisDelayedSaveDialogSchemaContractTest::dialogTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Dialog = KisDelayedSaveDialog;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<KoDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, KisImageSP, Dialog::Type, int, QWidget *>);
    static_assert(std::is_same_v<decltype(Dialog(std::declval<KisImageSP>(), Dialog::SaveDialog, 200)), Dialog>);
    static_assert(std::has_virtual_destructor_v<Dialog>);
}

void KisDelayedSaveDialogSchemaContractTest::resultTypeAndOrdinalsRemainStable()
{
    using Result = KisDelayedSaveDialog::ResultType;

    static_assert(std::is_enum_v<Result>);
    static_assert(static_cast<int>(Result::Rejected) == static_cast<int>(QDialog::Rejected));
    static_assert(static_cast<int>(Result::Accepted) == static_cast<int>(QDialog::Accepted));
    static_assert(Result::Ignored == 2);
}

void KisDelayedSaveDialogSchemaContractTest::presentationTypeAndOrdinalsRemainStable()
{
    using Type = KisDelayedSaveDialog::Type;

    static_assert(std::is_enum_v<Type>);
    static_assert(Type::SaveDialog == 0);
    static_assert(Type::GeneralDialog == 1);
    static_assert(Type::ForcedDialog == 2);
}

void KisDelayedSaveDialogSchemaContractTest::busyWaitControlSignaturesRemainStable()
{
    using Dialog = KisDelayedSaveDialog;

    static_assert(std::is_same_v<decltype(&Dialog::registerBusyWaitFeedback), void (*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::blockIfImageIsBusy), void (Dialog::*)()>);
}

QTEST_APPLESS_MAIN(KisDelayedSaveDialogSchemaContractTest)

#include "KisDelayedSaveDialogSchemaContractTest.moc"
