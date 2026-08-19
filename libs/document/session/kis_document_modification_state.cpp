/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_modification_state.h"

namespace Krita
{
namespace Document
{

bool ModificationState::isModified() const
{
    return m_modified;
}

bool ModificationState::setModified(bool modified)
{
    m_hasChangesAfterAutoSave = modified;
    m_modifiedWhileSaving = modified;

    if (!modified) {
        m_imageModifiedWithoutUndo = false;
    }

    const bool changed = modified != m_modified;
    m_modified = modified;
    return changed;
}

bool ModificationState::hasChangesAfterAutoSave() const
{
    return m_hasChangesAfterAutoSave;
}

void ModificationState::markAutoSaveStarted()
{
    m_hasChangesAfterAutoSave = false;
}

void ModificationState::restoreAutoSaveRequirement()
{
    m_hasChangesAfterAutoSave = m_modified;
}

bool ModificationState::wasModifiedWhileSaving() const
{
    return m_modifiedWhileSaving;
}

void ModificationState::markSavingStarted()
{
    m_modifiedWhileSaving = false;
}

bool ModificationState::imageModifiedWithoutUndo() const
{
    return m_imageModifiedWithoutUndo;
}

void ModificationState::markImageModifiedWithoutUndo()
{
    m_imageModifiedWithoutUndo = true;
}

void ModificationState::clearImageModifiedWithoutUndo()
{
    m_imageModifiedWithoutUndo = false;
}

ModificationState ModificationState::snapshotState() const
{
    ModificationState snapshot;
    snapshot.m_modified = m_modified;
    snapshot.m_imageModifiedWithoutUndo = m_imageModifiedWithoutUndo;
    return snapshot;
}

}
}
