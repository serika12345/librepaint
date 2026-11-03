/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <limits>

namespace LibrePaint::Image::Gpu {
LayerId OperationDocument::addCloneLayer(const RasterLayer &node, qsizetype index)
{
    const auto before = snapshot();
    if (checkMutation() != Result::Success || node.kind != LayerKind::Clone ||
        !Detail::validLayerName(node.name) || !node.tiles.isEmpty() || !node.writtenAreas.isEmpty() ||
        node.defaultPixel != RgbaPixel{} || node.filter || node.filterSelection || node.passThrough ||
        node.channelWriteMask != 15 || index < 0 || index > before->layers.size() ||
        before->layers.size() >= m_limits.layerCount ||
        m_nextLayer == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return 0;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    Layer added;
    added.id = m_nextLayer + 1;
    added.kind = LayerKind::Clone;
    added.uuid = node.uuid.isNull() ? QUuid::createUuid() : node.uuid;
    added.name = node.name;
    added.visible = node.visible;
    added.parentUuid = node.parentUuid;
    added.opacity = node.opacity;
    added.blendMode = node.blendMode;
    added.userLocked = node.userLocked;
    added.pinnedToTimeline = node.pinnedToTimeline;
    added.colorLabelIndex = node.colorLabelIndex;
    added.projectionChannelMask = node.projectionChannelMask;
    added.position = node.position;
    added.cloneSourceUuid = node.cloneSourceUuid;
    added.cloneFromOriginal = node.cloneFromOriginal;
    const auto id = added.id;
    version->layers.insert(index, added);
    if (!Detail::validLayerHierarchy(version->layers, version->format)) return 0;
    version->activeLayer = id;
    version->selectedLayers = {id};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    ++m_nextLayer;
    LayerHistoryChange change;
    change.layer = id;
    change.layerStructureAction = HistoryAction::AddCloneLayer;
    commitVersion(version, change);
    return id;
}

Result OperationDocument::setCloneSource(LayerId layer, const QUuid &source, bool fromOriginal)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Clone) return Result::LayerUnavailable;
    const auto &current = snapshot()->layers[index];
    if (current.cloneSourceUuid == source && current.cloneFromOriginal == fromOriginal) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].cloneSourceUuid = source;
    version->layers[index].cloneFromOriginal = fromOriginal;
    if (!Detail::validLayerHierarchy(version->layers, version->format)) return Result::InvalidInput;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layer = layer;
    change.cloneSourceLayers = {layer};
    change.layerStructureAction = HistoryAction::ChangeCloneSource;
    commitVersion(version, change);
    return Result::Success;
}
Result OperationDocument::writeCloneSource(LayerId id, const QUuid &source, bool fromOriginal)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(id);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Clone) return Result::LayerUnavailable;
    const auto &current = snapshot()->layers[index];
    if (current.cloneSourceUuid == source && current.cloneFromOriginal == fromOriginal) return Result::Success;
    const auto contains = [&](const Snapshot &version) {
        bool target = false, input = source.isNull();
        for (const auto &node : version->layers) {
            target |= node.id == id && node.kind == LayerKind::Clone && node.uuid == current.uuid;
            input |= node.uuid == source;
        }
        return target && input;
    };
    const auto changes = [&](int position) {
        const auto &change = m_layerHistoryChanges[position];
        return change.cloneSourceLayers.contains(id);
    };
    int first = m_index, last = m_index;
    while (first > 0 && !changes(first) && contains(m_history[first - 1])) --first;
    while (last + 1 < m_history.size() && !changes(last + 1) && contains(m_history[last + 1])) ++last;
    if (quint64(last - first + 1) > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto revision = m_nextRevision;
    for (int i = first; i <= last; ++i) {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*history[i]));
        for (auto &node : version->layers) if (node.id == id) {
            node.cloneSourceUuid = source;
            node.cloneFromOriginal = fromOriginal;
        }
        if (!Detail::validLayerHierarchy(version->layers, version->format)) return Result::InvalidInput;
        version->m_revision = ++revision;
        history[i] = version;
    }
    m_history.swap(history);
    m_nextRevision = revision;
    return Result::Success;
}

OperationId OperationDocument::beginCloneSourceEdit(const QList<LayerId> &layers)
{
    if (checkMutation() != Result::Success || layers.isEmpty() ||
        m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return {};
    QSet<LayerId> seen;
    for (const auto id : layers) {
        const auto index = layerIndex(id);
        if (seen.contains(id) || index < 0 || snapshot()->layers[index].kind != LayerKind::Clone ||
            snapshot()->layers[index].userLocked) return {};
        seen.insert(id);
    }
    const OperationId id{m_identity, ++m_nextOperation};
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_cloneSourceEdit = CloneSourceEdit{id, layers};
    m_preview = version;
    return id;
}

Result OperationDocument::updateCloneSourceEdit(OperationId id, const QUuid &source)
{
    if (m_closed) return Result::Closed;
    if (!m_cloneSourceEdit || m_cloneSourceEdit->id != id) return Result::StaleOperation;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*preview()));
    for (auto &node : version->layers)
        if (m_cloneSourceEdit->layers.contains(node.id)) node.cloneSourceUuid = source;
    if (!Detail::validLayerHierarchy(version->layers, version->format)) return Result::InvalidInput;
    version->m_revision = ++m_nextRevision;
    m_preview = version;
    return Result::Success;
}

Result OperationDocument::commitCloneSourceEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_cloneSourceEdit || m_cloneSourceEdit->id != id) return Result::StaleOperation;
    bool changed = false;
    for (const auto &node : preview()->layers) if (m_cloneSourceEdit->layers.contains(node.id))
        changed |= snapshot()->layers[layerIndex(node.id)].cloneSourceUuid != node.cloneSourceUuid;
    if (!changed) return cancelCloneSourceEdit(id);
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*preview()));
    version->m_pendingOperation = {};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.cloneSourceLayers = m_cloneSourceEdit->layers;
    change.layerStructureAction = HistoryAction::ChangeCloneSource;
    m_preview.clear();
    m_cloneSourceEdit.reset();
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::cancelCloneSourceEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_cloneSourceEdit || m_cloneSourceEdit->id != id) return Result::StaleOperation;
    m_preview.clear();
    m_cloneSourceEdit.reset();
    return Result::Success;
}
}
