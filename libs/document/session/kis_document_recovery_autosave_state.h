/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_RECOVERY_AUTOSAVE_STATE_H
#define KIS_DOCUMENT_RECOVERY_AUTOSAVE_STATE_H

#include <QString>

#include <kritadocument_export.h>

namespace Krita
{
namespace Document
{

/**
 * Coordinates the state of one non-blocking recovery autosave request.
 *
 * Save scheduling, document cloning, file validation, notifications, and
 * recovery policy remain with the document session coordinator.
 */
class KRITADOCUMENT_EXPORT RecoveryAutoSaveState
{
public:
    struct Completion {
        bool available = false;
        QString filePath;
        bool success = false;
    };

    enum class CompletionHandling {
        Ignored,
        Deferred,
        Ready
    };

    bool hasPendingRequest() const;
    void beginRequest();
    void cancelRequest();

    void beginSaveStart();
    Completion markSaveStartSucceeded();
    void markSaveStartDidNotSucceed();

    CompletionHandling recordCompletion(const QString &filePath, bool success);

    void recordJoinedSave(const QString &filePath, bool usable);
    QString joinedSavePath() const;
    void clearJoinedSavePath();

private:
    bool m_pendingRequest = false;
    bool m_saveStartInProgress = false;
    Completion m_deferredCompletion;
    QString m_joinedSavePath;
};

}
}

#endif
