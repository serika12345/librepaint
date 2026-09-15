/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <dialogs/kis_dlg_hlg_import.h>

#include <QTest>

#include <type_traits>

class KisDlgHLGImportSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hlgImportDialogSchemaRemainStable();
};

void KisDlgHLGImportSchemaContractTest::hlgImportDialogSchemaRemainStable()
{
    using Dialog = KisDlgHLGImport;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<KoDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, bool, float, float, QWidget *>);
    static_assert(std::is_same_v<decltype(&Dialog::applyOOTF), bool (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::gamma), float (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::nominalPeakBrightness), float (Dialog::*)()>);
}

QTEST_APPLESS_MAIN(KisDlgHLGImportSchemaContractTest)

#include "KisDlgHLGImportSchemaContractTest.moc"
