/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"

#include <limits>
#include <algorithm>
#include <stdexcept>

namespace Krita::Image::Gpu {
using namespace Detail;

QList<InputSample> PaintOperation::inputSamples() const
{
    QList<InputSample> result;
    result.reserve(sampleCount);
    for (const auto &block : sampleBlocks) {
        result.append(*block);
    }
    return result;
}

OperationDocument::OperationDocument(QSize size, Limits limits)
    : m_limits(limits)
    , m_identity(QUuid::createUuid())
{
    if (size.width() <= 0 || size.height() <= 0 ||
        size.width() > CoordinateLimit || size.height() > CoordinateLimit ||
        limits.samplesPerStroke <= 0 || limits.samplesPerDocument <= 0 || limits.layerCount <= 0 ||
        limits.recoveryBytes <= 0 || limits.recoveryBytes > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("invalid operation document dimensions or limits");
    }
    QSharedPointer<DocumentVersion> initial(new DocumentVersion);
    initial->m_document = m_identity;
    initial->m_committed = true;
    initial->size = size;
    initial->layers.append({1, QStringLiteral("Paint Layer 1"), true, {}});
    initial->activeLayer = 1;
    m_history.append(initial);
}

Snapshot OperationDocument::snapshot() const { return m_history[m_index]; }
Snapshot OperationDocument::preview() const { return m_preview ? m_preview : snapshot(); }
LayerId OperationDocument::activeLayer() const { return snapshot()->activeLayer; }
int OperationDocument::undoCount() const { return m_index; }
int OperationDocument::redoCount() const { return m_history.size() - m_index - 1; }
bool OperationDocument::isModified() const { return snapshot()->revision() != m_savedRevision; }
bool OperationDocument::hasActiveStroke() const { return !m_edit.isNull(); }
OperationId OperationDocument::activeOperation() const { return m_edit ? m_edit->id : OperationId{}; }

qsizetype OperationDocument::layerIndex(LayerId layer) const
{
    const auto &layers = snapshot()->layers;
    for (qsizetype i = 0; i < layers.size(); ++i) {
        if (layers[i].id == layer) {
            return i;
        }
    }
    return -1;
}

Result OperationDocument::checkMutation() const
{
    if (m_closed) return Result::Closed;
    if (m_edit) return Result::EditInProgress;
    return Result::Success;
}

Result OperationDocument::checkOperation(OperationId id) const
{
    if (m_closed) return Result::Closed;
    if (!m_edit || m_edit->id != id) return Result::StaleOperation;
    return Result::Success;
}

OperationId OperationDocument::beginStroke(LayerId layer, const Brush &brush, quint64 randomSeed)
{
    const qsizetype index = layerIndex(layer);
    if (checkMutation() != Result::Success || index < 0 || !snapshot()->layers[index].visible ||
        !validBrush(brush) || m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return {};
    }
    QSharedPointer<PaintOperation> operation(new PaintOperation);
    operation->id = {m_identity, ++m_nextOperation};
    operation->layer = layer;
    operation->brush = brush;
    operation->randomSeed = randomSeed;
    m_edit = operation;
    return operation->id;
}

Result OperationDocument::appendSamples(OperationId id, const QList<InputSample> &samples)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    quint64 time = m_edit->sampleBlocks.empty() ? 0 : m_edit->sampleBlocks.back()->back().timestamp;
    for (const auto &sample : samples) {
        if (!validSample(sample) || sample.timestamp < time) return Result::InvalidInput;
        time = sample.timestamp;
    }
    const qsizetype count = m_edit->sampleCount;
    if (samples.size() > m_limits.samplesPerStroke - count ||
        count > m_limits.samplesPerDocument - snapshot()->sampleCount ||
        samples.size() > m_limits.samplesPerDocument - snapshot()->sampleCount - count ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return Result::CapacityExceeded;
    }
    if (samples.empty()) return Result::Success;
    QSharedPointer<PaintOperation> operation(new PaintOperation(*m_edit));
    constexpr qsizetype blockSize = 256;
    qsizetype offset = 0;
    if (!operation->sampleBlocks.empty() && operation->sampleBlocks.back()->size() < blockSize) {
        auto tail = QSharedPointer<QList<InputSample>>::create(*operation->sampleBlocks.back());
        const auto count = std::min(blockSize - tail->size(), samples.size());
        tail->append(samples.first(count));
        operation->sampleBlocks.back() = tail;
        offset = count;
    }
    while (offset < samples.size()) {
        const auto count = std::min(blockSize, samples.size() - offset);
        operation->sampleBlocks.append(QSharedPointer<const QList<InputSample>>(new QList<InputSample>(samples.mid(offset, count))));
        offset += count;
    }
    operation->sampleCount += samples.size();
    ++operation->version;
    replacePreview(operation);
    return Result::Success;
}

Result OperationDocument::updateShape(OperationId id, const Shape &shape)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    if (!validShape(shape)) return Result::InvalidInput;
    if (m_edit->sampleCount == 0) return Result::EmptyOperation;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<PaintOperation> operation(new PaintOperation(*m_edit));
    operation->shape = shape;
    ++operation->version;
    replacePreview(operation);
    return Result::Success;
}

void OperationDocument::replacePreview(const QSharedPointer<const PaintOperation> &operation)
{
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->layers[layerIndex(operation->layer)].operations.append(operation);
    version->sampleCount += operation->sampleCount;
    version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    m_edit = operation;
    m_preview = version;
}

void OperationDocument::commitVersion(const Snapshot &version)
{
    auto history = m_history;
    history.erase(history.begin() + m_index + 1, history.end());
    history.append(version);
    int index = m_index + 1;
    if (index > HistoryLimit) {
        history.removeFirst();
        --index;
    }
    version->m_committed = true;
    m_history.swap(history);
    m_index = index;
}

Result OperationDocument::commitStroke(OperationId id)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    if (!m_preview) return Result::EmptyOperation;
    commitVersion(m_preview);
    m_edit.clear();
    m_preview.clear();
    return Result::Success;
}

Result OperationDocument::cancelStroke(OperationId id)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    m_edit.clear();
    m_preview.clear();
    return Result::Success;
}

LayerId OperationDocument::addLayer(const QString &name)
{
    if (checkMutation() != Result::Success || snapshot()->layers.size() >= m_limits.layerCount ||
        name.size() > LayerNameByteLimit || name.toUtf8().size() > LayerNameByteLimit ||
        m_nextLayer == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return 0;
    }
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    const LayerId layer = ++m_nextLayer;
    version->layers.append({layer, name, true, {}});
    version->activeLayer = layer;
    version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return layer;
}

Result OperationDocument::selectLayer(LayerId layer)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (layerIndex(layer) < 0) return Result::LayerUnavailable;
    if (layer == activeLayer()) return Result::Success;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->activeLayer = layer;
    m_history[m_index] = version;
    return Result::Success;
}

Result OperationDocument::setLayerVisible(LayerId layer, bool visible)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const qsizetype index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].visible == visible) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->layers[index].visible = visible;
    version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return Result::Success;
}

Result OperationDocument::undo()
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (m_index == 0) return Result::NothingToUndo;
    --m_index;
    return Result::Success;
}

Result OperationDocument::redo()
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (redoCount() == 0) return Result::NothingToRedo;
    ++m_index;
    return Result::Success;
}

Result OperationDocument::markSaved(const Snapshot &saved)
{
    if (m_closed) return Result::Closed;
    if (!saved || saved->documentId() != m_identity || !saved->m_committed) return Result::StaleVersion;
    m_savedRevision = saved->revision();
    return Result::Success;
}

void OperationDocument::close()
{
    QList<Snapshot> remaining{snapshot()};
    m_edit.clear();
    m_preview.clear();
    m_history.swap(remaining);
    m_index = 0;
    m_closed = true;
}

}
