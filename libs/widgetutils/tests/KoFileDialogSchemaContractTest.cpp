/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPreviewFileDialog.h"
#include "kis_file_name_requester.h"

#include <KoFileDialog.h>
#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_FILE_DIALOG_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoFileDialog::method)), signature>)

#define ASSERT_MEMBER_SIGNATURE(owner, method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&owner::method)), signature>)

namespace FileSelectionContract
{
class FileIconCreatorConstructionProbe final : public KisAbstractFileIconCreator
{
public:
    using KisAbstractFileIconCreator::KisAbstractFileIconCreator;

    bool createFileIcon(QString path, QIcon &icon, qreal devicePixelRatioF, QSize iconSize) override;
};
} // namespace FileSelectionContract

class KoFileDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileDialogTypeSchemaRemainsStable();
    void fileDialogLifetimeSchemaRemainsStable();
    void fileDialogConfigurationSignaturesRemainStable();
    void fileDialogSelectionResultSignaturesRemainStable();
    void fileDialogFilterNotificationSignatureRemainsStable();
    void fileIconCreatorTypeAndLifetimeSchemaRemainStable();
    void fileIconProviderTypeAndDispatchSchemaRemainStable();
    void previewFileDialogTypeAndNotificationSchemaRemainStable();
    void fileNameRequesterTypeAndConfigurationSchemaRemainStable();
    void fileNameRequesterSelectionAndNotificationSchemaRemainStable();
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

void KoFileDialogSchemaContractTest::fileIconCreatorTypeAndLifetimeSchemaRemainStable()
{
    using Creator = KisAbstractFileIconCreator;
    using ConstructionProbe = FileSelectionContract::FileIconCreatorConstructionProbe;

    static_assert(std::is_class_v<Creator>);
    static_assert(std::is_abstract_v<Creator>);
    static_assert(std::is_default_constructible_v<ConstructionProbe>);
    static_assert(std::has_virtual_destructor_v<Creator>);
    ASSERT_MEMBER_SIGNATURE(Creator, createFileIcon, bool (Creator::*)(QString, QIcon &, qreal, QSize));
}

void KoFileDialogSchemaContractTest::fileIconProviderTypeAndDispatchSchemaRemainStable()
{
    using Provider = KisFileIconProvider;

    static_assert(std::is_class_v<Provider>);
    static_assert(std::is_base_of_v<QFileIconProvider, Provider>);
    static_assert(std::is_constructible_v<Provider, qreal>);
    ASSERT_MEMBER_SIGNATURE(Provider, icon, QIcon (Provider::*)(QFileIconProvider::IconType) const);
    ASSERT_MEMBER_SIGNATURE(Provider, icon, QIcon (Provider::*)(const QFileInfo &) const);
}

void KoFileDialogSchemaContractTest::previewFileDialogTypeAndNotificationSchemaRemainStable()
{
    using Dialog = KisPreviewFileDialog;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<QFileDialog, Dialog>);
    static_assert(std::is_same_v<decltype(Dialog::s_iconCreator), KisAbstractFileIconCreator *>);
    static_assert(std::is_default_constructible_v<Dialog>);
    static_assert(std::is_constructible_v<Dialog, QWidget *, const QString &, const QString &, const QString &>);
    ASSERT_MEMBER_SIGNATURE(Dialog, onCurrentChanged, void (Dialog::*)(const QString &));
    ASSERT_MEMBER_SIGNATURE(Dialog, previewToggled, void (Dialog::*)(bool));
    ASSERT_MEMBER_SIGNATURE(Dialog, resetIconProvider, void (Dialog::*)());
}

void KoFileDialogSchemaContractTest::fileNameRequesterTypeAndConfigurationSchemaRemainStable()
{
    using Requester = KisFileNameRequester;

    static_assert(std::is_class_v<Requester>);
    static_assert(std::is_base_of_v<QWidget, Requester>);
    static_assert(std::is_default_constructible_v<Requester>);
    static_assert(std::is_constructible_v<Requester, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Requester>);
    ASSERT_MEMBER_SIGNATURE(Requester, setStartDir, void (Requester::*)(const QString &));
    ASSERT_MEMBER_SIGNATURE(Requester, setConfigurationName, void (Requester::*)(const QString &));
    ASSERT_MEMBER_SIGNATURE(Requester, setMode, void (Requester::*)(KoFileDialog::DialogType));
    ASSERT_MEMBER_SIGNATURE(Requester, mode, KoFileDialog::DialogType (Requester::*)() const);
    ASSERT_MEMBER_SIGNATURE(Requester, setReadOnlyText, void (Requester::*)(bool));
}

void KoFileDialogSchemaContractTest::fileNameRequesterSelectionAndNotificationSchemaRemainStable()
{
    using Requester = KisFileNameRequester;

    ASSERT_MEMBER_SIGNATURE(Requester, fileName, QString (Requester::*)() const);
    ASSERT_MEMBER_SIGNATURE(Requester, setMimeTypeFilters, void (Requester::*)(const QStringList &, QString));
    static_assert(
        std::is_same_v<decltype(std::declval<Requester &>().setMimeTypeFilters(std::declval<const QStringList &>())),
                       void>);
    ASSERT_MEMBER_SIGNATURE(Requester, setValidator, void (Requester::*)(QValidator *));
    ASSERT_MEMBER_SIGNATURE(Requester, slotSelectFile, void (Requester::*)());
    ASSERT_MEMBER_SIGNATURE(Requester, setFileName, void (Requester::*)(const QString &));
    ASSERT_MEMBER_SIGNATURE(Requester, textChanged, void (Requester::*)(const QString &));
    ASSERT_MEMBER_SIGNATURE(Requester, fileSelected, void (Requester::*)(const QString &));
}

QTEST_GUILESS_MAIN(KoFileDialogSchemaContractTest)
#include "KoFileDialogSchemaContractTest.moc"
