/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef KIS_DOCUMENT_UNDO_STACK_P_H
#define KIS_DOCUMENT_UNDO_STACK_P_H

#include <kundo2stack.h>
#include <kis_types.h>
#include <gpu/KisGpuPaintImage.h>
#include <QHash>
#include <QQueue>
#include <QTimer>

class KisDocument;

/** Document-owned history navigation with serialized reentrant requests.
 * GPU command objects describe retained versions; navigation selects those
 * versions through the image owner without executing plugin effects again.
 */
class KisDocumentUndoStack final : public KUndo2Stack
{
public:
    explicit KisDocumentUndoStack(KisDocument *document);
    void setImage(KisImageSP image);
    void beginDocumentClose();
    void setIndex(int index) override;
    void undo() override;
    void redo() override;

protected:
    void notifySetIndexChangedOneCommand() override;

private:
    KisImageSP image();
    bool setIndexImpl(int index, bool notify);
    bool undoImpl(bool notify);
    bool redoImpl(bool notify);
    void processPostponedJobs();
    void synchronizeGpuHistory();

    struct PostponedJob {
        enum Type { Undo, Redo, SetIndex };
        Type type = Undo;
        int index = 0;
        bool notified = false;
    };
    int m_recursionCounter = 0;
    QQueue<PostponedJob> m_postponedJobs;
    QTimer m_retryTimer;
    quint64 m_imageGeneration = 0;
    bool m_closing = false;
    KisDocument *m_document;
    LibrePaint::Image::Gpu::PaintImageSP m_gpuImage;
    QMetaObject::Connection m_gpuChanges;
    QMetaObject::Connection m_gpuSaved;
    QHash<quint64, KUndo2Command *> m_gpuCommands;
    QUuid m_gpuIdentity;
    quint64 m_gpuRevision = 0;
};
#endif
