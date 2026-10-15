/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"
#include "KisGpuOperationValues_p.h"

#include <limits>
#include <algorithm>
#include <stdexcept>
#include <QScopeGuard>
#include <QSet>

namespace LibrePaint::Image::Gpu {
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
    : OperationDocument(RasterImage{size, {{QStringLiteral("Paint Layer 1"), true, {}, {}}}, 0}, limits) {}

std::unique_ptr<OperationDocument> OperationDocument::cloneCurrent(bool preserveLayerUuids) const
{
    auto result = std::make_unique<OperationDocument>(RasterImage{snapshot()->size, {}, 0}, m_limits);
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->m_document = result->m_identity;
    for (auto &layer : version->layers) {
        if (!preserveLayerUuids) layer.uuid = QUuid::createUuid();
        for (auto &operation : layer.operations) {
            if (const auto *paint = std::get_if<PaintSnapshot>(&operation)) {
                auto copied = QSharedPointer<PaintOperation>::create(**paint);
                copied->id.document = result->m_identity;
                operation = PaintSnapshot(copied);
            } else {
                auto copied = QSharedPointer<TileWriteOperation>::create(*std::get<TileWriteSnapshot>(operation));
                copied->id.document = result->m_identity;
                operation = TileWriteSnapshot(copied);
            }
        }
    }
    result->m_history = {version};
    result->m_nextLayer = m_nextLayer;
    result->m_nextOperation = m_nextOperation;
    result->m_nextRevision = m_nextRevision;
    result->m_savedRevision = version->m_modifiedRevision;
    return result;
}

Snapshot OperationDocument::snapshot() const { return m_history[m_index]; }
Snapshot OperationDocument::preview() const { return m_preview ? m_preview : snapshot(); }
LayerId OperationDocument::activeLayer() const { return snapshot()->activeLayer; }
int OperationDocument::undoCount() const { return m_index; }
int OperationDocument::redoCount() const { return m_history.size() - m_index - 1; }
QList<Snapshot> OperationDocument::historyVersions() const { return m_history; }
HistoryAction OperationDocument::historyAction(int index) const
{
    if (index <= 0 || index >= m_history.size()) return HistoryAction::None;
    switch (m_propertyChanges[index].property) {
    case LayerProperty::Name: return HistoryAction::RenameLayer;
    case LayerProperty::Opacity: return HistoryAction::ChangeOpacity;
    case LayerProperty::Visible: return HistoryAction::ChangeVisibility;
    case LayerProperty::Locked: return HistoryAction::ChangeLock;
    case LayerProperty::PinnedToTimeline: return HistoryAction::ChangeTimelinePin;
    case LayerProperty::None: break;
    }
    const auto &before = m_history[index - 1]->layers;
    const auto &after = m_history[index]->layers;
    if (after.size() > before.size()) return HistoryAction::AddLayer;
    if (after.size() < before.size()) return HistoryAction::RemoveLayer;
    if (before.size() == 1 && after.size() == 1 && before.first().id != after.first().id)
        return HistoryAction::RemoveLayer;
    for (int layer = 0; layer < after.size(); ++layer) {
        if (before[layer].id != after[layer].id) return HistoryAction::MoveLayer;
        if (after[layer].operations.size() > before[layer].operations.size()) {
            if (const auto *paint = std::get_if<PaintSnapshot>(&after[layer].operations.last())) {
                switch ((*paint)->shape.kind) {
                case ShapeKind::Freehand: return HistoryAction::PaintStroke;
                case ShapeKind::Line: return HistoryAction::DrawLine;
                case ShapeKind::Ellipse: return HistoryAction::DrawEllipse;
                case ShapeKind::Rectangle:
                case ShapeKind::RoundedRectangle: return HistoryAction::DrawRectangle;
                }
            }
        }
    }
    return HistoryAction::ReplacePixels;
}
int OperationDocument::savedHistoryIndex() const
{
    for (int index = 0; index < m_history.size(); ++index)
        if (m_history[index]->m_modifiedRevision == m_savedRevision) return index;
    return -1;
}
bool OperationDocument::isModified() const { return snapshot()->m_modifiedRevision != m_savedRevision; }
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

Result OperationDocument::writeProjectionBackground(const std::array<quint8, 4> &rgba)
{
    if (m_closed) return Result::Closed;
    if (snapshot()->projectionBackground == rgba) return Result::Success;
    const auto count = quint64(m_history.size()) + quint64(bool(m_preview));
    if (count > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto nextRevision = m_nextRevision;
    for (auto &state : history) {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*state));
        version->projectionBackground = rgba;
        version->m_revision = ++nextRevision;
        state = version;
    }
    auto preview = m_preview;
    if (preview) {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*preview));
        version->projectionBackground = rgba;
        version->m_revision = ++nextRevision;
        preview = version;
    }
    m_history.swap(history);
    m_preview = preview;
    m_nextRevision = nextRevision;
    return Result::Success;
}

template<class T>
Result OperationDocument::writeLayerProperty(LayerId layer, LayerProperty property, T Layer::*member, const T &value)
{
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    const T previous = snapshot()->layers[index].*member;
    if (previous == value) return Result::Success;
    const auto containsLayer = [&](const Snapshot &state) {
        for (const auto &candidate : state->layers)
            if (candidate.id == layer) return true;
        return false;
    };
    const auto changesProperty = [&](int index) {
        return m_propertyChanges[index].layer == layer && m_propertyChanges[index].property == property;
    };
    int first = m_index, last = m_index;
    while (first > 0 && !changesProperty(first) && containsLayer(m_history[first - 1])) --first;
    while (last + 1 < m_history.size() && !changesProperty(last + 1) && containsLayer(m_history[last + 1])) ++last;
    const auto count = quint64(last - first + 1) + quint64(bool(m_preview));
    if (count > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto nextRevision = m_nextRevision;
    const auto update = [&](const Snapshot &state) -> Snapshot {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*state));
        for (auto &candidate : version->layers) {
            if (candidate.id != layer) continue;
            candidate.*member = value;
            break;
        }
        version->m_revision = ++nextRevision;
        return version;
    };
    for (int i = first; i <= last; ++i) history[i] = update(history[i]);
    const auto preview = m_preview ? update(m_preview) : Snapshot{};
    m_history.swap(history);
    m_preview = preview;
    m_nextRevision = nextRevision;
    return Result::Success;
}

Result OperationDocument::writeLayerName(LayerId layer, const QString &name)
{
    if (m_closed) return Result::Closed;
    if (!validLayerName(name)) return Result::InvalidInput;
    return writeLayerProperty(layer, LayerProperty::Name, &Layer::name, name);
}
Result OperationDocument::writeLayerVisible(LayerId layer, bool visible)
{ return writeLayerProperty(layer, LayerProperty::Visible, &Layer::visible, visible); }
Result OperationDocument::writeLayerOpacity(LayerId layer, quint8 opacity)
{ return writeLayerProperty(layer, LayerProperty::Opacity, &Layer::opacity, opacity); }
Result OperationDocument::writeLayerLocked(LayerId layer, bool locked)
{ return writeLayerProperty(layer, LayerProperty::Locked, &Layer::userLocked, locked); }
Result OperationDocument::writeLayerPinnedToTimeline(LayerId layer, bool pinned)
{ return writeLayerProperty(layer, LayerProperty::PinnedToTimeline, &Layer::pinnedToTimeline, pinned); }

Result OperationDocument::restorePropertyChange(const PropertyChange &change, bool forward)
{
    const auto &value = forward ? change.after : change.before;
    switch (change.property) {
    case LayerProperty::None: return Result::Success;
    case LayerProperty::Name: return writeLayerName(change.layer, value.toString());
    case LayerProperty::Opacity: return writeLayerOpacity(change.layer, quint8(value.toUInt()));
    case LayerProperty::Visible: return writeLayerVisible(change.layer, value.toBool());
    case LayerProperty::Locked: return writeLayerLocked(change.layer, value.toBool());
    case LayerProperty::PinnedToTimeline: return writeLayerPinnedToTimeline(change.layer, value.toBool());
    }
    return Result::InvalidInput;
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
    if (checkMutation() != Result::Success || index < 0 || !snapshot()->layers[index].visible || snapshot()->layers[index].userLocked ||
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
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = operation->id;
    m_edit = operation;
    m_preview = version;
}

void OperationDocument::commitVersion(const Snapshot &version)
{ commitVersion(version, {}); }

void OperationDocument::commitVersion(const Snapshot &version, const PropertyChange &change)
{
    auto history = m_history;
    auto changes = m_propertyChanges;
    history.erase(history.begin() + m_index + 1, history.end());
    changes.erase(changes.begin() + m_index + 1, changes.end());
    history.append(version);
    changes.append(change);
    int index = m_index + 1;
    if (index > HistoryLimit) {
        history.removeFirst();
        changes.removeFirst();
        changes[0] = {};
        --index;
    }
    version->m_committed = true;
    m_history.swap(history);
    m_propertyChanges.swap(changes);
    m_index = index;
}

void OperationDocument::commitPropertyVersion(const Snapshot &version, PropertyChange change, PropertyHistory history)
{
    change.mergeable = history == PropertyHistory::MergeConsecutive;
    const auto previous = m_propertyChanges[m_index];
    if (change.mergeable && previous.mergeable && m_index > 0 && !redoCount() &&
        previous.property == change.property && previous.layer == change.layer &&
        previous.after == change.before && snapshot()->m_modifiedRevision != m_savedRevision) {
        change.before = previous.before;
        auto states = m_history;
        auto changes = m_propertyChanges;
        int index = m_index;
        if (change.property != LayerProperty::Name && change.before == change.after) {
            states.removeLast();
            changes.removeLast();
            --index;
            auto current = QSharedPointer<DocumentVersion>(new DocumentVersion(*states[index]));
            current->activeLayer = version->activeLayer;
            current->m_revision = version->m_revision;
            states[index] = current;
        } else {
            version->m_committed = true;
            states[index] = version;
            changes[index] = change;
        }
        m_history.swap(states);
        m_propertyChanges.swap(changes);
        m_index = index;
        return;
    }
    commitVersion(version, change);
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
{ return addLayer(name, snapshot()->layers.size(), false); }

LayerId OperationDocument::addLayer(const QString &name, qsizetype index, bool pinnedToTimeline)
{
    if (checkMutation() != Result::Success || snapshot()->layers.size() >= m_limits.layerCount ||
        !validLayerName(name) || index < 0 || index > snapshot()->layers.size() ||
        m_nextLayer == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return 0;
    }
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    const LayerId layer = ++m_nextLayer;
    Layer added{layer, name, true, {}, QUuid::createUuid()};
    added.pinnedToTimeline = pinnedToTimeline;
    version->layers.insert(index, added);
    version->activeLayer = layer;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return layer;
}

Result OperationDocument::selectLayer(LayerId layer)
{
    if (m_closed) return Result::Closed;
    if (layerIndex(layer) < 0) return Result::LayerUnavailable;
    if (layer == activeLayer()) return Result::Success;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->activeLayer = layer;
    Snapshot preview = m_preview;
    if (preview) {
        QSharedPointer<DocumentVersion> selected(new DocumentVersion(*preview));
        selected->activeLayer = layer;
        preview = selected;
    }
    m_history[m_index] = version;
    m_preview = preview;
    return Result::Success;
}

Result OperationDocument::setLayerVisible(LayerId layer, bool visible)
{ return setLayerVisible(layer, visible, PropertyHistory::Separate); }

Result OperationDocument::setLayerVisible(LayerId layer, bool visible, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const qsizetype index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].visible == visible) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->layers[index].visible = visible;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::Visible, layer, QVariant::fromValue(snapshot()->layers[index].visible), QVariant::fromValue(visible)}, history);
    return Result::Success;
}

Result OperationDocument::setLayerOpacity(LayerId layer, quint8 opacity)
{ return setLayerOpacity(layer, opacity, PropertyHistory::Separate); }

Result OperationDocument::setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].opacity == opacity) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].opacity = opacity;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::Opacity, layer, QVariant::fromValue(snapshot()->layers[index].opacity), QVariant::fromValue(opacity)}, history);
    return Result::Success;
}

Result OperationDocument::setLayerLocked(LayerId layer, bool locked)
{ return setLayerLocked(layer, locked, PropertyHistory::Separate); }

Result OperationDocument::setLayerLocked(LayerId layer, bool locked, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].userLocked == locked) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].userLocked = locked;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::Locked, layer, QVariant::fromValue(snapshot()->layers[index].userLocked), QVariant::fromValue(locked)}, history);
    return Result::Success;
}

Result OperationDocument::renameLayer(LayerId layer, const QString &name)
{ return renameLayer(layer, name, PropertyHistory::Separate); }

Result OperationDocument::renameLayer(LayerId layer, const QString &name, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (!validLayerName(name)) return Result::InvalidInput;
    if (snapshot()->layers[index].name == name) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->layers[index].name = name;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::Name, layer, QVariant::fromValue(snapshot()->layers[index].name), QVariant::fromValue(name)}, history);
    return Result::Success;
}

Result OperationDocument::moveLayer(LayerId layer, qsizetype index)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto previous = layerIndex(layer);
    if (previous < 0) return Result::LayerUnavailable;
    if (index < 0 || index >= snapshot()->layers.size()) return Result::InvalidInput;
    if (previous == index) return Result::Success;
    QList<LayerId> order;
    for (const auto &value : snapshot()->layers) order.append(value.id);
    order.move(previous, index);
    return reorderLayers(order);
}

Result OperationDocument::reorderLayers(const QList<LayerId> &order)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (order.size() != snapshot()->layers.size()) return Result::InvalidInput;
    QSet<LayerId> included;
    QList<Layer> layers;
    layers.reserve(order.size());
    for (const auto id : order) {
        if (included.contains(id)) return Result::InvalidInput;
        const auto index = layerIndex(id);
        if (index < 0) return Result::LayerUnavailable;
        included.insert(id);
        layers.append(snapshot()->layers[index]);
    }
    bool changed = false;
    for (qsizetype i = 0; i < order.size(); ++i) changed |= snapshot()->layers[i].id != order[i];
    if (!changed) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->layers.swap(layers);
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return Result::Success;
}

Result OperationDocument::removeLayer(LayerId layer)
{ return removeLayers({layer}); }

Result OperationDocument::removeLayers(const QList<LayerId> &layers, const QString &replacementName)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (layers.isEmpty()) return Result::Success;
    if (!replacementName.isEmpty() && !validLayerName(replacementName)) return Result::InvalidInput;
    QSet<LayerId> removed;
    for (const auto layer : layers) {
        if (removed.contains(layer)) return Result::InvalidInput;
        if (layerIndex(layer) < 0) return Result::LayerUnavailable;
        removed.insert(layer);
    }
    const bool replace = removed.size() == snapshot()->layers.size() && !replacementName.isEmpty();
    if (replace && m_nextLayer == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    if (removed.contains(version->activeLayer)) {
        const auto active = layerIndex(version->activeLayer);
        version->activeLayer = 0;
        for (qsizetype i = active + 1; i < version->layers.size(); ++i) {
            if (!removed.contains(version->layers[i].id)) {
                version->activeLayer = version->layers[i].id;
                break;
            }
        }
        if (!version->activeLayer) {
            for (qsizetype i = active - 1; i >= 0; --i) {
                if (!removed.contains(version->layers[i].id)) {
                    version->activeLayer = version->layers[i].id;
                    break;
                }
            }
        }
    }
    for (qsizetype i = version->layers.size(); i-- > 0;) {
        if (!removed.contains(version->layers[i].id)) continue;
        for (const auto &operation : version->layers[i].operations) {
            version->sampleCount -= operationSamples(operation);
            version->rasterBytes -= operationRasterBytes(operation);
        }
        version->layers.removeAt(i);
    }
    if (replace) {
        Layer replacement{m_nextLayer + 1, replacementName, true, {}, QUuid::createUuid()};
        version->activeLayer = replacement.id;
        version->layers.append(std::move(replacement));
        ++m_nextLayer;
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return Result::Success;
}

Result OperationDocument::undo()
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (m_index == 0) return Result::NothingToUndo;
    return setHistoryIndex(m_index - 1);
}

Result OperationDocument::redo()
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (redoCount() == 0) return Result::NothingToRedo;
    return setHistoryIndex(m_index + 1);
}

Result OperationDocument::markSaved(const Snapshot &saved)
{
    if (m_closed) return Result::Closed;
    if (!saved || saved->documentId() != m_identity || !saved->m_committed) return Result::StaleVersion;
    m_savedRevision = saved->m_modifiedRevision;
    return Result::Success;
}

Result OperationDocument::setHistoryIndex(int index)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (index < 0 || index >= m_history.size()) return Result::InvalidInput;
    const auto previousHistory = m_history;
    const auto previousIndex = m_index;
    const auto previousRevision = m_nextRevision;
    bool completed = false;
    const auto rollback = qScopeGuard([&] {
        if (completed) return;
        m_history = previousHistory;
        m_index = previousIndex;
        m_nextRevision = previousRevision;
    });
    while (m_index != index) {
        const bool forward = index > m_index;
        const auto change = m_propertyChanges[m_index + (forward ? 1 : 0)];
        m_index += forward ? 1 : -1;
        const auto result = restorePropertyChange(change, forward);
        if (result != Result::Success) return result;
    }
    completed = true;
    return Result::Success;
}

void OperationDocument::close()
{
    QList<Snapshot> remaining{snapshot()};
    m_edit.clear();
    m_preview.clear();
    m_history.swap(remaining);
    m_propertyChanges = {PropertyChange{}};
    m_index = 0;
    m_closed = true;
}

}
