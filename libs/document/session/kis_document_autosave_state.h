/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_AUTOSAVE_STATE_H
#define KIS_DOCUMENT_AUTOSAVE_STATE_H

#include <kritadocument_export.h>

namespace Krita
{
namespace Document
{

/**
 * Tracks progress and retry escalation for document autosave operations.
 *
 * Timer scheduling, document cloning, file output, notifications, and
 * recovery coordination remain with the document session coordinator.
 */
class KRITADOCUMENT_EXPORT AutoSaveState
{
public:
    bool isExportingAutoSave() const;
    void markAutoSaveExportStarted();
    void markAutoSaveExportFinished();

    bool shouldUseCloneFallback() const;
    void recordFailure();
    void clearFailures();

private:
    static constexpr int cloneFallbackFailureThreshold = 3;

    bool m_exportingAutoSave = false;
    int m_failureCount = 0;
};

}
}

#endif
