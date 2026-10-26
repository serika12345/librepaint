/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuOperationDocument.h"
#include "KisGpuOperationValidation_p.h"

#include <limits>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <QScopeGuard>
#include <QSet>
#include <QHash>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
namespace {
qsizetype operationSamples(const LayerOperation &value)
{
    const auto paint = std::get_if<PaintSnapshot>(&value);
    return paint ? (*paint)->sampleCount : 0;
}
qsizetype operationRasterBytes(const LayerOperation &value)
{
    if (const auto paint = std::get_if<PaintSnapshot>(&value))
        return (*paint)->figureStyle.pattern ? (*paint)->figureStyle.pattern->byteCount() : 0;
    const auto write = std::get_if<TileWriteSnapshot>(&value);
    return write ? (*write)->tiles.size() * RgbaTile::ByteCount : 0;
}
}

bool freezeLumaCoefficients(const std::array<qreal, 3> &coefficients,
                            std::array<double, 3> *frozen)
{
    if (!frozen) return false;
    std::array<qreal, 3> effective = coefficients;
    for (const qreal coefficient : effective) {
        if (!std::isfinite(coefficient)) return false;
    }
    // The product transformation substitutes its Rec.709 defaults when any
    // coefficient supplied by the document color space is non-positive.
    if (effective[0] <= 0.0 || effective[1] <= 0.0 || effective[2] <= 0.0)
        effective = {0.2126, 0.7152, 0.0722};
    const std::array<float, 3> rounded{
        float(effective[0]), float(effective[1]), float(effective[2])};
    for (const float coefficient : rounded) {
        // The YUV rule divides by (1 - R) and (1 - B); zero and one are not
        // usable document coefficients for any of the luma-dependent modes.
        if (!std::isfinite(coefficient) || coefficient <= 0.0f || coefficient >= 1.0f)
            return false;
    }
    *frozen = {double(effective[0]), double(effective[1]), double(effective[2])};
    return true;
}

namespace {
SelectionMaskSP selectionCoverage(const DocumentVersion &version)
{
    const QRect clip(QPoint(0, 0), version.size);
    if (version.selection.isEmpty() || clip.isEmpty()) return {};
    SelectionMaskSP coverage;
    if (version.selectionKind == SelectionKind::Freehand && version.selectionMask)
        coverage = version.selectionMask;
    else if (version.selectionKind == SelectionKind::Ellipse)
        coverage = SelectionMask::fromEllipse(version.selection, clip);
    else
        coverage = SelectionMask::fromRectangle(version.selection, clip);
    if (!coverage || coverage->isEmpty()) return {};
    return version.selectionInverted ? coverage->inverted(clip) : coverage;
}

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
        } else if (const auto filter = std::get_if<FilterSnapshot>(&sourceOperation)) {
            auto operation = QSharedPointer<FilterOperation>::create(**filter);
            operation->id = id;
            operation->layer = copy.id;
            copy.operations.append(FilterSnapshot(operation));
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
            } else if (const auto filter = std::get_if<FilterSnapshot>(&operation)) {
                auto copied = QSharedPointer<FilterOperation>::create(**filter);
                copied->id.document = result->m_identity;
                operation = FilterSnapshot(copied);
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
    if (m_layerHistoryChanges[index].addedTransparencyMask) return HistoryAction::AddTransparencyMask;
    if (m_layerHistoryChanges[index].removedTransparencyMask) return HistoryAction::RemoveTransparencyMask;
    if (m_layerHistoryChanges[index].changedLayerProperties) return HistoryAction::ChangeLayerProperties;
    if (m_layerHistoryChanges[index].duplicatedLayers) return HistoryAction::DuplicateLayer;
    if (m_layerHistoryChanges[index].clearedLayers) return HistoryAction::ClearPixels;
    if (m_layerHistoryChanges[index].filledLayer) return HistoryAction::FillLayer;
    if (m_layerHistoryChanges[index].filteredLayer) {
        switch (m_layerHistoryChanges[index].filterKind) {
        case FilterKind::Grayscale: return HistoryAction::Grayscale;
        case FilterKind::HsvAdjustment: return HistoryAction::HsvAdjustment;
        case FilterKind::InvertColors: break;
        }
        return HistoryAction::InvertColors;
    }
    if (m_layerHistoryChanges[index].changedSelection &&
        m_history[index - 1]->layers.size() == m_history[index]->layers.size())
        return HistoryAction::ChangeSelection;
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
    case LayerProperty::ProjectionMask: return HistoryAction::ChangeProjectionChannels;
    case LayerProperty::Position: return HistoryAction::TranslateLayer;
    case LayerProperty::None: break;
    case LayerProperty::ColorLabel: break;
    case LayerProperty::MaskVisible: return HistoryAction::ChangeMaskVisibility;
    case LayerProperty::PassThrough: return HistoryAction::ChangePassThrough;
    case LayerProperty::BlendMode: return HistoryAction::ChangeBlendMode;
    }
    const auto &before = m_history[index - 1]->layers;
    const auto &after = m_history[index]->layers;
    if (after.size() > before.size()) return HistoryAction::AddLayer;
    if (after.size() < before.size()) return HistoryAction::RemoveLayer;
    if (before.size() == 1 && after.size() == 1 && before.first().id != after.first().id)
        return HistoryAction::RemoveLayer;
    for (int layer = 0; layer < after.size(); ++layer) {
        if (before[layer].id != after[layer].id || before[layer].parentUuid != after[layer].parentUuid)
            return HistoryAction::MoveLayer;
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
    // A retained mask-only edit leaves the layer inputs untouched and is
    // identified by its coverage.
    for (int layer = 0; layer < after.size(); ++layer)
        if (before[layer].transparencyMask != after[layer].transparencyMask) return HistoryAction::PaintStroke;
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
bool OperationDocument::hasActiveStroke() const { return m_edit || m_translation || !m_externalOperation.isNull(); }
OperationId OperationDocument::activeOperation() const
{
    if (m_edit) return m_edit->id;
    if (m_translation) return m_translation->id;
    return m_externalOperation;
}

Result OperationDocument::beginExternalStroke(const OperationId &id)
{
    if (m_closed) return Result::Closed;
    if (hasActiveStroke()) return Result::EditInProgress;
    if (id.isNull()) return Result::InvalidInput;
    m_externalOperation = id;
    return Result::Success;
}

Result OperationDocument::endExternalStroke(const OperationId &id)
{
    if (m_closed) return Result::Closed;
    if (m_externalOperation.isNull() || m_externalOperation != id) return Result::StaleOperation;
    m_externalOperation = {};
    return Result::Success;
}

Result OperationDocument::setSelectionRect(const QRect &rect, bool inverted)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto normalized = rect.normalized();
    if (!normalized.isEmpty() && !Detail::validSelectionRect(normalized)) return Result::InvalidInput;
    const bool effectiveInverted = !normalized.isEmpty() && inverted;
    if (normalized == snapshot()->selection && effectiveInverted == snapshot()->selectionInverted &&
        snapshot()->selectionKind == SelectionKind::Rectangle) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->selection = normalized;
    version->selectionInverted = effectiveInverted;
    version->selectionKind = SelectionKind::Rectangle;
    version->selectionMask.clear();
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.changedSelection = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::setSelectionEllipse(const QRect &rect, bool inverted)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto normalized = rect.normalized();
    if (normalized.isEmpty() || !Detail::validSelectionRect(normalized)) return Result::InvalidInput;
    const bool effectiveInverted = !normalized.isEmpty() && inverted;
    if (normalized == snapshot()->selection && snapshot()->selectionKind == SelectionKind::Ellipse &&
        effectiveInverted == snapshot()->selectionInverted) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->selection = normalized;
    version->selectionInverted = effectiveInverted;
    version->selectionKind = SelectionKind::Ellipse;
    version->selectionMask.clear();
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.changedSelection = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::setSelectionMask(const SelectionMaskSP &mask, bool inverted)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!mask) return setSelectionRect({}, false);
    if (mask->isEmpty() || mask->bounds().isEmpty() || mask->byteCount() > SelectionMask::MaximumBytes)
        return Result::InvalidInput;
    if (snapshot()->selectionKind == SelectionKind::Freehand && snapshot()->selectionMask == mask &&
        snapshot()->selectionInverted == inverted) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->selection = mask->bounds();
    version->selectionInverted = inverted;
    version->selectionKind = SelectionKind::Freehand;
    version->selectionMask = mask;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.changedSelection = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::clearSelection()
{ return setSelectionRect({}, false); }

QRect OperationDocument::selectionRect() const { return snapshot()->selection; }

bool OperationDocument::selectionInverted() const { return snapshot()->selectionInverted; }

SelectionKind OperationDocument::selectionKind() const { return snapshot()->selectionKind; }

SelectionMaskSP OperationDocument::selectionMask() const { return snapshot()->selectionMask; }

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
Result OperationDocument::writeLayerProperty(LayerId layer, LayerProperty property, T Layer::*member,
                                            const T &value, quint8 componentMask)
{
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    const T previous = snapshot()->layers[index].*member;
    const bool components = property == LayerProperty::ChannelMask || property == LayerProperty::ProjectionMask;
    if constexpr (std::is_same_v<T, quint8>) {
        if (components ? (previous & componentMask) == (value & componentMask) : previous == value) return Result::Success;
    } else if (previous == value) return Result::Success;
    const auto containsLayer = [&](const Snapshot &state) {
        for (const auto &candidate : state->layers)
            if (candidate.id == layer) return true;
        return false;
    };
    const auto changesProperty = [&](int index) {
        if (property == LayerProperty::None) return false;
        const auto &change = m_layerHistoryChanges[index];
        if (change.layer == layer && change.property == property) return true;
        for (const auto &edit : change.propertyEdits)
            if (edit.layer == layer && edit.property == property &&
                (!components || (edit.componentMask & componentMask))) return true;
        return false;
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
            if constexpr (std::is_same_v<T, quint8>) {
                candidate.*member = components ? quint8(((candidate.*member) & ~componentMask) | (value & componentMask)) : value;
            } else candidate.*member = value;
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

Result OperationDocument::writeLayerMask(LayerId layer, LayerProperty property, quint8 Layer::*member, quint8 value)
{
    const auto history = m_history;
    const auto preview = m_preview;
    const auto revision = m_nextRevision;
    for (quint8 bit = 1; bit <= 8; bit <<= 1) {
        const auto result = writeLayerProperty(layer, property, member, value, bit);
        if (result == Result::Success) continue;
        m_history = history;
        m_preview = preview;
        m_nextRevision = revision;
        return result;
    }
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
Result OperationDocument::writeLayerColorLabel(LayerId layer, int index)
{ return writeLayerProperty(layer, LayerProperty::ColorLabel, &Layer::colorLabelIndex, index); }

Result OperationDocument::restoreLayerHistoryChange(const LayerHistoryChange &change, bool forward)
{
    const auto status = restoreLayerPropertyEdit({change.property, change.layer, change.before, change.after}, forward);
    if (status != Result::Success) return status;
    for (const auto &edit : change.propertyEdits) {
        const auto result = restoreLayerPropertyEdit(edit, forward);
        if (result != Result::Success) return result;
    }
    return Result::Success;
}

Result OperationDocument::restoreLayerPropertyEdit(const LayerPropertyEdit &change, bool forward)
{
    const auto &value = forward ? change.after : change.before;
    switch (change.property) {
    case LayerProperty::ChannelMask:
    case LayerProperty::ProjectionMask: {
        const auto index = layerIndex(change.layer);
        if (index < 0) return Result::LayerUnavailable;
        const bool projection = change.property == LayerProperty::ProjectionMask;
        const auto &layer = snapshot()->layers[index];
        const auto current = projection ? layer.projectionChannelMask : layer.channelWriteMask;
        const auto mask = quint8((current & ~change.componentMask) | (value.toUInt() & change.componentMask));
        return projection ? writeLayerProjectionMask(change.layer, mask) : writeLayerChannelMask(change.layer, mask);
    }
    case LayerProperty::Position: return writeLayerPosition(change.layer, value.toPoint());
    case LayerProperty::None: return Result::Success;
    case LayerProperty::Name: return writeLayerName(change.layer, value.toString());
    case LayerProperty::Opacity: return writeLayerOpacity(change.layer, quint8(value.toUInt()));
    case LayerProperty::Visible: return writeLayerVisible(change.layer, value.toBool());
    case LayerProperty::Locked: return writeLayerLocked(change.layer, value.toBool());
    case LayerProperty::PinnedToTimeline: return writeLayerPinnedToTimeline(change.layer, value.toBool());
    case LayerProperty::ColorLabel: return writeLayerColorLabel(change.layer, value.toInt());
    case LayerProperty::PassThrough: return writeLayerPassThrough(change.layer, value.toBool());
    case LayerProperty::BlendMode: return writeLayerBlendMode(change.layer, LayerBlendMode(value.toUInt()));
    case LayerProperty::MaskVisible:
        // The retained versions carry their own mask state, so history
        // navigation restores it without rewriting neighbouring versions.
        return Result::Success;
    }
    return Result::InvalidInput;
}

Result OperationDocument::collectLayerPropertyEdits(const Snapshot &before, const Snapshot &after,
                                                   const QList<LayerPropertiesSelection> &selection,
                                                   QList<LayerPropertyEdit> &edits) const
{
    if (!before || !after || before->documentId() != m_identity || after != snapshot() ||
        !before->isCommitted()) return Result::StaleVersion;
    QHash<LayerId, const Layer *> originals;
    for (const auto &layer : before->layers) originals.insert(layer.id, &layer);
    QSet<LayerId> included;
    for (const auto &fields : selection) {
        const auto index = layerIndex(fields.layer);
        if (index < 0) return Result::LayerUnavailable;
        if (included.contains(fields.layer) || fields.protectedChannels > 15 || fields.projectionChannels > 15 ||
            (fields.protectedChannels && after->layers[index].kind != LayerKind::Paint) ||
            (fields.passThrough && after->layers[index].kind != LayerKind::Group)) return Result::InvalidInput;
        included.insert(fields.layer);
        const auto *old = originals.value(fields.layer);
        if (!old) return Result::LayerUnavailable;
        const auto &current = after->layers[index];
        if (old->uuid != current.uuid || old->kind != current.kind) return Result::StaleVersion;
        const auto add = [&](bool enabled, LayerProperty property, const auto &previous, const auto &value, quint8 mask = 15) {
            if (enabled) edits.append({property, current.id, QVariant::fromValue(previous), QVariant::fromValue(value), mask});
        };
        add(fields.name, LayerProperty::Name, old->name, current.name);
        add(fields.opacity, LayerProperty::Opacity, old->opacity, current.opacity);
        add(fields.visible, LayerProperty::Visible, old->visible, current.visible);
        add(fields.locked, LayerProperty::Locked, old->userLocked, current.userLocked);
        add(fields.protectedChannels, LayerProperty::ChannelMask, old->channelWriteMask, current.channelWriteMask, fields.protectedChannels);
        add(fields.projectionChannels, LayerProperty::ProjectionMask, old->projectionChannelMask, current.projectionChannelMask, fields.projectionChannels);
        add(fields.colorLabel, LayerProperty::ColorLabel, old->colorLabelIndex, current.colorLabelIndex);
        add(fields.passThrough, LayerProperty::PassThrough, old->passThrough, current.passThrough);
        add(fields.blendMode, LayerProperty::BlendMode, quint8(old->blendMode), quint8(current.blendMode));
    }
    return Result::Success;
}

Result OperationDocument::restoreLayerPropertyEdits(const QList<LayerPropertyEdit> &edits)
{
    const auto history = m_history;
    const auto preview = m_preview;
    const auto revision = m_nextRevision;
    for (const auto &edit : edits) {
        const auto result = restoreLayerPropertyEdit(edit, false);
        if (result == Result::Success) continue;
        m_history = history;
        m_preview = preview;
        m_nextRevision = revision;
        return result;
    }
    return Result::Success;
}

Result OperationDocument::restoreLayerProperties(const Snapshot &before,
                                                const QList<LayerPropertiesSelection> &selection)
{
    if (m_closed) return Result::Closed;
    QList<LayerPropertyEdit> edits;
    const auto status = collectLayerPropertyEdits(before, snapshot(), selection, edits);
    return status == Result::Success ? restoreLayerPropertyEdits(edits) : status;
}

Result OperationDocument::recordLayerProperties(const Snapshot &before, const Snapshot &after,
                                               const QList<LayerPropertiesSelection> &selection)
{
    const auto mutation = checkMutation();
    if (mutation != Result::Success) return mutation;
    LayerHistoryChange change;
    change.changedLayerProperties = true;
    const auto status = collectLayerPropertyEdits(before, after, selection, change.propertyEdits);
    if (status != Result::Success) return status;
    const auto previousHistory = m_history;
    const auto previousRevision = m_nextRevision;
    bool completed = false;
    const auto rollback = qScopeGuard([&] {
        if (completed) return;
        m_history = previousHistory;
        m_nextRevision = previousRevision;
    });
    const auto result = restoreLayerPropertyEdits(change.propertyEdits);
    if (result != Result::Success) return result;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*after));
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version, change);
    completed = true;
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
    operation->selectionRect = snapshot()->selection;
    operation->selectionInverted = snapshot()->selectionInverted;
    operation->selectionKind = snapshot()->selectionKind;
    operation->selectionMask = snapshot()->selectionMask;
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
    if (!validBrush(brush) || !validShape(shape) || !validFigureStyle(shape, style, brush) ||
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
        m_nextRevision > std::numeric_limits<quint64>::max() - (samples.isEmpty() ? 1 : 2) ||
        (style.pattern && (style.pattern->byteCount() > m_limits.rasterBytes ||
                           snapshot()->rasterBytes > m_limits.rasterBytes - style.pattern->byteCount()))) {
        return Result::CapacityExceeded;
    }
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
    if (!validShape(shape) || !validFigureStyle(shape, style, m_edit->brush) ||
        (m_edit->brush.basicSmoothing && shape.kind != ShapeKind::Freehand)) return Result::InvalidInput;
    if (m_edit->sampleCount == 0 && shape.kind != ShapeKind::Path) return Result::EmptyOperation;
    if (!validShapeSampleCount(shape, m_edit->sampleCount)) return Result::InvalidInput;
    if (style.pattern && (style.pattern->byteCount() > m_limits.rasterBytes ||
                          snapshot()->rasterBytes > m_limits.rasterBytes - style.pattern->byteCount()))
        return Result::CapacityExceeded;
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
    version->rasterBytes += operationRasterBytes(LayerOperation(PaintSnapshot(operation)));
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
    const bool gaussianCompletion = [&] {
        if (!m_edit->brush.gaussianWash || m_edit->shape.kind != ShapeKind::Freehand ||
            !m_edit->sampleCount) return false;
        if (m_edit->brush.pixelPerfect) return true;
        const auto position = m_edit->sampleBlocks.first()->first().position;
        for (const auto &block : m_edit->sampleBlocks)
            for (const auto &point : *block)
                if (point.position.x() != position.x() || point.position.y() != position.y()) return false;
        return true;
    }();
    // Completing Gaussian input can add a tap after pixel correction.
    // Its pending pixels belong to a different operation version.
    if (m_edit->brush.basicSmoothing || gaussianCompletion) {
        if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
        auto operation = QSharedPointer<PaintOperation>::create(*m_edit);
        if (operation->brush.basicSmoothing) operation->smoothingFinished = true;
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

LayerId OperationDocument::addLayer(const QString &name, qsizetype index, bool pinnedToTimeline, const QUuid &parentUuid)
{
    if (checkMutation() != Result::Success || snapshot()->layers.size() >= m_limits.layerCount ||
        !validLayerName(name) || index < 0 || index > snapshot()->layers.size() ||
        m_nextLayer == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return 0;
    }
    if (!parentUuid.isNull()) {
        const auto parent = std::find_if(snapshot()->layers.cbegin(), snapshot()->layers.cend(),
            [&](const Layer &layer) { return layer.uuid == parentUuid && layer.kind == LayerKind::Group; });
        if (parent == snapshot()->layers.cend()) return 0;
    }
    QSharedPointer<DocumentVersion> version(new DocumentVersion(*snapshot()));
    const LayerId layer = ++m_nextLayer;
    Layer added{layer, name, true, {}, QUuid::createUuid()};
    added.pinnedToTimeline = pinnedToTimeline;
    added.parentUuid = parentUuid;
    version->layers.insert(index, added);
    version->activeLayer = layer;
    version->selectedLayers = {layer};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version);
    return layer;
}

LayerId OperationDocument::insertLayer(const OperationDocument &source, LayerId sourceLayer,
                                      const RasterLayer &properties, qsizetype index, bool activate)
{ return insertVersion(source.snapshot(), sourceLayer, properties, index, activate); }

LayerId OperationDocument::insertVersion(const Snapshot &source, LayerId sourceLayer,
                                        const RasterLayer &properties, qsizetype index, bool activate)
{
    if (!source) return 0;
    const auto found = std::find_if(source->layers.cbegin(), source->layers.cend(),
                                   [sourceLayer](const Layer &layer) { return layer.id == sourceLayer; });
    if (found == source->layers.cend() ||
        (found->kind == LayerKind::Group &&
         std::any_of(source->layers.cbegin(), source->layers.cend(),
                     [&](const Layer &layer) { return layer.parentUuid == found->uuid; }))) return 0;
    if (insertLayers({{source, sourceLayer, properties}}, index, activate) != Result::Success) return 0;
    if (activate) return snapshot()->activeLayer;
    for (const auto &layer : snapshot()->layers)
        if (layer.uuid == properties.uuid) return layer.id;
    return 0;
}

Result OperationDocument::insertLayers(const QList<LayerInsertion> &layers, qsizetype index, bool activate,
                                       bool clearPixelSelection)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto before = snapshot();
    if (layers.isEmpty() || index < 0 || index > before->layers.size()) return Result::InvalidInput;
    if (layers.size() > m_limits.layerCount - before->layers.size() ||
        quint64(layers.size()) > std::numeric_limits<quint64>::max() - m_nextLayer ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSet<QUuid> identities, inserted;
    for (const auto &version : m_history)
        for (const auto &layer : version->layers) identities.insert(layer.uuid);
    QList<Layer> added;
    auto samples = before->sampleCount;
    auto pixels = before->rasterBytes;
    auto nextLayer = m_nextLayer;
    auto nextOperation = m_nextOperation;
    for (const auto &input : layers) {
        const auto &properties = input.properties;
        if (!input.source || !validLayerName(properties.name) || properties.uuid.isNull() ||
            identities.contains(properties.uuid) || properties.channelWriteMask > 15 || properties.projectionChannelMask > 15 ||
            !properties.tiles.isEmpty()) return Result::InvalidInput;
        identities.insert(properties.uuid);
        inserted.insert(properties.uuid);
        const auto found = std::find_if(input.source->layers.cbegin(), input.source->layers.cend(),
            [&](const Layer &layer) { return layer.id == input.sourceLayer; });
        if (found == input.source->layers.cend()) return Result::LayerUnavailable;
        if (found->kind != properties.kind || found->defaultPixel != properties.defaultPixel ||
            (found->kind == LayerKind::Group &&
             (properties.channelWriteMask != 15 || !found->operations.isEmpty()))) return Result::InvalidInput;
        for (const auto &operation : found->operations) {
            if (operationSamples(operation) > m_limits.samplesPerStroke ||
                operationSamples(operation) > m_limits.samplesPerDocument - samples ||
                operationRasterBytes(operation) > m_limits.rasterBytes - pixels ||
                nextOperation == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
            samples += operationSamples(operation);
            pixels += operationRasterBytes(operation);
            ++nextOperation;
        }
        Layer copy = *found;
        copy.id = ++nextLayer;
        copy.name = properties.name;
        copy.uuid = properties.uuid;
        copy.visible = properties.visible;
        copy.opacity = properties.opacity;
        copy.userLocked = properties.userLocked;
        copy.pinnedToTimeline = properties.pinnedToTimeline;
        copy.colorLabelIndex = properties.colorLabelIndex;
        copy.channelWriteMask = properties.channelWriteMask;
        copy.projectionChannelMask = properties.projectionChannelMask;
        copy.passThrough = properties.passThrough;
        copy.blendMode = properties.blendMode;
        copy.position = properties.position;
        copy.parentUuid = properties.parentUuid;
        added.append(std::move(copy));
    }
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    for (qsizetype i = 0; i < added.size(); ++i) version->layers.insert(index + i, added[i]);
    if (!Detail::validLayerHierarchy(version->layers)) return Result::InvalidInput;
    nextOperation = m_nextOperation;
    version->selectedLayers.clear();
    for (qsizetype i = 0; i < added.size(); ++i) {
        auto &copy = version->layers[index + i];
        copyLayerOperations(added[i], copy, m_identity, nextOperation);
        if (!inserted.contains(copy.parentUuid)) version->selectedLayers.append(copy.id);
    }
    if (activate) version->activeLayer = version->selectedLayers.first();
    else {
        version->activeLayer = before->activeLayer;
        version->selectedLayers = before->selectedLayers;
    }
    LayerHistoryChange change;
    if (clearPixelSelection) {
        change.changedSelection = !before->selection.isEmpty() || before->selectionInverted ||
            before->selectionKind != SelectionKind::Rectangle || bool(before->selectionMask);
        version->selection = {};
        version->selectionInverted = false;
        version->selectionKind = SelectionKind::Rectangle;
        version->selectionMask.clear();
    }
    version->sampleCount = samples;
    version->rasterBytes = pixels;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    m_nextLayer = nextLayer;
    m_nextOperation = nextOperation;
    commitVersion(version, change);
    return Result::Success;
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
    QHash<QUuid, qsizetype> indices;
    QHash<QUuid, QList<qsizetype>> children;
    for (qsizetype i = 0; i < original->layers.size(); ++i) {
        indices.insert(original->layers[i].uuid, i);
        children[original->layers[i].parentUuid].append(i);
    }
    QList<QPair<qsizetype, bool>> pending;
    const auto rootIndices = children.value(QUuid());
    for (auto it = rootIndices.crbegin(); it != rootIndices.crend(); ++it) pending.append({*it, false});
    QList<qsizetype> selectedRoots, ordered;
    QSet<QUuid> selectedSubtree;
    QHash<QUuid, bool> editable;
    while (!pending.isEmpty()) {
        const auto [index, visited] = pending.takeLast();
        const auto &layer = original->layers[index];
        if (visited) {
            if (included.contains(layer.id)) ordered.append(index);
            continue;
        }
        const bool parentSelected = selectedSubtree.contains(layer.parentUuid);
        if (parentSelected) copyNames.remove(layer.id);
        if (copyNames.contains(layer.id)) selectedRoots.append(index);
        if (parentSelected || copyNames.contains(layer.id)) {
            selectedSubtree.insert(layer.uuid);
            included.insert(layer.id);
        }
        editable.insert(layer.uuid, !layer.userLocked &&
            (layer.parentUuid.isNull() || editable.value(layer.parentUuid)));
        pending.append({index, true});
        const auto branches = children.value(layer.uuid);
        for (auto it = branches.crbegin(); it != branches.crend(); ++it) pending.append({*it, false});
    }
    auto anchor = selectedRoots.last();
    while (!original->layers[anchor].parentUuid.isNull() &&
           !editable.value(original->layers[anchor].parentUuid))
        anchor = indices.value(original->layers[anchor].parentUuid);
    const auto destinationParent = original->layers[anchor].parentUuid;
    if (included.size() > m_limits.layerCount - original->layers.size() ||
        quint64(included.size()) > std::numeric_limits<quint64>::max() - m_nextLayer ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    qsizetype samples = original->sampleCount, pixels = original->rasterBytes;
    quint64 nextOperation = m_nextOperation;
    for (const auto index : ordered) {
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
    auto insertion = anchor + 1;
    version->selectedLayers.clear();
    QSet<QUuid> identities;
    for (const auto &layer : original->layers) identities.insert(layer.uuid);
    QHash<QUuid, QUuid> copiedIdentities;
    QList<Layer> copies;
    const auto active = copyNames.contains(activeSource) ? activeSource : original->layers[selectedRoots.first()].id;
    for (const auto index : ordered) {
        const auto &source = original->layers[index];
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
        copy.parentUuid = copiedIdentities.value(copy.parentUuid, destinationParent);
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

Result OperationDocument::writeLayerPassThrough(LayerId layer, bool enabled)
{
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Group) return Result::LayerUnavailable;
    return writeLayerProperty(layer, LayerProperty::PassThrough, &Layer::passThrough, enabled);
}

Result OperationDocument::setLayerPassThrough(LayerId layer, bool enabled, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Group) return Result::LayerUnavailable;
    if (snapshot()->layers[index].passThrough == enabled) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].passThrough = enabled;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::PassThrough, layer,
        QVariant::fromValue(snapshot()->layers[index].passThrough), QVariant::fromValue(enabled)}, history);
    return Result::Success;
}

Result OperationDocument::writeLayerBlendMode(LayerId layer, LayerBlendMode mode)
{
    if (mode > LayerBlendMode::LinearDodge) return Result::InvalidInput;
    return writeLayerProperty(layer, LayerProperty::BlendMode, &Layer::blendMode, mode);
}

Result OperationDocument::setLayerBlendMode(LayerId layer, LayerBlendMode mode, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (mode > LayerBlendMode::LinearDodge ||
        (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive)) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].blendMode == mode) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].blendMode = mode;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::BlendMode, layer,
        QVariant::fromValue(quint8(snapshot()->layers[index].blendMode)), QVariant::fromValue(quint8(mode))}, history);
    return Result::Success;
}

Result OperationDocument::addTransparencyMaskFromSelection(LayerId layer)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint) return Result::LayerUnavailable;
    if (snapshot()->layers[index].userLocked) return Result::LayerUnavailable;
    if (snapshot()->layers[index].transparencyMask) return Result::InvalidInput;
    const auto mask = selectionCoverage(*snapshot());
    if (!mask || mask->byteCount() > SelectionMask::MaximumBytes) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMask = mask;
    version->layers[index].transparencyMaskVisible = true;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layer = layer;
    change.addedTransparencyMask = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::setTransparencyMaskVisible(LayerId layer, bool visible)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (snapshot()->layers[index].transparencyMaskVisible == visible) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMaskVisible = visible;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitVersion(version, {LayerProperty::MaskVisible, layer,
        QVariant::fromValue(snapshot()->layers[index].transparencyMaskVisible), QVariant::fromValue(visible)});
    return Result::Success;
}

Result OperationDocument::removeTransparencyMask(LayerId layer)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMask.clear();
    version->layers[index].transparencyMaskVisible = true;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layer = layer;
    change.removedTransparencyMask = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::replaceTransparencyMask(LayerId layer, const SelectionMaskSP &mask, bool filled)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint ||
        !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (!mask || mask->isEmpty() || mask->bounds().isEmpty() ||
        mask->byteCount() > SelectionMask::MaximumBytes) return Result::InvalidInput;
    if (snapshot()->layers[index].transparencyMask == mask) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMask = mask;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    // The changed coverage identifies the brush action; a fill additionally
    // records its operation kind for the history display.
    change.filledLayer = filled;
    commitVersion(version, change);
    return Result::Success;
}

SelectionMaskSP OperationDocument::layerTransparencyMask(LayerId layer) const
{
    const auto index = layerIndex(layer);
    return index < 0 ? SelectionMaskSP{} : snapshot()->layers[index].transparencyMask;
}

bool OperationDocument::layerTransparencyMaskVisible(LayerId layer) const
{
    const auto index = layerIndex(layer);
    return index >= 0 && snapshot()->layers[index].transparencyMask &&
        snapshot()->layers[index].transparencyMaskVisible;
}

Result OperationDocument::writeLayerProjectionMask(LayerId layer, quint8 mask)
{
    if (mask > 15) return Result::InvalidInput;
    return writeLayerMask(layer, LayerProperty::ProjectionMask, &Layer::projectionChannelMask, mask);
}

Result OperationDocument::setLayerProjectionMask(LayerId layer, quint8 mask, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (mask > 15 || (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive)) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].projectionChannelMask == mask) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].projectionChannelMask = mask;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::ProjectionMask, layer,
        QVariant::fromValue(snapshot()->layers[index].projectionChannelMask), QVariant::fromValue(mask)}, history);
    return Result::Success;
}

Result OperationDocument::writeLayerChannelMask(LayerId layer, quint8 mask)
{
    if (mask > 15) return Result::InvalidInput;
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index >= 0 && snapshot()->layers[index].kind != LayerKind::Paint) return Result::LayerUnavailable;
    return writeLayerMask(layer, LayerProperty::ChannelMask, &Layer::channelWriteMask, mask);
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

Result OperationDocument::moveLayers(const QList<LayerId> &layers, const QUuid &parentUuid, const QUuid &aboveUuid,
                                     LayerId activeRoot)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (layers.isEmpty()) return Result::InvalidInput;
    const auto original = snapshot();
    QHash<QUuid, const Layer *> identities;
    QHash<LayerId, const Layer *> values;
    QHash<QUuid, QList<LayerId>> children;
    for (const auto &layer : original->layers) {
        identities.insert(layer.uuid, &layer);
        values.insert(layer.id, &layer);
        children[layer.parentUuid].append(layer.id);
    }
    const auto editable = [&](QUuid parent) {
        while (!parent.isNull()) {
            const auto layer = identities.value(parent);
            if (layer->userLocked) return false;
            parent = layer->parentUuid;
        }
        return true;
    };
    if (!parentUuid.isNull()) {
        const auto parent = identities.value(parentUuid);
        if (!parent) return Result::LayerUnavailable;
        if (parent->kind != LayerKind::Group) return Result::InvalidInput;
        if (!editable(parentUuid)) return Result::LayerUnavailable;
    }
    QSet<LayerId> requested;
    for (const auto id : layers) {
        if (requested.contains(id)) return Result::InvalidInput;
        const auto layer = values.value(id);
        if (!layer) return Result::LayerUnavailable;
        if (!editable(layer->parentUuid)) return Result::LayerUnavailable;
        requested.insert(id);
    }
    QSet<LayerId> moving;
    for (const auto id : layers) {
        auto parent = values.value(id)->parentUuid;
        while (!parent.isNull() && !requested.contains(identities.value(parent)->id))
            parent = identities.value(parent)->parentUuid;
        if (parent.isNull()) moving.insert(id);
    }
    if (activeRoot) {
        if (!values.contains(activeRoot)) return Result::LayerUnavailable;
        if (!moving.contains(activeRoot)) return Result::InvalidInput;
    }
    for (auto parent = parentUuid; !parent.isNull(); parent = identities.value(parent)->parentUuid)
        if (moving.contains(identities.value(parent)->id)) return Result::InvalidInput;
    if (!aboveUuid.isNull()) {
        const auto above = identities.value(aboveUuid);
        if (!above) return Result::LayerUnavailable;
        if (above->parentUuid != parentUuid || moving.contains(above->id)) return Result::InvalidInput;
    }
    QList<Layer> ordered;
    QList<LayerId> pending;
    const auto roots = children.value(QUuid());
    for (auto it = roots.crbegin(); it != roots.crend(); ++it) pending.append(*it);
    while (!pending.isEmpty()) {
        const auto layer = values.value(pending.takeLast());
        if (moving.contains(layer->id)) ordered.append(*layer);
        const auto branches = children.value(layer->uuid);
        for (auto it = branches.crbegin(); it != branches.crend(); ++it) pending.append(*it);
    }
    QList<Layer> remaining;
    QList<LayerId> selectedBefore;
    for (const auto &layer : original->layers) {
        if (moving.contains(layer.id)) selectedBefore.append(layer.id);
        else remaining.append(layer);
    }
    qsizetype insertion = remaining.size();
    for (qsizetype i = 0; i < remaining.size(); ++i) {
        if (!aboveUuid.isNull() ? remaining[i].uuid == aboveUuid : remaining[i].parentUuid == parentUuid) {
            insertion = i + (aboveUuid.isNull() ? 0 : 1);
            break;
        }
        if (aboveUuid.isNull() && remaining[i].uuid == parentUuid) insertion = i + 1;
    }
    QList<LayerId> selectedAfter;
    for (auto &layer : ordered) {
        layer.parentUuid = parentUuid;
        selectedAfter.append(layer.id);
        remaining.insert(insertion++, layer);
    }
    QHash<QUuid, QList<LayerId>> resultChildren;
    for (const auto &layer : remaining) resultChildren[layer.parentUuid].append(layer.id);
    if (resultChildren == children) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*original));
    version->layers.swap(remaining);
    version->selectedLayers = selectedAfter;
    if (activeRoot) version->activeLayer = activeRoot;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    if (selectedBefore != original->selectedLayers) {
        auto before = QSharedPointer<DocumentVersion>(new DocumentVersion(*original));
        before->selectedLayers = selectedBefore;
        m_history[m_index] = before;
    }
    commitVersion(version);
    return Result::Success;
}

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
    if (version->layers[index].transparencyMask)
        version->layers[index].transparencyMask =
            version->layers[index].transparencyMask->translated(QPoint(), rect);
    version->layers[index].operations.append(CropSnapshot(operation));
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    ++m_nextOperation;
    LayerHistoryChange change;
    change.crop = operation;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::fillLayer(LayerId layer, const std::array<quint8, 3> &rgb,
                                    quint8 opacity, bool erase, bool alphaLocked,
                                    LayerBlendMode blendMode)
{
    const std::array<quint8, 4> color{rgb[0], rgb[1], rgb[2], 255};
    FigureStyle style;
    style.outline = false;
    style.fill = color;
    style.fillBlendMode = erase ? LayerBlendMode::Normal : blendMode;
    return fillLayerStyle(layer, color, style, opacity, erase, alphaLocked);
}

Result OperationDocument::fillLayerPattern(LayerId layer, const PatternFill &pattern,
                                           quint8 opacity, bool erase, bool alphaLocked,
                                           LayerBlendMode blendMode)
{
    if (!pattern.isValid()) return Result::InvalidInput;
    FigureStyle style;
    style.outline = false;
    style.pattern = pattern;
    style.fillBlendMode = erase ? LayerBlendMode::Normal : blendMode;
    return fillLayerStyle(layer, {0, 0, 0, 255}, style, opacity, erase, alphaLocked);
}

Result OperationDocument::fillLayerStyle(LayerId layer, const std::array<quint8, 4> &color,
                                         const FigureStyle &style, quint8 opacity,
                                         bool erase, bool alphaLocked)
{
    if (style.pattern && (style.pattern->byteCount() > m_limits.rasterBytes ||
                          snapshot()->rasterBytes > m_limits.rasterBytes - style.pattern->byteCount()))
        return Result::CapacityExceeded;
    Brush brush;
    brush.rgba = color;
    brush.opacity = opacity;
    brush.erase = erase;
    brush.alphaLocked = alphaLocked;
    brush.channelWriteMask = alphaLocked ? 7 : 15;
    const QPointF end(snapshot()->size.width(), snapshot()->size.height());
    const auto result = paintFigure(layer, brush, {{{0, 0}, 1}, {end, 1}},
                                    {ShapeKind::Rectangle, {0, 0}, end, 0}, style, 0);
    if (result == Result::Success) m_layerHistoryChanges[m_index].filledLayer = true;
    return result;
}

Result OperationDocument::invertColors(LayerId layer)
{ return applyFilter(layer, FilterKind::InvertColors); }

Result OperationDocument::grayscale(LayerId layer)
{ return applyFilter(layer, FilterKind::Grayscale); }

Result OperationDocument::hsvAdjustment(LayerId layer, const HsvAdjustment &settings)
{
    if (!settings.isValid()) return Result::InvalidInput;
    return applyFilter(layer, FilterKind::HsvAdjustment, &settings);
}

Result OperationDocument::applyFilter(LayerId layer, FilterKind kind, const HsvAdjustment *settings)
{
    if (kind == FilterKind::HsvAdjustment && !settings) return Result::InvalidInput;
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || snapshot()->layers[index].kind != LayerKind::Paint ||
        snapshot()->layers[index].userLocked) return Result::LayerUnavailable;
    if (m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto operation = QSharedPointer<FilterOperation>::create();
    operation->id = {m_identity, ++m_nextOperation};
    operation->version = 1;
    operation->layer = layer;
    operation->kind = kind;
    if (kind == FilterKind::HsvAdjustment) {
        // hsvAdjustment() validated the settings before admission. The luma
        // coefficients are copied from the frozen value rather than resolved
        // from a live color-space object while rendering tiles.
        operation->hue = settings->hue;
        operation->saturation = settings->saturation;
        operation->value = settings->value;
        operation->hsvType = settings->type;
        operation->compatibilityMode = settings->compatibilityMode;
        operation->colorize = settings->colorize;
        operation->luma = settings->luma;
    }
    operation->selectionRect = snapshot()->selection;
    operation->selectionInverted = snapshot()->selectionInverted;
    operation->selectionKind = snapshot()->selectionKind;
    operation->selectionMask = snapshot()->selectionMask;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].operations.append(FilterSnapshot(operation));
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.filteredLayer = true;
    change.filterKind = kind;
    commitVersion(version, change);
    return Result::Success;
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
        auto active = layerIndex(version->activeLayer);
        QHash<QUuid, qsizetype> indices;
        for (qsizetype i = 0; i < version->layers.size(); ++i) indices.insert(version->layers[i].uuid, i);
        while (!version->layers[active].parentUuid.isNull()) {
            const auto parent = indices.value(version->layers[active].parentUuid);
            if (!removed.contains(version->layers[parent].id)) break;
            active = parent;
        }
        const auto parentUuid = version->layers[active].parentUuid;
        version->activeLayer = 0;
        for (qsizetype i = active + 1; i < version->layers.size(); ++i) {
            if (!removed.contains(version->layers[i].id) && version->layers[i].parentUuid == parentUuid) {
                version->activeLayer = version->layers[i].id;
                break;
            }
        }
        if (!version->activeLayer) {
            for (qsizetype i = active - 1; i >= 0; --i) {
                if (!removed.contains(version->layers[i].id) && version->layers[i].parentUuid == parentUuid) {
                    version->activeLayer = version->layers[i].id;
                    break;
                }
            }
        }
        if (!version->activeLayer && !parentUuid.isNull())
            version->activeLayer = version->layers[indices.value(parentUuid)].id;
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
    m_externalOperation = {};
    m_layerHistoryChanges = {LayerHistoryChange{}};
    m_index = 0;
    m_closed = true;
}

}
