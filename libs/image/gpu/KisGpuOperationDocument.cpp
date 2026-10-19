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
#include <QHash>

namespace LibrePaint::Image::Gpu {
using namespace Detail;

namespace {
void includeLayerDescendants(const QList<Layer> &layers, QSet<LayerId> &included)
{
    QHash<QUuid, QList<const Layer *>> children;
    QList<const Layer *> pending;
    for (const auto &layer : layers) {
        children[layer.parentUuid].append(&layer);
        if (included.contains(layer.id)) pending.append(&layer);
    }
    for (qsizetype i = 0; i < pending.size(); ++i) {
        for (const auto child : children.value(pending[i]->uuid)) {
            if (included.contains(child->id)) continue;
            included.insert(child->id);
            pending.append(child);
        }
    }
}

void copyLayerOperations(const Layer &source, Layer &copy, const QUuid &identity, quint64 &sequence)
{
    copy.operations.clear();
    QHash<quint64, quint64> writeGroups;
    for (const auto &sourceOperation : source.operations) {
        const OperationId id{identity, ++sequence};
        if (const auto paint = std::get_if<PaintSnapshot>(&sourceOperation)) {
            auto operation = QSharedPointer<PaintOperation>::create(**paint);
            operation->id = id;
            operation->layer = copy.id;
            copy.operations.append(PaintSnapshot(operation));
        } else if (const auto translation = std::get_if<TranslationSnapshot>(&sourceOperation)) {
            auto operation = QSharedPointer<TranslationOperation>::create(**translation);
            operation->id = id;
            operation->layer = copy.id;
            copy.operations.append(TranslationSnapshot(operation));
        } else if (const auto crop = std::get_if<CropSnapshot>(&sourceOperation)) {
            auto operation = QSharedPointer<CropOperation>::create(**crop);
            operation->id = id;
            operation->layer = copy.id;
            copy.operations.append(CropSnapshot(operation));
        } else {
            auto operation = QSharedPointer<TileWriteOperation>::create(*std::get<TileWriteSnapshot>(sourceOperation));
            operation->id = id;
            operation->layer = copy.id;
            if (operation->writeGroup) {
                if (!writeGroups.contains(operation->writeGroup)) writeGroups.insert(operation->writeGroup, id.sequence);
                operation->writeGroup = writeGroups.value(operation->writeGroup);
            }
            copy.operations.append(TileWriteSnapshot(operation));
        }
    }
}
}

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
{ return cloneVersion(snapshot(), preserveLayerUuids); }

std::unique_ptr<OperationDocument> OperationDocument::cloneVersion(const Snapshot &source, bool preserveLayerUuids,
                                                               LayerId onlyLayer) const
{
    if (!source || source->documentId() != m_identity) return {};
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*source));
    if (onlyLayer) {
        auto found = std::find_if(source->layers.cbegin(), source->layers.cend(),
                                 [onlyLayer](const Layer &layer) { return layer.id == onlyLayer; });
        if (found == source->layers.cend()) return {};
        QSet<LayerId> included{onlyLayer};
        includeLayerDescendants(source->layers, included);
        version->layers.clear();
        for (const auto &layer : source->layers) if (included.contains(layer.id)) {
            version->layers.append(layer);
            if (layer.id == onlyLayer) version->layers.last().parentUuid = QUuid();
        }
        version->activeLayer = onlyLayer;
        version->selectedLayers = {onlyLayer};
        version->projectionBackground = {};
        version->sampleCount = 0;
        version->rasterBytes = 0;
        for (const auto &layer : version->layers)
            for (const auto &operation : layer.operations) {
                version->sampleCount += operationSamples(operation);
                version->rasterBytes += operationRasterBytes(operation);
            }
    }
    auto result = std::make_unique<OperationDocument>(RasterImage{source->size, {}, 0}, m_limits);
    version->m_document = result->m_identity;
    QHash<QUuid, QUuid> identities;
    if (!preserveLayerUuids)
        for (const auto &layer : version->layers) identities.insert(layer.uuid, QUuid::createUuid());
    for (auto &layer : version->layers) {
        if (!preserveLayerUuids) {
            layer.uuid = identities.value(layer.uuid);
            layer.parentUuid = identities.value(layer.parentUuid);
        }
        for (auto &operation : layer.operations) {
            if (const auto *paint = std::get_if<PaintSnapshot>(&operation)) {
                auto copied = QSharedPointer<PaintOperation>::create(**paint);
                copied->id.document = result->m_identity;
                operation = PaintSnapshot(copied);
            } else if (const auto translation = std::get_if<TranslationSnapshot>(&operation)) {
                auto copied = QSharedPointer<TranslationOperation>::create(**translation);
                copied->id.document = result->m_identity;
                operation = TranslationSnapshot(copied);
            } else if (const auto crop = std::get_if<CropSnapshot>(&operation)) {
                auto copied = QSharedPointer<CropOperation>::create(**crop);
                copied->id.document = result->m_identity;
                operation = CropSnapshot(copied);
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
    if (m_layerHistoryChanges[index].duplicatedLayers) return HistoryAction::DuplicateLayer;
    if (m_layerHistoryChanges[index].clearedLayers) return HistoryAction::ClearPixels;
    if (m_layerHistoryChanges[index].filledLayer) return HistoryAction::FillLayer;
    if (m_layerHistoryChanges[index].crop) return HistoryAction::CropLayer;
    if (const auto &resize = m_layerHistoryChanges[index].canvasResize)
        return resize->cropLayers ? HistoryAction::CropImage : HistoryAction::ResizeCanvas;
    switch (m_layerHistoryChanges[index].property) {
    case LayerProperty::Name: return HistoryAction::RenameLayer;
    case LayerProperty::Opacity: return HistoryAction::ChangeOpacity;
    case LayerProperty::Visible: return HistoryAction::ChangeVisibility;
    case LayerProperty::Locked: return HistoryAction::ChangeLock;
    case LayerProperty::PinnedToTimeline: return HistoryAction::ChangeTimelinePin;
    case LayerProperty::ChannelMask: return HistoryAction::ChangeChannelLock;
    case LayerProperty::Position: return HistoryAction::TranslateLayer;
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
                case ShapeKind::Polyline: return HistoryAction::DrawPolyline;
                case ShapeKind::Polygon: return HistoryAction::DrawPolygon;
                case ShapeKind::Path: return HistoryAction::DrawPath;
                }
            }
        }
    }
    return HistoryAction::ReplacePixels;
}
CropSnapshot OperationDocument::historyCrop(int index) const
{ return index > 0 && index < m_layerHistoryChanges.size() ? m_layerHistoryChanges[index].crop : CropSnapshot(); }
std::optional<CanvasResize> OperationDocument::historyCanvasResize(int index) const
{ return index > 0 && index < m_layerHistoryChanges.size() ? m_layerHistoryChanges[index].canvasResize : std::nullopt; }

int OperationDocument::savedHistoryIndex() const
{
    for (int index = 0; index < m_history.size(); ++index)
        if (m_history[index]->m_modifiedRevision == m_savedRevision) return index;
    return -1;
}
bool OperationDocument::isModified() const { return snapshot()->m_modifiedRevision != m_savedRevision; }
bool OperationDocument::hasActiveStroke() const { return m_edit || m_translation; }
OperationId OperationDocument::activeOperation() const
{ return m_edit ? m_edit->id : m_translation ? m_translation->id : OperationId{}; }

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
    if (hasActiveStroke()) return Result::EditInProgress;
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
        return m_layerHistoryChanges[index].layer == layer && m_layerHistoryChanges[index].property == property;
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

Result OperationDocument::restoreLayerHistoryChange(const LayerHistoryChange &change, bool forward)
{
    const auto &value = forward ? change.after : change.before;
    switch (change.property) {
    case LayerProperty::ChannelMask: return writeLayerChannelMask(change.layer, quint8(value.toUInt()));
    case LayerProperty::Position: return writeLayerPosition(change.layer, value.toPoint());
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
    if (checkMutation() != Result::Success || index < 0 || snapshot()->layers[index].kind != LayerKind::Paint ||
        !snapshot()->layers[index].visible || snapshot()->layers[index].userLocked ||
        !validBrush(brush) || m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return {};
    }
    QSharedPointer<PaintOperation> operation(new PaintOperation);
    operation->id = {m_identity, ++m_nextOperation};
    operation->layer = layer;
    operation->brush = brush;
    if (!brush.mirrorHorizontal && !brush.mirrorVertical) operation->brush.mirrorCenter = {};
    operation->brush.channelWriteMask &= snapshot()->layers[index].channelWriteMask;
    operation->brush.alphaLocked |= !(operation->brush.channelWriteMask & 8);
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

Result OperationDocument::paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                                      const Shape &shape, const FigureStyle &style, quint64 randomSeed)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!validBrush(brush) || !validShape(shape) || !validFigureStyle(shape, style) ||
        (brush.basicSmoothing && shape.kind != ShapeKind::Freehand)) return Result::InvalidInput;
    if (samples.isEmpty() && shape.kind != ShapeKind::Path) return Result::EmptyOperation;
    if (!validShapeSampleCount(shape, samples.size())) return Result::InvalidInput;
    quint64 time = 0;
    for (const auto &sample : samples) {
        if (!validSample(sample) || sample.timestamp < time) return Result::InvalidInput;
        time = sample.timestamp;
    }
    if (samples.size() > m_limits.samplesPerStroke ||
        samples.size() > m_limits.samplesPerDocument - snapshot()->sampleCount ||
        m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision > std::numeric_limits<quint64>::max() - (samples.isEmpty() ? 1 : 2)) return Result::CapacityExceeded;
    const auto id = beginStroke(layer, brush, randomSeed);
    if (id.isNull()) return Result::LayerUnavailable;
    const auto rollback = qScopeGuard([&] { if (m_edit && m_edit->id == id) cancelStroke(id); });
    auto result = appendSamples(id, samples);
    if (result == Result::Success) result = updateFigure(id, shape, style);
    if (result == Result::Success) result = commitStroke(id);
    return result;
}

Result OperationDocument::updateShape(OperationId id, const Shape &shape)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    return updateFigure(id, shape, m_edit->figureStyle);
}

Result OperationDocument::updateFigure(OperationId id, const Shape &shape, const FigureStyle &style)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    if (!validShape(shape) || !validFigureStyle(shape, style) ||
        (m_edit->brush.basicSmoothing && shape.kind != ShapeKind::Freehand)) return Result::InvalidInput;
    if (m_edit->sampleCount == 0 && shape.kind != ShapeKind::Path) return Result::EmptyOperation;
    if (!validShapeSampleCount(shape, m_edit->sampleCount)) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<PaintOperation> operation(new PaintOperation(*m_edit));
    operation->shape = shape;
    operation->figureStyle = style;
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

void OperationDocument::commitVersion(const Snapshot &version, const LayerHistoryChange &change)
{
    auto history = m_history;
    auto changes = m_layerHistoryChanges;
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
    m_layerHistoryChanges.swap(changes);
    m_index = index;
}

void OperationDocument::commitPropertyVersion(const Snapshot &version, LayerHistoryChange change, PropertyHistory history)
{
    change.mergeable = history == PropertyHistory::MergeConsecutive;
    const auto previous = m_layerHistoryChanges[m_index];
    if (change.mergeable && previous.mergeable && m_index > 0 && !redoCount() &&
        previous.property == change.property && previous.layer == change.layer &&
        previous.after == change.before && snapshot()->m_modifiedRevision != m_savedRevision) {
        change.before = previous.before;
        auto states = m_history;
        auto changes = m_layerHistoryChanges;
        int index = m_index;
        if (change.property != LayerProperty::Name && change.before == change.after) {
            states.removeLast();
            changes.removeLast();
            --index;
            auto current = QSharedPointer<DocumentVersion>(new DocumentVersion(*states[index]));
            current->activeLayer = version->activeLayer;
            current->selectedLayers = version->selectedLayers;
            current->m_revision = version->m_revision;
            states[index] = current;
        } else {
            version->m_committed = true;
            states[index] = version;
            changes[index] = change;
        }
        m_history.swap(states);
        m_layerHistoryChanges.swap(changes);
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
    if (m_edit->brush.basicSmoothing) {
        if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
        auto operation = QSharedPointer<PaintOperation>::create(*m_edit);
        operation->smoothingFinished = true;
        ++operation->version;
        replacePreview(operation);
    }
    commitVersion(m_preview);
    m_edit.clear();
    m_preview.clear();
    return Result::Success;
}

Result OperationDocument::flushSmoothing(OperationId id)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    if (!m_edit->brush.basicSmoothing) return Result::InvalidInput;
    if (m_edit->sampleCount < 2) return Result::EmptyOperation;
    if (!m_edit->smoothingFlushes.isEmpty() && m_edit->smoothingFlushes.last() == m_edit->sampleCount)
        return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto operation = QSharedPointer<PaintOperation>::create(*m_edit);
    operation->smoothingFlushes.append(operation->sampleCount);
    ++operation->version;
    replacePreview(operation);
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
    version->selectedLayers = {layer};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return layer;
}

LayerId OperationDocument::insertLayer(const OperationDocument &source, LayerId sourceLayer,
                                      const RasterLayer &properties, qsizetype index)
{ return insertVersion(source.snapshot(), sourceLayer, properties, index); }

LayerId OperationDocument::insertVersion(const Snapshot &source, LayerId sourceLayer,
                                        const RasterLayer &properties, qsizetype index)
{
    if (checkMutation() != Result::Success || !source || index < 0 || index > snapshot()->layers.size() ||
        !validLayerName(properties.name) || properties.uuid.isNull() || properties.channelWriteMask > 15 ||
        !properties.tiles.isEmpty() || snapshot()->layers.size() >= m_limits.layerCount ||
        m_nextLayer == std::numeric_limits<quint64>::max() || m_nextRevision == std::numeric_limits<quint64>::max()) return 0;
    const auto found = std::find_if(source->layers.cbegin(), source->layers.cend(),
                                   [sourceLayer](const Layer &layer) { return layer.id == sourceLayer; });
    if (found == source->layers.cend() || found->kind != LayerKind::Paint || properties.kind != LayerKind::Paint ||
        found->defaultPixel != properties.defaultPixel) return 0;
    if (!properties.parentUuid.isNull()) {
        const auto parent = std::find_if(snapshot()->layers.cbegin(), snapshot()->layers.cend(),
            [&](const Layer &layer) { return layer.uuid == properties.parentUuid && layer.kind == LayerKind::Group; });
        if (parent == snapshot()->layers.cend()) return 0;
    }
    for (const auto &state : m_history)
        for (const auto &layer : state->layers) if (layer.uuid == properties.uuid) return 0;
    auto samples = snapshot()->sampleCount;
    auto pixels = snapshot()->rasterBytes;
    auto nextOperation = m_nextOperation;
    for (const auto &operation : found->operations) {
        if (operationSamples(operation) > m_limits.samplesPerStroke ||
            operationSamples(operation) > m_limits.samplesPerDocument - samples ||
            operationRasterBytes(operation) > m_limits.rasterBytes - pixels ||
            nextOperation == std::numeric_limits<quint64>::max()) return 0;
        samples += operationSamples(operation);
        pixels += operationRasterBytes(operation);
        ++nextOperation;
    }
    Layer added = *found;
    added.id = m_nextLayer + 1;
    added.name = properties.name;
    added.uuid = properties.uuid;
    added.visible = properties.visible;
    added.opacity = properties.opacity;
    added.userLocked = properties.userLocked;
    added.pinnedToTimeline = properties.pinnedToTimeline;
    added.channelWriteMask = properties.channelWriteMask;
    added.position = properties.position;
    added.parentUuid = properties.parentUuid;
    nextOperation = m_nextOperation;
    copyLayerOperations(*found, added, m_identity, nextOperation);
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers.insert(index, added);
    version->activeLayer = added.id;
    version->selectedLayers = {added.id};
    version->sampleCount = samples;
    version->rasterBytes = pixels;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    m_nextLayer = added.id;
    m_nextOperation = nextOperation;
    commitVersion(version);
    return added.id;
}

Result OperationDocument::duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (layers.isEmpty() || layers.size() != names.size()) return Result::InvalidInput;
    const auto original = snapshot();
    qsizetype previousIndex = -1;
    QSet<LayerId> included;
    QHash<LayerId, QString> copyNames;
    for (int i = 0; i < layers.size(); ++i) {
        const auto index = layerIndex(layers[i]);
        if (index < 0) return Result::LayerUnavailable;
        if (index <= previousIndex || !validLayerName(names[i])) return Result::InvalidInput;
        previousIndex = index;
        included.insert(layers[i]);
        copyNames.insert(layers[i], names[i]);
    }
    includeLayerDescendants(original->layers, included);
    if (included.size() > m_limits.layerCount - original->layers.size() ||
        quint64(included.size()) > std::numeric_limits<quint64>::max() - m_nextLayer ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    qsizetype samples = original->sampleCount, pixels = original->rasterBytes;
    quint64 nextOperation = m_nextOperation;
    for (qsizetype index = 0; index < original->layers.size(); ++index) {
        if (!included.contains(original->layers[index].id)) continue;
        previousIndex = index;
        for (const auto &operation : original->layers[index].operations) {
            if (operationSamples(operation) > m_limits.samplesPerDocument - samples ||
                operationRasterBytes(operation) > m_limits.rasterBytes - pixels ||
                nextOperation == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
            samples += operationSamples(operation);
            pixels += operationRasterBytes(operation);
            ++nextOperation;
        }
    }
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*original));
    auto nextLayer = m_nextLayer;
    nextOperation = m_nextOperation;
    auto insertion = previousIndex + 1;
    version->selectedLayers.clear();
    QSet<QUuid> identities;
    for (const auto &layer : original->layers) identities.insert(layer.uuid);
    QHash<QUuid, QUuid> copiedIdentities;
    QList<Layer> copies;
    const auto active = layers.contains(activeSource) ? activeSource : layers.first();
    for (const auto &source : original->layers) {
        if (!included.contains(source.id)) continue;
        Layer copy = source;
        copy.id = ++nextLayer;
        const auto sourceIdentity = copy.uuid;
        bool unique = false;
        for (qsizetype attempt = 0; attempt <= identities.size(); ++attempt) {
            const QByteArray name = QByteArrayLiteral("LibrePaint layer copy:") +
                QByteArray::number(copy.id) + ':' + QByteArray::number(attempt);
            copy.uuid = QUuid::createUuidV5(sourceIdentity, name);
            if (!identities.contains(copy.uuid)) { unique = true; break; }
        }
        if (!unique) return Result::CapacityExceeded;
        identities.insert(copy.uuid);
        copiedIdentities.insert(source.uuid, copy.uuid);
        if (copyNames.contains(source.id)) copy.name = copyNames.value(source.id);
        copyLayerOperations(source, copy, m_identity, nextOperation);
        if (source.id == active) version->activeLayer = copy.id;
        if (copyNames.contains(source.id)) version->selectedLayers.append(copy.id);
        copies.append(std::move(copy));
    }
    for (auto &copy : copies) {
        copy.parentUuid = copiedIdentities.value(copy.parentUuid, copy.parentUuid);
        version->layers.insert(insertion++, copy);
    }
    version->sampleCount = samples;
    version->rasterBytes = pixels;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    m_nextLayer = nextLayer;
    m_nextOperation = nextOperation;
    LayerHistoryChange change;
    change.duplicatedLayers = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::selectLayer(LayerId layer)
{
    return selectLayers(snapshot()->selectedLayers.contains(layer) ? snapshot()->selectedLayers : QList<LayerId>{layer}, layer);
}

Result OperationDocument::selectLayers(const QList<LayerId> &layers, LayerId active)
{
    if (m_closed) return Result::Closed;
    if (layerIndex(active) < 0) return Result::LayerUnavailable;
    if (layers.isEmpty()) return Result::InvalidInput;
    QSet<LayerId> selected;
    for (const auto layer : layers) {
        if (layerIndex(layer) < 0) return Result::LayerUnavailable;
        if (selected.contains(layer)) return Result::InvalidInput;
        selected.insert(layer);
    }
    QList<LayerId> ordered;
    for (const auto &layer : snapshot()->layers) if (selected.contains(layer.id)) ordered.append(layer.id);
    if (active == activeLayer() && ordered == snapshot()->selectedLayers) return Result::Success;
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    version->activeLayer = active;
    version->selectedLayers = ordered;
    Snapshot preview = m_preview;
    if (preview) {
        QSharedPointer<DocumentVersion> value(new DocumentVersion(*preview));
        value->activeLayer = active;
        value->selectedLayers = ordered;
        preview = value;
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

Result OperationDocument::writeLayerChannelMask(LayerId layer, quint8 mask)
{
    if (mask > 15) return Result::InvalidInput;
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index >= 0 && snapshot()->layers[index].kind != LayerKind::Paint) return Result::LayerUnavailable;
    return writeLayerProperty(layer, LayerProperty::ChannelMask, &Layer::channelWriteMask, mask);
}

Result OperationDocument::setLayerChannelMask(LayerId layer, quint8 mask, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (mask > 15 || (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive)) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint) return Result::LayerUnavailable;
    if (snapshot()->layers[index].channelWriteMask == mask) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].channelWriteMask = mask;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::ChannelMask, layer,
        QVariant::fromValue(snapshot()->layers[index].channelWriteMask), QVariant::fromValue(mask)}, history);
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

Result OperationDocument::clearLayers(const QList<LayerId> &layers)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (layers.isEmpty()) return Result::InvalidInput;
    QSet<LayerId> cleared;
    for (const auto layer : layers) {
        if (cleared.contains(layer)) return Result::InvalidInput;
        const auto index = layerIndex(layer);
        if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint ||
            snapshot()->layers[index].userLocked) return Result::LayerUnavailable;
        cleared.insert(layer);
    }
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    for (auto &layer : version->layers) {
        if (!cleared.contains(layer.id)) continue;
        for (const auto &operation : layer.operations) {
            version->sampleCount -= operationSamples(operation);
            version->rasterBytes -= operationRasterBytes(operation);
        }
        layer.operations.clear();
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.clearedLayers = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::cropLayer(LayerId layer, const QRect &rect)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint) return Result::LayerUnavailable;
    if (!validCropRect(rect)) return Result::InvalidInput;
    if (m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto operation = QSharedPointer<CropOperation>::create();
    operation->id = {m_identity, m_nextOperation + 1};
    operation->layer = layer;
    operation->rect = rect;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].operations.append(CropSnapshot(operation));
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    ++m_nextOperation;
    LayerHistoryChange change;
    change.crop = operation;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::fillLayer(LayerId layer, const std::array<quint8, 3> &rgb,
                                    quint8 opacity, bool erase, bool alphaLocked)
{
    Brush brush;
    brush.rgba = {rgb[0], rgb[1], rgb[2], 255};
    brush.opacity = opacity;
    brush.erase = erase;
    brush.alphaLocked = alphaLocked;
    brush.channelWriteMask = alphaLocked ? 7 : 15;
    FigureStyle style;
    style.outline = false;
    style.fill = brush.rgba;
    const QPointF end(snapshot()->size.width(), snapshot()->size.height());
    const auto result = paintFigure(layer, brush, {{{0, 0}, 1}, {end, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, end, 0}, style, 0);
    if (result == Result::Success) m_layerHistoryChanges[m_index].filledLayer = true;
    return result;
}

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
    includeLayerDescendants(snapshot()->layers, removed);
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
    const auto selected = version->selectedLayers;
    version->selectedLayers.clear();
    for (const auto &layer : version->layers)
        if (selected.contains(layer.id) || layer.id == version->activeLayer) version->selectedLayers.append(layer.id);
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
    const auto previousOperation = m_nextOperation;
    bool completed = false;
    const auto rollback = qScopeGuard([&] {
        if (completed) return;
        m_history = previousHistory;
        m_index = previousIndex;
        m_nextRevision = previousRevision;
        m_nextOperation = previousOperation;
    });
    while (m_index != index) {
        const bool forward = index > m_index;
        const auto change = m_layerHistoryChanges[m_index + (forward ? 1 : 0)];
        m_index += forward ? 1 : -1;
        const auto result = restoreLayerHistoryChange(change, forward);
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
    m_translation.clear();
    m_layerHistoryChanges = {LayerHistoryChange{}};
    m_index = 0;
    m_closed = true;
}

}
