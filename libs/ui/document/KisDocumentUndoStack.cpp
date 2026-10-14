/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "KisDocumentUndoStack_p.h"
#include "KisDocument.h"
#include <kis_image.h>
#include <QApplication>
#include <QPointer>
#include <QSet>
#include <algorithm>

namespace {
using namespace LibrePaint::Image::Gpu;
KUndo2MagicString historyLabel(HistoryAction action)
{
    switch (action) {
    case HistoryAction::None: return {};
    case HistoryAction::AddLayer: return kundo2_i18n("Add layer");
    case HistoryAction::RemoveLayer: return kundo2_i18n("Remove layer");
    case HistoryAction::MoveLayer: return kundo2_i18n("Move layer");
    case HistoryAction::RenameLayer: return kundo2_i18n("Rename layer");
    case HistoryAction::ChangeOpacity: return kundo2_i18n("Change layer opacity");
    case HistoryAction::ChangeVisibility: return kundo2_i18n("Change layer visibility");
    case HistoryAction::ChangeLock: return kundo2_i18n("Change layer lock");
    case HistoryAction::ChangeTimelinePin: return kundo2_i18n("Change timeline pin");
    case HistoryAction::PaintStroke: return kundo2_i18n("Paint stroke");
    case HistoryAction::DrawLine: return kundo2_i18n("Draw line");
    case HistoryAction::DrawEllipse: return kundo2_i18n("Draw ellipse");
    case HistoryAction::ReplacePixels: return kundo2_i18n("Replace pixels");
    }
    Q_UNREACHABLE();
}
}

KisDocumentUndoStack::KisDocumentUndoStack(KisDocument *document)
    : KUndo2Stack(document), m_document(document)
{
}

void KisDocumentUndoStack::setImage(KisImageSP image)
{
    const auto owner = image ? image->gpuImage() : PaintImageSP();
    if (!m_gpuImage && !owner) return;
    disconnect(m_gpuChanges);
    disconnect(m_gpuSaved);
    m_gpuImage = owner;
    m_gpuCommands.clear();
    m_gpuIdentity = {};
    KUndo2Stack::clear();
    if (!owner) return;
    m_gpuChanges = connect(owner.data(), &PaintImage::changed, this, [this] { synchronizeGpuHistory(); });
    m_gpuSaved = connect(owner.data(), &PaintImage::savedStateChanged, this, [this] { synchronizeGpuHistory(); });
    synchronizeGpuHistory();
}

void KisDocumentUndoStack::synchronizeGpuHistory()
{
    const auto owner = m_gpuImage;
    if (!owner) return;
    const auto &document = owner->operations();
    const auto current = document.snapshot();
    const int savedIndex = document.savedHistoryIndex();
    if (m_gpuIdentity == current->documentId() && m_gpuRevision == current->revision() &&
        index() == document.undoCount() && count() == document.undoCount() + document.redoCount() &&
        cleanIndex() == savedIndex) return;
    const auto versions = document.historyVersions();
    const bool sameIdentity = m_gpuIdentity == versions.first()->documentId();
    QSet<const KUndo2Command *> owned;
    for (int index = 0; index < count(); ++index) owned.insert(command(index));
    QList<KUndo2Command *> commands;
    QList<KUndo2Command *> allocated;
    QHash<quint64, KUndo2Command *> retained;
    for (int index = 1; index < versions.size(); ++index) {
        const quint64 revision = versions[index]->historyRevision();
        auto *command = sameIdentity ? m_gpuCommands.value(revision) : nullptr;
        if (!owned.contains(command)) command = nullptr;
        if (!command) {
            command = new KUndo2Command(historyLabel(document.historyAction(index)));
            allocated.append(command);
        }
        commands.append(command);
        retained.insert(revision, command);
    }
    const auto previousCommands = m_gpuCommands;
    const auto previousIdentity = m_gpuIdentity;
    const auto previousRevision = m_gpuRevision;
    m_gpuIdentity = versions.first()->documentId();
    m_gpuRevision = current->revision();
    m_gpuCommands = std::move(retained);
    const bool synchronized = synchronizeCommands(std::move(commands), document.undoCount(), savedIndex);
    if (!synchronized) {
        qDeleteAll(allocated);
        m_gpuCommands = previousCommands;
        m_gpuIdentity = previousIdentity;
        m_gpuRevision = previousRevision;
        qWarning("GPU history cannot replace a pending legacy command macro");
    }
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
    if (const auto owner = m_gpuImage) {
        if (owner->operations().hasActiveStroke()) owner->cancelStroke(owner->operations().activeOperation());
        owner->setHistoryIndex(std::clamp(index, 0, owner->operations().undoCount() + owner->operations().redoCount()));
        return;
    }
    KisImageWSP currentImage = image();
    currentImage->requestStrokeCancellation();
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::setIndex(index);
        currentImage->unlock();
    }
}

void KisDocumentUndoStack::undoImpl()
{
    if (const auto owner = m_gpuImage) {
        if (owner->operations().hasActiveStroke()) owner->cancelStroke(owner->operations().activeOperation());
        else owner->undo();
        return;
    }
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
    if (const auto owner = m_gpuImage) {
        if (!owner->operations().hasActiveStroke()) owner->redo();
        return;
    }
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
    QPointer<KisDocumentUndoStack> guard(this);
    while (!m_postponedJobs.isEmpty()) {
        const PostponedJob job = m_postponedJobs.dequeue();
        switch (job.type) {
        case PostponedJob::SetIndex: setIndexImpl(job.index); break;
        case PostponedJob::Redo: redoImpl(); break;
        case PostponedJob::Undo: undoImpl(); break;
        }
        if (!guard) return;
    }
    --m_recursionCounter;
}
