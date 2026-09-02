/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoFileDialog.h>
#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_FILE_DIALOG_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoFileDialog::method)), signature>)

class KoFileDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileDialogTypeSchemaRemainsStable();
    void fileDialogLifetimeSchemaRemainsStable();
    void fileDialogConfigurationSignaturesRemainStable();
    void fileDialogSelectionResultSignaturesRemainStable();
    void fileDialogFilterNotificationSignatureRemainsStable();
};

void KoFileDialogSchemaContractTest::fileDialogTypeSchemaRemainsStable()
{
    static_assert(std::is_class_v<KoFileDialog>);
    static_assert(std::is_enum_v<KoFileDialog::DialogType>);
    static_assert(KoFileDialog::OpenFile == 0);
    static_assert(KoFileDialog::OpenFiles == 1);
    static_assert(KoFileDialog::OpenDirectory == 2);
    static_assert(KoFileDialog::ImportFile == 3);
    static_assert(KoFileDialog::ImportFiles == 4);
    static_assert(KoFileDialog::ImportDirectory == 5);
    static_assert(KoFileDialog::SaveFile == 6);
}

void KoFileDialogSchemaContractTest::fileDialogLifetimeSchemaRemainsStable()
{
    static_assert(std::is_constructible_v<KoFileDialog, QWidget *, KoFileDialog::DialogType, const QString &>);
    static_assert(std::has_virtual_destructor_v<KoFileDialog>);
}

void KoFileDialogSchemaContractTest::fileDialogConfigurationSignaturesRemainStable()
{
    ASSERT_FILE_DIALOG_SIGNATURE(setCaption, void (KoFileDialog::*)(const QString &));
    ASSERT_FILE_DIALOG_SIGNATURE(setDefaultDir, void (KoFileDialog::*)(const QString &, bool));
    static_assert(
        std::is_same_v<decltype(std::declval<KoFileDialog &>().setDefaultDir(std::declval<const QString &>())), void>);
    ASSERT_FILE_DIALOG_SIGNATURE(setDirectoryUrl, void (KoFileDialog::*)(const QUrl &));
    ASSERT_FILE_DIALOG_SIGNATURE(setImageFilters, void (KoFileDialog::*)());
    ASSERT_FILE_DIALOG_SIGNATURE(setMimeTypeFilters, void (KoFileDialog::*)(const QStringList &, QString));
    static_assert(
        std::is_same_v<decltype(std::declval<KoFileDialog &>().setMimeTypeFilters(std::declval<const QStringList &>())),
                       void>);
    ASSERT_FILE_DIALOG_SIGNATURE(setNameFilter, void (KoFileDialog::*)(const QString &));
    ASSERT_FILE_DIALOG_SIGNATURE(selectNameFilter, void (KoFileDialog::*)(const QString &));
}

void KoFileDialogSchemaContractTest::fileDialogSelectionResultSignaturesRemainStable()
{
    ASSERT_FILE_DIALOG_SIGNATURE(filename, QString (KoFileDialog::*)());
    ASSERT_FILE_DIALOG_SIGNATURE(filenames, QStringList (KoFileDialog::*)());
    ASSERT_FILE_DIALOG_SIGNATURE(selectedMimeType, QString (KoFileDialog::*)() const);
    ASSERT_FILE_DIALOG_SIGNATURE(selectedNameFilter, QString (KoFileDialog::*)() const);
}

void KoFileDialogSchemaContractTest::fileDialogFilterNotificationSignatureRemainsStable()
{
    ASSERT_FILE_DIALOG_SIGNATURE(onFilterSelected, void (KoFileDialog::*)(const QString &));
}

QTEST_GUILESS_MAIN(KoFileDialogSchemaContractTest)
#include "KoFileDialogSchemaContractTest.moc"
