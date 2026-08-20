/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_DOCUMENT_UNDO_STORE_H
#define __KIS_DOCUMENT_UNDO_STORE_H

#include <kis_undo_store.h>

#include <kritadocumentui_export.h>

class KUndo2Stack;

/**
 * Connects an image-owned undo store to a document-owned undo stack.
 *
 * The stack is borrowed and must outlive this store. Both objects must belong
 * to the same thread so history notifications can be forwarded synchronously.
 */
class KRITADOCUMENTUI_EXPORT KisDocumentUndoStore : public KisUndoStore
{
public:
    explicit KisDocumentUndoStore(KUndo2Stack *undoStack);

    const KUndo2Command *presentCommand() override;
    void undoLastCommand() override;
    void addCommand(KUndo2Command *command) override;
    void beginMacro(const KUndo2MagicString &macroName) override;
    void endMacro() override;
    void purgeRedoState() override;

private:
    KUndo2Stack *m_undoStack;
};

#endif /* __KIS_DOCUMENT_UNDO_STORE_H */
