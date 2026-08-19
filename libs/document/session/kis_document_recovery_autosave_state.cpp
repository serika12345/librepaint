/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_recovery_autosave_state.h"

namespace Krita
{
namespace Document
{

bool RecoveryAutoSaveState::hasPendingRequest() const
{
    return m_pendingRequest;
}

void RecoveryAutoSaveState::beginRequest()
{
    m_pendingRequest = true;
    m_saveStartInProgress = false;
    m_deferredCompletion = Completion();
    m_joinedSavePath.clear();
}

void RecoveryAutoSaveState::cancelRequest()
{
    m_pendingRequest = false;
    m_saveStartInProgress = false;
    m_deferredCompletion = Completion();
    m_joinedSavePath.clear();
}

void RecoveryAutoSaveState::beginSaveStart()
{
    m_saveStartInProgress = true;
    m_deferredCompletion = Completion();
}

RecoveryAutoSaveState::Completion RecoveryAutoSaveState::markSaveStartSucceeded()
{
    m_saveStartInProgress = false;
    m_joinedSavePath.clear();

    const Completion completion = m_deferredCompletion;
    m_deferredCompletion = Completion();
    return completion;
}

void RecoveryAutoSaveState::markSaveStartDidNotSucceed()
{
    m_saveStartInProgress = false;
    m_deferredCompletion = Completion();
}

RecoveryAutoSaveState::CompletionHandling RecoveryAutoSaveState::recordCompletion(const QString &filePath,
                                                                                  bool success)
{
    if (!m_pendingRequest) {
        return CompletionHandling::Ignored;
    }

    if (m_saveStartInProgress) {
        m_deferredCompletion.available = true;
        m_deferredCompletion.filePath = filePath;
        m_deferredCompletion.success = success;
        return CompletionHandling::Deferred;
    }

    cancelRequest();
    return CompletionHandling::Ready;
}

void RecoveryAutoSaveState::recordJoinedSave(const QString &filePath, bool usable)
{
    if (!m_pendingRequest || !usable) {
        m_joinedSavePath.clear();
        return;
    }

    m_joinedSavePath = filePath;
}

QString RecoveryAutoSaveState::joinedSavePath() const
{
    return m_joinedSavePath;
}

void RecoveryAutoSaveState::clearJoinedSavePath()
{
    m_joinedSavePath.clear();
}

}
}
