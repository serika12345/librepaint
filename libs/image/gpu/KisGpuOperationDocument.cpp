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
        return ((*paint)->figureStyle.pattern ? (*paint)->figureStyle.pattern->byteCount() : 0) +
            ((*paint)->figureStyle.gradient ? (*paint)->figureStyle.gradient->byteCount() : 0) +
            ((*paint)->brush.texture ? (*paint)->brush.texture->alpha.size() : 0) +
            (*paint)->brush.transforms.size() * qsizetype(sizeof(BrushTransform));
    const auto write = std::get_if<TileWriteSnapshot>(&value);
    qsizetype bytes = 0;
    if (write) for (const auto &tile : (*write)->tiles) bytes += tile->byteCount();
    return bytes;
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
    copy.initialProjection = source.initialProjection;
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
        } else if (const auto affine = std::get_if<AffineSnapshot>(&sourceOperation)) {
            auto operation = QSharedPointer<AffineTransformOperation>::create(**affine);
            operation->id = id; operation->layer = copy.id;
            if (operation->selection) {
                auto selection = QSharedPointer<TranslationOperation>::create(*operation->selection);
                selection->id = id; selection->layer = copy.id; operation->selection = selection;
            }
            copy.operations.append(AffineSnapshot(operation));
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

qsizetype Layer::inputSampleCount() const
{
    qsizetype count = initialProjection ? initialProjection->source->sampleCount : 0;
    for (const auto &operation : operations) count += operationSamples(operation);
    return count;
}

qsizetype Layer::retainedRasterBytes() const
{
    qsizetype bytes = initialProjection ? initialProjection->source->rasterBytes + initialProjection->retainedBytes : 0;
    for (const auto &operation : operations) bytes += operationRasterBytes(operation);
    if (filter) bytes += filter->byteCount();
    return bytes;
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
        for (const auto &layer : version->layers) {
            version->sampleCount += layer.inputSampleCount();
            version->rasterBytes += layer.retainedRasterBytes();
        }
    }
    auto result = std::make_unique<OperationDocument>(RasterImage{source->size, {}, 0}, m_limits);
    version->m_document = result->m_identity;
    if (!preserveLayerUuids) version->rootUuid = QUuid::createUuid();
    QHash<QUuid, QUuid> identities;
    if (!preserveLayerUuids)
        for (const auto &layer : version->layers) identities.insert(layer.uuid, QUuid::createUuid());
    for (auto &layer : version->layers) {
        if (!preserveLayerUuids) {
            layer.uuid = identities.value(layer.uuid);
            if (layer.transparencyMask) layer.transparencyMaskUuid = QUuid::createUuidV5(layer.uuid, QByteArrayLiteral("transparency-mask"));
            layer.parentUuid = identities.value(layer.parentUuid);
            if (layer.kind == LayerKind::Clone) layer.cloneSourceUuid = identities.value(layer.cloneSourceUuid, layer.cloneSourceUuid);
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
            } else if (const auto affine = std::get_if<AffineSnapshot>(&operation)) {
                auto copied = QSharedPointer<AffineTransformOperation>::create(**affine);
                copied->id.document = result->m_identity;
                if (copied->selection) {
                    auto selection = QSharedPointer<TranslationOperation>::create(*copied->selection);
                    selection->id.document = result->m_identity; copied->selection = selection;
                }
                operation = AffineSnapshot(copied);
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
    if (m_layerHistoryChanges[index].layerStructureAction != HistoryAction::None)
        return m_layerHistoryChanges[index].layerStructureAction;
    if (m_layerHistoryChanges[index].transformedLayers) return HistoryAction::TransformLayer;
    if (m_layerHistoryChanges[index].movedSelectionPixels) return HistoryAction::MoveSelectionPixels;
    if (m_layerHistoryChanges[index].translatedLayers) return HistoryAction::TranslateLayer;
    if (m_layerHistoryChanges[index].convertedColorSpace) return HistoryAction::ConvertColorSpace;
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
        case FilterKind::MaximizeChannel: return HistoryAction::MaximizeChannel;
        case FilterKind::MinimizeChannel: return HistoryAction::MinimizeChannel;
        case FilterKind::ResetTransparent: return HistoryAction::ResetTransparent;
        case FilterKind::ColorOverlay: return HistoryAction::ColorOverlay;
        case FilterKind::Sharpen: return HistoryAction::Sharpen;
        case FilterKind::MeanRemoval: return HistoryAction::MeanRemoval;
        case FilterKind::EmbossLaplacian: return HistoryAction::EmbossLaplacian;
        case FilterKind::EmbossAllDirections: return HistoryAction::EmbossAllDirections;
        case FilterKind::EmbossHorizontalVertical: return HistoryAction::EmbossHorizontalVertical;
        case FilterKind::EmbossVertical: return HistoryAction::EmbossVertical;
        case FilterKind::EmbossHorizontal: return HistoryAction::EmbossHorizontal;
        case FilterKind::InvertColors: break;
        }
        return HistoryAction::InvertColors;
    }
    if (m_layerHistoryChanges[index].changedSelection && !m_layerHistoryChanges[index].canvasResize &&
        m_history[index - 1]->layers.size() == m_history[index]->layers.size())
        return HistoryAction::ChangeSelection;
    if (m_layerHistoryChanges[index].crop) return HistoryAction::CropLayer;
    if (const auto &resize = m_layerHistoryChanges[index].canvasResize)
        return resize->cropLayers ? HistoryAction::CropImage : HistoryAction::ResizeCanvas;
    switch (m_layerHistoryChanges[index].property) {
    case LayerProperty::Name:
    case LayerProperty::MaskName: return HistoryAction::RenameLayer;
    case LayerProperty::Opacity: return HistoryAction::ChangeOpacity;
    case LayerProperty::Visible: return HistoryAction::ChangeVisibility;
    case LayerProperty::Locked:
    case LayerProperty::MaskLocked: return HistoryAction::ChangeLock;
    case LayerProperty::PinnedToTimeline:
    case LayerProperty::MaskPinnedToTimeline: return HistoryAction::ChangeTimelinePin;
    case LayerProperty::ChannelMask: return HistoryAction::ChangeChannelLock;
    case LayerProperty::ProjectionMask: return HistoryAction::ChangeProjectionChannels;
    case LayerProperty::Position:
    case LayerProperty::MaskPosition: return HistoryAction::TranslateLayer;
    case LayerProperty::None: break;
    case LayerProperty::ColorLabel:
    case LayerProperty::MaskColorLabel: break;
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
        if (before[layer].transparencyMask != after[layer].transparencyMask ||
            before[layer].filterSelection != after[layer].filterSelection) return HistoryAction::PaintStroke;
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
bool OperationDocument::hasActiveStroke() const { return m_edit || m_translation || !m_affine.isEmpty() || !m_externalOperation.isNull() || m_filterNodeEdit.has_value() || m_cloneSourceEdit.has_value(); }
OperationId OperationDocument::activeOperation() const
{
    if (m_cloneSourceEdit) return m_cloneSourceEdit->id;
    if (m_filterNodeEdit) return m_filterNodeEdit->id;
    if (m_edit) return m_edit->id;
    if (m_translation) return m_translation->id;
    if (!m_affine.isEmpty()) return m_affine.first()->id;
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
    m_preview.clear();
    return Result::Success;
}

Result OperationDocument::updateExternalMask(const OperationId &id, LayerId layer, const SelectionMaskSP &mask, const QUuid &maskUuid)
{
    if (m_closed) return Result::Closed;
    if (m_externalOperation.isNull() || m_externalOperation != id) return Result::StaleOperation;
    const auto index = layerIndex(layer);
    if (index < 0 || !editableMaskCoverage(snapshot()->layers[index], maskUuid)) return Result::LayerUnavailable;
    if (!mask) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max() || !selectionFitsRetainedInputs(mask))
        return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    auto &target = version->layers[index];
    if (isFilterNode(target.kind) && (maskUuid.isNull() || maskUuid == target.uuid)) target.filterSelection = mask;
    else target.transparencyMask = mask;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    version->m_committed = false;
    version->m_pendingOperation = id;
    m_preview = version;
    return Result::Success;
}

Result OperationDocument::commitExternalMask(const OperationId &id, LayerId layer, const SelectionMaskSP &mask, const QUuid &maskUuid)
{
    if (m_closed) return Result::Closed;
    if (m_externalOperation.isNull() || m_externalOperation != id) return Result::StaleOperation;
    const auto preview = m_preview;
    m_externalOperation = {};
    m_preview.clear();
    const auto result = replaceTransparencyMask(layer, mask, false, false, maskUuid);
    if (result != Result::Success) { m_externalOperation = id; m_preview = preview; }
    return result;
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
    if (!selectionFitsRetainedInputs(mask)) return Result::CapacityExceeded;
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

Result OperationDocument::combineSelectionMask(const SelectionMaskSP &mask, SelectionCombination action) try
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!mask || action < SelectionCombination::Replace || action > SelectionCombination::SymmetricDifference)
        return Result::InvalidInput;
    const auto before = snapshot();
    // A retained ordinary selection can include off-canvas KRA coverage.
    // Inverted selections complement only the finite document canvas.
    const bool inverted = before->selectionInverted ||
        (before->selection.isEmpty() && action == SelectionCombination::Subtract);
    const QRect canvas(QPoint(), before->size);
    const QRect clip = inverted ? canvas : canvas.united(before->selection).united(mask->bounds());
    SelectionMaskSP previous = SelectionMask::empty();
    if (!before->selection.isEmpty()) {
        if (before->selectionKind == SelectionKind::Freehand) previous = before->selectionMask;
        else if (before->selectionKind == SelectionKind::Ellipse)
            previous = SelectionMask::fromEllipse(before->selection, clip);
        else previous = SelectionMask::fromRectangle(before->selection, clip);
        if (!previous && before->selection.intersects(clip)) return Result::CapacityExceeded;
        if (!previous) previous = SelectionMask::empty();
    }
    const auto result = action == SelectionCombination::Replace && (mask->isEmpty() || clip.contains(mask->bounds()))
        ? mask : previous->combined(*mask, action, clip, inverted);
    if (!result) return Result::CapacityExceeded;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    if (!selectionFitsRetainedInputs(result)) return Result::CapacityExceeded;
    const auto coverage = !inverted && *result == *previous ? previous : result;
    // Native selection tools retain their accepted transaction even when the
    // resulting coverage is unchanged or an intersection leaves no selection.
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    version->selection = coverage->bounds();
    version->selectionInverted = false;
    version->selectionKind = coverage->isEmpty() ? SelectionKind::Rectangle : SelectionKind::Freehand;
    version->selectionMask = coverage->isEmpty() ? SelectionMaskSP() : coverage;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.changedSelection = true;
    commitVersion(version, change);
    return Result::Success;
}
catch (const std::bad_alloc &) { return Result::CapacityExceeded; }

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

Result OperationDocument::writeProjectionBackground(const RgbaPixel &rgba)
{
    if (m_closed) return Result::Closed;
    if (!validPixel(rgba, snapshot()->format)) return Result::InvalidInput;
    if (snapshot()->projectionBackground == rgba) return Result::Success;
    const auto count = quint64(m_history.size()) + quint64(bool(m_preview));
    if (count > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto nextRevision = m_nextRevision;
    for (auto &state : history) {
        if (state->format != snapshot()->format || state->colorProfile != snapshot()->colorProfile) continue;
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
    const bool maskProperty = property == LayerProperty::MaskVisible || property == LayerProperty::MaskLocked ||
        property == LayerProperty::MaskName || property == LayerProperty::MaskColorLabel ||
        property == LayerProperty::MaskPinnedToTimeline;
    const auto maskIdentity = snapshot()->layers[index].transparencyMaskUuid;
    const bool components = property == LayerProperty::ChannelMask || property == LayerProperty::ProjectionMask;
    if constexpr (std::is_same_v<T, quint8>) {
        if (components ? (previous & componentMask) == (value & componentMask) : previous == value) return Result::Success;
    } else if (previous == value) return Result::Success;
    const auto containsLayer = [&](const Snapshot &state) {
        for (const auto &candidate : state->layers)
            if (candidate.id == layer && (!maskProperty ||
                (candidate.transparencyMask && candidate.transparencyMaskUuid == maskIdentity))) return true;
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

Result OperationDocument::writeLayerDefaultPixel(LayerId layer, const RgbaPixel &rgba)
{ return applyLayerDefaultPixel(layer, rgba, nullptr); }

Result OperationDocument::applyLayerDefaultPixel(LayerId layer, const RgbaPixel &rgba, Snapshot *retainedVersion) try
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto source = retainedVersion ? *retainedVersion : snapshot();
    if (!source || source->documentId() != m_identity) return Result::StaleVersion;
    if (!validPixel(rgba, source->format)) return Result::InvalidInput;
    const Layer *selected = nullptr;
    for (const auto &value : source->layers) if (value.id == layer) selected = &value;
    if (!selected || selected->kind != LayerKind::Paint) return Result::LayerUnavailable;
    if (selected->defaultPixel == rgba) return Result::Success;
    if (quint64(m_history.size()) + 1 > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto retained = source;
    auto nextRevision = m_nextRevision;
    const auto update = [&](const Snapshot &before) -> Snapshot {
        if (before->format != source->format || before->colorProfile != source->colorProfile) return before;
        for (qsizetype i = 0; i < before->layers.size(); ++i) {
            const auto &candidate = before->layers[i];
            if (candidate.id != layer || candidate.uuid != selected->uuid || candidate.defaultPixel != selected->defaultPixel) continue;
            auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
            version->layers[i].defaultPixel = rgba; version->m_revision = ++nextRevision;
            return version;
        }
        return before;
    };
    for (auto &version : history) {
        const auto before = version; version = update(before);
        if (before == source) retained = version;
    }
    if (retained == source) retained = update(source);
    m_history.swap(history); m_nextRevision = nextRevision;
    if (retainedVersion) *retainedVersion = retained;
    return Result::Success;
}
catch (const std::bad_alloc &) { return Result::CapacityExceeded; }

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
    const auto &filter = forward ? change.afterFilter : change.beforeFilter;
    if (filter) return writeFilterDefinition(change.layer, *filter);
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
    case LayerProperty::MaskPosition:
        return writeLayerProperty(change.layer, LayerProperty::MaskPosition, &Layer::transparencyMaskPosition, value.toPoint());
    case LayerProperty::None: return Result::Success;
    case LayerProperty::Name: return writeLayerName(change.layer, value.toString());
    case LayerProperty::Opacity: return writeLayerOpacity(change.layer, quint8(value.toUInt()));
    case LayerProperty::Visible: return writeLayerVisible(change.layer, value.toBool());
    case LayerProperty::Locked: return writeLayerLocked(change.layer, value.toBool());
    case LayerProperty::PinnedToTimeline: return writeLayerPinnedToTimeline(change.layer, value.toBool());
    case LayerProperty::ColorLabel: return writeLayerColorLabel(change.layer, value.toInt());
    case LayerProperty::PassThrough: return writeLayerPassThrough(change.layer, value.toBool());
    case LayerProperty::BlendMode: return writeLayerBlendMode(change.layer, LayerBlendMode(value.toUInt()));
    case LayerProperty::MaskLocked: return writeTransparencyMaskLocked(change.layer, value.toBool());
    case LayerProperty::MaskVisible: return writeTransparencyMaskVisible(change.layer, value.toBool());
    case LayerProperty::MaskName: return writeTransparencyMaskName(change.layer, value.toString());
    case LayerProperty::MaskColorLabel: return writeTransparencyMaskColorLabel(change.layer, value.toInt());
    case LayerProperty::MaskPinnedToTimeline: return writeTransparencyMaskPinnedToTimeline(change.layer, value.toBool());
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
    const qint64 resourceBytes = (brush.texture ? brush.texture->alpha.size() : 0) +
        qint64(brush.transforms.size()) * sizeof(BrushTransform);
    if (checkMutation() != Result::Success || index < 0 || snapshot()->layers[index].kind != LayerKind::Paint ||
        !snapshot()->layers[index].visible || snapshot()->layers[index].userLocked ||
        !validBrush(brush, snapshot()->format) ||
        brush.transforms.size() > m_limits.samplesPerStroke ||
        resourceBytes > m_limits.rasterBytes || snapshot()->rasterBytes > m_limits.rasterBytes - resourceBytes ||
        m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return {};
    }
    QSharedPointer<PaintOperation> operation(new PaintOperation);
    operation->id = {m_identity, ++m_nextOperation};
    operation->layer = layer;
    operation->brush = brush;
    if (brush.additiveMaskingBrush)
        operation->brush.additiveMaskingBrush = QSharedPointer<const Brush>::create(*brush.additiveMaskingBrush);
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
{ return appendSamplesWithin(id, samples, nullptr); }

Result OperationDocument::appendPoint(OperationId id, const InputSample &point)
{ return appendPainterPrimitives(id, {point}, {PathCommand::Point}); }

Result OperationDocument::appendLine(OperationId id, const InputSample &first, const InputSample &last)
{
    return appendPainterPrimitives(id, {first, last}, {PathCommand::MoveTo, PathCommand::LineTo});
}

Result OperationDocument::appendPainterPrimitives(OperationId id, const QList<InputSample> &samples,
                                               const QList<PathCommand> &commands)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    if (samples.size() != commands.size()) return Result::InvalidInput;
    for (qsizetype i = 0; i < commands.size(); ++i) {
        if (commands[i] == PathCommand::Point) continue;
        if (commands[i] != PathCommand::MoveTo || i + 1 == commands.size() ||
            commands[i + 1] != PathCommand::LineTo || samples[i + 1].timestamp < samples[i].timestamp)
            return Result::InvalidInput;
        ++i;
    }
    if (samples.size() > m_limits.samplesPerStroke - m_edit->sampleCount) return Result::CapacityExceeded;
    if (m_edit->brush.basicSmoothing || m_edit->brush.pixelPerfect || hasFigureFill(m_edit->figureStyle) ||
        (m_edit->sampleCount && (m_edit->shape.kind != ShapeKind::Path || !m_edit->shape.pathCalls.isEmpty())))
        return Result::InvalidInput;
    Shape shape = m_edit->sampleCount ? m_edit->shape : Shape{};
    shape.kind = ShapeKind::Path;
    shape.pathCommands.append(commands);
    return appendSamplesWithin(id, samples, &shape);
}

Result OperationDocument::appendSamplesWithin(OperationId id, const QList<InputSample> &samples, const Shape *painterShape)
{
    const auto status = checkOperation(id);
    if (status != Result::Success) return status;
    quint64 time = m_edit->sampleBlocks.empty() ? 0 : m_edit->sampleBlocks.back()->back().timestamp;
    for (const auto &sample : samples) {
        if (!validSample(sample) || (!painterShape && sample.timestamp < time)) return Result::InvalidInput;
        for (const auto &hand : m_edit->brush.transforms) {
            auto mapped = sample;
            mapped.position = hand.mapPoint(sample.position);
            if (!validSample(mapped)) return Result::InvalidInput;
        }
        time = sample.timestamp;
    }
    const qsizetype count = m_edit->sampleCount;
    const qsizetype handCount = std::max(qsizetype(1), m_edit->brush.transforms.size());
    if (samples.size() > m_limits.samplesPerStroke / handCount - count ||
        count > m_limits.samplesPerDocument - snapshot()->sampleCount ||
        samples.size() > m_limits.samplesPerDocument - snapshot()->sampleCount - count ||
        m_nextRevision == std::numeric_limits<quint64>::max()) {
        return Result::CapacityExceeded;
    }
    if (painterShape && (!validShape(*painterShape) || !validShapeSampleCount(*painterShape, count + samples.size())))
        return Result::InvalidInput;
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
    if (painterShape) operation->shape = *painterShape;
    ++operation->version;
    replacePreview(operation);
    return Result::Success;
}

Result OperationDocument::paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                                      const Shape &shape, const FigureStyle &style, quint64 randomSeed)
{
    return paintFigureWithin(layer, brush, samples, shape, style, randomSeed, {});
}

Result OperationDocument::paintFigureWithin(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                                            const Shape &shape, const FigureStyle &style, quint64 randomSeed,
                                            const std::optional<QRect> &compositionBounds, const RegionFillSP &region)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!validBrush(brush, snapshot()->format) || !validShape(shape) || !validFigureStyle(shape, style, snapshot()->format) ||
        (style.fill && !validPixel(*style.fill, snapshot()->format)) ||
        (brush.basicSmoothing && shape.kind != ShapeKind::Freehand)) return Result::InvalidInput;
    if (samples.isEmpty() && shape.kind != ShapeKind::Path) return Result::EmptyOperation;
    if (!validShapeSampleCount(shape, samples.size())) return Result::InvalidInput;
    quint64 time = 0;
    for (const auto &sample : samples) {
        if (!validSample(sample) || sample.timestamp < time) return Result::InvalidInput;
        time = sample.timestamp;
    }
    const qsizetype resourceBytes = (style.pattern ? style.pattern->byteCount() : 0) +
        (style.gradient ? style.gradient->byteCount() : 0) +
        (brush.texture ? brush.texture->alpha.size() : 0) + brush.transforms.size() * qsizetype(sizeof(BrushTransform));
    if (samples.size() > m_limits.samplesPerStroke / std::max(qsizetype(1), brush.transforms.size()) ||
        samples.size() > m_limits.samplesPerDocument - snapshot()->sampleCount ||
        m_nextOperation == std::numeric_limits<quint64>::max() ||
        m_nextRevision > std::numeric_limits<quint64>::max() - (samples.isEmpty() ? 1 : 2) ||
        resourceBytes > m_limits.rasterBytes || snapshot()->rasterBytes > m_limits.rasterBytes - resourceBytes) {
        return Result::CapacityExceeded;
    }
    const auto id = beginStroke(layer, brush, randomSeed);
    if (id.isNull()) return Result::LayerUnavailable;
    if (compositionBounds) {
        auto operation = QSharedPointer<PaintOperation>::create(*m_edit);
        operation->compositionBounds = compositionBounds;
        operation->region = region;
        m_edit = operation;
    }
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
    if (!validShape(shape) || !validFigureStyle(shape, style, snapshot()->format) ||
        (style.fill && !validPixel(*style.fill, snapshot()->format)) ||
        (m_edit->brush.basicSmoothing && shape.kind != ShapeKind::Freehand)) return Result::InvalidInput;
    if (m_edit->sampleCount == 0 && shape.kind != ShapeKind::Path) return Result::EmptyOperation;
    if (!validShapeSampleCount(shape, m_edit->sampleCount)) return Result::InvalidInput;
    const qsizetype resourceBytes = (style.pattern ? style.pattern->byteCount() : 0) +
        (style.gradient ? style.gradient->byteCount() : 0) +
        (m_edit->brush.texture ? m_edit->brush.texture->alpha.size() : 0) +
        m_edit->brush.transforms.size() * qsizetype(sizeof(BrushTransform));
    if (resourceBytes > m_limits.rasterBytes || snapshot()->rasterBytes > m_limits.rasterBytes - resourceBytes)
        return Result::CapacityExceeded;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    QSharedPointer<PaintOperation> operation(new PaintOperation(*m_edit));
    operation->shape = shape;
    operation->figureStyle = style;
    operation->selectionRect = style.ignoreSelection ? QRect() : snapshot()->selection;
    operation->selectionKind = style.ignoreSelection ? SelectionKind::Rectangle : snapshot()->selectionKind;
    operation->selectionInverted = !style.ignoreSelection && snapshot()->selectionInverted;
    operation->selectionMask = style.ignoreSelection ? SelectionMaskSP() : snapshot()->selectionMask;
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
    const bool completionAddsDab = [&] {
        if (m_edit->shape.kind != ShapeKind::Freehand || !m_edit->sampleCount) return false;
        if (m_edit->brush.pixelPerfect) return true;
        const auto position = m_edit->sampleBlocks.first()->first().position;
        for (const auto &block : m_edit->sampleBlocks)
            for (const auto &point : *block)
                if (point.position.x() != position.x() || point.position.y() != position.y()) return false;
        return true;
    }();
    // Completing stationary input adds a tap; pixel correction can change final dabs.
    // Pending pixel caches retain the preceding operation version.
    if (m_edit->brush.basicSmoothing || completionAddsDab) {
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
                                       bool clearPixelSelection, qsizetype effectIndex)
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
        if (!input.source || input.source->format != before->format || !validPixel(properties.defaultPixel, before->format) || !validLayerName(properties.name) || properties.uuid.isNull() ||
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
        if (found->initialProjection) {
            const auto count = found->initialProjection->source->sampleCount;
            const auto bytes = found->initialProjection->source->rasterBytes + found->initialProjection->retainedBytes;
            if (count > m_limits.samplesPerDocument - samples || bytes > m_limits.rasterBytes - pixels)
                return Result::CapacityExceeded;
            samples += count; pixels += bytes;
        }
        for (const auto &operation : found->operations) {
            if (operationSamples(operation) > m_limits.samplesPerStroke ||
                operationSamples(operation) > m_limits.samplesPerDocument - samples ||
                operationRasterBytes(operation) > m_limits.rasterBytes - pixels ||
                nextOperation == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
            samples += operationSamples(operation);
            pixels += operationRasterBytes(operation);
            ++nextOperation;
        }
        if (found->filterSelection && !selectionFitsRetainedInputs(found->filterSelection)) return Result::CapacityExceeded;
        if (found->filter) {
            if (!filterFitsRetainedHistory(*found->filter) || found->filter->byteCount() > m_limits.rasterBytes - pixels)
                return Result::CapacityExceeded;
            pixels += found->filter->byteCount();
        }
        Layer copy = *found;
        copy.id = ++nextLayer;
        copy.name = properties.name;
        copy.uuid = properties.uuid;
        copy.transparencyMaskIndex = properties.transparencyMaskIndex;
        if (!properties.transparencyMaskUuid.isNull()) {
            if (!properties.transparencyMask || !selectionFitsRetainedInputs(properties.transparencyMask)) return Result::InvalidInput;
            copy.transparencyMask = properties.transparencyMask;
            copy.transparencyMaskPosition = properties.transparencyMaskPosition;
            copy.transparencyMaskUuid = properties.transparencyMaskUuid;
            copy.transparencyMaskName = properties.transparencyMaskName;
            copy.transparencyMaskVisible = properties.transparencyMaskVisible;
            copy.transparencyMaskLocked = properties.transparencyMaskLocked;
            copy.transparencyMaskColorLabelIndex = properties.transparencyMaskColorLabelIndex;
            copy.transparencyMaskPinnedToTimeline = properties.transparencyMaskPinnedToTimeline;
        } else if (copy.transparencyMask) copy.transparencyMaskUuid = QUuid::createUuidV5(copy.uuid, QByteArrayLiteral("transparency-mask"));
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
    QHash<QUuid, qsizetype> insertedMasks;
    for (const auto &node : added)
        if (node.kind == LayerKind::FilterMask && !inserted.contains(node.parentUuid)) ++insertedMasks[node.parentUuid];
    if (effectIndex < -1 || (effectIndex >= 0 && (insertedMasks.size() != 1 || insertedMasks.cbegin().value() != added.size())))
        return Result::InvalidInput;
    for (auto &node : version->layers) {
        if (!node.transparencyMask || !insertedMasks.contains(node.uuid)) continue;
        qsizetype below = 0, total = 0;
        for (qsizetype i = 0; i < before->layers.size(); ++i)
            if (before->layers[i].parentUuid == node.uuid && before->layers[i].kind == LayerKind::FilterMask) {
                ++total;
                if (i < index) ++below;
            }
        if (effectIndex > total + 1) return Result::InvalidInput;
        // The explicit sibling position distinguishes either side of the native
        // transparency child, which has no entry in the flat layer list.
        if ((effectIndex >= 0 ? effectIndex : below) <= node.transparencyMaskIndex)
            node.transparencyMaskIndex += insertedMasks.value(node.uuid);
    }
    if (!Detail::validLayerHierarchy(version->layers, before->format)) return Result::InvalidInput;
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
        if (original->layers[index].filter) {
            const auto &filter = *original->layers[index].filter;
            if (!filterFitsRetainedHistory(filter) || filter.byteCount() > m_limits.rasterBytes - pixels)
                return Result::CapacityExceeded;
            pixels += filter.byteCount();
        }
        if (const auto &initial = original->layers[index].initialProjection) {
            const auto bytes = initial->source->rasterBytes + initial->retainedBytes;
            if (initial->source->sampleCount > m_limits.samplesPerDocument - samples || bytes > m_limits.rasterBytes - pixels)
                return Result::CapacityExceeded;
            samples += initial->source->sampleCount; pixels += bytes;
        }
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
        if (copy.transparencyMask) copy.transparencyMaskUuid = QUuid::createUuidV5(copy.uuid, QByteArrayLiteral("transparency-mask"));
        copiedIdentities.insert(source.uuid, copy.uuid);
        if (copyNames.contains(source.id)) copy.name = copyNames.value(source.id);
        copyLayerOperations(source, copy, m_identity, nextOperation);
        if (source.id == active) version->activeLayer = copy.id;
        if (copyNames.contains(source.id)) version->selectedLayers.append(copy.id);
        copies.append(std::move(copy));
    }
    for (auto &copy : copies) {
        copy.parentUuid = copiedIdentities.value(copy.parentUuid, destinationParent);
        if (copy.kind == LayerKind::Clone) copy.cloneSourceUuid = copiedIdentities.value(copy.cloneSourceUuid, copy.cloneSourceUuid);
        version->layers.insert(insertion++, copy);
    }
    if (!destinationParent.isNull()) {
        const auto ownerIndex = indices.value(destinationParent, -1);
        if (ownerIndex >= 0 && original->layers[ownerIndex].transparencyMask) {
            qsizetype below = 0, added = 0;
            for (qsizetype i = 0; i <= anchor; ++i)
                if (original->layers[i].parentUuid == destinationParent && original->layers[i].kind == LayerKind::FilterMask) ++below;
            for (const auto &copy : copies)
                if (copy.parentUuid == destinationParent && copy.kind == LayerKind::FilterMask) ++added;
            if (below <= original->layers[ownerIndex].transparencyMaskIndex)
                for (auto &owner : version->layers) if (owner.uuid == destinationParent) owner.transparencyMaskIndex += added;
        }
    }
    if (!Detail::validLayerHierarchy(version->layers, original->format)) return Result::InvalidInput;
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
    if (!validBlendMode(mode, snapshot()->format)) return Result::InvalidInput;
    return writeLayerProperty(layer, LayerProperty::BlendMode, &Layer::blendMode, mode);
}

Result OperationDocument::setLayerBlendMode(LayerId layer, LayerBlendMode mode, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!validBlendMode(mode, snapshot()->format) ||
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
{ return addTransparencyMaskWithin(layer, true); }

Result OperationDocument::addTransparencyMask(LayerId layer)
{ return addTransparencyMaskWithin(layer, false); }

Result OperationDocument::addTransparencyMask(LayerId layer, const QString &name)
{ return addTransparencyMaskWithin(layer, false, {}, name); }

Result OperationDocument::insertTransparencyMask(LayerId layer, const RasterLayer &source)
{
    if (!source.transparencyMask || source.transparencyMaskUuid.isNull()) return Result::InvalidInput;
    for (const auto &version : m_history)
        for (const auto &value : version->layers)
            if (value.uuid == source.transparencyMaskUuid ||
                (value.transparencyMask && value.transparencyMaskUuid == source.transparencyMaskUuid)) return Result::InvalidInput;
    return addTransparencyMaskWithin(layer, true, source.transparencyMask, source.transparencyMaskName, &source);
}

Result OperationDocument::addTransparencyMaskWithin(LayerId layer, bool requireSelection, const SelectionMaskSP &prepared,
                                                   const QString &name, const RasterLayer *imported)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!validLayerName(name)) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0) return Result::LayerUnavailable;
    if (snapshot()->layers[index].userLocked) return Result::LayerUnavailable;
    if (snapshot()->layers[index].transparencyMask) return Result::InvalidInput;
    auto mask = prepared ? prepared : selectionCoverage(*snapshot());
    if (!mask && !requireSelection && snapshot()->selection.isEmpty()) mask = SelectionMask::uniform(255);
    if (!mask || mask->byteCount() > SelectionMask::MaximumBytes) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    if (!selectionFitsRetainedInputs(mask)) return Result::CapacityExceeded;
    auto sequence = m_nextOperation;
    QUuid identity;
    const auto occupied = [&](const QUuid &candidate) {
        for (const auto &retained : m_history)
            for (const auto &value : retained->layers)
                if (value.uuid == candidate || (value.transparencyMask && value.transparencyMaskUuid == candidate)) return true;
        return false;
    };
    if (imported) identity = imported->transparencyMaskUuid;
    while (identity.isNull() || occupied(identity)) {
        if (sequence == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
        const QByteArray name = QByteArrayLiteral("LibrePaint transparency mask:") + QByteArray::number(++sequence);
        identity = QUuid::createUuidV5(snapshot()->layers[index].uuid, name);
    }
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMask = mask;
    version->layers[index].transparencyMaskPosition = imported ? imported->transparencyMaskPosition : QPoint{};
    version->layers[index].transparencyMaskVisible = imported ? imported->transparencyMaskVisible : true;
    version->layers[index].transparencyMaskLocked = imported ? imported->transparencyMaskLocked : false;
    version->layers[index].transparencyMaskUuid = identity;
    version->layers[index].transparencyMaskName = name;
    version->layers[index].transparencyMaskColorLabelIndex = imported ? imported->transparencyMaskColorLabelIndex : 0;
    version->layers[index].transparencyMaskPinnedToTimeline = imported && imported->transparencyMaskPinnedToTimeline;
    qsizetype filterCount = 0;
    for (const auto &child : version->layers)
        if (child.kind == LayerKind::FilterMask && child.parentUuid == version->layers[index].uuid) ++filterCount;
    if (imported && (imported->transparencyMaskIndex < 0 || imported->transparencyMaskIndex > filterCount)) return Result::InvalidInput;
    version->layers[index].transparencyMaskIndex = imported ? imported->transparencyMaskIndex : filterCount;
    if (!requireSelection) {
        version->selection = {};
        version->selectionKind = SelectionKind::Rectangle;
        version->selectionInverted = false;
        version->selectionMask.clear();
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layer = layer;
    change.addedTransparencyMask = true;
    commitVersion(version, change);
    m_nextOperation = sequence;
    return Result::Success;
}

Result OperationDocument::setTransparencyMaskVisible(LayerId layer, bool visible, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (snapshot()->layers[index].transparencyMaskVisible == visible) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMaskVisible = visible;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::MaskVisible, layer,
        QVariant::fromValue(snapshot()->layers[index].transparencyMaskVisible), QVariant::fromValue(visible)}, history);
    return Result::Success;
}

Result OperationDocument::writeTransparencyMaskVisible(LayerId layer, bool visible)
{
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    return writeLayerProperty(layer, LayerProperty::MaskVisible, &Layer::transparencyMaskVisible, visible);
}

Result OperationDocument::writeTransparencyMaskLocked(LayerId layer, bool locked)
{
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    return writeLayerProperty(layer, LayerProperty::MaskLocked, &Layer::transparencyMaskLocked, locked);
}

Result OperationDocument::setTransparencyMaskLocked(LayerId layer, bool locked, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (snapshot()->layers[index].transparencyMaskLocked == locked) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMaskLocked = locked;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::MaskLocked, layer,
        QVariant::fromValue(snapshot()->layers[index].transparencyMaskLocked), QVariant::fromValue(locked)}, history);
    return Result::Success;
}

Result OperationDocument::renameTransparencyMask(LayerId layer, const QString &name, PropertyHistory history)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (history != PropertyHistory::Separate && history != PropertyHistory::MergeConsecutive) return Result::InvalidInput;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (!validLayerName(name)) return Result::InvalidInput;
    const auto previous = snapshot()->layers[index].transparencyMaskName;
    if (previous == name) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    version->layers[index].transparencyMaskName = name;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    commitPropertyVersion(version, {LayerProperty::MaskName, layer, QVariant::fromValue(previous), QVariant::fromValue(name)}, history);
    return Result::Success;
}

Result OperationDocument::writeTransparencyMaskName(LayerId layer, const QString &name)
{
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (!validLayerName(name)) return Result::InvalidInput;
    return writeLayerProperty(layer, LayerProperty::MaskName, &Layer::transparencyMaskName, name);
}

Result OperationDocument::writeTransparencyMaskColorLabel(LayerId layer, int index)
{
    if (m_closed) return Result::Closed;
    const auto position = layerIndex(layer);
    if (position < 0 || !snapshot()->layers[position].transparencyMask) return Result::LayerUnavailable;
    return writeLayerProperty(layer, LayerProperty::MaskColorLabel, &Layer::transparencyMaskColorLabelIndex, index);
}

Result OperationDocument::writeTransparencyMaskPinnedToTimeline(LayerId layer, bool pinned)
{
    if (m_closed) return Result::Closed;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    return writeLayerProperty(layer, LayerProperty::MaskPinnedToTimeline, &Layer::transparencyMaskPinnedToTimeline, pinned);
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
    version->layers[index].transparencyMaskLocked = false;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.layer = layer;
    change.removedTransparencyMask = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::writeTransparencyMask(LayerId layer, const SelectionMaskSP &mask)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || !snapshot()->layers[index].transparencyMask) return Result::LayerUnavailable;
    if (!mask || mask->byteCount() > SelectionMask::MaximumBytes) return Result::InvalidInput;
    if (snapshot()->layers[index].transparencyMask == mask) return Result::Success;
    if (!selectionFitsRetainedInputs(mask)) return Result::CapacityExceeded;
    const auto identity = snapshot()->layers[index].transparencyMaskUuid;
    QList<int> affected;
    for (int i = 0; i < m_history.size(); ++i)
        for (const auto &value : m_history[i]->layers)
            if (value.id == layer && value.transparencyMask && value.transparencyMaskUuid == identity) { affected.append(i); break; }
    if (quint64(affected.size()) > std::numeric_limits<quint64>::max() - m_nextRevision) return Result::CapacityExceeded;
    auto history = m_history;
    auto revision = m_nextRevision;
    for (const auto i : affected) {
        auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*history[i]));
        for (auto &value : version->layers) if (value.id == layer) { value.transparencyMask = mask; break; }
        version->m_revision = ++revision;
        history[i] = version;
    }
    m_history.swap(history);
    m_nextRevision = revision;
    return Result::Success;
}

Result OperationDocument::replaceTransparencyMask(LayerId layer, const SelectionMaskSP &mask, bool filled, bool cleared, const QUuid &maskUuid)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto index = layerIndex(layer);
    if (index < 0 || !editableMaskCoverage(snapshot()->layers[index], maskUuid)) return Result::LayerUnavailable;
    if (!mask || (!mask->defaultCoverage() && !mask->isEmpty() && mask->bounds().isEmpty()) ||
        mask->byteCount() > SelectionMask::MaximumBytes) return Result::InvalidInput;
    if (editableMaskCoverage(snapshot()->layers[index], maskUuid) == mask) return Result::Success;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    if (!selectionFitsRetainedInputs(mask)) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*snapshot()));
    auto &target = version->layers[index];
    if (isFilterNode(target.kind) && (maskUuid.isNull() || maskUuid == target.uuid)) target.filterSelection = mask;
    else target.transparencyMask = mask;
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    // The changed coverage identifies the brush action; a fill additionally
    // records its operation kind for the history display.
    change.filledLayer = filled;
    change.clearedLayers = cleared;
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
        if (parent->kind == LayerKind::FilterMask) return Result::InvalidInput;
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
    bool movingMasks = false, movingLayers = false;
    for (const auto id : moving) {
        if (values.value(id)->kind == LayerKind::FilterMask) movingMasks = true;
        else movingLayers = true;
    }
    if (movingMasks && (movingLayers || parentUuid.isNull())) return Result::InvalidInput;
    if (!movingMasks && !parentUuid.isNull() && identities.value(parentUuid)->kind != LayerKind::Group)
        return Result::InvalidInput;
    for (auto parent = parentUuid; !parent.isNull(); parent = identities.value(parent)->parentUuid)
        if (moving.contains(identities.value(parent)->id)) return Result::InvalidInput;
    bool aboveTransparency = false;
    if (!aboveUuid.isNull()) {
        const auto above = identities.value(aboveUuid);
        const auto parent = identities.value(parentUuid);
        aboveTransparency = parent && parent->transparencyMask && parent->transparencyMaskUuid == aboveUuid;
        if (!above && !aboveTransparency) return Result::LayerUnavailable;
        if (above && (above->parentUuid != parentUuid || moving.contains(above->id))) return Result::InvalidInput;
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
    QHash<QUuid, qsizetype> maskPositions;
    for (const auto &owner : original->layers) {
        if (!owner.transparencyMask) continue;
        qsizetype position = 0, removedBelow = 0;
        for (const auto child : children.value(owner.uuid)) {
            if (values.value(child)->kind != LayerKind::FilterMask) continue;
            if (position++ < owner.transparencyMaskIndex && moving.contains(child)) ++removedBelow;
        }
        maskPositions.insert(owner.uuid, owner.transparencyMaskIndex - removedBelow);
    }
    qsizetype insertion = remaining.size();
    for (qsizetype i = 0; i < remaining.size(); ++i) {
        if (!aboveUuid.isNull() ? remaining[i].uuid == aboveUuid : remaining[i].parentUuid == parentUuid) {
            insertion = i + (aboveUuid.isNull() ? 0 : 1);
            break;
        }
        if (aboveUuid.isNull() && remaining[i].uuid == parentUuid) insertion = i + 1;
    }
    if (aboveTransparency) {
        qsizetype position = 0;
        for (qsizetype i = 0; i < remaining.size(); ++i) {
            if (remaining[i].uuid == parentUuid) insertion = i + 1;
            if (remaining[i].parentUuid != parentUuid || remaining[i].kind != LayerKind::FilterMask) continue;
            if (position++ == maskPositions.value(parentUuid)) { insertion = i; break; }
            insertion = i + 1;
        }
    }
    if (movingMasks && maskPositions.contains(parentUuid) && !aboveTransparency) {
        qsizetype position = 0;
        for (qsizetype i = 0; i < insertion; ++i)
            if (remaining[i].parentUuid == parentUuid && remaining[i].kind == LayerKind::FilterMask) ++position;
        if (position <= maskPositions.value(parentUuid)) maskPositions[parentUuid] += moving.size();
    }
    QList<LayerId> selectedAfter;
    for (auto &layer : ordered) {
        layer.parentUuid = parentUuid;
        selectedAfter.append(layer.id);
        remaining.insert(insertion++, layer);
    }
    QHash<QUuid, QList<LayerId>> resultChildren;
    for (const auto &layer : remaining) resultChildren[layer.parentUuid].append(layer.id);
    bool masksChanged = false;
    for (auto &owner : remaining) if (owner.transparencyMask) {
        const auto position = maskPositions.value(owner.uuid);
        masksChanged |= owner.transparencyMaskIndex != position;
        owner.transparencyMaskIndex = position;
    }
    if (resultChildren == children && !masksChanged) return Result::Success;
    if (!Detail::validLayerHierarchy(remaining, original->format)) return Result::InvalidInput;
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
    LayerHistoryChange change;
    change.layerStructureAction = HistoryAction::MoveLayer;
    commitVersion(version, change);
    return Result::Success;
}

std::unique_ptr<OperationDocument> OperationDocument::layerGroupingDraft() const
{
    auto draft = std::make_unique<OperationDocument>(RasterImage{snapshot()->size, {}, 0}, m_limits);
    draft->m_identity = m_identity;
    draft->m_history = {snapshot()};
    draft->m_layerHistoryChanges = {LayerHistoryChange{}};
    draft->m_index = 0;
    draft->m_nextLayer = m_nextLayer;
    draft->m_nextOperation = m_nextOperation;
    // These intermediate revisions never reach execution or a document reader.
    // Publication assigns one fresh revision on this owner after all steps pass.
    draft->m_nextRevision = 0;
    return draft;
}

Result OperationDocument::groupLayers(const QList<LayerId> &layers, LayerId activeRoot,
                                      const QString &groupName, const QString &clippingLayerName)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (!validLayerName(groupName) || (!clippingLayerName.isEmpty() && !validLayerName(clippingLayerName)) ||
        layers.isEmpty()) return Result::InvalidInput;
    const auto active = layerIndex(activeRoot);
    if (active < 0) return Result::LayerUnavailable;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    const auto before = snapshot();
    auto draft = layerGroupingDraft();
    RasterLayer group;
    group.name = groupName;
    group.kind = LayerKind::Group;
    group.uuid = QUuid::createUuid();
    RasterImage input;
    input.size = before->size;
    input.format = before->format;
    input.colorProfile = before->colorProfile;
    input.layers = {group};
    OperationDocument source(input, m_limits);
    group.parentUuid = before->layers[active].parentUuid;
    auto result = draft->insertLayers({{source.snapshot(), source.activeLayer(), group}}, active + 1);
    if (result != Result::Success) return result;
    result = draft->moveLayers(layers, group.uuid, {}, layers.contains(activeRoot) ? activeRoot : layers.first());
    if (result != Result::Success) return result;
    if (!clippingLayerName.isEmpty()) {
        const auto mask = draft->addLayer(clippingLayerName, draft->snapshot()->layers.size(), false, group.uuid);
        if (!mask) return Result::CapacityExceeded;
        result = draft->writeLayerProjectionMask(mask, 7);
        if (result != Result::Success) return result;
    }
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*draft->snapshot()));
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    m_nextLayer = draft->m_nextLayer;
    LayerHistoryChange change;
    change.layerStructureAction = clippingLayerName.isEmpty() ? HistoryAction::GroupLayers : HistoryAction::CreateClippingGroup;
    // Undoing the native batch's initial group insertion reselects its former
    // active node. Keep this selection separate from pinned pixel versions.
    if (before->activeLayer != activeRoot || before->selectedLayers != QList<LayerId>{activeRoot}) {
        auto selectedBefore = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
        selectedBefore->activeLayer = activeRoot;
        selectedBefore->selectedLayers = {activeRoot};
        m_history[m_index] = selectedBefore;
    }
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::ungroupLayers(const QList<LayerId> &layers, LayerId activeRoot,
                                        const QString &replacementName)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto active = layerIndex(activeRoot);
    if (active < 0) return Result::LayerUnavailable;
    if (!replacementName.isEmpty() && !validLayerName(replacementName)) return Result::InvalidInput;
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    const auto before = snapshot();
    const auto &node = before->layers[active];
    if (node.userLocked) return Result::LayerUnavailable;
    const bool activeGroup = node.kind == LayerKind::Group;
    const auto parent = activeGroup ? active : [&] {
        for (qsizetype i = 0; i < before->layers.size(); ++i)
            if (before->layers[i].uuid == node.parentUuid) return i;
        return qsizetype(-1);
    }();
    if (parent < 0) return Result::LayerUnavailable;
    const auto &group = before->layers[parent];
    if (group.userLocked) return Result::LayerUnavailable;
    for (auto uuid = group.parentUuid; !uuid.isNull();) {
        const auto ancestor = std::find_if(before->layers.cbegin(), before->layers.cend(),
            [&](const Layer &layer) { return layer.uuid == uuid; });
        if (ancestor == before->layers.cend() || ancestor->userLocked) return Result::LayerUnavailable;
        uuid = ancestor->parentUuid;
    }
    QList<LayerId> children;
    for (const auto &layer : before->layers)
        if (layer.parentUuid == group.uuid) children.append(layer.id);
    const auto moving = activeGroup ? children : layers;
    const bool removeGroup = activeGroup ||
        QSet<LayerId>(moving.cbegin(), moving.cend()) == QSet<LayerId>(children.cbegin(), children.cend());
    auto draft = layerGroupingDraft();
    if (!moving.isEmpty()) {
        const auto result = draft->moveLayers(moving, group.parentUuid, group.uuid,
            activeGroup ? 0 : moving.contains(activeRoot) ? activeRoot : moving.first());
        if (result != Result::Success) return result;
    } else if (!activeGroup) return Result::InvalidInput;
    if (removeGroup) {
        const auto result = draft->removeLayers({group.id}, replacementName);
        if (result != Result::Success) return result;
    }
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*draft->snapshot()));
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    m_nextLayer = draft->m_nextLayer;
    LayerHistoryChange change;
    change.layerStructureAction = HistoryAction::UngroupLayers;
    commitVersion(version, change);
    return Result::Success;
}

FrozenLayerProjectionSP OperationDocument::freezeAdjustmentProjection(const Snapshot &before,
    LayerId node, QRect extent, const QList<QRect> &blocks) const
{
    LayerId scope = 0;
    const auto inputs = before->projectionInputsThrough(node, &scope);
    QHash<LayerId, LayerProjectionInput> roots;
    QSet<LayerId> included;
    for (const auto &input : inputs) { roots.insert(input.layer, input); included.insert(input.layer); }
    includeLayerDescendants(before->layers, included);
    auto source = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    source->layers.clear(); source->selection = {}; source->selectionMask.clear();
    source->sampleCount = 0; source->rasterBytes = 0;
    source->projectionBackground = scope ? RgbaPixel{} : before->projectionBackground;
    source->activeLayer = node; source->selectedLayers = {node};
    auto projection = QSharedPointer<FrozenLayerProjection>::create();
    projection->extent = extent; projection->allocatedBlocks = blocks;
    projection->nativeSources = true;
    projection->sourceBounds.insert(node, extent);
    projection->retainedBytes = sizeof(FrozenLayerProjection) + sizeof(DocumentVersion) + blocks.size() * qsizetype(sizeof(QRect));
    for (auto layer : before->layers) {
        if (!included.contains(layer.id)) continue;
        if (roots.contains(layer.id)) {
            const auto input = roots.value(layer.id);
            layer.parentUuid = {}; layer.passThrough = false;
            layer.opacity = input.opacity; layer.projectionChannelMask = input.channelMask;
            layer.blendMode = input.blendMode;
        }
        if (layer.id == node) {
            layer.visible = true; layer.opacity = 255;
            layer.projectionChannelMask = 15; layer.blendMode = LayerBlendMode::Copy;
        }
        const auto samples = layer.inputSampleCount(), bytes = layer.retainedRasterBytes();
        if (samples > m_limits.samplesPerDocument - source->sampleCount || bytes > m_limits.rasterBytes - source->rasterBytes) return {};
        source->sampleCount += samples; source->rasterBytes += bytes;
        projection->retainedBytes += sizeof(Layer) + layer.name.size() * sizeof(QChar) + layer.operations.size() * sizeof(LayerOperation);
        source->layers.append(std::move(layer));
    }
    if (projection->retainedBytes > m_limits.rasterBytes || !validLayerHierarchy(source->layers, source->format)) return {};
    projection->source = source;
    return projection;
}

Result OperationDocument::mergeLayers(const LayerMergeRequest &request) try
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto before = snapshot();
    if (request.before != before) return Result::StaleVersion;
    if (request.layers.isEmpty() || !validLayerName(request.name) ||
        int(request.mode) > int(LayerMergeMode::NewFromVisible) || request.colorLabelIndex < 0 || request.colorLabelIndex > 8)
        return Result::InvalidInput;
    const bool wholeImage = request.mode == LayerMergeMode::FlattenImage || request.mode == LayerMergeMode::NewFromVisible;
    QHash<LayerId, const Layer *> values;
    QHash<QUuid, const Layer *> identities;
    for (const auto &layer : before->layers) { values.insert(layer.id, &layer); identities.insert(layer.uuid, &layer); }
    const auto anchor = values.value(request.above);
    if (!anchor || anchor->parentUuid != request.parentUuid) return Result::LayerUnavailable;
    for (auto parent = request.parentUuid; !parent.isNull();) {
        const auto node = identities.value(parent);
        if (!node || node->kind != LayerKind::Group || node->userLocked) return Result::LayerUnavailable;
        parent = node->parentUuid;
    }
    QSet<LayerId> roots;
    QList<const Layer *> visible;
    for (const auto id : request.layers) {
        const auto layer = values.value(id);
        if (!layer || roots.contains(id) || !request.sourceBounds.contains(id)) return Result::InvalidInput;
        const auto area = request.sourceBounds.value(id);
        if (area.width() < 0 || area.height() < 0 || (!area.isEmpty() &&
            (area.left() < -CoordinateLimit || area.top() < -CoordinateLimit ||
             qint64(area.x()) + area.width() > CoordinateLimit || qint64(area.y()) + area.height() > CoordinateLimit)))
            return Result::InvalidInput;
        roots.insert(id);
        if (layer->visible) visible.append(layer);
    }
    for (const auto id : request.layers)
        for (auto parent = values.value(id)->parentUuid; !parent.isNull(); parent = identities.value(parent)->parentUuid)
            if (roots.contains(identities.value(parent)->id)) return Result::InvalidInput;
    if (request.mode == LayerMergeMode::Down) {
        if (request.layers.size() != 2) return Result::InvalidInput;
        const auto lower = values.value(request.layers[0]), upper = values.value(request.layers[1]);
        if (lower->parentUuid != upper->parentUuid) return Result::InvalidInput;
        LayerId previous = 0;
        for (const auto &layer : before->layers) if (layer.parentUuid == upper->parentUuid) {
            if (layer.id == upper->id && previous != lower->id) return Result::InvalidInput;
            previous = layer.id;
        }
    }
    if ((!wholeImage && visible.isEmpty()) || (request.mode == LayerMergeMode::Selected && request.layers.size() < 2))
        return Result::Success;
    if (request.homogeneousComposition && !wholeImage) for (const auto layer : visible) {
        if (layer->blendMode != visible.first()->blendMode || layer->projectionChannelMask != visible.first()->projectionChannelMask ||
            (request.mode == LayerMergeMode::Down && layer->opacity != visible.first()->opacity)) return Result::InvalidInput;
    }
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    const bool keepSources = request.mode == LayerMergeMode::NewFromVisible;
    QSet<LayerId> removed = keepSources ? QSet<LayerId>{} : roots;
    if (!keepSources) includeLayerDescendants(before->layers, removed);
    const auto action = [&] {
        switch (request.mode) {
        case LayerMergeMode::Down: return HistoryAction::MergeDown;
        case LayerMergeMode::Selected: return HistoryAction::MergeSelected;
        case LayerMergeMode::FlattenLayer: return HistoryAction::FlattenLayer;
        case LayerMergeMode::FlattenImage: return HistoryAction::FlattenImage;
        case LayerMergeMode::NewFromVisible: return HistoryAction::NewFromVisible;
        }
        return HistoryAction::None;
    }();
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
    QList<Layer> added;
    auto nextLayer = m_nextLayer;
    auto nextOperation = m_nextOperation;
    if (visible.size() == 1 && (request.mode == LayerMergeMode::Down ||
        (request.mode == LayerMergeMode::Selected && request.layers.size() > 1))) {
        removed.remove(visible.first()->id);
        QSet<LayerId> retained{visible.first()->id};
        includeLayerDescendants(before->layers, retained);
        removed.subtract(retained);
        version->activeLayer = visible.first()->id;
    } else {
        if (nextLayer == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
        QSet<QUuid> used;
        for (const auto &retained : m_history) {
            used.insert(retained->rootUuid);
            for (const auto &layer : retained->layers) {
                used.insert(layer.uuid);
                if (layer.transparencyMask) used.insert(layer.transparencyMaskUuid);
            }
        }
        if (request.destinationUuid.isNull() || used.contains(request.destinationUuid)) return Result::InvalidInput;
        used.insert(request.destinationUuid);
        Layer output;
        output.id = ++nextLayer;
        output.uuid = request.destinationUuid;
        output.name = request.name;
        output.parentUuid = request.parentUuid;
        output.colorLabelIndex = request.colorLabelIndex;
        output.pinnedToTimeline = request.pinnedToTimeline;
        const bool groupDown = request.mode == LayerMergeMode::Down && request.homogeneousComposition &&
            visible.size() == 2 && visible[0]->kind == LayerKind::Group && visible[1]->kind == LayerKind::Group;
        if (groupDown) {
            const auto id = output.id;
            const auto uuid = output.uuid;
            output = *visible.first();
            output.id = id; output.uuid = uuid; output.name = request.name; output.parentUuid = request.parentUuid;
            output.pinnedToTimeline = visible[0]->pinnedToTimeline || visible[1]->pinnedToTimeline;
            added.append(output);
            QHash<QUuid, QUuid> copied{{visible[0]->uuid, output.uuid}, {visible[1]->uuid, output.uuid}};
            QList<const Layer *> pending{visible[0], visible[1]};
            for (qsizetype i = 0; i < pending.size(); ++i) {
                for (const auto &child : before->layers) if (child.parentUuid == pending[i]->uuid) {
                    if (nextLayer == std::numeric_limits<quint64>::max() ||
                        quint64(child.operations.size()) > std::numeric_limits<quint64>::max() - nextOperation)
                        return Result::CapacityExceeded;
                    auto clone = child;
                    clone.id = ++nextLayer;
                    clone.uuid = request.copiedUuids.value(child.uuid);
                    if (clone.uuid.isNull() || used.contains(clone.uuid)) return Result::InvalidInput;
                    used.insert(clone.uuid);
                    clone.parentUuid = copied.value(child.parentUuid);
                    if (clone.transparencyMask) {
                        clone.transparencyMaskUuid = request.copiedUuids.value(child.transparencyMaskUuid);
                        if (clone.transparencyMaskUuid.isNull() || used.contains(clone.transparencyMaskUuid)) return Result::InvalidInput;
                        used.insert(clone.transparencyMaskUuid);
                    }
                    copyLayerOperations(child, clone, m_identity, nextOperation);
                    copied.insert(child.uuid, clone.uuid);
                    added.append(std::move(clone)); pending.append(&child);
                }
            }
        } else {
            const bool down = request.mode == LayerMergeMode::Down;
            if (request.homogeneousComposition && !wholeImage) {
                output.blendMode = visible.first()->blendMode;
                output.projectionChannelMask = visible.first()->projectionChannelMask;
                if (down) output.opacity = visible.first()->opacity;
            }
            QSet<LayerId> included;
            for (const auto layer : visible) included.insert(layer->id);
            includeLayerDescendants(before->layers, included);
            QHash<LayerId, FrozenLayerProjectionSP> adjustments;
            QSet<QUuid> adjustmentIdentities;
            if (!wholeImage) for (const auto &layer : before->layers) {
                if (!included.contains(layer.id) || layer.kind != LayerKind::Adjustment) continue;
                QRect extent = request.sourceBounds.value(layer.id);
                if (extent.isEmpty()) extent = QRect(QPoint(), before->size);
                const auto frozen = freezeAdjustmentProjection(before, layer.id, extent, request.allocatedBlocks);
                if (!frozen) return Result::CapacityExceeded;
                adjustments.insert(layer.id, frozen);
                adjustmentIdentities.insert(layer.uuid);
            }
            for (const auto &layer : before->layers)
                if (layer.kind == LayerKind::FilterMask && adjustmentIdentities.contains(layer.parentUuid)) included.remove(layer.id);
            auto source = QSharedPointer<DocumentVersion>(new DocumentVersion(*before));
            source->layers.clear(); source->selection = {}; source->selectionMask.clear();
            source->sampleCount = 0; source->rasterBytes = 0;
            auto projection = QSharedPointer<FrozenLayerProjection>::create();
            projection->normalizeRootProjection = wholeImage;
            if (request.allocatedBlocks.size() > m_limits.rasterBytes / qsizetype(sizeof(QRect))) return Result::CapacityExceeded;
            for (const auto &block : request.allocatedBlocks) {
                if (block.isEmpty() || block.left() < -CoordinateLimit || block.top() < -CoordinateLimit ||
                    qint64(block.x()) + block.width() > CoordinateLimit || qint64(block.y()) + block.height() > CoordinateLimit)
                    return Result::InvalidInput;
            }
            projection->allocatedBlocks = request.allocatedBlocks;
            if (wholeImage) projection->extent = QRect(QPoint(), before->size);
            projection->retainedBytes = sizeof(FrozenLayerProjection) + sizeof(DocumentVersion) + request.allocatedBlocks.size() * sizeof(QRect);
            const auto defaults = before->projectionDefaultPixels();
            if (down && request.homogeneousComposition) {
                output.defaultPixel = defaults.value(visible.first()->id, visible.first()->defaultPixel);
                if (hasEnabledTransparencyMask(*visible.first())) {
                    const auto coverage = visible.first()->transparencyMask->defaultCoverage();
                    output.defaultPixel[3] = quint16((quint64(output.defaultPixel[3]) * coverage + 127u) / 255u);
                }
                output.position = visible.first()->position;
            }
            source->projectionBackground = wholeImage ? before->projectionBackground : output.defaultPixel;
            for (auto layer : before->layers) {
                if (!included.contains(layer.id)) continue;
                if (adjustments.contains(layer.id)) {
                    layer.kind = LayerKind::Paint; layer.filter.reset(); layer.filterSelection.clear();
                    layer.transparencyMask.clear(); layer.transparencyMaskUuid = {}; layer.transparencyMaskIndex = 0;
                    layer.defaultPixel = {}; layer.operations.clear();
                    layer.initialProjection = adjustments.value(layer.id);
                }
                if (roots.contains(layer.id)) {
                    layer.parentUuid = {};
                    if (!wholeImage) layer.passThrough = false;
                    if (request.homogeneousComposition && !wholeImage) {
                        layer.blendMode = LayerBlendMode::Normal;
                        layer.projectionChannelMask |= 8u;
                    }
                    if (down) {
                        if (request.homogeneousComposition) {
                            layer.opacity = 255; layer.projectionChannelMask = 15;
                            if (layer.id == visible.first()->id) layer.blendMode = LayerBlendMode::Copy;
                        } else if (layer.id == visible.first()->id ||
                                   bool(visible[0]->projectionChannelMask & 8u) == bool(visible[1]->projectionChannelMask & 8u)) {
                            layer.projectionChannelMask |= 8u;
                        }
                    }
                    const auto area = request.sourceBounds.value(layer.id);
                    projection->sourceBounds.insert(layer.id, area);
                    projection->extent |= area;
                }
                source->sampleCount += layer.inputSampleCount(); source->rasterBytes += layer.retainedRasterBytes();
                projection->retainedBytes += sizeof(Layer) + layer.name.size() * sizeof(QChar) + layer.operations.size() * sizeof(LayerOperation);
                source->layers.append(std::move(layer));
            }
            source->activeLayer = visible.isEmpty() ? 0 : visible.first()->id;
            source->selectedLayers = source->activeLayer ? QList<LayerId>{source->activeLayer} : QList<LayerId>{};
            projection->source = source;
            output.initialProjection = projection;
            added.append(std::move(output));
        }
        version->activeLayer = added.first().id;
    }
    if (added.size() > m_limits.layerCount - (before->layers.size() - removed.size())) return Result::CapacityExceeded;
    QList<Layer> finalLayers;
    for (const auto &layer : before->layers) {
        if (!removed.contains(layer.id)) finalLayers.append(layer);
        if (layer.id == request.above) finalLayers.append(added);
    }
    version->layers = std::move(finalLayers);
    if (request.mode == LayerMergeMode::FlattenImage) version->rootUuid = QUuid::createUuid();
    if (!validLayerHierarchy(version->layers, version->format)) return Result::InvalidInput;
    version->sampleCount = 0; version->rasterBytes = 0;
    for (const auto &layer : version->layers) {
        const auto samples = layer.inputSampleCount(), bytes = layer.retainedRasterBytes();
        if (samples > m_limits.samplesPerDocument - version->sampleCount || bytes > m_limits.rasterBytes - version->rasterBytes)
            return Result::CapacityExceeded;
        version->sampleCount += samples; version->rasterBytes += bytes;
    }
    version->selectedLayers = {version->activeLayer};
    version->m_modifiedRevision = version->m_revision = m_nextRevision + 1;
    LayerHistoryChange change; change.layerStructureAction = action;
    commitVersion(version, change);
    m_nextRevision = version->revision(); m_nextLayer = nextLayer; m_nextOperation = nextOperation;
    return Result::Success;
}
catch (const std::bad_alloc &) { return Result::CapacityExceeded; }

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
        version->sampleCount -= layer.inputSampleCount();
        version->rasterBytes -= layer.retainedRasterBytes();
        layer.operations.clear();
        layer.initialProjection.clear();
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    LayerHistoryChange change;
    change.clearedLayers = true;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::clearSelectedPixels(const QList<LayerId> &layers)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto current = snapshot();
    if (layers.isEmpty() || current->selection.isEmpty()) return Result::InvalidInput;
    QSet<LayerId> selected;
    for (const auto id : layers) {
        if (selected.contains(id)) return Result::InvalidInput;
        const auto index = layerIndex(id);
        if (index < 0 || current->layers[index].kind != LayerKind::Paint || current->layers[index].userLocked)
            return Result::LayerUnavailable;
        selected.insert(id);
    }
    if (m_limits.samplesPerStroke < 2 || layers.size() > (m_limits.samplesPerDocument - current->sampleCount) / 2 ||
        quint64(layers.size()) > std::numeric_limits<quint64>::max() - m_nextOperation ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    const QRect region = current->selectionInverted ? QRect(QPoint(), current->size) : current->selection;
    const QPointF first(region.topLeft()), last(region.x() + region.width(), region.y() + region.height());
    const auto samples = QSharedPointer<const QList<InputSample>>::create(QList<InputSample>{{first, 1}, {last, 1}});
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*current));
    auto sequence = m_nextOperation;
    for (auto &layer : version->layers) {
        if (!selected.contains(layer.id)) continue;
        auto operation = QSharedPointer<PaintOperation>::create();
        operation->id = {m_identity, ++sequence};
        operation->version = 1;
        operation->layer = layer.id;
        operation->brush.rgba = layer.defaultPixel;
        operation->brush.erase = true;
        operation->shape = {ShapeKind::Rectangle, first, last, 0};
        operation->sampleBlocks = {samples};
        operation->sampleCount = 2;
        operation->figureStyle.outline = false;
        auto color = layer.defaultPixel;
        color[3] = maximumComponent(current->format);
        operation->figureStyle.fill = color;
        operation->compositionBounds = region;
        operation->clearsSelectedPixels = true;
        operation->selectionRect = current->selection;
        operation->selectionInverted = current->selectionInverted;
        operation->selectionKind = current->selectionKind;
        operation->selectionMask = current->selectionMask;
        layer.operations.append(PaintSnapshot(operation));
        version->sampleCount += 2;
    }
    version->m_modifiedRevision = version->m_revision = m_nextRevision + 1;
    LayerHistoryChange change;
    change.clearedLayers = true;
    commitVersion(version, change);
    m_nextOperation = sequence;
    ++m_nextRevision;
    return Result::Success;
}

QList<LayerId> OperationDocument::layerCropParticipants(LayerId root, const QUuid &maskUuid) const
{
    const auto index = layerIndex(root);
    if (index < 0) return {};
    if (!maskUuid.isNull()) return snapshot()->layers[index].transparencyMask &&
        snapshot()->layers[index].transparencyMaskUuid == maskUuid ? QList<LayerId>{root} : QList<LayerId>{};
    QHash<QUuid, QList<LayerId>> children;
    QHash<LayerId, QUuid> identities;
    for (const auto &layer : snapshot()->layers) {
        children[layer.parentUuid].append(layer.id);
        identities.insert(layer.id, layer.uuid);
    }
    QList<LayerId> result{root};
    for (qsizetype i = 0; i < result.size(); ++i) result.append(children.value(identities.value(result[i])));
    return result;
}

Result OperationDocument::cropLayer(LayerId layer, const QRect &rect)
{ return cropLayerWithMasks(layer, rect, {}); }

Result OperationDocument::cropLayerWithMasks(LayerId root, const QRect &rect,
    const QHash<QUuid, SelectionMaskSP> &masks, const QUuid &maskUuid)
{
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    const auto participants = layerCropParticipants(root, maskUuid);
    if (participants.isEmpty()) return Result::LayerUnavailable;
    if (!validCropRect(rect)) return Result::InvalidInput;
    const auto source = snapshot();
    quint64 count = 0;
    for (const auto id : participants) if (maskUuid.isNull() && source->layers[layerIndex(id)].kind == LayerKind::Paint) ++count;
    const bool separateDescriptor = !maskUuid.isNull() || source->layers[layerIndex(root)].kind != LayerKind::Paint;
    count += separateDescriptor;
    if (count > std::numeric_limits<quint64>::max() - m_nextOperation ||
        m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto sequence = m_nextOperation;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*source));
    CropSnapshot descriptor;
    const auto cropMask = [&](const SelectionMaskSP &coverage, const QUuid &uuid) {
        auto result = masks.value(uuid);
        if (!result && rect.normalized().isEmpty()) result = SelectionMask::uniform(coverage->defaultCoverage());
        if (!result && !coverage->isGpuGenerated()) result = coverage->translated({}, rect.normalized());
        return result && result->byteCount() <= m_limits.rasterBytes && selectionFitsRetainedInputs(result) ? result : SelectionMaskSP{};
    };
    for (const auto id : participants) {
        auto &layer = version->layers[layerIndex(id)];
        if (maskUuid.isNull() && layer.kind == LayerKind::Paint) {
            auto crop = QSharedPointer<CropOperation>::create();
            crop->id = {m_identity, ++sequence}; crop->layer = id; crop->rect = rect;
            layer.operations.append(CropSnapshot(crop));
            if (id == root) descriptor = crop;
        }
        if (maskUuid.isNull() && layer.filterSelection) {
            const auto mask = cropMask(layer.filterSelection, layer.uuid);
            if (!mask) return Result::CapacityExceeded;
            layer.filterSelection = mask;
        }
        if (layer.transparencyMask) {
            const auto mask = cropMask(layer.transparencyMask, layer.transparencyMaskUuid);
            if (!mask) return Result::CapacityExceeded;
            layer.transparencyMask = mask;
        }
    }
    if (separateDescriptor) {
        auto crop = QSharedPointer<CropOperation>::create();
        crop->id = {m_identity, ++sequence}; crop->layer = root; crop->rect = rect; crop->maskUuid = maskUuid;
        descriptor = crop;
    }
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    m_nextOperation = sequence;
    LayerHistoryChange change; change.crop = descriptor;
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::fillLayer(LayerId layer, const std::array<quint8, 3> &rgb,
                                    qreal opacity, bool erase, bool alphaLocked,
                                    LayerBlendMode blendMode)
{
    const quint16 scale = snapshot()->format == PixelFormat::Rgba16 ? 257 : 1;
    return fillLayerColor(layer, {quint16(rgb[0] * scale), quint16(rgb[1] * scale),
                                 quint16(rgb[2] * scale), quint16(255 * scale)},
                          opacity, erase, alphaLocked, blendMode);
}

Result OperationDocument::fillLayerColor(LayerId layer, const RgbaPixel &color,
                                         qreal opacity, bool erase, bool alphaLocked,
                                         LayerBlendMode blendMode)
{
    FigureStyle style;
    style.outline = false;
    style.fill = color;
    style.fillBlendMode = erase ? LayerBlendMode::Normal : blendMode;
    return fillLayerStyle(layer, color, style, opacity, erase, alphaLocked);
}

Result OperationDocument::paintGradient(LayerId layer, const Brush &brush, const GradientFill &gradient)
{
    if (!gradient.isValid()) return Result::InvalidInput;
    const auto version = snapshot();
    QRect bounds(QPoint(), version->size);
    if (!version->selection.isEmpty() && !version->selectionInverted) bounds &= version->selection;
    if (bounds.isEmpty()) return Result::EmptyOperation;
    FigureStyle style;
    style.outline = false;
    style.gradient = gradient;
    style.fillBlendMode = brush.erase ? LayerBlendMode::Normal : brush.blendMode;
    const QPointF start = bounds.topLeft(), end = start + QPointF(bounds.width(), bounds.height());
    const auto result = paintFigureWithin(layer, brush, {{start, 1}, {end, 1}},
        {ShapeKind::Rectangle, start, end, 0}, style, 0, bounds);
    if (result == Result::Success) m_layerHistoryChanges[m_index].filledLayer = true;
    return result;
}

Result OperationDocument::fillLayerPattern(LayerId layer, const PatternFill &pattern,
                                           qreal opacity, bool erase, bool alphaLocked,
                                           LayerBlendMode blendMode)
{
    if (!pattern.isValid()) return Result::InvalidInput;
    FigureStyle style;
    style.outline = false;
    style.pattern = pattern;
    style.fillBlendMode = erase ? LayerBlendMode::Normal : blendMode;
    return fillLayerStyle(layer, {0, 0, 0, 255}, style, opacity, erase, alphaLocked);
}

Result OperationDocument::fillLayerStyle(LayerId layer, const RgbaPixel &color,
                                         const FigureStyle &style, qreal opacity,
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
    const auto version = snapshot();
    const QRect bounds = !version->selection.isEmpty() && !version->selectionInverted ? version->selection :
        QRect(QPoint(), version->size);
    const QPointF start = bounds.topLeft();
    const QPointF end = start + QPointF(bounds.width(), bounds.height());
    const auto result = paintFigureWithin(layer, brush, {{start, 1}, {end, 1}},
                                         {ShapeKind::Rectangle, start, end, 0}, style, 0, bounds);
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

Result OperationDocument::colorFilter(LayerId layer, FilterKind kind, const ColorOverlay &overlay, const QRect &applyRect)
{
    if (kind < FilterKind::MaximizeChannel || kind > FilterKind::ColorOverlay ||
        (kind == FilterKind::ColorOverlay && !overlay.isValid(snapshot()->format))) return Result::InvalidInput;
    return applyFilter(layer, kind, nullptr, &overlay, applyRect);
}

Result OperationDocument::convolutionFilter(LayerId layer, const FilterSettings &settings, const QRect &applyRect)
{
    if (!isConvolutionFilter(settings.kind) || !settings.isValid(snapshot()->format)) return Result::InvalidInput;
    return applyFilter(layer, settings.kind, nullptr, nullptr, applyRect, &settings);
}

Result OperationDocument::applyFilter(LayerId layer, FilterKind kind, const HsvAdjustment *settings,
                                      const ColorOverlay *overlay, const QRect &applyRect, const FilterSettings *convolution)
{
    if (kind == FilterKind::HsvAdjustment && !settings) return Result::InvalidInput;
    const auto status = checkMutation();
    if (status != Result::Success) return status;
    if (snapshot()->format == PixelFormat::Rgba16 && kind < FilterKind::MaximizeChannel) return Result::InvalidInput;
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
    if (convolution) static_cast<FilterSettings &>(*operation) = *convolution;
    if (overlay) operation->overlay = *overlay;
    operation->applyRect = (applyRect.isEmpty() ? QRect(QPoint(), snapshot()->size) : applyRect)
        .intersected(QRect(QPoint(), snapshot()->size));
    if (convolution) operation->samplingBounds = QRect(QPoint(), snapshot()->size);
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
{ return removeLayers(layers, replacementName, {}); }

Result OperationDocument::removeLayers(const QList<LayerId> &layers, const QString &replacementName,
                                       const QHash<LayerId, Layer> &detachedClones)
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
    for (auto &owner : version->layers) {
        if (!owner.transparencyMask || removed.contains(owner.id)) continue;
        qsizetype position = 0, removedBelow = 0;
        for (const auto &child : snapshot()->layers) {
            if (child.kind != LayerKind::FilterMask || child.parentUuid != owner.uuid) continue;
            if (position++ < owner.transparencyMaskIndex && removed.contains(child.id)) ++removedBelow;
        }
        owner.transparencyMaskIndex -= removedBelow;
    }
    for (qsizetype i = version->layers.size(); i-- > 0;) {
        if (!removed.contains(version->layers[i].id)) continue;
        version->sampleCount -= version->layers[i].inputSampleCount();
        version->rasterBytes -= version->layers[i].retainedRasterBytes();
        version->layers.removeAt(i);
    }
    QSet<QUuid> removedIdentities;
    for (const auto &layer : snapshot()->layers) if (removed.contains(layer.id)) removedIdentities.insert(layer.uuid);
    auto nextOperation = m_nextOperation;
    for (auto &layer : version->layers) {
        if (layer.kind != LayerKind::Clone || !removedIdentities.contains(layer.cloneSourceUuid)) continue;
        const auto prepared = detachedClones.constFind(layer.id);
        if (prepared == detachedClones.cend() || prepared->id != layer.id || prepared->uuid != layer.uuid ||
            prepared->kind != LayerKind::Paint || !prepared->cloneSourceUuid.isNull()) return Result::InvalidInput;
        if (quint64(prepared->operations.size()) > std::numeric_limits<quint64>::max() - nextOperation)
            return Result::CapacityExceeded;
        const auto source = *prepared;
        layer = source;
        copyLayerOperations(source, layer, m_identity, nextOperation);
    }
    version->sampleCount = 0;
    version->rasterBytes = 0;
    for (const auto &layer : version->layers) {
        const auto samples = layer.inputSampleCount(), bytes = layer.retainedRasterBytes();
        if (samples > m_limits.samplesPerDocument - version->sampleCount ||
            bytes > m_limits.rasterBytes - version->rasterBytes) return Result::CapacityExceeded;
        version->sampleCount += samples;
        version->rasterBytes += bytes;
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
    m_nextOperation = nextOperation;
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
    m_affine.clear();
    m_translation.clear();
    m_translationTargets.clear();
    m_translationIds.clear();
    m_externalOperation = {};
    m_filterNodeEdit.reset();
    m_cloneSourceEdit.reset();
    m_layerHistoryChanges = {LayerHistoryChange{}};
    m_index = 0;
    m_closed = true;
}

}
