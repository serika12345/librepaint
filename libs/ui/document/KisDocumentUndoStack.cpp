/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "KisDocumentUndoStack_p.h"
#include <filter/kis_filter.h>
#include <filter/kis_filter_registry.h>
#include "KisDocument.h"
#include <kis_image.h>
#include <kis_node.h>
#include <kis_layer_utils.h>
#include <kis_crop_saved_extra_data.h>
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
    case HistoryAction::DuplicateLayer: return kundo2_i18n("Duplicate Nodes");
    case HistoryAction::RemoveLayer: return kundo2_i18n("Remove layer");
    case HistoryAction::MoveLayer: return kundo2_i18n("Move layer");
    case HistoryAction::TranslateLayer: return kundo2_i18n("Move");
    case HistoryAction::TransformLayer: return kundo2_i18n("Transform");
    case HistoryAction::GroupLayers: return kundo2_i18n("Quick Group");
    case HistoryAction::CreateClippingGroup: return kundo2_i18n("Quick Clipping Group");
    case HistoryAction::UngroupLayers: return kundo2_i18n("Quick Ungroup");
    case HistoryAction::MergeDown: return kundo2_i18n("Merge Down");
    case HistoryAction::MergeSelected: return kundo2_i18n("Merge Selected Nodes");
    case HistoryAction::FlattenLayer: return kundo2_i18n("Flatten Layer");
    case HistoryAction::FlattenImage: return kundo2_i18n("Flatten Image");
    case HistoryAction::NewFromVisible: return kundo2_i18n("New From Visible");
    case HistoryAction::MoveSelectionPixels: return kundo2_i18n("Move Selection");
    case HistoryAction::RenameLayer: return kundo2_i18n("Rename layer");
    case HistoryAction::ChangeOpacity: return kundo2_i18n("Change layer opacity");
    case HistoryAction::ChangeVisibility: return kundo2_i18n("Change layer visibility");
    case HistoryAction::ChangeLock: return kundo2_i18n("Change layer lock");
    case HistoryAction::ChangeChannelLock: return kundo2_i18n("Change layer channel protection");
    case HistoryAction::ChangeProjectionChannels: return kundo2_i18n("Change layer channel visibility");
    case HistoryAction::ChangeLayerProperties: return kundo2_i18n("Change Layer Properties");
    case HistoryAction::ChangePassThrough: return kundo2_i18n("Change group pass-through mode");
    case HistoryAction::ChangeBlendMode: return kundo2_i18n("Change blending mode");
    case HistoryAction::ChangeSelection: return kundo2_i18n("Change selection");
    case HistoryAction::AddTransparencyMask: return kundo2_i18n("Add transparency mask");
    case HistoryAction::RemoveTransparencyMask: return kundo2_i18n("Remove transparency mask");
    case HistoryAction::ChangeMaskVisibility: return kundo2_i18n("Change transparency mask visibility");
    case HistoryAction::ChangeTimelinePin: return kundo2_i18n("Change timeline pin");
    case HistoryAction::PaintStroke: return kundo2_i18n("Paint stroke");
    case HistoryAction::DrawLine: return kundo2_i18n("Draw line");
    case HistoryAction::DrawEllipse: return kundo2_i18n("Draw ellipse");
    case HistoryAction::DrawRectangle: return kundo2_i18n("Draw rectangle");
    case HistoryAction::DrawPolyline: return kundo2_i18n("Draw polyline");
    case HistoryAction::DrawPolygon: return kundo2_i18n("Draw polygon");
    case HistoryAction::DrawPath: return kundo2_i18n("Draw path");
    case HistoryAction::ReplacePixels: return kundo2_i18n("Replace pixels");
    case HistoryAction::ClearPixels: return kundo2_i18n("Clear");
    case HistoryAction::FillLayer: return kundo2_i18n("Flood Fill Layer");
    case HistoryAction::Gradient: return kundo2_i18n("Gradient");
    case HistoryAction::FloodFill: return kundo2_i18n("Flood Fill");
    case HistoryAction::SmartPatch: return kundo2_i18n("Smart Patch");
    case HistoryAction::EncloseAndFill: return kundo2_i18n("Enclose and Fill");
    case HistoryAction::InvertColors: return kundo2_i18n("Invert Colors");
    case HistoryAction::Grayscale: return kundo2_i18n("Grayscale");
    case HistoryAction::ConvertColorSpace: return kundo2_i18n("Convert Image Color Space");
    case HistoryAction::HsvAdjustment: return kundo2_i18n("HSV Adjustment");
    case HistoryAction::AddFilterMask: return kundo2_i18n("Add Filter Mask");
    case HistoryAction::AddAdjustmentLayer: return kundo2_i18n("Add Filter Layer");
    case HistoryAction::ChangeFilter: return kundo2_i18n("Change Filter");
    case HistoryAction::MaximizeChannel: return kundo2_i18n("Maximize Channel");
    case HistoryAction::MinimizeChannel: return kundo2_i18n("Minimize Channel");
    case HistoryAction::ResetTransparent: return kundo2_i18n("Reset Transparent");
    case HistoryAction::ColorOverlay: return kundo2_i18n("Fast Color Overlay");
    case HistoryAction::Sharpen: return kundo2_i18n("Sharpen");
    case HistoryAction::MeanRemoval: return kundo2_i18n("Mean Removal");
    case HistoryAction::EmbossLaplacian: return kundo2_i18n("Emboss (Laplacian)");
    case HistoryAction::EmbossAllDirections: return kundo2_i18n("Emboss in All Directions");
    case HistoryAction::EmbossHorizontalVertical: return kundo2_i18n("Emboss Horizontal & Vertical");
    case HistoryAction::EmbossVertical: return kundo2_i18n("Emboss Vertical Only");
    case HistoryAction::EmbossHorizontal: return kundo2_i18n("Emboss Horizontal Only");
    case HistoryAction::CropLayer: return kundo2_i18n("Crop Layer");
    case HistoryAction::CropImage: return kundo2_i18n("Crop Image");
    case HistoryAction::ResizeCanvas: return kundo2_i18n("Resize Image");
    }
    Q_UNREACHABLE();
}
}

KisDocumentUndoStack::KisDocumentUndoStack(KisDocument *document)
    : KUndo2Stack(document), m_retryTimer(this), m_document(document)
{
    m_retryTimer.setSingleShot(true);
    m_retryTimer.setInterval(16);
    connect(&m_retryTimer, &QTimer::timeout, this, &KisDocumentUndoStack::processPostponedJobs);
}

void KisDocumentUndoStack::setImage(KisImageSP image)
{
    if (m_closing) return;
    ++m_imageGeneration;
    m_postponedJobs.clear();
    m_retryTimer.stop();
    const auto owner = image ? image->gpuImage() : PaintImageSP();
    if (!m_gpuImage && !owner) return;
    disconnect(m_gpuChanges);
    disconnect(m_gpuSaved);
    m_gpuImage = owner;
    m_gpuCommands.clear();
    m_gpuIdentity = QUuid{};
    KUndo2Stack::clear();
    if (!owner) return;
    m_gpuChanges = connect(owner.data(), &PaintImage::changed, this, [this] { synchronizeGpuHistory(); });
    m_gpuSaved = connect(owner.data(), &PaintImage::savedStateChanged, this, [this] { synchronizeGpuHistory(); });
    synchronizeGpuHistory();
}

void KisDocumentUndoStack::beginDocumentClose()
{
    m_closing = true;
    ++m_imageGeneration;
    m_postponedJobs.clear();
    m_retryTimer.stop();
    disconnect(m_gpuChanges);
    disconnect(m_gpuSaved);
}

bool KisDocumentUndoStack::prepareCpuMigration()
{
    if (m_closing || m_recursionCounter || !m_postponedJobs.isEmpty() || !m_gpuImage) return false;
    const auto currentImage = image();
    if (!currentImage || currentImage->gpuImage() != m_gpuImage) return false;
    if (index() != m_gpuImage->operations().undoCount()) return false;
    QList<KUndo2Command *> current;
    for (int i = 0; i < count(); ++i) current.append(const_cast<KUndo2Command *>(command(i)));
    // Reusing the same command list checks for an open native macro without
    // changing history, emitting navigation signals or executing any command.
    return synchronizeCommands(std::move(current), index(), cleanIndex());
}

void KisDocumentUndoStack::adoptCpuMigration()
{
    // The migrated CPU document starts a new history. The old GPU versions are
    // intentionally discarded; the document owner preserves file and modified
    // state around this cut.
    ++m_imageGeneration;
    m_retryTimer.stop();
    disconnect(m_gpuChanges);
    disconnect(m_gpuSaved);
    m_gpuImage.clear();
    m_gpuCommands.clear();
    m_gpuIdentity = {};
    m_gpuRevision = 0;
    KUndo2Stack::clear();
}

void KisDocumentUndoStack::synchronizeGpuHistory()
{
    if (m_closing) return;
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
            auto label = historyLabel(document.historyAction(index));
            if (const auto filter = KisFilterRegistry::instance()->value(document.historyFilterId(index)))
                label = kundo2_i18nc("Filter as an effect", "Filter \"%1\"", filter->name());
            if (const auto crop = document.historyCrop(index)) {
                bool mask = !crop->maskUuid.isNull();
                for (const auto &layer : versions[index]->layers)
                    if (layer.id == crop->layer) mask |= layer.kind == LayerKind::FilterMask;
                if (mask) label = kundo2_i18n("Crop Mask");
            }
            command = new KUndo2Command(label);
            allocated.append(command);
        }
        if (!command->extraData()) {
            if (const auto resize = document.historyCanvasResize(index))
                command->setExtraData(new KisCropSavedExtraData(resize->cropLayers ? KisCropSavedExtraData::CROP_IMAGE :
                                                                                 KisCropSavedExtraData::RESIZE_IMAGE,
                                                              resize->rect));
            if (const auto crop = document.historyCrop(index)) {
                QUuid uuid = crop->maskUuid;
                if (uuid.isNull()) for (const auto &layer : versions[index]->layers)
                    if (layer.id == crop->layer) uuid = layer.uuid;
                const auto currentImage = image();
                const auto node = currentImage ? KisLayerUtils::findNodeByUuid(currentImage->root(), uuid) : KisNodeSP();
                if (node) {
                    command->setExtraData(new KisCropSavedExtraData(KisCropSavedExtraData::CROP_LAYER, crop->rect, node));
                }
            }
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
    if (m_closing) return;
    m_postponedJobs.append({PostponedJob::SetIndex, index});
    processPostponedJobs();
}

void KisDocumentUndoStack::undo()
{
    if (m_closing) return;
    m_postponedJobs.append({PostponedJob::Undo, 0});
    processPostponedJobs();
}

void KisDocumentUndoStack::redo()
{
    if (m_closing) return;
    m_postponedJobs.append({PostponedJob::Redo, 0});
    processPostponedJobs();
}

KisImageSP KisDocumentUndoStack::image()
{
    return m_document->image();
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

bool KisDocumentUndoStack::setIndexImpl(int index, bool notify)
{
    if (const auto owner = m_gpuImage) {
        if (owner->operations().hasActiveStroke()) owner->cancelStroke(owner->operations().activeOperation());
        owner->setHistoryIndex(std::clamp(index, 0, owner->operations().undoCount() + owner->operations().redoCount()));
        return true;
    }
    KisImageSP currentImage = image();
    if (!currentImage) return true;
    if (notify) {
        const auto generation = m_imageGeneration;
        QPointer<KisDocumentUndoStack> guard(this);
        currentImage->requestStrokeCancellation();
        if (!guard || generation != m_imageGeneration) return true;
    }
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::setIndex(index);
        currentImage->unlock();
        return true;
    }
    return false;
}

bool KisDocumentUndoStack::undoImpl(bool notify)
{
    if (const auto owner = m_gpuImage) {
        if (owner->operations().hasActiveStroke()) owner->cancelStroke(owner->operations().activeOperation());
        else owner->undo();
        return true;
    }
    KisImageSP currentImage = image();
    if (!currentImage) return true;
    if (notify) {
        const bool toolOwnsInput = currentImage->hasOpenStrokes();
        const auto generation = m_imageGeneration;
        QPointer<KisDocumentUndoStack> guard(this);
        currentImage->requestUndoDuringStroke();
        if (!guard || generation != m_imageGeneration || toolOwnsInput) return true;
        if (currentImage->tryUndoUnfinishedLod0Stroke() == UNDO_OK) return true;
    }
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::undo();
        currentImage->unlock();
        return true;
    }
    return false;
}

bool KisDocumentUndoStack::redoImpl(bool notify)
{
    if (const auto owner = m_gpuImage) {
        if (!owner->operations().hasActiveStroke()) owner->redo();
        return true;
    }
    KisImageSP currentImage = image();
    if (!currentImage) return true;
    if (notify) {
        const bool toolOwnsInput = currentImage->hasOpenStrokes();
        const auto generation = m_imageGeneration;
        QPointer<KisDocumentUndoStack> guard(this);
        currentImage->requestRedoDuringStroke();
        if (!guard || generation != m_imageGeneration || toolOwnsInput) return true;
    }
    if (currentImage->tryBarrierLock()) {
        KUndo2Stack::redo();
        currentImage->unlock();
        return true;
    }
    return false;
}

void KisDocumentUndoStack::processPostponedJobs()
{
    // Blocking command notifications can reenter navigation from the history panel.
    if (m_closing || m_recursionCounter > 0) return;
    ++m_recursionCounter;
    QPointer<KisDocumentUndoStack> guard(this);
    while (!m_postponedJobs.isEmpty()) {
        PostponedJob job = m_postponedJobs.dequeue();
        const auto generation = m_imageGeneration;
        const bool notify = !job.notified;
        job.notified = true;
        bool completed = false;
        switch (job.type) {
        case PostponedJob::SetIndex: completed = setIndexImpl(job.index, notify); break;
        case PostponedJob::Redo: completed = redoImpl(notify); break;
        case PostponedJob::Undo: completed = undoImpl(notify); break;
        }
        if (!guard) return;
        if (!completed && generation == m_imageGeneration) {
            m_postponedJobs.prepend(job);
            break;
        }
    }
    --m_recursionCounter;
    if (!m_postponedJobs.isEmpty()) m_retryTimer.start();
}
