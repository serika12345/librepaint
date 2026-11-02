/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <QSet>
#include <algorithm>
#include <limits>

namespace LibrePaint::Image::Gpu {
OperationId OperationDocument::beginAffineTransform(const QList<AffineTransformTarget> &layers, QRect copyRect, QRect clearRect)
{
    if (checkMutation() != Result::Success || layers.isEmpty() ||
        quint64(layers.size()) > std::numeric_limits<quint64>::max() - m_nextOperation) return {};
    const bool selected = !copyRect.isEmpty();
    if (selected && (!Detail::validSelectionRect(copyRect) || !Detail::validSelectionRect(clearRect) ||
                     clearRect.isEmpty() || !copyRect.contains(clearRect) || (snapshot()->selection.isEmpty() && !snapshot()->selectionInverted))) return {};
    if (!selected && !clearRect.isEmpty()) return {};
    QList<AffineSnapshot> operations; QSet<LayerId> seen;
    auto sequence = m_nextOperation;
    for (const auto &target : layers) {
        const auto layer = target.layer;
        const int index = layerIndex(layer);
        if (index < 0 || seen.contains(layer) ||
            (target.pixels && snapshot()->layers[index].kind != LayerKind::Paint) ||
            snapshot()->layers[index].userLocked || (!target.pixels && !target.mask && !target.filterSelection) ||
            (target.mask && (!snapshot()->layers[index].transparencyMask || snapshot()->layers[index].transparencyMaskLocked)) ||
            (target.filterSelection && (!isFilterNode(snapshot()->layers[index].kind) || !snapshot()->layers[index].filterSelection))) return {};
        seen.insert(layer);
        auto operation = QSharedPointer<AffineTransformOperation>::create();
        operation->id = {m_identity, ++sequence}; operation->layer = layer;
        operation->transformPixels = target.pixels; operation->transformMask = target.mask;
        operation->transformFilterSelection = target.filterSelection;
        if (selected) {
            auto selection = QSharedPointer<TranslationOperation>::create();
            selection->id = operation->id; selection->layer = layer; selection->selectedPixels = true;
            selection->copyRect = copyRect; selection->clearRect = clearRect;
            selection->selectionRect = snapshot()->selection; selection->selectionKind = snapshot()->selectionKind;
            selection->selectionMask = snapshot()->selectionMask; selection->selectionInverted = snapshot()->selectionInverted;
            operation->selection = selection;
        }
        operations.append(operation);
    }
    m_nextOperation = sequence; m_affine = operations;
    return operations.first()->id;
}
Result OperationDocument::updateAffineTransform(OperationId id, const QList<AffineLayerPlan> &plans,
                                                 const SelectionMaskSP &selection, bool inverted)
{
    if (m_closed) return Result::Closed;
    if (m_affine.isEmpty() || m_affine.first()->id != id) return Result::StaleOperation;
    if (plans.size() != m_affine.size()) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max() || m_affine.first()->version == std::numeric_limits<quint64>::max())
        return Result::CapacityExceeded;
    const bool selected = std::any_of(m_affine.cbegin(), m_affine.cend(), [](const auto &operation) { return bool(operation->selection); });
    if (!selected && (selection || inverted)) return Result::InvalidInput;
    if (selection && (selection->byteCount() > SelectionMask::MaximumBytes || !selectionFitsRetainedInputs(selection)))
        return Result::CapacityExceeded;
    QSet<LayerId> seen;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    QList<AffineSnapshot> operations;
    bool changed = false;
    for (const auto &original : m_affine) {
        const auto found = std::find_if(plans.cbegin(), plans.cend(), [&](const auto &value) { return value.layer == original->layer; });
        if (found == plans.cend() || seen.contains(found->layer) || !found->plan || !validAffinePlan(*found->plan)) return Result::InvalidInput;
        seen.insert(found->layer);
        auto operation = QSharedPointer<AffineTransformOperation>::create(*original);
        operation->plan = found->plan; ++operation->version;
        const int index = layerIndex(original->layer);
        if (!selected && original->transformPixels) {
            const auto before = version->layers[index].position;
            const qint64 x = qint64(before.x()) + found->plan->positionDelta.x(), y = qint64(before.y()) + found->plan->positionDelta.y();
            if (x < std::numeric_limits<qint32>::min() || x > std::numeric_limits<qint32>::max() ||
                y < std::numeric_limits<qint32>::min() || y > std::numeric_limits<qint32>::max()) return Result::InvalidInput;
            version->layers[index].position = {int(x), int(y)};
        }
        if (original->transformPixels && (!found->plan->passes.isEmpty() || (selected && found->recomposeSelection))) {
            version->layers[index].operations.append(AffineSnapshot(operation)); changed = true;
        }
        if (original->transformMask) {
            if (!found->mask) return Result::InvalidInput;
            if (found->mask->byteCount() > SelectionMask::MaximumBytes || !selectionFitsRetainedInputs(found->mask))
                return Result::CapacityExceeded;
            const auto &before = version->layers[index].transparencyMask;
            changed |= *before != *found->mask;
            version->layers[index].transparencyMask = found->mask;
        } else if (found->mask) return Result::InvalidInput;
        if (original->transformFilterSelection) {
            if (!found->filterSelection) return Result::InvalidInput;
            if (!selectionFitsRetainedInputs(found->filterSelection)) return Result::CapacityExceeded;
            changed |= *version->layers[index].filterSelection != *found->filterSelection;
            version->layers[index].filterSelection = found->filterSelection;
        } else if (found->filterSelection) return Result::InvalidInput;
        operations.append(operation);
    }
    if (!changed) { m_preview.clear(); return Result::Success; }
    if (selected) {
        version->selectionMask = selection && !selection->isEmpty() ? selection : SelectionMaskSP{};
        version->selection = version->selectionMask ? selection->bounds() : QRect{};
        version->selectionInverted = inverted;
        version->selectionKind = version->selectionMask ? SelectionKind::Freehand : SelectionKind::Rectangle;
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false; version->m_pendingOperation = id;
    m_affine = operations; m_preview = version;
    return Result::Success;
}
Result OperationDocument::commitAffineTransform(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (m_affine.isEmpty() || m_affine.first()->id != id) return Result::StaleOperation;
    if (m_preview) {
        LayerHistoryChange change; change.transformedLayers = true;
        change.changedSelection = std::any_of(m_affine.cbegin(), m_affine.cend(), [](const auto &operation) { return bool(operation->selection); });
        for (const auto &operation : m_affine) {
            const int index = layerIndex(operation->layer);
            if (m_preview->layers[index].position != snapshot()->layers[index].position)
                change.propertyEdits.append({LayerProperty::Position, operation->layer,
                    snapshot()->layers[index].position, m_preview->layers[index].position});
        }
        commitVersion(m_preview, change);
    }
    m_affine.clear(); m_preview.clear(); return Result::Success;
}
Result OperationDocument::cancelAffineTransform(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (m_affine.isEmpty() || m_affine.first()->id != id) return Result::StaleOperation;
    m_affine.clear(); m_preview.clear(); return Result::Success;
}
}
