/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_AUTOSAVE_FILES_H
#define KIS_DOCUMENT_AUTOSAVE_FILES_H

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

#include <kritadocumentfiles_export.h>

namespace Krita::Document
{

struct KRITADOCUMENTFILES_EXPORT KisDocumentAutoSaveFile
{
    QString fileName;
    QString path;
    QDateTime lastModified;
    QByteArray previewData;
};

class KRITADOCUMENTFILES_EXPORT KisDocumentAutoSaveFiles
{
public:
    static QString directory();

    static QString filePath(const QString &documentPath,
                            const QString &recoveryDirectory,
                            qint64 processId,
                            const QString &documentObjectName,
                            bool hidden);

    static QList<KisDocumentAutoSaveFile> recoverableFiles(const QString &recoveryDirectory = QString());
    static bool isUsable(const QString &filePath);
    static bool remove(const QString &filePath);

    static void removeForDocument(const QString &autoSaveBaseName,
                                  bool wasRecovered,
                                  const QString &recoveryDirectory,
                                  qint64 processId,
                                  const QString &documentObjectName,
                                  bool hidden);
};

}

#endif
