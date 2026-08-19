/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_autosave_state.h"

namespace Krita
{
namespace Document
{

bool AutoSaveState::isExportingAutoSave() const
{
    return m_exportingAutoSave;
}

void AutoSaveState::markAutoSaveExportStarted()
{
    m_exportingAutoSave = true;
}

void AutoSaveState::markAutoSaveExportFinished()
{
    m_exportingAutoSave = false;
}

bool AutoSaveState::shouldUseCloneFallback() const
{
    return m_failureCount >= cloneFallbackFailureThreshold;
}

void AutoSaveState::recordFailure()
{
    ++m_failureCount;
}

void AutoSaveState::clearFailures()
{
    m_failureCount = 0;
}

}
}
