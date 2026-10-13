/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef KIS_DOCUMENT_UNDO_STACK_P_H
#define KIS_DOCUMENT_UNDO_STACK_P_H

#include <kundo2stack.h>
#include <kis_types.h>
#include <QQueue>

class KisDocument;

/** Document-owned history navigation with serialized reentrant requests. */
class KisDocumentUndoStack final : public KUndo2Stack
{
public:
    explicit KisDocumentUndoStack(KisDocument *document);
    void setIndex(int index) override;
    void undo() override;
    void redo() override;

protected:
    void notifySetIndexChangedOneCommand() override;

private:
    KisImageWSP image();
    void setIndexImpl(int index);
    void undoImpl();
    void redoImpl();
    void processPostponedJobs();

    struct PostponedJob {
        enum Type { Undo, Redo, SetIndex };
        Type type = Undo;
        int index = 0;
    };
    int m_recursionCounter = 0;
    QQueue<PostponedJob> m_postponedJobs;
    KisDocument *m_document;
};
#endif
