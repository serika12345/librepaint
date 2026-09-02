/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FileDialog.h"

#include <QTest>

#include <type_traits>
#include <utility>

class FileDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileDialogTypeAndModeValuesRemainStable();
    void fileDialogLifecycleAndLocationSignaturesRemainStable();
    void fileDialogFilterConfigurationSignaturesRemainStable();
    void fileDialogSelectionResultSignaturesRemainStable();
    void fileDialogStaticSelectionSignaturesRemainStable();
};

void FileDialogSchemaContractTest::fileDialogTypeAndModeValuesRemainStable()
{
    static_assert(std::is_class_v<FileDialog>);
    static_assert(std::is_enum_v<FileDialog::DialogType>);
    static_assert(FileDialog::OpenFile == 0);
    static_assert(FileDialog::OpenFiles == 1);
    static_assert(FileDialog::OpenDirectory == 2);
    static_assert(FileDialog::ImportFile == 3);
    static_assert(FileDialog::ImportFiles == 4);
    static_assert(FileDialog::ImportDirectory == 5);
    static_assert(FileDialog::SaveFile == 6);
}

void FileDialogSchemaContractTest::fileDialogLifecycleAndLocationSignaturesRemainStable()
{
    using SetCaption = void (FileDialog::*)(const QString &);
    using SetDefaultDir = void (FileDialog::*)(const QString &, bool);
    using SetDirectoryUrl = void (FileDialog::*)(const QUrl &);

    static_assert(std::is_constructible_v<FileDialog>);
    static_assert(std::is_constructible_v<FileDialog, QWidget *>);
    static_assert(std::is_constructible_v<FileDialog, QWidget *, FileDialog::DialogType>);
    static_assert(std::is_constructible_v<FileDialog, QWidget *, FileDialog::DialogType, const QString &>);
    static_assert(std::is_destructible_v<FileDialog>);
    static_assert(std::is_same_v<decltype(static_cast<SetCaption>(&FileDialog::setCaption)), SetCaption>);
    static_assert(std::is_same_v<decltype(static_cast<SetDefaultDir>(&FileDialog::setDefaultDir)), SetDefaultDir>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetDirectoryUrl>(&FileDialog::setDirectoryUrl)), SetDirectoryUrl>);
    static_assert(
        std::is_same_v<decltype(std::declval<FileDialog &>().setDefaultDir(std::declval<const QString &>())), void>);
}

void FileDialogSchemaContractTest::fileDialogFilterConfigurationSignaturesRemainStable()
{
    using StringSetter = void (FileDialog::*)(const QString &);
    using SetImageFilters = void (FileDialog::*)();
    using SetMimeTypeFilters = void (FileDialog::*)(const QStringList &, QString);

    static_assert(std::is_same_v<decltype(static_cast<StringSetter>(&FileDialog::onFilterSelected)), StringSetter>);
    static_assert(std::is_same_v<decltype(static_cast<StringSetter>(&FileDialog::selectNameFilter)), StringSetter>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetImageFilters>(&FileDialog::setImageFilters)), SetImageFilters>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetMimeTypeFilters>(&FileDialog::setMimeTypeFilters)), SetMimeTypeFilters>);
    static_assert(std::is_same_v<decltype(static_cast<StringSetter>(&FileDialog::setNameFilter)), StringSetter>);
    static_assert(
        std::is_same_v<decltype(std::declval<FileDialog &>().setMimeTypeFilters(std::declval<const QStringList &>())),
                       void>);
}

void FileDialogSchemaContractTest::fileDialogSelectionResultSignaturesRemainStable()
{
    using Filename = QString (FileDialog::*)();
    using Filenames = QStringList (FileDialog::*)();
    using SelectedValue = QString (FileDialog::*)() const;

    static_assert(std::is_same_v<decltype(static_cast<Filename>(&FileDialog::filename)), Filename>);
    static_assert(std::is_same_v<decltype(static_cast<Filenames>(&FileDialog::filenames)), Filenames>);
    static_assert(std::is_same_v<decltype(static_cast<SelectedValue>(&FileDialog::selectedMimeType)), SelectedValue>);
    static_assert(std::is_same_v<decltype(static_cast<SelectedValue>(&FileDialog::selectedNameFilter)), SelectedValue>);
}

void FileDialogSchemaContractTest::fileDialogStaticSelectionSignaturesRemainStable()
{
    using ExistingDirectory = QString (*)(QWidget *, const QString &, const QString &, const QString &);
    using OpenFileName =
        QString (*)(QWidget *, const QString &, const QString &, const QString &, const QString &, const QString &);
    using OpenFileNames =
        QStringList (*)(QWidget *, const QString &, const QString &, const QString &, const QString &, const QString &);
    using SaveFileName = OpenFileName;

    static_assert(std::is_same_v<decltype(&FileDialog::getExistingDirectory), ExistingDirectory>);
    static_assert(std::is_same_v<decltype(&FileDialog::getOpenFileName), OpenFileName>);
    static_assert(std::is_same_v<decltype(&FileDialog::getOpenFileNames), OpenFileNames>);
    static_assert(std::is_same_v<decltype(&FileDialog::getSaveFileName), SaveFileName>);
    static_assert(std::is_same_v<decltype(FileDialog::getExistingDirectory()), QString>);
    static_assert(std::is_same_v<decltype(FileDialog::getOpenFileName()), QString>);
    static_assert(std::is_same_v<decltype(FileDialog::getOpenFileNames()), QStringList>);
    static_assert(std::is_same_v<decltype(FileDialog::getSaveFileName()), QString>);
}

QTEST_APPLESS_MAIN(FileDialogSchemaContractTest)

#include "FileDialogSchemaContractTest.moc"
