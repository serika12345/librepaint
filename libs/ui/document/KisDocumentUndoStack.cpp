/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "KisDocumentUndoStack_p.h"
#include "KisDocument.h"
#include <kis_image.h>
#include <QApplication>

KisDocumentUndoStack::KisDocumentUndoStack(KisDocument *document)
    : KUndo2Stack(document), m_document(document)
{
}

void KisDocumentUndoStack::setIndex(int index)
{
    m_postponedJobs.append({PostponedJob::SetIndex, index});
    processPostponedJobs();
}

void KisDocumentUndoStack::undo()
{
    m_postponedJobs.append({PostponedJob::Undo, 0});
    processPostponedJobs();
}

void KisDocumentUndoStack::redo()
{
    m_postponedJobs.append({PostponedJob::Redo, 0});
    processPostponedJobs();
}

KisImageWSP KisDocumentUndoStack::image()
{
    KisImageWSP currentImage = m_document->image();
    Q_ASSERT(currentImage);
    return currentImage;
}

void KisDocumentUndoStack::notifySetIndexChangedOneCommand()
{
    KisImageWSP currentImage = image();
    currentImage->unlock();
    // Commands may deliver blocking GUI notifications while the image is locked.
    while (!currentImage->tryBarrierLock()) {
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

void KisDocumentUndoStack::setIndexImpl(int index)
{
    KisImageWSP currentImage = image();
    currentImage->requestStrokeCancellation();
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::setIndex(index);
        currentImage->unlock();
    }
}

void KisDocumentUndoStack::undoImpl()
{
    KisImageWSP currentImage = image();
    currentImage->requestUndoDuringStroke();
    if (currentImage->tryUndoUnfinishedLod0Stroke() == UNDO_OK) return;
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::undo();
        currentImage->unlock();
    }
}

void KisDocumentUndoStack::redoImpl()
{
    KisImageWSP currentImage = image();
    currentImage->requestRedoDuringStroke();
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::redo();
        currentImage->unlock();
    }
}

void KisDocumentUndoStack::processPostponedJobs()
{
    // Blocking command notifications can reenter navigation from the history panel.
    if (m_recursionCounter > 0) return;
    ++m_recursionCounter;
    while (!m_postponedJobs.isEmpty()) {
        const PostponedJob job = m_postponedJobs.dequeue();
        switch (job.type) {
        case PostponedJob::SetIndex: setIndexImpl(job.index); break;
        case PostponedJob::Redo: redoImpl(); break;
        case PostponedJob::Undo: undoImpl(); break;
        }
    }
    --m_recursionCounter;
}
