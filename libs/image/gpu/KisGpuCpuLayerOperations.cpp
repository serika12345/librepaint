/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include <limits>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
OperationId OperationDocument::beginCpuLayerEdit(LayerId layer)
{
    const auto index = layerIndex(layer);
    if (checkMutation() != Result::Success || index < 0 || !snapshot()->layers[index].cpuDefinition ||
        m_nextOperation == std::numeric_limits<quint64>::max()) return {};
    const OperationId id{m_identity, m_nextOperation + 1};
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    if (beginExternalStroke(id) != Result::Success) return {};
    ++m_nextOperation;
    m_cpuLayerEditLayer = layer;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_preview = version;
    return id;
}

Result OperationDocument::commitCpuLayerEdit(OperationId id, const QString &name)
{
    if (m_closed) return Result::Closed;
    if (!m_cpuLayerEditLayer || id != m_externalOperation) return Result::StaleOperation;
    if (!Detail::validLayerName(name)) return Result::InvalidInput;
    const auto index = layerIndex(m_cpuLayerEditLayer);
    const auto &before = snapshot()->layers[index];
    if (before.name == name && snapshot()->revision() == preview()->revision()) return cancelCpuLayerEdit(id);
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*preview()));
    version->layers[index].name = name;
    version->m_pendingOperation = {};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layer = m_cpuLayerEditLayer;
    change.layerStructureAction = HistoryAction::ChangeFilter;
    endExternalStroke(id);
    m_cpuLayerEditLayer = 0;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::cancelCpuLayerEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_cpuLayerEditLayer || id != m_externalOperation) return Result::StaleOperation;
    m_cpuLayerEditLayer = 0;
    return endExternalStroke(id);
}

Result OperationDocument::replaceCpuLayer(LayerId id, const RasterLayer &rendered, const Snapshot &expected)
{
    if (m_closed) return Result::Closed;
    const auto status = checkMutation();
    const bool editing = m_cpuLayerEditLayer && m_cpuLayerEditLayer == id && !m_externalOperation.isNull();
    if (status != Result::Success && !editing) return status;
    if (!expected || expected != preview()) return Result::StaleVersion;
    const auto index = layerIndex(id);
    if (index < 0 || !snapshot()->layers[index].cpuDefinition || !rendered.cpuDefinition)
        return Result::LayerUnavailable;
    if (rendered.cpuDefinition->feature != snapshot()->layers[index].cpuDefinition->feature) return Result::InvalidInput;
    if (rendered.tiles.size() > m_limits.rasterBytes / RgbaTile::byteCount(expected->format))
        return Result::CapacityExceeded;
    if (m_nextRevision == std::numeric_limits<quint64>::max() ||
        m_nextOperation == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    try {
        RasterImage input;
        input.size = expected->size;
        input.format = expected->format;
        input.layers = {rendered};
        input.layers.first().parentUuid = {};
        OperationDocument prepared(input, m_limits);
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*preview()));
        auto &target = version->layers[index];
        const auto &source = prepared.snapshot()->layers.first();
        target.cpuDefinition = source.cpuDefinition;
        target.defaultPixel = source.defaultPixel;
        target.pixels.reset();
        target.operations.clear();
        for (const auto &value : source.operations) {
            auto operation = QSharedPointer<TileWriteOperation>::create(*std::get<TileWriteSnapshot>(value));
            operation->id = {m_identity, m_nextOperation + 1};
            operation->layer = id;
            if (operation->writeGroup) operation->writeGroup = operation->id.sequence;
            target.operations.append(TileWriteSnapshot(operation));
        }
        version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
        ++m_nextOperation;
        if (editing) {
            version->m_committed = false;
            version->m_pendingOperation = m_externalOperation;
            m_preview = version;
        } else {
            LayerHistoryChange change;
            change.layer = id;
            change.layerStructureAction = HistoryAction::ChangeFilter;
            commitVersion(version, change);
        }
        return Result::Success;
    } catch (const std::invalid_argument &) { return Result::InvalidInput; }
      catch (const std::bad_alloc &) { return Result::CapacityExceeded; }
}

}
