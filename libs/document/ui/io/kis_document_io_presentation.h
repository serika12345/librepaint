/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_IO_PRESENTATION_H
#define KIS_DOCUMENT_IO_PRESENTATION_H

#include <QObject>
#include <QString>

#include "kritadocumentui_export.h"

class KisImportExportErrorCode;

class KRITADOCUMENTUI_EXPORT KisDocumentIoPresentation : public QObject
{
    Q_OBJECT

public:
    explicit KisDocumentIoPresentation(QObject *parent = nullptr);

    void presentSaveResult(const QString &filePath,
                           const KisImportExportErrorCode &status,
                           const QString &errorMessage,
                           const QString &warningMessage,
                           bool batchMode);
    void notifySaveSucceeded(const QString &filePath);

    void presentMissingFile(const QString &filePath, bool batchMode);
    void presentLoadFailure(const QString &prettyPath,
                            const KisImportExportErrorCode &status,
                            const QString &errorMessage,
                            const QString &warningMessage,
                            bool batchMode);
    void presentLoadWarning(const QString &prettyPath,
                            const QString &warningMessage,
                            bool batchMode);
    void notifyLoadSucceeded();

    void notifyAutoSaveStarted(const QString &filePath);
    void notifyAutoSavePostponed();
    void notifyAutoSaveFinished(const QString &fileName);
    void notifyAutoSaveFailed(const QString &fileName,
                              const KisImportExportErrorCode &status,
                              const QString &errorMessage);

Q_SIGNALS:
    void statusBarMessage(const QString &text, int timeout);
    void savingCompleted();
    void savingFinished(const QString &filePath);
    void loadingFinished();
};

#endif
