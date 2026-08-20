/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_document_undo_store.h"

#include <kundo2stack.h>

#include <kis_assert.h>

KisDocumentUndoStore::KisDocumentUndoStore(KUndo2Stack *undoStack)
    : m_undoStack(undoStack)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_undoStack);

    KIS_SAFE_ASSERT_RECOVER_NOOP(m_undoStack->thread() == thread());

    // Use a direct connection to avoid queueing the signal forwarding
    // (BUG:447985).
    connect(m_undoStack,
            &KUndo2Stack::indexChanged,
            this,
            &KisUndoStore::historyStateChanged,
            Qt::DirectConnection);
}

const KUndo2Command *KisDocumentUndoStore::presentCommand()
{
    return m_undoStack->command(m_undoStack->index() - 1);
}

void KisDocumentUndoStore::undoLastCommand()
{
    /**
     * FIXME: Added as a workaround for being able to cancel creation
     * of the new adjustment mask (or any other mask whose
     * creation can be cancelled).
     *
     * Ideally, we should use "addToIndex-commit" technique like git does.
     * When a user presses Create button, we call command->redo()
     * and save this command in a cache. When the user confirms creation
     * of the layer with "OK" button, we "commit" the command to the undoStack.
     * If the user changes his mind and presses Cancel, we just call
     * command->undo() and remove the cache without committing it
     * to the undoStack
     */
    m_undoStack->undo();
}

void KisDocumentUndoStore::addCommand(KUndo2Command *command)
{
    if (!command) {
        return;
    }
    m_undoStack->push(command);
}

void KisDocumentUndoStore::beginMacro(const KUndo2MagicString &macroName)
{
    m_undoStack->beginMacro(macroName);
}

void KisDocumentUndoStore::endMacro()
{
    m_undoStack->endMacro();
}

void KisDocumentUndoStore::purgeRedoState()
{
    m_undoStack->purgeRedoState();
}
