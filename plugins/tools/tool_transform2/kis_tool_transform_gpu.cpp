/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_tool_transform.h"
#include "kis_transform_utils.h"
#include "kis_transform_strategy_base.h"
#include <gpu/KisGpuExecution.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_transparency_mask.h>
#include <kis_transform_worker.h>
#include <algorithm>

namespace Gpu = LibrePaint::Image::Gpu;
namespace {
bool hasOpaqueDefault(const Gpu::Layer &layer, Gpu::PixelFormat format)
{
    const auto alpha = layer.defaultPixel[3];
    return format == Gpu::PixelFormat::Rgba16 ? ((alpha - (alpha >> 8) + 128) >> 8) != 0 : alpha != 0;
}
void previewSelection(KisImageSP image, const Gpu::Snapshot &version)
{
    if (version->selectionKind == Gpu::SelectionKind::Freehand)
        image->previewGpuSelectionMask(version->selectionMask, version->selectionInverted);
    else image->previewGpuSelectionRect(version->selection, version->selectionKind == Gpu::SelectionKind::Ellipse,
                                        version->selectionInverted);
}
}

bool KisToolTransform::hasStroke() const { return m_strokeId || !m_gpuTransform.isNull(); }

void KisToolTransform::startGpuTransform(ToolTransformArgs::TransformMode mode, const KisNodeList &roots,
                                       KisSelectionSP selection, KisPaintDeviceSP externalSource)
{
    const auto owner = image()->gpuImage();
    if (!owner || !owner->handle() || mode != ToolTransformArgs::FREE_TRANSFORM || externalSource) {
        Q_EMIT statusTextChanged(i18n("This transform mode is not connected to GPU editing yet."));
        return;
    }
    for (const auto &root : roots) if (!root || !root->isEditable()) return;
    const auto source = owner->operations().snapshot();
    const auto nodes = KisTransformUtils::fetchNodesList(mode, roots, false, selection);
    QHash<QUuid, Gpu::LayerId> identities;
    for (const auto &layer : source->layers) identities.insert(layer.uuid, layer.id);
    QList<Gpu::AffineTransformTarget> targets;
    QHash<Gpu::LayerId, qsizetype> indices;
    for (const auto &node : nodes) {
        const bool mask = bool(qobject_cast<const KisTransparencyMask *>(node.data()));
        if (qobject_cast<const KisGroupLayer *>(node.data())) continue;
        if (!mask && !qobject_cast<const KisPaintLayer *>(node.data())) {
            Q_EMIT statusTextChanged(i18n("This layer type is not connected to GPU transformation yet.")); return;
        }
        const auto layer = identities.value((mask ? node->parent() : node)->uuid());
        if (!layer) return;
        if (!indices.contains(layer)) { indices.insert(layer, targets.size()); targets.append({layer, false, false}); }
        auto &target = targets[indices.value(layer)];
        if (mask) target.mask = true; else target.pixels = true;
    }
    if (targets.isEmpty()) return;
    auto coverage = selection ? source->selectionKind == Gpu::SelectionKind::Freehand ? source->selectionMask :
        source->selectionKind == Gpu::SelectionKind::Ellipse ? Gpu::SelectionMask::fromEllipse(source->selection, source->selection) :
        Gpu::SelectionMask::fromRectangle(source->selection, source->selection) : Gpu::SelectionMaskSP{};
    if (selection && !coverage) coverage = Gpu::SelectionMask::empty();
    const auto copy = selection ? selection->selectedExactRect() : QRect{};
    QHash<Gpu::LayerId, QRect> bounds, nondefault;
    QRect handles;
    QHash<Gpu::LayerId, Gpu::SelectionMaskSP> maskSources;
    for (const auto &target : targets) {
        const auto layer = std::find_if(source->layers.cbegin(), source->layers.cend(), [&](const auto &v) { return v.id == target.layer; });
        if (target.pixels) {
            Gpu::PixelReadbackOutcome read;
            if (selection) {
                Gpu::TranslationOperation captured;
                captured.layer = target.layer; captured.selectedPixels = true; captured.copyRect = captured.clearRect = copy;
                captured.selectionRect = source->selection; captured.selectionKind = source->selectionKind;
                captured.selectionMask = source->selectionMask; captured.selectionInverted = source->selectionInverted;
                read = owner->execution().readSelectedBounds(owner->handle(), source, captured);
            } else read = owner->execution().readBounds(owner->handle(), source, target.layer, false, true);
            if (read.code != Gpu::ExecutionCode::Success || !read.bounds) return;
            bounds[target.layer] = *read.bounds;
            if (!selection && hasOpaqueDefault(*layer, source->format)) bounds[target.layer] |= image()->bounds();
            handles |= bounds[target.layer];
            if (!selection && hasOpaqueDefault(*layer, source->format)) {
                const auto raw = owner->execution().readBounds(owner->handle(), source, target.layer, true);
                if (raw.code != Gpu::ExecutionCode::Success || !raw.bounds) return;
                nondefault[target.layer] = *raw.bounds;
            }
        }
        if (target.mask) {
            const auto captured = coverage ? layer->transparencyMask->selectedCopy(*coverage, source->selectionInverted, copy) : layer->transparencyMask;
            if (!captured) return;
            maskSources[target.layer] = captured; handles |= captured->bounds();
        }
    }
    if (selection) handles = copy;
    if (handles.isEmpty()) return;
    const auto token = owner->beginAffineTransform(targets, copy, copy);
    if (token.isNull()) return;
    m_gpuImage = owner; m_gpuTransform = token; m_gpuTransformSource = source;
    m_gpuTransformTargets = targets; m_gpuTransformBounds = bounds; m_gpuNonDefaultBounds = nondefault;
    m_gpuSelection = coverage; m_gpuSelectionBounds = copy; m_gpuMaskSources = maskSources;
    m_currentlyUsingOverlayPreviewStyle = false;
    m_transaction = TransformTransactionProperties(QRectF(handles), &m_currentArgs, roots, nodes);
    m_currentArgs = KisTransformUtils::resetArgsForMode(mode, m_currentArgs.filterId(), m_transaction, {});
    m_gpuAcceptedArgs = m_currentArgs;
    initThumbnailImage({});
    commitChanges();
    initGuiAfterTransformMode();
}

void KisToolTransform::updateGpuTransform()
{
    if (m_gpuTransform.isNull()) return;
    if (m_gpuUpdating) { m_gpuUpdatePending = true; return; }
    const ToolTransformArgs requested = m_currentArgs;
    const auto owner = m_gpuImage;
    const auto token = m_gpuTransform;
    const auto source = m_gpuTransformSource;
    bool valid = requested.mode() == ToolTransformArgs::FREE_TRANSFORM &&
        requested.aX() == 0 && requested.aY() == 0;
    QList<Gpu::AffineLayerPlan> plans;
    Gpu::SelectionMaskSP selected;
    auto worker = KisTransformUtils::createTransformWorker(requested, {}, {});
    for (const auto &target : m_gpuTransformTargets) {
        if (!valid) break;
        const auto layer = std::find_if(source->layers.cbegin(), source->layers.cend(), [&](const auto &v) { return v.id == target.layer; });
        const auto plan = worker.gpuPlan(m_gpuTransformBounds.value(target.layer),
            !m_gpuSelection && hasOpaqueDefault(*layer, source->format) ? image()->bounds() : QRect{}, m_gpuNonDefaultBounds.value(target.layer));
        if (!plan) { valid = false; break; }
        Gpu::SelectionMaskSP mask;
        if (target.mask && requested.isIdentity()) mask = layer->transparencyMask;
        else if (target.mask) {
            const auto captured = m_gpuMaskSources.value(target.layer);
            const auto maskPlan = worker.gpuPlan(captured->bounds());
            if (!maskPlan) { valid = false; break; }
            mask = maskPlan->passes.isEmpty() ? captured : captured->transformed(*maskPlan);
            if (mask && m_gpuSelection) mask = layer->transparencyMask->replaceSelectedPixels(*m_gpuSelection,
                source->selectionInverted, m_gpuSelectionBounds, *mask);
            if (!mask) { valid = false; break; }
        }
        plans.append({target.layer, QSharedPointer<Gpu::AffinePlan>::create(*plan), mask, bool(m_gpuSelection) && !requested.isIdentity()});
    }
    if (valid && m_gpuSelection) {
        const auto effective = source->selectionInverted ? m_gpuSelection->inverted(image()->bounds()) :
            m_gpuSelection->translated({}, m_gpuSelectionBounds);
        const auto selectionPlan = worker.gpuPlan(m_gpuSelectionBounds);
        if (!selectionPlan) valid = false;
        else {
            const auto finite = effective ? effective : Gpu::SelectionMask::empty();
            selected = finite->transformed(*selectionPlan); valid = bool(selected);
        }
    }
    QPointer<KisToolTransform> guard(this);
    m_gpuUpdating = true;
    const auto result = valid ? owner->updateAffineTransform(token, plans, selected, false) : Gpu::Result::InvalidInput;
    if (!guard || m_gpuImage != owner || m_gpuTransform != token) return;
    if (result == Gpu::Result::Success) {
        m_gpuAcceptedArgs = requested;
        if (m_gpuSelection) previewSelection(image(), owner->operations().preview());
    } else {
        if (m_currentArgs == requested) {
            m_currentArgs = m_gpuAcceptedArgs;
            currentStrategy()->externalConfigChanged();
            updateOptionWidget();
        }
        Q_EMIT statusTextChanged(i18n("The transform could not be applied. The previous preview is preserved."));
    }
    if (!guard || m_gpuImage != owner || m_gpuTransform != token) return;
    m_gpuUpdating = false;
    if (m_gpuCommitPending) { m_gpuCommitPending = false; finishGpuTransform(true); }
    else if (m_gpuUpdatePending) {
        m_gpuUpdatePending = false;
        QMetaObject::invokeMethod(this, &KisToolTransform::updateGpuTransform, Qt::QueuedConnection);
    }
}

void KisToolTransform::finishGpuTransform(bool commit)
{
    if (commit && m_gpuUpdating) { m_gpuCommitPending = true; return; }
    if (commit && m_gpuUpdatePending) {
        m_gpuUpdatePending = false;
        const auto previous = m_gpuTransform;
        QPointer<KisToolTransform> guard(this);
        updateGpuTransform();
        if (!guard || m_gpuTransform != previous) return;
    }
    const auto owner = m_gpuImage;
    const auto token = m_gpuTransform;
    const auto retainedImage = image();
    const bool selected = bool(m_gpuSelection);
    m_gpuTransform = {}; m_gpuImage.clear(); m_gpuTransformSource.clear();
    m_gpuTransformTargets.clear(); m_gpuTransformBounds.clear(); m_gpuNonDefaultBounds.clear();
    m_gpuMaskSources.clear(); m_gpuSelection.clear(); m_gpuUpdating = m_gpuUpdatePending = m_gpuCommitPending = false;
    m_changesTracker.reset();
    m_transaction = TransformTransactionProperties(QRectF(), &m_currentArgs, {}, {});
    QPointer<KisToolTransform> guard(this);
    if (commit) owner->commitAffineTransform(token); else owner->cancelAffineTransform(token);
    if (!guard) return;
    if (selected && retainedImage->gpuImage() == owner) previewSelection(retainedImage, owner->operations().snapshot());
    if (!guard) return;
    outlineChanged();
}
