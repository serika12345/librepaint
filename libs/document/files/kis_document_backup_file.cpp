/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_backup_file.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <KisBackup.h>

namespace Krita::Document
{

KisDocumentBackupResult KisDocumentBackupFile::create(const QString &filePath,
                                                       const KisDocumentBackupOptions &options,
                                                       const QString &autoSaveDirectory)
{
#ifndef Q_OS_ANDROID
    Q_UNUSED(autoSaveDirectory);
#endif

    if (!options.enabled || options.numberOfBackups < 1 || !QFileInfo::exists(filePath)) {
        return {};
    }

    QString backupDirectory;
    switch (options.location) {
    case KisDocumentBackupLocation::Home:
        backupDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        break;
    case KisDocumentBackupLocation::Temporary:
        backupDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        break;
    case KisDocumentBackupLocation::Adjacent:
#ifdef Q_OS_ANDROID
        backupDirectory = autoSaveDirectory;
        QDir().mkpath(backupDirectory);
#endif
        if (options.forceTemporaryDirectory) {
            backupDirectory = QDir::tempPath();
        }
        break;
    }

    bool created = false;
    if (options.numberOfBackups == 1) {
        created = KisBackup::simpleBackupFile(filePath, backupDirectory, options.suffix);
    } else {
        created = KisBackup::numberedBackupFile(filePath,
                                                backupDirectory,
                                                options.suffix,
                                                options.numberOfBackups);
    }

    return {created ? KisDocumentBackupStatus::Created : KisDocumentBackupStatus::Failed,
            backupDirectory};
}

}
