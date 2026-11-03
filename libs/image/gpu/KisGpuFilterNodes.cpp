/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <QSet>
#include <limits>

namespace LibrePaint::Image::Gpu {
bool OperationDocument::filterFitsRetainedHistory(const FilterDefinition &filter) const
{
    QSet<const char *> seen;
    qsizetype bytes = 0;
    const auto add = [&](const FilterDefinition &value) {
        if (seen.contains(value.xml.constData())) return true;
        const auto size = value.byteCount();
        if (size > m_limits.rasterBytes - bytes) return false;
        seen.insert(value.xml.constData());
        bytes += size;
        return true;
    };
    for (const auto &version : m_history)
        for (const auto &layer : version->layers) if (layer.filter && !add(*layer.filter)) return false;
    if (m_preview) for (const auto &layer : m_preview->layers)
        if (layer.filter && !add(*layer.filter)) return false;
    for (const auto &change : m_layerHistoryChanges) {
        if (change.beforeFilter && !add(*change.beforeFilter)) return false;
        if (change.afterFilter && !add(*change.afterFilter)) return false;
    }
    return add(filter);
}

OperationId OperationDocument::beginFilterNodeEdit(LayerId existing, const RasterLayer &node,
                                                   qsizetype index, qsizetype effectIndex, bool clearPixelSelection)
{
    if (checkMutation() != Result::Success || m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return {};
    const auto before = snapshot();
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    LayerId target = existing;
    if (existing) {
        const auto position = layerIndex(existing);
        if (position < 0 || !isFilterNode(before->layers[position].kind) || before->layers[position].userLocked) return {};
    } else {
        if (!isFilterNode(node.kind) || !node.filter || !node.filter->isValid(before->format) ||
            !Detail::validLayerName(node.name) || !filterFitsRetainedHistory(*node.filter) ||
            !selectionFitsInputs(node.filterSelection) || !node.tiles.isEmpty() ||
            !node.writtenAreas.isEmpty() || node.defaultPixel != RgbaPixel{} ||
            before->layers.size() >= m_limits.layerCount || index < 0 || index > before->layers.size() ||
            m_nextLayer == std::numeric_limits<quint64>::max()) return {};
        const Layer *parent = nullptr;
        for (const auto &layer : before->layers) if (layer.uuid == node.parentUuid) parent = &layer;
        if (node.kind == LayerKind::FilterMask) {
            if (!parent || parent->kind == LayerKind::FilterMask) return {};
        } else if (!node.parentUuid.isNull() && (!parent || parent->kind != LayerKind::Group)) return {};
        auto ancestor = parent;
        while (ancestor) {
            if (ancestor->userLocked) return {};
            const auto uuid = ancestor->parentUuid;
            ancestor = nullptr;
            for (const auto &layer : before->layers) if (layer.uuid == uuid) { ancestor = &layer; break; }
        }
        const auto bytes = node.filter->byteCount();
        if (bytes > m_limits.rasterBytes) return {};
        Layer added;
        added.id = m_nextLayer + 1;
        added.kind = node.kind;
        added.name = node.name;
        added.uuid = node.uuid.isNull() ? QUuid::createUuid() : node.uuid;
        added.parentUuid = node.parentUuid;
        added.opacity = node.opacity;
        added.blendMode = node.blendMode;
        added.filter = node.filter;
        added.filterSelection = node.filterSelection ? node.filterSelection : SelectionMask::uniform(255);
        if (!added.filterSelection) return {};
        if (parent && node.kind == LayerKind::FilterMask && parent->transparencyMask && effectIndex >= 0) {
            auto &owner = version->layers[layerIndex(parent->id)];
            qsizetype masks = 0;
            for (const auto &layer : before->layers)
                if (layer.parentUuid == parent->uuid && layer.kind == LayerKind::FilterMask) ++masks;
            if (effectIndex > masks + 1) return {};
            if (effectIndex <= owner.transparencyMaskIndex) ++owner.transparencyMaskIndex;
        }
        target = added.id;
        version->layers.insert(index, added);
        if (!Detail::validLayerHierarchy(version->layers, before->format)) return {};
        ++m_nextLayer;
        if (clearPixelSelection) {
            version->selection = {};
            version->selectionKind = SelectionKind::Rectangle;
            version->selectionInverted = false;
            version->selectionMask.clear();
        }
    }
    const OperationId id{m_identity, ++m_nextOperation};
    version->activeLayer = target;
    version->selectedLayers = {target};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_filterNodeEdit = FilterNodeEdit{id, target, !existing};
    m_preview = version;
    return id;
}

Result OperationDocument::updateFilterNodeEdit(OperationId id, const FilterDefinition &filter, const QString &name)
{
    if (m_closed) return Result::Closed;
    if (!m_filterNodeEdit || m_filterNodeEdit->id != id) return Result::StaleOperation;
    if (!filter.isValid(preview()->format) || !Detail::validLayerName(name)) return Result::InvalidInput;
    if (!filterFitsRetainedHistory(filter) || m_nextRevision == std::numeric_limits<quint64>::max())
        return Result::CapacityExceeded;
    const auto current = preview();
    qsizetype index = -1;
    for (qsizetype i = 0; i < current->layers.size(); ++i)
        if (current->layers[i].id == m_filterNodeEdit->node) { index = i; break; }
    if (index < 0) return Result::LayerUnavailable;
    const auto &previous = current->layers[index];
    if (previous.filter && *previous.filter == filter && previous.name == name) return Result::Success;
    if (filter.byteCount() > m_limits.rasterBytes) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*current));
    version->layers[index].filter = filter;
    version->layers[index].name = name;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_preview = version;
    return Result::Success;
}

Result OperationDocument::commitFilterNodeEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_filterNodeEdit || m_filterNodeEdit->id != id) return Result::StaleOperation;
    const auto edit = *m_filterNodeEdit;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*m_preview));
    qsizetype index = -1;
    for (qsizetype i = 0; i < version->layers.size(); ++i)
        if (version->layers[i].id == edit.node) { index = i; break; }
    if (index < 0) return Result::LayerUnavailable;
    if (!edit.inserted) {
        const auto &old = snapshot()->layers[layerIndex(edit.node)];
        const auto &updated = version->layers[index];
        if (old.name == updated.name && old.filter == updated.filter && old.filterSelection == updated.filterSelection)
            return cancelFilterNodeEdit(id);
    }
    version->m_pendingOperation = {};
    LayerHistoryChange change;
    change.layer = edit.node;
    change.layerStructureAction = edit.inserted ?
        (version->layers[index].kind == LayerKind::FilterMask ? HistoryAction::AddFilterMask : HistoryAction::AddAdjustmentLayer) :
        HistoryAction::ChangeFilter;
    if (!edit.inserted) {
        const auto &before = snapshot()->layers[layerIndex(edit.node)];
        const auto &after = version->layers[index];
        if (!(before.filter == after.filter)) {
            change.beforeFilter = before.filter;
            change.afterFilter = after.filter;
        } else change.layerStructureAction = HistoryAction::RenameLayer;
        if (before.name != after.name)
            change.propertyEdits.append({LayerProperty::Name, edit.node, before.name, after.name});
    }
    m_preview.clear();
    m_filterNodeEdit.reset();
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::cancelFilterNodeEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_filterNodeEdit || m_filterNodeEdit->id != id) return Result::StaleOperation;
    m_preview.clear();
    m_filterNodeEdit.reset();
    return Result::Success;
}

LayerId OperationDocument::addFilterNode(const RasterLayer &node, qsizetype index, qsizetype effectIndex)
{
    const auto id = beginFilterNodeEdit(0, node, index, effectIndex);
    if (id.isNull()) return 0;
    const auto layer = m_filterNodeEdit->node;
    return commitFilterNodeEdit(id) == Result::Success ? layer : 0;
}

Result OperationDocument::setFilterDefinition(LayerId node, const FilterDefinition &filter, const QString &name)
{
    const auto index = layerIndex(node);
    if (index < 0 || !isFilterNode(snapshot()->layers[index].kind)) return Result::LayerUnavailable;
    const auto &previous = snapshot()->layers[index];
    const auto resolvedName = name.isEmpty() ? previous.name : name;
    if (!filter.isValid(snapshot()->format) || !Detail::validLayerName(resolvedName)) return Result::InvalidInput;
    if (previous.filter && *previous.filter == filter && previous.name == resolvedName) return Result::Success;
    const auto id = beginFilterNodeEdit(node, {}, 0);
    if (id.isNull()) return hasActiveStroke() ? Result::EditInProgress : Result::LayerUnavailable;
    const auto result = updateFilterNodeEdit(id, filter, resolvedName);
    if (result != Result::Success) { cancelFilterNodeEdit(id); return result; }
    return commitFilterNodeEdit(id);
}

Result OperationDocument::setFilterSelection(LayerId node, const SelectionMaskSP &coverage)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(node);
    if (index < 0 || !isFilterNode(snapshot()->layers[index].kind) || snapshot()->layers[index].userLocked)
        return Result::LayerUnavailable;
    if (!coverage || coverage->byteCount() > SelectionMask::MaximumBytes) return Result::InvalidInput;
    if (!selectionFitsInputs(coverage) || m_nextRevision == std::numeric_limits<quint64>::max())
        return Result::CapacityExceeded;
    if (snapshot()->layers[index].filterSelection == coverage) return Result::Success;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].filterSelection = coverage;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layerStructureAction = HistoryAction::PaintStroke;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::writeFilterDefinition(LayerId node, const FilterDefinition &filter)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(node);
    if (index < 0 || !isFilterNode(snapshot()->layers[index].kind)) return Result::LayerUnavailable;
    const auto current = snapshot();
    if (!filter.isValid(current->format)) return Result::InvalidInput;
    if (current->layers[index].filter && *current->layers[index].filter == filter) return Result::Success;
    if (!filterFitsRetainedHistory(filter)) return Result::CapacityExceeded;
    const auto contains = [&](const Snapshot &version) {
        if (version->format != current->format || version->colorProfile != current->colorProfile) return false;
        for (const auto &layer : version->layers)
            if (layer.id == node && layer.uuid == current->layers[index].uuid && layer.filter) return true;
        return false;
    };
    const auto changesFilter = [&](int position) {
        const auto &change = m_layerHistoryChanges[position];
        return change.layer == node && bool(change.beforeFilter) && bool(change.afterFilter);
    };
    int first = m_index, last = m_index;
    while (first > 0 && !changesFilter(first) && contains(m_history[first - 1])) --first;
    while (last + 1 < m_history.size() && !changesFilter(last + 1) && contains(m_history[last + 1])) ++last;
    if (quint64(last - first + 1) > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto revision = m_nextRevision;
    for (int i = first; i <= last; ++i) {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*history[i]));
        for (auto &layer : version->layers) if (layer.id == node) {
            if (filter.byteCount() > m_limits.rasterBytes) return Result::CapacityExceeded;
            layer.filter = filter;
            break;
        }
        version->m_revision = ++revision;
        history[i] = version;
    }
    m_history.swap(history);
    m_nextRevision = revision;
    return Result::Success;
}
}
