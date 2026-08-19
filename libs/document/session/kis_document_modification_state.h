/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DOCUMENT_MODIFICATION_STATE_H
#define KIS_DOCUMENT_MODIFICATION_STATE_H

#include <kritadocument_export.h>

namespace Krita
{
namespace Document
{

/**
 * Tracks whether a document session has unsaved changes.
 *
 * The state distinguishes the document's current modified flag from changes
 * made after an autosave checkpoint, changes made while a save is running,
 * and image changes that cannot be represented by the undo history. Timers,
 * save execution, editing-time updates, and notifications remain with the
 * document session coordinator.
 */
class KRITADOCUMENT_EXPORT ModificationState
{
public:
    bool isModified() const;

    /**
     * Applies a public modified-state request and returns whether the primary
     * modified flag changed. Repeated true requests still record a change for
     * autosave and an in-progress save.
     */
    bool setModified(bool modified);

    bool hasChangesAfterAutoSave() const;
    void markAutoSaveStarted();
    void restoreAutoSaveRequirement();

    bool wasModifiedWhileSaving() const;
    void markSavingStarted();

    bool imageModifiedWithoutUndo() const;
    void markImageModifiedWithoutUndo();
    void clearImageModifiedWithoutUndo();

    /**
     * Copies document change state for a saving snapshot without copying the
     * source session's active save and autosave progress.
     */
    ModificationState snapshotState() const;

private:
    bool m_modified = false;
    bool m_hasChangesAfterAutoSave = false;
    bool m_modifiedWhileSaving = false;
    bool m_imageModifiedWithoutUndo = false;
};

}
}

#endif
