/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_io_presentation.h"

#include <KisImportExportErrorCode.h>

#include <KLocalizedString>

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextEdit>

namespace
{
constexpr int errorMessageTimeout = 5000;
constexpr int successMessageTimeout = 1000;

class DocumentIoMessageDialog : public QMessageBox
{
public:
    DocumentIoMessageDialog(const QString &title,
                            const QString &message,
                            const QStringList &warnings = {},
                            const QString &details = {})
        : QMessageBox(QMessageBox::Warning,
                      title,
                      message,
                      QMessageBox::Ok,
                      qApp->activeWindow())
    {
        if (!details.isEmpty()) {
            setInformativeText(details);
        }
        if (!warnings.isEmpty()) {
            setWarningDetails(warnings);
        }
    }

private:
    void setWarningDetails(const QStringList &warnings)
    {
        QMessageBox::setDetailedText(warnings.first());

        QTextEdit *messageBox = findChild<QTextEdit *>();
        if (!messageBox) {
            return;
        }

        messageBox->setAcceptRichText(true);

        QString warning = QStringLiteral("<html><body><ul>");
        for (const QString &item : warnings) {
            warning += QStringLiteral("\n<li>") + item + QStringLiteral("</li>");
        }
        warning += QStringLiteral("</ul></body></html>");
        messageBox->setText(warning);
    }
};
}

KisDocumentIoPresentation::KisDocumentIoPresentation(QObject *parent)
    : QObject(parent)
{
}

void KisDocumentIoPresentation::presentSaveResult(
    const QString &filePath,
    const KisImportExportErrorCode &status,
    const QString &errorMessage,
    const QString &warningMessage,
    bool batchMode)
{
    if (status.isCancelled()) {
        return;
    }

    const QString fileName = QFileInfo(filePath).fileName();

    if (!status.isOk()) {
        Q_EMIT statusBarMessage(i18nc("%1 --- failing file name, %2 --- error message",
                                    "Error during saving %1: %2",
                                    fileName,
                                    errorMessage),
                                errorMessageTimeout);

        if (!batchMode) {
            DocumentIoMessageDialog dialog(
                i18nc("@title:window", "LibrePaint"),
                i18n("Could not save %1.", filePath),
                errorMessage.split("\n", Qt::SkipEmptyParts)
                    + warningMessage.split("\n", Qt::SkipEmptyParts),
                status.errorMessage());
            dialog.exec();
        }
        return;
    }

    if (!batchMode && !warningMessage.isEmpty()) {
        const QStringList reasons = warningMessage.split("\n", Qt::SkipEmptyParts);
        DocumentIoMessageDialog dialog(
            i18nc("@title:window", "LibrePaint"),
            i18nc("dialog box shown to the user if there were warnings while saving the document, "
                  "%1 is the file path",
                  "%1 has been saved but is incomplete.",
                  filePath),
            reasons,
            reasons.isEmpty()
                ? QString()
                : i18nc("dialog box shown to the user if there were warnings while saving the document",
                        "Some problems were encountered when saving."));
        dialog.exec();
    }
}

void KisDocumentIoPresentation::notifySaveSucceeded(const QString &filePath)
{
    Q_EMIT savingCompleted();
    Q_EMIT savingFinished(filePath);
    Q_EMIT statusBarMessage(i18n("Finished saving %1", QFileInfo(filePath).fileName()),
                            successMessageTimeout);
}

void KisDocumentIoPresentation::presentMissingFile(const QString &filePath, bool batchMode)
{
    if (batchMode) {
        return;
    }

    QMessageBox::critical(qApp->activeWindow(),
                          i18nc("@title:window", "LibrePaint"),
                          i18n("File %1 does not exist.", filePath));
}

void KisDocumentIoPresentation::presentLoadFailure(
    const QString &prettyPath,
    const KisImportExportErrorCode &status,
    const QString &errorMessage,
    const QString &warningMessage,
    bool batchMode)
{
    const QString details = status.errorMessage();
    if (details.isEmpty() || batchMode) {
        return;
    }

    DocumentIoMessageDialog dialog(
        i18nc("@title:window", "LibrePaint"),
        i18n("Could not open %1.", prettyPath),
        errorMessage.split("\n", Qt::SkipEmptyParts)
            + warningMessage.split("\n", Qt::SkipEmptyParts),
        details);
    dialog.exec();
}

void KisDocumentIoPresentation::presentLoadWarning(const QString &prettyPath,
                                                   const QString &warningMessage,
                                                   bool batchMode)
{
    if (warningMessage.isEmpty() || batchMode) {
        return;
    }

    DocumentIoMessageDialog dialog(
        i18nc("@title:window", "LibrePaint"),
        i18n("There were problems opening %1.", prettyPath),
        warningMessage.split("\n", Qt::SkipEmptyParts));
    dialog.exec();
}

void KisDocumentIoPresentation::notifyLoadSucceeded()
{
    Q_EMIT loadingFinished();
}

void KisDocumentIoPresentation::notifyAutoSaveStarted(const QString &filePath)
{
    Q_EMIT statusBarMessage(i18n("Autosaving... %1", filePath), successMessageTimeout);
}

void KisDocumentIoPresentation::notifyAutoSavePostponed()
{
    Q_EMIT statusBarMessage(i18n("Autosaving postponed: document is busy..."),
                            errorMessageTimeout);
}

void KisDocumentIoPresentation::notifyAutoSaveFinished(const QString &fileName)
{
    Q_EMIT statusBarMessage(i18n("Finished autosaving %1", fileName), successMessageTimeout);
}

void KisDocumentIoPresentation::notifyAutoSaveFailed(
    const QString &fileName,
    const KisImportExportErrorCode &status,
    const QString &errorMessage)
{
    const QString userMessage = errorMessage.isEmpty() ? status.errorMessage() : errorMessage;
    Q_EMIT statusBarMessage(i18nc("%1 --- failing file name, %2 --- error message",
                                "Error during autosaving %1: %2",
                                fileName,
                                userMessage),
                            errorMessageTimeout);
}
