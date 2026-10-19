/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <QHash>
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
    if (checkMutation() != Result::Success || index < 0 || snapshot()->layers[index].kind != LayerKind::Paint ||
        !snapshot()->layers[index].visible ||
        snapshot()->layers[index].userLocked || m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return {};
    auto operation = QSharedPointer<TranslationOperation>::create();
    operation->id = {m_identity, ++m_nextOperation};
    operation->layer = layer;
    m_translation = operation;
    return operation->id;
}

Result OperationDocument::updateLayerTranslation(OperationId id, QPoint offset)
{
    if (m_closed) return Result::Closed;
    if (!m_translation || m_translation->id != id) return Result::StaleOperation;
    if (!checkedOffset(offset.x(), offset.y())) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max() ||
        m_translation->version == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    const auto index = layerIndex(m_translation->layer);
    const auto &source = snapshot()->layers[index];
    const auto position = checkedPosition(source.position, offset);
    if (!position) return Result::InvalidInput;
    const auto previous = source.operations.isEmpty() ? nullptr :
        std::get_if<TranslationSnapshot>(&source.operations.last());
    const auto combined = previous ?
        checkedOffset(qint64((*previous)->offset.x()) + offset.x(), qint64((*previous)->offset.y()) + offset.y()) :
        std::optional<QPoint>(offset);
    if (!combined) return Result::InvalidInput;
    auto operation = QSharedPointer<TranslationOperation>::create(*m_translation);
    operation->offset = *combined;
    ++operation->version;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    auto &target = version->layers[index];
    if (previous) target.operations.removeLast();
    target.operations.append(TranslationSnapshot(operation));
    target.position = *position;
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
    const auto index = layerIndex(m_translation->layer);
    if (m_preview && m_preview->layers[index].position != snapshot()->layers[index].position) {
        LayerHistoryChange change;
        change.property = LayerProperty::Position;
        change.layer = m_translation->layer;
        change.before = snapshot()->layers[index].position;
        change.after = m_preview->layers[index].position;
        commitVersion(m_preview, change);
    }
    m_translation.clear();
    m_preview.clear();
    return Result::Success;
}

Result OperationDocument::cancelLayerTranslation(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_translation || m_translation->id != id) return Result::StaleOperation;
    m_translation.clear();
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
        return m_layerHistoryChanges[index].layer == layer &&
            m_layerHistoryChanges[index].property == LayerProperty::Position;
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
            const auto previous = candidate.operations.isEmpty() ? nullptr :
                std::get_if<TranslationSnapshot>(&candidate.operations.last());
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
