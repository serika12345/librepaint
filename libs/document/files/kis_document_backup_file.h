/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_BACKUP_FILE_H
#define KIS_DOCUMENT_BACKUP_FILE_H

#include <QString>

#include <kritadocumentfiles_export.h>

namespace Krita::Document
{

enum class KisDocumentBackupLocation {
    Adjacent,
    Home,
    Temporary,
};

struct KRITADOCUMENTFILES_EXPORT KisDocumentBackupOptions
{
    bool enabled {false};
    KisDocumentBackupLocation location {KisDocumentBackupLocation::Adjacent};
    int numberOfBackups {1};
    QString suffix {QStringLiteral("~")};
    bool forceTemporaryDirectory {false};
};

enum class KisDocumentBackupStatus {
    NotNeeded,
    Created,
    Failed,
};

struct KRITADOCUMENTFILES_EXPORT KisDocumentBackupResult
{
    KisDocumentBackupStatus status {KisDocumentBackupStatus::NotNeeded};
    QString directory;
};

class KRITADOCUMENTFILES_EXPORT KisDocumentBackupFile
{
public:
    static KisDocumentBackupResult create(const QString &filePath,
                                          const KisDocumentBackupOptions &options,
                                          const QString &autoSaveDirectory);
};

}

#endif
