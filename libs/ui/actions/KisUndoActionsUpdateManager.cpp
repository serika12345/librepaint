/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisUndoActionsUpdateManager.h"
#include <klocalizedstring.h>
#include "KoID.h"

#include <QAction>
#include <kundo2stack.h>

#include <document/KisDocument.h>
#include <kis_image.h>
#include <gpu/KisGpuPaintImage.h>
#include <qobject.h>
#include <qobjectdefs.h>


KisUndoActionsUpdateManager::KisUndoActionsUpdateManager(QAction *undoAction, QAction *redoAction, QObject *parent)
    : QObject(parent),
      m_undoAction(undoAction),
      m_redoAction(redoAction)
{

}

void KisUndoActionsUpdateManager::setCurrentDocument(KisDocument *document)
{
    m_documentConnections.clear();

    if (document) {

        KUndo2Stack *stack = document->undoStack();

        m_documentConnections.addConnection(stack, SIGNAL(undoTextChanged(QString)), this, SLOT(slotUndoTextChanged(QString)));
        m_documentConnections.addConnection(stack, SIGNAL(redoTextChanged(QString)), this, SLOT(slotRedoTextChanged(QString)));
        m_documentConnections.addConnection(stack, &KUndo2Stack::canUndoChanged, m_undoAction, &QAction::setEnabled);
        m_documentConnections.addConnection(stack, &KUndo2Stack::canRedoChanged, m_redoAction, &QAction::setEnabled);

        slotUndoTextChanged(stack->undoText());
        slotRedoTextChanged(stack->redoText());

        const auto updateActions = [this, document = QPointer<KisDocument>(document)] {
            const auto owner = document && document->image() ? document->image()->gpuImage() : nullptr;
            const bool pending = owner && owner->operations().hasActiveStroke();
            m_undoAction->setEnabled(document && (document->undoStack()->canUndo() || pending));
            m_redoAction->setEnabled(document && document->undoStack()->canRedo() && !pending);
        };
        if (const auto owner = document->image() ? document->image()->gpuImage() : nullptr)
            m_documentConnections.addConnection(owner.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this, updateActions);
        updateActions();
    } else {
        m_undoAction->setEnabled(false);
        m_redoAction->setEnabled(false);
    }
}

void KisUndoActionsUpdateManager::slotUndoTextChanged(const QString &text)
{
    m_undoAction->setText(i18n("Undo %1", text));
}

void KisUndoActionsUpdateManager::slotRedoTextChanged(const QString &text)
{
    m_redoAction->setText(i18n("Redo %1", text));
}
