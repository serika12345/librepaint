/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <QHash>
#include <QSet>
#include <limits>

namespace LibrePaint::Image::Gpu {
namespace {
std::optional<QPoint> checkedOffset(qint64 x, qint64 y)
{
    constexpr qint64 limit = 2 * RgbaTile::CoordinateLimit;
    if (x < -limit || y < -limit || x > limit || y > limit) return std::nullopt;
    return QPoint(int(x), int(y));
}

std::optional<QPoint> checkedPosition(QPoint start, QPoint offset)
{
    const qint64 x = qint64(start.x()) + offset.x(), y = qint64(start.y()) + offset.y();
    if (x < std::numeric_limits<qint32>::min() || x > std::numeric_limits<qint32>::max() ||
        y < std::numeric_limits<qint32>::min() || y > std::numeric_limits<qint32>::max()) return std::nullopt;
    return QPoint(int(x), int(y));
}
}

OperationId OperationDocument::beginLayerTranslation(LayerId layer)
{
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint || !snapshot()->layers[index].visible) return {};
    return beginLayerTranslation(QList<LayerTranslationTarget>{{layer, true, false}});
}

OperationId OperationDocument::beginLayerTranslation(const QList<LayerTranslationTarget> &targets)
{
    if (checkMutation() != Result::Success || targets.isEmpty() ||
        quint64(targets.size()) > std::numeric_limits<quint64>::max() - m_nextOperation ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return {};
    QSet<LayerId> seen;
    for (const auto &target : targets) {
        const auto index = layerIndex(target.layer);
        if (index < 0 || seen.contains(target.layer) || (!target.pixels && !target.mask)) return {};
        const auto &source = snapshot()->layers[index];
        if (source.userLocked || (target.mask && source.transparencyMaskLocked) || (!target.pixels && !source.transparencyMask)) return {};
        seen.insert(target.layer);
    }
    QHash<LayerId, OperationId> identities;
    auto sequence = m_nextOperation;
    for (const auto &target : targets) identities.insert(target.layer, {m_identity, ++sequence});
    auto operation = QSharedPointer<TranslationOperation>::create();
    operation->id = identities.value(targets.first().layer);
    operation->layer = targets.first().layer;
    m_translationTargets = targets;
    m_translationIds = std::move(identities);
    m_translation = operation;
    m_nextOperation = sequence;
    return operation->id;
}

OperationId OperationDocument::beginSelectionTranslation(LayerId layer, QRect copyRect, QRect clearRect)
{
    const auto valid = [](QRect rect) {
        constexpr qint64 limit = RgbaTile::CoordinateLimit;
        return !rect.isEmpty() && rect.x() >= -limit && rect.y() >= -limit &&
            qint64(rect.x()) + rect.width() <= limit && qint64(rect.y()) + rect.height() <= limit;
    };
    const auto before = snapshot();
    if (before->selection.isEmpty() || !valid(copyRect) || !valid(clearRect) || !copyRect.contains(clearRect)) return {};
    const auto id = beginLayerTranslation(layer);
    if (id.isNull()) return {};
    auto operation = QSharedPointer<TranslationOperation>::create(*m_translation);
    operation->selectedPixels = true;
    operation->copyRect = copyRect;
    operation->clearRect = clearRect;
    operation->selectionRect = before->selection;
    operation->selectionKind = before->selectionKind;
    operation->selectionMask = before->selectionMask;
    operation->selectionInverted = before->selectionInverted;
    m_translation = operation;
    return id;
}

Result OperationDocument::updateSelectionTranslation(OperationId id, QPoint offset, const SelectionMaskSP &preparedSelection)
{
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    const auto before = snapshot();
    const auto movedRect = before->selection.translated(offset);
    if (qint64(before->selection.left()) + offset.x() < -limit ||
        qint64(before->selection.right()) + offset.x() >= limit ||
        qint64(before->selection.top()) + offset.y() < -limit ||
        qint64(before->selection.bottom()) + offset.y() >= limit) return Result::InvalidInput;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    auto operation = QSharedPointer<TranslationOperation>::create(*m_translation);
    operation->offset = offset;
    ++operation->version;
    const auto index = layerIndex(operation->layer);
    version->layers[index].operations.append(TranslationSnapshot(operation));
    version->selection = movedRect;
    if (before->selectionMask && !offset.isNull()) {
        version->selectionMask = before->selectionMask->isGpuGenerated() ? preparedSelection :
            before->selectionMask->translated(offset, QRect(-limit, -limit, 2 * limit, 2 * limit));
        if (!version->selectionMask || !selectionFitsRetainedInputs(version->selectionMask)) return Result::CapacityExceeded;
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_translation = operation;
    m_preview = version;
    return Result::Success;
}

Result OperationDocument::updateLayerTranslation(OperationId id, QPoint offset)
{ return updateLayerTranslationWithMasks(id, offset, {}); }

Result OperationDocument::updateLayerTranslationWithMasks(OperationId id, QPoint offset,
    const QHash<LayerId, SelectionMaskSP> &masks, const SelectionMaskSP &selection)
{
    if (m_closed) return Result::Closed;
    if (!m_translation || m_translation->id != id) return Result::StaleOperation;
    if (!checkedOffset(offset.x(), offset.y())) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max() ||
        m_translation->version == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    if (m_translation->selectedPixels) return updateSelectionTranslation(id, offset, selection);
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    for (const auto &participant : m_translationTargets) {
        const auto index = layerIndex(participant.layer);
        const auto &source = snapshot()->layers[index];
        auto &target = version->layers[index];
        if (participant.pixels) {
            const auto position = checkedPosition(source.position, offset);
            if (!position) return Result::InvalidInput;
            target.position = *position;
            if (source.kind == LayerKind::Paint) {
                auto previous = source.operations.isEmpty() ? nullptr :
                    std::get_if<TranslationSnapshot>(&source.operations.last());
                if (previous && (*previous)->selectedPixels) previous = nullptr;
                const auto combined = previous ?
                    checkedOffset(qint64((*previous)->offset.x()) + offset.x(), qint64((*previous)->offset.y()) + offset.y()) :
                    std::optional<QPoint>(offset);
                if (!combined) return Result::InvalidInput;
                auto operation = QSharedPointer<TranslationOperation>::create();
                operation->id = m_translationIds.value(participant.layer);
                operation->layer = participant.layer;
                operation->version = m_translation->version + 1;
                operation->offset = *combined;
                if (previous) target.operations.removeLast();
                target.operations.append(TranslationSnapshot(operation));
            }
        }
        if (participant.mask && source.transparencyMask && !source.transparencyMask->isEmpty() && !offset.isNull()) {
            const auto bounds = source.transparencyMask->bounds();
            if (qint64(bounds.left()) + offset.x() < -limit || qint64(bounds.right()) + offset.x() >= limit ||
                qint64(bounds.top()) + offset.y() < -limit || qint64(bounds.bottom()) + offset.y() >= limit)
                return Result::InvalidInput;
            target.transparencyMask = source.transparencyMask->isGpuGenerated() ? masks.value(participant.layer) :
                source.transparencyMask->translated(offset, domain);
            if (!target.transparencyMask || !selectionFitsRetainedInputs(target.transparencyMask)) return Result::CapacityExceeded;
        }
    }
    auto operation = QSharedPointer<TranslationOperation>::create(*m_translation);
    operation->offset = offset;
    ++operation->version;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_translation = operation;
    m_preview = version;
    return Result::Success;
}

Result OperationDocument::commitLayerTranslation(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_translation || m_translation->id != id) return Result::StaleOperation;
    if (m_preview && (m_translation->selectedPixels || !m_translation->offset.isNull())) {
        LayerHistoryChange change;
        change.property = m_translation->selectedPixels || !m_translationTargets.first().pixels ? LayerProperty::None : LayerProperty::Position;
        change.layer = m_translation->layer;
        change.changedSelection = m_translation->selectedPixels;
        change.movedSelectionPixels = m_translation->selectedPixels;
        change.translatedLayers = !m_translation->selectedPixels;
        for (const auto &participant : m_translationTargets) {
            const auto index = layerIndex(participant.layer);
            const auto before = snapshot()->layers[index].position;
            const auto after = m_preview->layers[index].position;
            if (participant.layer == change.layer) {
                change.before = before;
                change.after = after;
            } else if (participant.pixels && !m_translation->selectedPixels) change.propertyEdits.append({LayerProperty::Position, participant.layer, before, after});
        }
        commitVersion(m_preview, change);
    }
    m_translation.clear();
    m_translationTargets.clear();
    m_translationIds.clear();
    m_preview.clear();
    return Result::Success;
}

Result OperationDocument::cancelLayerTranslation(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_translation || m_translation->id != id) return Result::StaleOperation;
    m_translation.clear();
    m_translationTargets.clear();
    m_translationIds.clear();
    m_preview.clear();
    return Result::Success;
}

Result OperationDocument::writeLayerPosition(LayerId layer, QPoint position)
{
    return applyLayerPosition(layer, position, nullptr);
}

QList<Snapshot> OperationDocument::layerPositionVersions(const Snapshot &source, LayerId layer) const
{
    const int index = m_history.indexOf(source);
    if (index < 0) return {source};
    const auto containsLayer = [&](const Snapshot &state) {
        for (const auto &candidate : state->layers) if (candidate.id == layer) return true;
        return false;
    };
    const auto changesPosition = [&](int index) {
        if (const auto &resize = m_layerHistoryChanges[index].canvasResize)
            if (!resize->rect.topLeft().isNull()) return true;
        const auto &change = m_layerHistoryChanges[index];
        if (change.layer == layer && change.property == LayerProperty::Position) return true;
        for (const auto &edit : change.propertyEdits)
            if (edit.layer == layer && edit.property == LayerProperty::Position) return true;
        return false;
    };
    int first = index, last = index;
    while (first > 0 && !changesPosition(first) && containsLayer(m_history[first - 1])) --first;
    while (last + 1 < m_history.size() && !changesPosition(last + 1) && containsLayer(m_history[last + 1])) ++last;
    return m_history.mid(first, last - first + 1);
}

Result OperationDocument::applyLayerPosition(LayerId layer, QPoint position, Snapshot *retainedVersion)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto source = retainedVersion ? *retainedVersion : snapshot();
    if (!source || source->documentId() != snapshot()->documentId()) return Result::StaleVersion;
    const Layer *selected = nullptr;
    for (const auto &candidate : source->layers) if (candidate.id == layer) selected = &candidate;
    if (!selected) return Result::LayerUnavailable;
    if (selected->position == position) return Result::Success;
    const auto versions = layerPositionVersions(source, layer);
    const quint64 count = quint64(versions.size());
    if (count > std::numeric_limits<quint64>::max() - m_nextRevision ||
        count > std::numeric_limits<quint64>::max() - m_nextOperation) return Result::CapacityExceeded;
    auto history = m_history;
    auto retained = source;
    auto nextRevision = m_nextRevision, nextOperation = m_nextOperation;
    QHash<QPair<quintptr, QPoint>, TranslationSnapshot> translations;
    for (const auto &before : versions) {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
        for (auto &candidate : version->layers) {
            if (candidate.id != layer) continue;
            const auto offset = checkedOffset(qint64(position.x()) - candidate.position.x(),
                                               qint64(position.y()) - candidate.position.y());
            if (!offset) return Result::InvalidInput;
            if (candidate.kind == LayerKind::Group) {
                candidate.position = position;
                break;
            }
            auto previous = candidate.operations.isEmpty() ? nullptr :
                std::get_if<TranslationSnapshot>(&candidate.operations.last());
            if (previous && (*previous)->selectedPixels) previous = nullptr;
            const auto combined = previous ?
                checkedOffset(qint64((*previous)->offset.x()) + offset->x(), qint64((*previous)->offset.y()) + offset->y()) : offset;
            if (!combined) return Result::InvalidInput;
            const auto key = qMakePair(previous ? quintptr(previous->data()) : quintptr(0), *offset);
            if (!combined->isNull() && !translations.contains(key)) {
                auto operation = QSharedPointer<TranslationOperation>::create();
                operation->id = {m_identity, ++nextOperation};
                operation->version = 1;
                operation->layer = layer;
                operation->offset = *combined;
                translations.insert(key, operation);
            }
            if (previous) candidate.operations.removeLast();
            if (!combined->isNull()) candidate.operations.append(translations.value(key));
            candidate.position = position;
            break;
        }
        version->m_revision = ++nextRevision;
        for (auto &state : history) if (state == before) state = version;
        if (before == source) retained = version;
    }
    m_history.swap(history);
    m_nextRevision = nextRevision;
    m_nextOperation = nextOperation;
    if (retainedVersion) *retainedVersion = retained;
    return Result::Success;
}
}
