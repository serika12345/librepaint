/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintImage_p.h"
#include "KisGpuOperationValidation_p.h"
#include <QThread>
#include <QRegion>
#include <algorithm>
#include <new>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
namespace {
Execution &requireExecution(const std::shared_ptr<Execution> &execution)
{
    if (!execution) throw std::invalid_argument("Image construction requires an execution owner");
    return *execution;
}

Result resultFor(ExecutionCode code)
{
    switch (code) {
    case ExecutionCode::Success: return Result::Success;
    case ExecutionCode::CapacityExceeded: return Result::CapacityExceeded;
    case ExecutionCode::Busy: return Result::EditInProgress;
    case ExecutionCode::DeviceFailure: return Result::DeviceFailure;
    case ExecutionCode::InvalidRequest:
    case ExecutionCode::SurfaceOutOfDate: return Result::InvalidInput;
    }
    return Result::DeviceFailure;
}

const Layer *maskPaintLayer(const Snapshot &version, LayerId layer)
{
    if (!version) return nullptr;
    for (const auto &value : version->layers)
        if (value.id == layer && value.kind == LayerKind::Paint && value.transparencyMask && !value.transparencyMaskLocked &&
            !value.userLocked && value.visible) return &value;
    return nullptr;
}


}
PaintImage::PaintImage(Execution &execution, QSize size, Limits limits)
    : PaintImage(execution, std::make_unique<OperationDocument>(size, limits), limits) {}
PaintImage::PaintImage(Execution &execution, const RasterImage &image, Limits limits)
    : PaintImage(execution, std::make_unique<OperationDocument>(image, limits), limits) {}
PaintImage::PaintImage(std::shared_ptr<Execution> execution, QSize size, Limits limits)
    : PaintImage(requireExecution(execution), std::make_unique<OperationDocument>(size, limits), limits, execution) {}
PaintImage::PaintImage(std::shared_ptr<Execution> execution, const RasterImage &image, Limits limits)
    : PaintImage(requireExecution(execution), std::make_unique<OperationDocument>(image, limits), limits, execution) {}
PaintImage::PaintImage(Execution &execution, std::unique_ptr<OperationDocument> document, Limits limits,
                       std::shared_ptr<Execution> retainedExecution)
    : m_d(new Private{std::move(retainedExecution), execution, std::move(document), limits})
{
    const auto admitted = execution.open(m_d->document->snapshot());
    if (admitted.code == ExecutionCode::CapacityExceeded) throw std::length_error(admitted.diagnostic.toStdString());
    if (admitted.code != ExecutionCode::Success) throw std::invalid_argument(admitted.diagnostic.toStdString());
    m_d->handle = admitted.handle;
}
PaintImage::~PaintImage()
{
    if (m_d->handle) m_d->execution.close(m_d->handle);
}
const OperationDocument &PaintImage::operations() const { return *m_d->document; }
Limits PaintImage::limits() const { return m_d->limits; }
quint64 PaintImage::handle() const { return m_d->handle; }
Execution &PaintImage::execution() const { return m_d->execution; }
PaintImageSP PaintImage::clone(bool preserveLayerUuids) const
{ return cloneTo(*this, preserveLayerUuids); }

PaintImageSP PaintImage::cloneTo(const PaintImage &destination, bool preserveLayerUuids) const
{
    if (!onThread() || !destination.onThread()) return {};
    return PaintImageSP(new PaintImage(destination.m_d->execution, m_d->document->cloneCurrent(preserveLayerUuids),
                                       m_d->limits, destination.m_d->retainedExecution));
}

PaintImageSP PaintImage::cloneTo(Execution &destination, bool preserveLayerUuids) const
{
    if (!onThread()) return {};
    return PaintImageSP(new PaintImage(destination, m_d->document->cloneCurrent(preserveLayerUuids), m_d->limits,
        &destination == &m_d->execution ? m_d->retainedExecution : std::shared_ptr<Execution>{}));
}
PaintImageSP PaintImage::cloneLayerPixels(const LayerPixelReferenceSP &pixels) const
{
    if (!onThread() || !pixels) return {};
    auto document = m_d->document->cloneVersion(pixels->version(), false, pixels->layer());
    if (!document) return {};
    return PaintImageSP(new PaintImage(m_d->execution, std::move(document), m_d->limits, m_d->retainedExecution));
}
PaintImageSP PaintImage::cloneLayerForest(const QList<LayerInsertion> &layers) const
{
    if (!onThread()) return {};
    return cloneLayerForest(layers, m_d->document->snapshot()->size);
}
PaintImageSP PaintImage::cloneLayerForest(const QList<LayerInsertion> &layers, QSize size) const
{
    RasterImage input;
    input.size = size;
    input.format = m_d->document->snapshot()->format;
    input.colorProfile = m_d->document->snapshot()->colorProfile;
    OperationDocument prepared(input, m_d->limits);
    if (prepared.insertLayers(layers, 0) != Result::Success) return {};
    return PaintImageSP(new PaintImage(m_d->execution, prepared.cloneCurrent(true),
                                       m_d->limits, m_d->retainedExecution));
}
ExecutionOutcome PaintImage::lastRequestOutcome() const { return m_d->lastRequest; }
PixelBytesOutcome PaintImage::readRgbaPixels(LayerId layer, QRect region, qsizetype maximumBytes) const
{
    if (!onThread()) {
        PixelBytesOutcome outcome;
        outcome.diagnostic = QStringLiteral("Image pixel reads require the owning thread");
        return outcome;
    }
    return m_d->execution.readRgbaPixels(m_d->handle, m_d->document->snapshot(), region, layer, maximumBytes);
}
bool PaintImage::onThread() const { return QThread::currentThread() == thread(); }
Result PaintImage::writeProjectionBackground(const RgbaPixel &rgba)
{
    return onThread() ? requestAfterWrite(m_d->document->writeProjectionBackground(rgba)) : Result::InvalidInput;
}
Result PaintImage::requestAfterWrite(Result result)
{
    if (result == Result::Success) requestCurrentVersion();
    return result;
}
Result PaintImage::writeLayerDefaultPixel(const LayerPixelReferenceSP &pixels, const RgbaPixel &rgba)
{
    if (!onThread() || !pixels || m_d->layerPixels.value(pixels->m_uuid).toStrongRef() != pixels) return Result::InvalidInput;
    return requestAfterWrite(m_d->document->applyLayerDefaultPixel(pixels->m_layer, rgba, &pixels->m_version));
}
Result PaintImage::writeLayerName(LayerId layer, const QString &name)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerName(layer, name)) : Result::InvalidInput; }
Result PaintImage::writeLayerVisible(LayerId layer, bool visible)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerVisible(layer, visible)) : Result::InvalidInput; }
Result PaintImage::writeLayerOpacity(LayerId layer, quint8 opacity)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerOpacity(layer, opacity)) : Result::InvalidInput; }
Result PaintImage::writeLayerLocked(LayerId layer, bool locked)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerLocked(layer, locked)) : Result::InvalidInput; }
Result PaintImage::writeLayerPinnedToTimeline(LayerId layer, bool pinned)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerPinnedToTimeline(layer, pinned)) : Result::InvalidInput; }
Result PaintImage::writeLayerColorLabel(LayerId layer, int index)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerColorLabel(layer, index)) : Result::InvalidInput; }
Result PaintImage::recordLayerProperties(const Snapshot &before, const Snapshot &after,
                                        const QList<LayerPropertiesSelection> &selection)
{ return onThread() ? publish(m_d->document->recordLayerProperties(before, after, selection)) : Result::InvalidInput; }
Result PaintImage::restoreLayerProperties(const Snapshot &before, const QList<LayerPropertiesSelection> &selection)
{ return onThread() ? publish(m_d->document->restoreLayerProperties(before, selection)) : Result::InvalidInput; }
LayerPixelReferenceSP PaintImage::retainLayerPixels(LayerId layer)
{
    if (!onThread() || !m_d->handle) return {};
    const auto version = layerPixelVersion();
    const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [layer](const auto &value) { return value.id == layer; });
    if (layer && found == version->layers.cend()) return {};
    const auto uuid = layer ? found->uuid : version->rootUuid;
    auto reference = m_d->layerPixels.value(uuid).toStrongRef();
    if (!reference) {
        reference.reset(new LayerPixelReference(layer, uuid, version));
        m_d->layerPixels.insert(uuid, reference);
    }
    return reference;
}
OperationId PaintImage::beginColorConversion()
{ return onThread() ? m_d->document->beginColorConversion() : OperationId{}; }
Result PaintImage::cancelColorConversion(OperationId id)
{ return onThread() ? m_d->document->cancelColorConversion(id) : Result::InvalidInput; }
Result PaintImage::commitColorConversion(OperationId id, const Snapshot &before, PixelFormat format,
    const QByteArray &profile, const RgbaPixel &background, const QList<ConvertedLayerPixels> &layers)
{
    if (!onThread()) return Result::InvalidInput;
    Result result;
    try { result = m_d->document->commitColorConversion(id, before, format, profile, background, layers); }
    catch (const std::bad_alloc &) { return Result::CapacityExceeded; }
    return publish(result);
}

Result PaintImage::setSelectionRect(const QRect &rect, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    const bool effectiveInverted = !rect.isEmpty() && inverted;
    if (rect.normalized() == m_d->document->selectionRect() &&
        effectiveInverted == m_d->document->selectionInverted() &&
        m_d->document->selectionKind() == SelectionKind::Rectangle) return Result::Success;
    return publish(m_d->document->setSelectionRect(rect, inverted));
}
Result PaintImage::setSelectionEllipse(const QRect &rect, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    const bool effectiveInverted = !rect.isEmpty() && inverted;
    if (rect.normalized() == m_d->document->selectionRect() &&
        m_d->document->selectionKind() == SelectionKind::Ellipse &&
        effectiveInverted == m_d->document->selectionInverted()) return Result::Success;
    return publish(m_d->document->setSelectionEllipse(rect, inverted));
}
Result PaintImage::setSelectionMask(const SelectionMaskSP &mask, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    if (mask && m_d->document->selectionKind() == SelectionKind::Freehand &&
        m_d->document->selectionMask() == mask && m_d->document->selectionInverted() == inverted)
        return Result::Success;
    return publish(m_d->document->setSelectionMask(mask, inverted));
}
Result PaintImage::clearSelection()
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->document->selectionRect().isEmpty() && !m_d->document->selectionInverted() &&
        m_d->document->selectionKind() == SelectionKind::Rectangle) return Result::Success;
    return publish(m_d->document->clearSelection());
}
Result PaintImage::combineSelectionMask(const SelectionMaskSP &mask, SelectionCombination action)
{
    if (!onThread()) return Result::InvalidInput;
    const auto before = m_d->document->snapshot();
    if (!mask || (action == SelectionCombination::Replace &&
        (!before->selectionInverted || QRect(QPoint(), before->size).contains(mask->bounds()))) ||
        (!mask->isGpuGenerated() && (!before->selectionMask || !before->selectionMask->isGpuGenerated())))
        return publish(m_d->document->combineSelectionMask(mask, action));
    const auto status = m_d->document->checkMutation();
    if (status != Result::Success) return status;
    if (action < SelectionCombination::Replace || action > SelectionCombination::SymmetricDifference) return Result::InvalidInput;
    try {
        const bool inverted = before->selectionInverted || (before->selection.isEmpty() && action == SelectionCombination::Subtract);
        const QRect canvas(QPoint(), before->size);
        const QRect clip = inverted ? canvas : canvas.united(before->selection).united(mask->bounds());
        SelectionMaskSP previous = SelectionMask::empty();
        if (!before->selection.isEmpty()) {
            previous = before->selectionKind == SelectionKind::Freehand ? before->selectionMask :
                before->selectionKind == SelectionKind::Ellipse ? SelectionMask::fromEllipse(before->selection, clip) :
                                                                SelectionMask::fromRectangle(before->selection, clip);
            if (!previous) return Result::CapacityExceeded;
        }
        SelectionMaskSP result;
        const auto checked = m_d->execution.combineSelectionCoverage(previous, mask, action, clip, inverted, &result);
        if (checked.code != ExecutionCode::Success) { m_d->lastRequest = checked; return resultFor(checked.code); }
        if (m_d->document->snapshot() != before) return Result::StaleVersion;
        return publish(m_d->document->combineSelectionMask(result, SelectionCombination::Replace));
    } catch (const std::bad_alloc &) { return Result::CapacityExceeded; }
}
SelectionMaskSP PaintImage::complementSelectionMask(const SelectionMaskSP &mask, QRect clip)
{
    if (!onThread() || !mask) return {};
    if (!mask->isGpuGenerated()) return mask->inverted(clip);
    try {
        SelectionMaskSP result;
        const auto checked = m_d->execution.combineSelectionCoverage(mask, SelectionMask::empty(),
            SelectionCombination::Subtract, clip, true, &result);
        if (checked.code != ExecutionCode::Success) { m_d->lastRequest = checked; return {}; }
        return result;
    } catch (const std::bad_alloc &) { return {}; }
}
SelectionMaskSP PaintImage::translatedSelectionMask(const SelectionMaskSP &mask, QPoint offset, QRect clip)
{
    if (!onThread() || !mask) return {};
    if (mask->defaultCoverage()) {
        if (mask->bounds().isEmpty()) return mask;
    }
    try {
        if (offset.isNull() && clip.contains(mask->bounds())) return mask;
        if (!mask->isGpuGenerated() && !mask->defaultCoverage()) return mask->translated(offset, clip);
        SelectionMaskSP result;
        const auto checked = m_d->execution.translateSelectionCoverage(mask, offset, clip, &result);
        if (checked.code != ExecutionCode::Success) { m_d->lastRequest = checked; return {}; }
        return result;
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("Selection movement exceeded allocation capacity")};
        return {};
    }
}

SelectionMaskSP PaintImage::transformedSelectionMask(const SelectionMaskSP &mask, const AffinePlan &plan)
{
    if (!onThread() || !mask || !validAffinePlan(plan)) return {};
    if (mask->defaultCoverage() && mask->bounds().isEmpty()) return mask;
    if (mask->isEmpty() || plan.passes.isEmpty()) return mask;
    try {
        const auto image = createMaskImage(m_d->document->snapshot(), mask);
        if (!image) return {};
        auto status = image->clearSelection();
        const auto layer = image->operations().activeLayer();
        const auto id = status == Result::Success ? image->beginAffineTransform({{layer, true, false}}) : OperationId{};
        if (id.isNull()) status = Result::InvalidInput;
        else status = image->updateAffineTransform(id, {{layer, QSharedPointer<AffinePlan>::create(plan), {}, false}});
        if (status == Result::Success) status = image->commitAffineTransform(id);
        if (status != Result::Success) {
            m_d->lastRequest = {status == Result::CapacityExceeded ? ExecutionCode::CapacityExceeded :
                status == Result::Busy ? ExecutionCode::Busy : status == Result::DeviceFailure ? ExecutionCode::DeviceFailure : ExecutionCode::InvalidRequest,
                QStringLiteral("GPU mask transform failed (%1)").arg(int(status))};
            return {};
        }
        SelectionMaskSP result;
        const auto checked = m_d->execution.paintedSelectionCoverage(image->handle(), image->operations().snapshot(), layer, &result);
        if (checked.code != ExecutionCode::Success) { m_d->lastRequest = checked; return {}; }
        return result;
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("Mask transform allocation exceeded capacity")};
        return {};
    }
}

SelectionMaskSP PaintImage::transformedTransparencyMask(const SelectionMaskSP &mask, const SelectionMaskSP &selection,
    bool inverted, QRect copyRect, const AffinePlan &plan, QRect defaultBounds)
{
    if (!onThread()) return {};
    try {
        SelectionMaskSP result;
        const auto checked = m_d->execution.transformedMaskCoverage(mask, selection, inverted, copyRect, plan, defaultBounds, &result);
        if (checked.code != ExecutionCode::Success) { m_d->lastRequest = checked; return {}; }
        return result;
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("Mask transform allocation exceeded capacity")};
        return {};
    }
}

std::optional<QRect> PaintImage::transformedLayerDefaultBounds(LayerId layer, const AffinePlan &plan)
{
    if (!onThread() || !validAffinePlan(plan)) return {};
    const auto source = m_d->document->snapshot();
    const auto found = std::find_if(source->layers.cbegin(), source->layers.cend(),
        [layer](const auto &value) { return value.id == layer && value.kind == LayerKind::Paint; });
    if (found == source->layers.cend()) return {};
    try {
        const auto pixels = LayerPixelReferenceSP(new LayerPixelReference(layer, found->uuid, source));
        const auto image = cloneLayerPixels(pixels);
        if (!image) return {};
        const auto target = image->operations().activeLayer();
        const auto token = image->beginAffineTransform({{target, true, false}});
        const auto status = token.isNull() ? Result::InvalidInput :
            image->updateAffineTransform(token, {{target, QSharedPointer<AffinePlan>::create(plan), {}, false}});
        if (status != Result::Success) {
            m_d->lastRequest = image->lastRequestOutcome();
            return {};
        }
        const auto allocation = m_d->execution.layerTileCoordinates(image->handle(), image->operations().preview(), target,
            m_d->limits.rasterBytes / RgbaTile::byteCount(source->format));
        if (allocation.code != ExecutionCode::Success) {
            m_d->lastRequest = {allocation.code, allocation.diagnostic};
            return {};
        }
        QRect bounds(QPoint(), source->size);
        for (const auto &area : allocation.writtenAreas) bounds |= area;
        return bounds;
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("Parent transform bounds exceeded allocation capacity")};
    } catch (const std::length_error &error) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QString::fromUtf8(error.what())};
    } catch (const std::invalid_argument &error) {
        m_d->lastRequest = {ExecutionCode::InvalidRequest, QString::fromUtf8(error.what())};
    }
    return {};
}

QRect PaintImage::selectionRect() const
{ return onThread() ? m_d->document->selectionRect() : QRect{}; }
bool PaintImage::selectionInverted() const
{ return onThread() && m_d->document->selectionInverted(); }
SelectionKind PaintImage::selectionKind() const
{ return onThread() ? m_d->document->selectionKind() : SelectionKind::Rectangle; }
SelectionMaskSP PaintImage::selectionMask() const
{ return onThread() ? m_d->document->selectionMask() : SelectionMaskSP{}; }
Result PaintImage::addTransparencyMaskFromSelection(LayerId layer)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->addTransparencyMaskFromSelection(layer));
}
Result PaintImage::insertTransparencyMask(LayerId layer, const RasterLayer &source)
{ return onThread() ? publish(m_d->document->insertTransparencyMask(layer, source)) : Result::InvalidInput; }
Result PaintImage::addTransparencyMask(LayerId layer)
{ return addTransparencyMask(layer, QStringLiteral("Transparency Mask")); }
Result PaintImage::addTransparencyMask(LayerId layer, const QString &name)
{
    if (!onThread()) return Result::InvalidInput;
    const auto status = m_d->document->checkMutation();
    if (status != Result::Success) return status;
    const auto version = m_d->document->snapshot();
    if (!version->selection.isEmpty()) {
        constexpr int limit = RgbaTile::CoordinateLimit;
        const QRect clip(-limit, -limit, 2 * limit, 2 * limit);
        const auto source = version->selectionKind == SelectionKind::Freehand ? version->selectionMask :
            version->selectionKind == SelectionKind::Ellipse ? SelectionMask::fromEllipse(version->selection, clip) :
                                                             SelectionMask::fromRectangle(version->selection, clip);
        if (!source) return Result::CapacityExceeded;
        auto mask = source;
        if (version->selectionInverted) {
            auto operation = QSharedPointer<SelectionCombinationSource>::create();
            operation->first = source; operation->second = SelectionMask::empty();
            operation->pixels = SelectionCombinationSource::Pixels::InvertTransparency;
            operation->extent = source->bounds(); operation->tiles = source->tileCoordinates();
            const auto prepared = m_d->execution.evaluateSelectionCombination(operation, &mask);
            if (prepared.code != ExecutionCode::Success) { m_d->lastRequest = prepared; return resultFor(prepared.code); }
        }
        if (m_d->document->snapshot() != version) return Result::StaleVersion;
        return publish(m_d->document->addTransparencyMaskWithin(layer, false, mask, name));
    }
    return publish(m_d->document->addTransparencyMask(layer, name));
}
Result PaintImage::setTransparencyMaskVisible(LayerId layer, bool visible, PropertyHistory history)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->setTransparencyMaskVisible(layer, visible, history));
}
Result PaintImage::setTransparencyMaskLocked(LayerId layer, bool locked, PropertyHistory history)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->setTransparencyMaskLocked(layer, locked, history));
}
Result PaintImage::writeTransparencyMaskVisible(LayerId layer, bool visible)
{ return onThread() ? requestAfterWrite(m_d->document->writeTransparencyMaskVisible(layer, visible)) : Result::InvalidInput; }
Result PaintImage::writeTransparencyMaskLocked(LayerId layer, bool locked)
{ return onThread() ? requestAfterWrite(m_d->document->writeTransparencyMaskLocked(layer, locked)) : Result::InvalidInput; }
Result PaintImage::renameTransparencyMask(LayerId layer, const QString &name, PropertyHistory history)
{ return onThread() ? publish(m_d->document->renameTransparencyMask(layer, name, history)) : Result::InvalidInput; }
Result PaintImage::writeTransparencyMaskName(LayerId layer, const QString &name)
{ return onThread() ? requestAfterWrite(m_d->document->writeTransparencyMaskName(layer, name)) : Result::InvalidInput; }
Result PaintImage::writeTransparencyMaskColorLabel(LayerId layer, int index)
{ return onThread() ? requestAfterWrite(m_d->document->writeTransparencyMaskColorLabel(layer, index)) : Result::InvalidInput; }
Result PaintImage::writeTransparencyMaskPinnedToTimeline(LayerId layer, bool pinned)
{ return onThread() ? requestAfterWrite(m_d->document->writeTransparencyMaskPinnedToTimeline(layer, pinned)) : Result::InvalidInput; }
Result PaintImage::removeTransparencyMask(LayerId layer)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->removeTransparencyMask(layer));
}
SelectionMaskSP PaintImage::layerTransparencyMask(LayerId layer) const
{
    return onThread() ? m_d->document->layerTransparencyMask(layer) : SelectionMaskSP{};
}
bool PaintImage::layerTransparencyMaskVisible(LayerId layer) const
{
    return onThread() && m_d->document->layerTransparencyMaskVisible(layer);
}
void PaintImage::refreshLayerPixelReferences()
{
    const auto version = layerPixelVersion();
    for (auto it = m_d->layerPixels.begin(); it != m_d->layerPixels.end();) {
        const auto reference = it.value().toStrongRef();
        if (!reference) {
            it = m_d->layerPixels.erase(it);
            continue;
        }
        const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
            [&](const auto &layer) { return layer.uuid == reference->m_uuid; });
        if (!reference->m_layer && version->rootUuid == reference->m_uuid) {
            reference->m_version = version;
        } else if (found != version->layers.cend()) {
            reference->m_layer = found->id;
            reference->m_version = version;
        }
        ++it;
    }
}
Snapshot PaintImage::layerPixelVersion() const
{
    const auto preview = m_d->document->preview();
    if (m_d->maskStroke && preview->pendingOperation() == m_d->maskStroke->id) return preview;
    if ((m_d->document->m_translation && preview->pendingOperation() == m_d->document->m_translation->id) ||
        (!m_d->document->m_affine.isEmpty() && preview->pendingOperation() == m_d->document->m_affine.first()->id)) return preview;
    return m_d->document->snapshot();
}
void PaintImage::requestCurrentVersion()
{
    try {
        m_d->lastRequest = m_d->execution.request(m_d->handle, m_d->document->preview());
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("GPU request allocation exceeded capacity")};
    }
    refreshLayerPixelReferences();
}
Result PaintImage::publish(Result result)
{
    if (result == Result::Success) {
        requestCurrentVersion();
        Q_EMIT changed();
    }
    return result;
}
OperationId PaintImage::beginStroke(LayerId layer, const Brush &brush, quint64 seed)
{ return onThread() ? m_d->document->beginStroke(layer, brush, seed) : OperationId{}; }
QSharedPointer<PaintImage> PaintImage::createMaskImage(const Snapshot &version, const SelectionMaskSP &mask)
{
    try {
        const auto coordinates = mask->tileCoordinates();
        if (coordinates.size() > m_d->limits.rasterBytes / RgbaTile::ByteCount) {
            m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("Mask input exceeds its retained tile capacity")};
            return {};
        }
        RasterImage raster;
        raster.size = version->size;
        raster.layers = {{QStringLiteral("Transparency coverage")}};
        if (mask->defaultCoverage()) raster.layers.first().defaultPixel = {255, 255, 255, mask->defaultCoverage()};
        if (!mask->allocatedBlocks().isEmpty()) {
            const auto origin = mask->allocatedBlocks().first().topLeft();
            raster.layers.first().position = {origin.x() % 64, origin.y() % 64};
        }
        PaintImageSP image(new PaintImage(m_d->execution, raster, m_d->limits));
        const auto layer = image->operations().activeLayer();
        const auto accepted = [&](Result result) {
            if (result == Result::Success) return true;
            m_d->lastRequest = {result == Result::CapacityExceeded ? ExecutionCode::CapacityExceeded : ExecutionCode::InvalidRequest,
                QStringLiteral("GPU mask input preparation failed (%1)").arg(int(result))};
            return false;
        };
        {
            Brush copy; copy.blendMode = LayerBlendMode::Copy;
            FigureStyle clear; clear.outline = false; clear.fill = RgbaPixel{};
            clear.fillBlendMode = LayerBlendMode::Copy; clear.ignoreSelection = true;
            Brush white; white.rgba = {255, 255, 255, 255};
            FigureStyle fill; fill.outline = false; fill.fill = white.rgba;
            QRegion allocated;
            for (const auto &block : mask->allocatedBlocks()) allocated += block;
            for (const auto &tile : allocated) {
                const QPointF first(tile.topLeft()), last(tile.topLeft() + QPoint(tile.width(), tile.height()));
                if (!accepted(image->paintFigure(layer, copy, {{first, 1}, {last, 1}},
                    {ShapeKind::Rectangle, first, last, 0}, clear, 0))) return {};
                SelectionMaskSP finite;
                const auto clipped = m_d->execution.combineSelectionCoverage(SelectionMask::empty(), mask,
                    SelectionCombination::Replace, tile, false, &finite);
                if (clipped.code != ExecutionCode::Success) { m_d->lastRequest = clipped; return {}; }
                if (finite->isEmpty()) continue;
                if (!accepted(image->setSelectionMask(finite)) ||
                    !accepted(image->paintFigure(layer, white, {{first, 1}, {last, 1}},
                        {ShapeKind::Rectangle, first, last, 0}, fill, 0))) return {};
            }
        }
        const auto selected = version->selectionKind == SelectionKind::Freehand ? image->setSelectionMask(version->selectionMask, version->selectionInverted) :
            version->selectionKind == SelectionKind::Ellipse ? image->setSelectionEllipse(version->selection, version->selectionInverted) :
                                                             image->setSelectionRect(version->selection, version->selectionInverted);
        return accepted(selected) ? image : PaintImageSP{};
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("Mask input allocation failed")};
    } catch (const std::length_error &error) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QString::fromUtf8(error.what())};
    } catch (const std::invalid_argument &error) {
        m_d->lastRequest = {ExecutionCode::InvalidRequest, QString::fromUtf8(error.what())};
    }
    return {};
}

OperationId PaintImage::beginTransparencyMaskStroke(LayerId layer, const Brush &brush, quint64 seed)
{
    if (!onThread() || !m_d->handle || m_d->maskStroke || m_d->document->hasActiveStroke()) return {};
    const auto version = m_d->document->snapshot();
    const Layer *target = maskPaintLayer(version, layer);
    if (!target) return {};
    const auto image = createMaskImage(version, target->transparencyMask);
    if (!image) return {};
    Brush maskBrush = brush;
    // Mask coverage owns its own alpha; layer channel locks and alpha locks
    // must not prevent the retained mask from changing.
    maskBrush.maskPainting = true;
    maskBrush.alphaLocked = false;
    maskBrush.channelWriteMask = 15;
    const LayerId imageLayer = image->operations().activeLayer();
    const auto id = image->beginStroke(imageLayer, maskBrush, seed);
    if (id.isNull()) return {};
    if (m_d->document->beginExternalStroke(id) != Result::Success) {
        image->cancelStroke(id);
        return {};
    }
    m_d->maskStroke.reset(new MaskStroke{image, layer, imageLayer, id});
    return id;
}
Result PaintImage::appendSamples(OperationId id, const QList<InputSample> &samples)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return refreshMaskStrokePreview(m_d->maskStroke->image->appendSamples(id, samples));
    }
    return publish(m_d->document->appendSamples(id, samples));
}
Result PaintImage::appendPoint(OperationId id, const InputSample &point)
{
    if (!onThread()) return Result::InvalidInput;
    try {
        if (m_d->maskStroke) {
            if (m_d->maskStroke->id != id) return Result::StaleOperation;
            return refreshMaskStrokePreview(m_d->maskStroke->image->appendPoint(id, point));
        }
        return publish(m_d->document->appendPoint(id, point));
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}
Result PaintImage::appendLine(OperationId id, const InputSample &first, const InputSample &last)
{
    if (!onThread()) return Result::InvalidInput;
    try {
        if (m_d->maskStroke) {
            if (m_d->maskStroke->id != id) return Result::StaleOperation;
            return refreshMaskStrokePreview(m_d->maskStroke->image->appendLine(id, first, last));
        }
        return publish(m_d->document->appendLine(id, first, last));
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}
Result PaintImage::appendPainterPrimitives(OperationId id, const QList<InputSample> &samples,
                                          const QList<PathCommand> &commands)
{
    if (!onThread()) return Result::InvalidInput;
    try {
        if (m_d->maskStroke) {
            if (m_d->maskStroke->id != id) return Result::StaleOperation;
            return refreshMaskStrokePreview(m_d->maskStroke->image->appendPainterPrimitives(id, samples, commands));
        }
        return publish(m_d->document->appendPainterPrimitives(id, samples, commands));
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}
Result PaintImage::flushSmoothing(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return refreshMaskStrokePreview(m_d->maskStroke->image->flushSmoothing(id));
    }
    const auto before = m_d->document->preview();
    const auto result = m_d->document->flushSmoothing(id);
    return m_d->document->preview() == before ? result : publish(result);
}
Result PaintImage::updateShape(OperationId id, const Shape &shape)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return refreshMaskStrokePreview(m_d->maskStroke->image->updateShape(id, shape));
    }
    return publish(m_d->document->updateShape(id, shape));
}
Result PaintImage::updateFigure(OperationId id, const Shape &shape, const FigureStyle &style)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return refreshMaskStrokePreview(m_d->maskStroke->image->updateFigure(id, shape, style));
    }
    return publish(m_d->document->updateFigure(id, shape, style));
}
Result PaintImage::paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                               const Shape &shape, const FigureStyle &style, quint64 randomSeed)
{ return onThread() ? publish(m_d->document->paintFigure(layer, brush, samples, shape, style, randomSeed)) : Result::InvalidInput; }

Result PaintImage::paintTransparencyMaskFigure(LayerId layer, const Brush &brush,
                                               const QList<InputSample> &samples, const Shape &shape,
                                               const FigureStyle &style, quint64 randomSeed)
{
    return paintTransparencyMaskFigure(layer, brush, samples, shape, style, randomSeed, false);
}

Result PaintImage::paintTransparencyMaskFigure(LayerId layer, const Brush &brush,
                                               const QList<InputSample> &samples, const Shape &shape,
                                               const FigureStyle &style, quint64 randomSeed, bool filled)
{
    if (!onThread()) return Result::InvalidInput;
    if (!m_d->handle) return Result::Closed;
    if (m_d->maskStroke || m_d->document->hasActiveStroke()) return Result::EditInProgress;
    const auto version = m_d->document->snapshot();
    const Layer *target = maskPaintLayer(version, layer);
    if (!target) return Result::LayerUnavailable;
    const auto image = createMaskImage(version, target->transparencyMask);
    if (!image) return resultFor(m_d->lastRequest.code);
    Brush maskBrush = brush;
    maskBrush.maskPainting = true;
    maskBrush.alphaLocked = false;
    maskBrush.channelWriteMask = 15;
    const LayerId imageLayer = image->operations().activeLayer();
    const auto result = image->paintFigure(imageLayer, maskBrush, samples, shape, style, randomSeed);
    if (result != Result::Success) return result;
    return replaceMaskFromImage(layer, *image, imageLayer, filled);
}

Result PaintImage::fillTransparencyMask(LayerId layer, const std::array<quint8, 3> &rgb,
                                        qreal opacity, bool erase, bool alphaLocked)
{
    return fillTransparencyMask(layer, rgb, opacity, erase, alphaLocked, LayerBlendMode::Normal);
}

Result PaintImage::fillTransparencyMask(LayerId layer, const std::array<quint8, 3> &rgb,
                                        qreal opacity, bool erase, bool alphaLocked, LayerBlendMode mode)
{
    Q_UNUSED(alphaLocked);
    Brush brush;
    brush.blendMode = mode;
    brush.rgba = {rgb[0], rgb[1], rgb[2], 255};
    brush.opacity = opacity;
    brush.erase = erase;
    brush.channelWriteMask = 15;
    FigureStyle style;
    style.outline = false;
    style.fillBlendMode = mode;
    style.fill = brush.rgba;
    const QSize size = m_d->document->snapshot()->size;
    const QPointF end(size.width(), size.height());
    return paintTransparencyMaskFigure(layer, brush, {{{0, 0}, 1}, {end, 1}},
                                       {ShapeKind::Rectangle, {0, 0}, end, 0}, style, 0, true);
}

Result PaintImage::fillTransparencyMaskPattern(LayerId layer, const PatternFill &pattern,
                                               qreal opacity, bool erase, bool alphaLocked)
{
    return fillTransparencyMaskPattern(layer, pattern, opacity, erase, alphaLocked, LayerBlendMode::Normal);
}

Result PaintImage::fillTransparencyMaskPattern(LayerId layer, const PatternFill &pattern,
                                               qreal opacity, bool erase, bool alphaLocked, LayerBlendMode mode)
{
    Q_UNUSED(alphaLocked);
    if (!pattern.isValid()) return Result::InvalidInput;
    Brush brush;
    brush.rgba = {0, 0, 0, 255};
    brush.blendMode = mode;
    brush.opacity = opacity;
    brush.erase = erase;
    brush.channelWriteMask = 15;
    FigureStyle style;
    style.outline = false;
    style.fillBlendMode = mode;
    style.pattern = pattern;
    const QSize size = m_d->document->snapshot()->size;
    const QPointF end(size.width(), size.height());
    return paintTransparencyMaskFigure(layer, brush, {{{0, 0}, 1}, {end, 1}},
                                       {ShapeKind::Rectangle, {0, 0}, end, 0}, style, 0, true);
}

Result PaintImage::replaceTransparencyMask(LayerId layer, const SelectionMaskSP &mask)
{
    if (!onThread()) return Result::InvalidInput;
    return publish(m_d->document->replaceTransparencyMask(layer, mask));
}
Result PaintImage::writeTransparencyMask(LayerId layer, const SelectionMaskSP &mask)
{ return onThread() ? publish(m_d->document->writeTransparencyMask(layer, mask)) : Result::InvalidInput; }

Result PaintImage::replaceMaskFromImage(LayerId layer, const PaintImage &source, LayerId sourceLayer, bool filled)
{
    if (!onThread()) return Result::InvalidInput;
    SelectionMaskSP mask;
    const auto read = m_d->execution.paintedSelectionCoverage(source.handle(), source.operations().snapshot(), sourceLayer, &mask);
    if (read.code != ExecutionCode::Success) { m_d->lastRequest = read; return resultFor(read.code); }
    return publish(m_d->document->replaceTransparencyMask(layer, mask, filled));
}

Result PaintImage::refreshMaskStrokePreview(Result result)
{
    if (result != Result::Success) return result;
    auto &session = *m_d->maskStroke;
    SelectionMaskSP mask;
    const auto coverage = m_d->execution.paintedSelectionCoverage(session.image->handle(), session.image->operations().preview(),
        session.imageLayer, &mask);
    if (coverage.code != ExecutionCode::Success) { m_d->lastRequest = coverage; return resultFor(coverage.code); }
    return publish(m_d->document->updateExternalMask(session.id, session.layer, mask));
}

Result PaintImage::commitMaskStroke()
{
    auto &session = *m_d->maskStroke;
    if (!session.committed) {
        const auto committed = session.image->commitStroke(session.id);
        if (committed != Result::Success) return committed;
        session.committed = true;
    }
    SelectionMaskSP mask;
    const auto coverage = m_d->execution.paintedSelectionCoverage(session.image->handle(), session.image->operations().snapshot(),
        session.imageLayer, &mask);
    if (coverage.code != ExecutionCode::Success) { m_d->lastRequest = coverage; return resultFor(coverage.code); }
    const auto result = m_d->document->commitExternalMask(session.id, session.layer, mask);
    if (result == Result::Success) m_d->maskStroke.reset();
    return publish(result);
}

Result PaintImage::commitStroke(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        return commitMaskStroke();
    }
    return publish(m_d->document->commitStroke(id));
}
Result PaintImage::cancelStroke(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    if (m_d->maskStroke) {
        if (m_d->maskStroke->id != id) return Result::StaleOperation;
        std::unique_ptr<MaskStroke> session = std::move(m_d->maskStroke);
        const auto cancelled = session->committed ? Result::Success : session->image->cancelStroke(id);
        m_d->document->endExternalStroke(id);
        return publish(cancelled);
    }
    return publish(m_d->document->cancelStroke(id));
}
OperationId PaintImage::beginAffineTransform(const QList<AffineTransformTarget> &layers, QRect copyRect, QRect clearRect)
{ return onThread() ? m_d->document->beginAffineTransform(layers, copyRect, clearRect) : OperationId{}; }
Result PaintImage::updateAffineTransform(OperationId id, const QList<AffineLayerPlan> &plans,
                                       const SelectionMaskSP &selection, bool inverted)
{
    if (!onThread()) return Result::InvalidInput;
    auto &document = *m_d->document;
    const auto previousPreview = document.m_preview;
    const auto previousOperations = document.m_affine;
    const auto previousRevision = document.m_nextRevision;
    auto result = document.updateAffineTransform(id, plans, selection, inverted);
    if (result != Result::Success) return result;
    const auto draft = document.preview();
    if (!document.m_preview) return publish(Result::Success);
    // Admission evaluates canonical pixels with summary-only reads before any
    // view or external pixel reference can observe the new version.
    QSharedPointer<DocumentVersion> admitted;
    auto operations = document.m_affine;
    try {
        admitted.reset(new DocumentVersion(*draft));
        for (qsizetype i = 0; i < operations.size(); ++i) {
            const auto &operation = operations[i];
            if (!operation->transformPixels) continue;
            const auto layerIndex = document.layerIndex(operation->layer);
            const auto &edits = draft->layers[layerIndex].operations;
            const auto appended = edits.isEmpty() ? nullptr : std::get_if<AffineSnapshot>(&edits.last());
            if (!appended || *appended != operation) continue;
            const auto source = operation->selection ?
                m_d->execution.readSelectedBounds(m_d->handle, document.snapshot(), *operation->selection, true) :
                m_d->execution.readBounds(m_d->handle, document.snapshot(), operation->layer, true);
            result = resultFor(source.code);
            if (result != Result::Success || !source.bounds) break;
            auto content = *source.bounds;
            constexpr int limit = RgbaTile::CoordinateLimit;
            const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
            for (const auto &pass : operation->plan->passes) {
                content = affineOutputRegion(pass, content);
                if (!content.isEmpty() && !domain.contains(content)) { result = Result::InvalidInput; break; }
            }
            if (result != Result::Success) break;
            QList<QRect> blocks;
            const auto evaluated = m_d->execution.readNonDefaultBlocks(m_d->handle, draft, operation->layer, blocks);
            result = resultFor(evaluated.code);
            if (result == Result::Success && operation->selection) {
                const auto captured = m_d->execution.readAffineSelectionBounds(m_d->handle, document.snapshot(), *operation);
                result = resultFor(captured.code);
                if (result == Result::Success) {
                    const auto old = m_d->execution.layerTileCoordinates(m_d->handle, document.snapshot(), operation->layer,
                        m_d->limits.rasterBytes / RgbaTile::byteCount(draft->format));
                    result = resultFor(old.code);
                    if (result == Result::Success) {
                        const auto index = document.layerIndex(operation->layer);
                        const auto origin = draft->layers[index].position;
                        const auto aligned = [](QRect area, QPoint origin) {
                            if (area.isEmpty()) return QRect{};
                            area.translate(-origin);
                            const auto floor = [](int value) { return value / 64 - (value % 64 < 0); };
                            return QRect(QPoint(floor(area.left()) * 64, floor(area.top()) * 64) + origin,
                                QPoint((floor(area.right()) + 1) * 64 - 1, (floor(area.bottom()) + 1) * 64 - 1) + origin);
                        };
                        QRegion allocated, previous;
                        for (const auto &area : blocks) allocated += area;
                        for (const auto &area : old.writtenAreas) previous += area;
                        allocated += previous.subtracted(aligned(operation->selection->clearRect, origin));
                        const auto sourceExtent = aligned(*captured.bounds, operation->plan->positionDelta);
                        allocated += aligned(sourceExtent, origin);
                        blocks.clear(); for (const auto &area : allocated) blocks.append(area);
                    }
                }
            }
            if (result == Result::Success) {
                auto frozen = QSharedPointer<AffineTransformOperation>::create(*operation);
                frozen->allocatedBlocks = std::move(blocks);
                const auto index = document.layerIndex(operation->layer);
                admitted->layers[index].operations.last() = AffineSnapshot(frozen);
                operations[i] = frozen;
            }
            if (result != Result::Success) break;
        }
    } catch (const std::bad_alloc &) { result = Result::CapacityExceeded; }
    if (result != Result::Success) {
        document.m_preview = previousPreview; document.m_affine = previousOperations;
        document.m_nextRevision = previousRevision;
        return result;
    }
    document.m_preview = admitted; document.m_affine = operations;
    return publish(Result::Success);
}
Result PaintImage::commitAffineTransform(OperationId id)
{ return onThread() ? publish(m_d->document->commitAffineTransform(id)) : Result::InvalidInput; }
Result PaintImage::cancelAffineTransform(OperationId id)
{ return onThread() ? publish(m_d->document->cancelAffineTransform(id)) : Result::InvalidInput; }
OperationId PaintImage::beginLayerTranslation(LayerId layer)
{ return onThread() ? m_d->document->beginLayerTranslation(layer) : OperationId{}; }
OperationId PaintImage::beginLayerTranslation(const QList<LayerTranslationTarget> &targets)
{ return onThread() ? m_d->document->beginLayerTranslation(targets) : OperationId{}; }
OperationId PaintImage::beginSelectionTranslation(LayerId layer, QRect copyRect, QRect clearRect)
{
    if (!onThread()) return {};
    const auto token = m_d->document->beginSelectionTranslation(layer, copyRect, clearRect);
    if (token.isNull()) return {};
    const auto bounds = m_d->execution.readSelectedBounds(m_d->handle, m_d->document->snapshot(),
                                                         *m_d->document->m_translation);
    if (bounds.code != ExecutionCode::Success || !bounds.bounds || bounds.bounds->isEmpty() ||
        !m_d->handle || m_d->document->activeOperation() != token) {
        m_d->document->cancelLayerTranslation(token);
        return {};
    }
    m_d->selectionTranslationHandles = *bounds.bounds;
    const auto sourceBounds = m_d->document->snapshot()->format == PixelFormat::Rgba16 ?
        m_d->execution.readSelectedBounds(m_d->handle, m_d->document->snapshot(), *m_d->document->m_translation, true) : bounds;
    if (sourceBounds.code != ExecutionCode::Success || !sourceBounds.bounds ||
        !m_d->handle || m_d->document->activeOperation() != token) {
        m_d->document->cancelLayerTranslation(token);
        return {};
    }
    m_d->selectionTranslationBounds = *sourceBounds.bounds;
    return token;
}
QRect PaintImage::selectionTranslationBounds(OperationId id) const
{
    return onThread() && m_d->document->m_translation && m_d->document->m_translation->id == id &&
        m_d->document->m_translation->selectedPixels ? m_d->selectionTranslationHandles : QRect();
}
Result PaintImage::updateLayerTranslation(OperationId id, QPoint offset)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    if (document.m_closed) return Result::Closed;
    if (!document.m_translation || document.m_translation->id != id) return Result::StaleOperation;
    constexpr int limit = RgbaTile::CoordinateLimit;
    const QRect domain(-limit, -limit, 2 * limit, 2 * limit);
    const auto version = document.snapshot();
    if (document.m_translation->selectedPixels) {
        const auto bounds = m_d->selectionTranslationBounds;
        if (qint64(bounds.left()) + offset.x() < -limit || qint64(bounds.right()) + offset.x() >= limit ||
            qint64(bounds.top()) + offset.y() < -limit || qint64(bounds.bottom()) + offset.y() >= limit)
            return Result::InvalidInput;
        SelectionMaskSP selection;
        if (version->selectionMask && version->selectionMask->isGpuGenerated() && !offset.isNull()) {
            selection = translatedSelectionMask(version->selectionMask, offset, domain);
            if (!selection) return m_d->lastRequest.code == ExecutionCode::Busy ? Result::Busy : resultFor(m_d->lastRequest.code);
        }
        return publish(m_d->document->updateLayerTranslationWithMasks(id, offset, {}, selection));
    }
    for (const auto &target : document.m_translationTargets) {
        const auto layer = std::find_if(version->layers.cbegin(), version->layers.cend(),
            [&](const auto &value) { return value.id == target.layer; });
        if (!target.pixels || layer->kind != LayerKind::Paint) continue;
        const auto admitted = checkLayerTranslation(version, target.layer, offset);
        if (admitted != Result::Success) return admitted;
    }
    QHash<LayerId, SelectionMaskSP> masks;
    for (const auto &target : document.m_translationTargets) {
        const auto mask = document.layerTransparencyMask(target.layer);
        if (!target.mask || !mask || (!mask->isGpuGenerated() && !mask->defaultCoverage()) || mask->isEmpty() || offset.isNull()) continue;
        const auto bounds = mask->bounds();
        if (qint64(bounds.left()) + offset.x() < -limit || qint64(bounds.right()) + offset.x() >= limit ||
            qint64(bounds.top()) + offset.y() < -limit || qint64(bounds.bottom()) + offset.y() >= limit)
            return Result::InvalidInput;
        const auto moved = translatedSelectionMask(mask, offset, domain);
        if (!moved) return m_d->lastRequest.code == ExecutionCode::Busy ? Result::Busy : resultFor(m_d->lastRequest.code);
        masks.insert(target.layer, moved);
    }
    return publish(m_d->document->updateLayerTranslationWithMasks(id, offset, masks));
}
Result PaintImage::commitLayerTranslation(OperationId id)
{ return onThread() ? publish(m_d->document->commitLayerTranslation(id)) : Result::InvalidInput; }
Result PaintImage::cancelLayerTranslation(OperationId id)
{ return onThread() ? publish(m_d->document->cancelLayerTranslation(id)) : Result::InvalidInput; }
Result PaintImage::writeLayerPosition(LayerId layer, QPoint position)
{ return applyLayerPosition(layer, position, nullptr); }

Result PaintImage::writeLayerPosition(const LayerPixelReferenceSP &pixels, QPoint position)
{
    if (!onThread() || !pixels || m_d->layerPixels.value(pixels->m_uuid).toStrongRef() != pixels)
        return Result::InvalidInput;
    return applyLayerPosition(pixels->m_layer, position, &pixels->m_version);
}

Result PaintImage::applyLayerPosition(LayerId layer, QPoint position, Snapshot *retainedVersion)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    const auto source = retainedVersion ? *retainedVersion : document.snapshot();
    if (!source || source->documentId() != document.snapshot()->documentId()) return Result::StaleVersion;
    bool available = false;
    for (const auto &value : source->layers) if (value.id == layer) available = true;
    if (!available) return Result::LayerUnavailable;
    for (const auto &version : document.layerPositionVersions(source, layer)) {
        for (const auto &value : version->layers) {
            if (value.id != layer) continue;
            const qint64 x = qint64(position.x()) - value.position.x();
            const qint64 y = qint64(position.y()) - value.position.y();
            constexpr qint64 limit = 2 * RgbaTile::CoordinateLimit;
            if (x < -limit || x > limit || y < -limit || y > limit) return Result::InvalidInput;
            if (value.kind == LayerKind::Group) continue;
            const auto admitted = checkLayerTranslation(version, layer, QPoint(int(x), int(y)));
            if (admitted != Result::Success) return admitted;
        }
    }
    return requestAfterWrite(m_d->document->applyLayerPosition(layer, position, retainedVersion));
}

Result PaintImage::checkLayerTranslation(const Snapshot &version, LayerId layer, QPoint offset) const
{
    if (offset.isNull()) return Result::Success;
    constexpr qint64 limit = RgbaTile::CoordinateLimit;
    if (qAbs(qint64(offset.x())) > 2 * limit || qAbs(qint64(offset.y())) > 2 * limit) return Result::InvalidInput;
    const auto content = m_d->execution.readBounds(m_d->handle, version, layer, true);
    if (content.code == ExecutionCode::CapacityExceeded) return Result::CapacityExceeded;
    if (content.code == ExecutionCode::Busy) return Result::Busy;
    if (content.code != ExecutionCode::Success) return Result::DeviceFailure;
    const auto bounds = *content.bounds;
    if (!bounds.isEmpty() && (qint64(bounds.left()) + offset.x() < -limit ||
        qint64(bounds.right()) + offset.x() >= limit || qint64(bounds.top()) + offset.y() < -limit ||
        qint64(bounds.bottom()) + offset.y() >= limit)) return Result::InvalidInput;
    return Result::Success;
}
Result PaintImage::replaceTiles(LayerId layer, const QList<RgbaTile> &tiles)
{ return onThread() ? publish(m_d->document->replaceTiles(layer, tiles)) : Result::InvalidInput; }
LayerId PaintImage::addLayer(const QString &name)
{
    if (!onThread()) return 0;
    return addLayer(name, m_d->document->snapshot()->layers.size(), false);
}

LayerId PaintImage::addLayer(const QString &name, qsizetype index, bool pinnedToTimeline, const QUuid &parentUuid)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->addLayer(name, index, pinnedToTimeline, parentUuid);
    if (id) publish(Result::Success);
    return id;
}
LayerId PaintImage::insertLayer(const OperationDocument &source, LayerId layer,
                               const RasterLayer &properties, qsizetype index, bool activate)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->insertLayer(source, layer, properties, index, activate);
    if (id) publish(Result::Success);
    return id;
}

LayerId PaintImage::insertLayer(const PaintImage &source, const LayerPixelReferenceSP &pixels,
                               const RasterLayer &properties, qsizetype index, bool activate)
{
    if (!onThread() || !source.onThread() || !pixels || !pixels->version() ||
        pixels->version()->documentId() != source.operations().snapshot()->documentId()) return 0;
    const auto id = m_d->document->insertVersion(pixels->version(), pixels->layer(), properties, index, activate);
    if (id) publish(Result::Success);
    return id;
}

Result PaintImage::insertLayers(const QList<LayerInsertion> &layers, qsizetype index, bool activate,
                                bool clearPixelSelection)
{ return onThread() ? publish(m_d->document->insertLayers(layers, index, activate, clearPixelSelection)) : Result::InvalidInput; }

Result PaintImage::duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource)
{ return onThread() ? publish(m_d->document->duplicateLayers(layers, names, activeSource)) : Result::InvalidInput; }
Result PaintImage::clearLayers(const QList<LayerId> &layers)
{ return onThread() ? publish(m_d->document->clearLayers(layers)) : Result::InvalidInput; }
Result PaintImage::clearSelectedPixels(const QList<LayerId> &layers)
{ return onThread() ? publish(m_d->document->clearSelectedPixels(layers)) : Result::InvalidInput; }
Result PaintImage::cropLayer(LayerId layer, const QRect &rect)
{
    if (!onThread()) return Result::InvalidInput;
    const auto status = m_d->document->checkMutation();
    if (status != Result::Success) return status;
    const auto before = m_d->document->snapshot();
    SelectionMaskSP mask;
    const auto source = m_d->document->layerTransparencyMask(layer);
    if (source && (source->isGpuGenerated() || source->defaultCoverage())) {
        mask = translatedSelectionMask(source, {}, rect);
        if (!mask) return resultFor(m_d->lastRequest.code);
    }
    if (m_d->document->snapshot() != before) return Result::StaleVersion;
    return publish(m_d->document->cropLayerWithMask(layer, rect, mask));
}
Result PaintImage::resizeCanvas(const QRect &rect, bool cropLayers)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    const auto status = document.checkMutation();
    if (status != Result::Success) return status;
    if (!Detail::validCanvasRect(rect)) return Result::InvalidInput;
    const auto before = document.snapshot();
    if (!cropLayers) {
        for (const auto &layer : before->layers) {
            if (layer.kind == LayerKind::Group) continue;
            const auto admitted = checkLayerTranslation(before, layer.id, -rect.topLeft());
            if (admitted != Result::Success) return admitted;
        }
    }
    QHash<LayerId, SelectionMaskSP> masks;
    for (const auto &layer : before->layers) {
        if (!layer.transparencyMask || (!layer.transparencyMask->isGpuGenerated() && !layer.transparencyMask->defaultCoverage())) continue;
        const auto mask = translatedSelectionMask(layer.transparencyMask, -rect.topLeft(), QRect(QPoint(), rect.size()));
        if (!mask) return resultFor(m_d->lastRequest.code);
        masks.insert(layer.id, mask);
    }
    if (document.snapshot() != before) return Result::StaleVersion;
    const auto result = m_d->document->resizeCanvasWithMasks(rect, cropLayers, masks);
    return document.snapshot() == before ? result : publish(result);
}
Result PaintImage::fillLayer(LayerId layer, const std::array<quint8, 3> &rgb,
                             qreal opacity, bool erase, bool alphaLocked,
                             LayerBlendMode blendMode)
{ return onThread() ? publish(m_d->document->fillLayer(layer, rgb, opacity, erase, alphaLocked, blendMode)) : Result::InvalidInput; }
Result PaintImage::fillLayerColor(LayerId layer, const RgbaPixel &color,
                                  qreal opacity, bool erase, bool alphaLocked,
                                  LayerBlendMode blendMode)
{ return onThread() ? publish(m_d->document->fillLayerColor(layer, color, opacity, erase, alphaLocked, blendMode)) : Result::InvalidInput; }
Result PaintImage::fillLayerPattern(LayerId layer, const PatternFill &pattern,
                                    qreal opacity, bool erase, bool alphaLocked,
                                    LayerBlendMode blendMode)
{ return onThread() ? publish(m_d->document->fillLayerPattern(layer, pattern, opacity, erase, alphaLocked, blendMode)) : Result::InvalidInput; }

Result PaintImage::fillRegion(LayerId layer, const RegionFillSettings &settings, const RgbaPixel &color,
                            const std::optional<PatternFill> &pattern, qreal opacity, bool erase,
                            bool alphaLocked, LayerBlendMode blendMode)
{
    if (!onThread()) return Result::InvalidInput;
    try {
        RegionFillSP region;
        const auto prepared = m_d->document->prepareRegionFill(layer, settings, &region);
        if (prepared != Result::Success) return prepared;
        QRect bounds;
        const auto checked = m_d->execution.checkRegionFill(m_d->handle, region, &bounds);
        if (checked.code != ExecutionCode::Success) {
            m_d->lastRequest = checked;
            return resultFor(checked.code);
        }
        if (bounds.isEmpty()) return Result::EmptyOperation;
        auto bounded = QSharedPointer<RegionFill>::create(*region);
        bounded->bounds = bounds;
        return publish(m_d->document->fillRegion(layer, bounded, color, pattern, opacity, erase, alphaLocked, blendMode));
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}
Result PaintImage::selectRegion(LayerId reference, const RegionFillSettings &settings, SelectionCombination action)
{
    if (!onThread() || action < SelectionCombination::Replace || action > SelectionCombination::SymmetricDifference)
        return Result::InvalidInput;
    try {
        auto input = settings;
        input.purpose = RegionCoveragePurpose::Selection;
        if (input.reference == RegionFillReference::CurrentLayer) input.reference = RegionFillReference::CurrentProjection;
        RegionFillSP region;
        const auto prepared = m_d->document->prepareRegionFill(reference, input, &region);
        if (prepared != Result::Success) return prepared;
        QRect bounds;
        SelectionMaskSP selection;
        const auto checked = m_d->execution.checkRegionFill(m_d->handle, region, &bounds, &selection);
        if (checked.code != ExecutionCode::Success) {
            m_d->lastRequest = checked;
            return resultFor(checked.code);
        }
        if (region->preparedFor.toStrongRef() != m_d->document->snapshot()) return Result::StaleVersion;
        return combineSelectionMask(selection, action);
    } catch (const std::bad_alloc &) {
        return Result::CapacityExceeded;
    }
}

Result PaintImage::invertColors(LayerId layer)
{ return onThread() ? publish(m_d->document->invertColors(layer)) : Result::InvalidInput; }
Result PaintImage::grayscale(LayerId layer)
{ return onThread() ? publish(m_d->document->grayscale(layer)) : Result::InvalidInput; }
Result PaintImage::hsvAdjustment(LayerId layer, const HsvAdjustment &settings)
{ return onThread() ? publish(m_d->document->hsvAdjustment(layer, settings)) : Result::InvalidInput; }
Result PaintImage::selectLayers(const QList<LayerId> &layers, LayerId active)
{ return onThread() ? publish(m_d->document->selectLayers(layers, active)) : Result::InvalidInput; }
Result PaintImage::selectLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->selectLayer(layer)) : Result::InvalidInput; }
Result PaintImage::setLayerVisible(LayerId layer, bool visible)
{ return onThread() ? publish(m_d->document->setLayerVisible(layer, visible)) : Result::InvalidInput; }
Result PaintImage::setLayerVisible(LayerId layer, bool visible, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerVisible(layer, visible, history)) : Result::InvalidInput; }
Result PaintImage::setLayerOpacity(LayerId layer, quint8 opacity)
{ return onThread() ? publish(m_d->document->setLayerOpacity(layer, opacity)) : Result::InvalidInput; }
Result PaintImage::setLayerOpacity(LayerId layer, quint8 opacity, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerOpacity(layer, opacity, history)) : Result::InvalidInput; }
Result PaintImage::setLayerLocked(LayerId layer, bool locked)
{ return onThread() ? publish(m_d->document->setLayerLocked(layer, locked)) : Result::InvalidInput; }
Result PaintImage::setLayerLocked(LayerId layer, bool locked, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerLocked(layer, locked, history)) : Result::InvalidInput; }
Result PaintImage::setLayerChannelMask(LayerId layer, quint8 mask, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerChannelMask(layer, mask, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerChannelMask(LayerId layer, quint8 mask)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerChannelMask(layer, mask)) : Result::InvalidInput; }
Result PaintImage::setLayerProjectionMask(LayerId layer, quint8 mask, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerProjectionMask(layer, mask, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerProjectionMask(LayerId layer, quint8 mask)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerProjectionMask(layer, mask)) : Result::InvalidInput; }
Result PaintImage::setLayerPassThrough(LayerId layer, bool enabled, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerPassThrough(layer, enabled, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerPassThrough(LayerId layer, bool enabled)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerPassThrough(layer, enabled)) : Result::InvalidInput; }
Result PaintImage::setLayerBlendMode(LayerId layer, LayerBlendMode mode, PropertyHistory history)
{ return onThread() ? publish(m_d->document->setLayerBlendMode(layer, mode, history)) : Result::InvalidInput; }
Result PaintImage::writeLayerBlendMode(LayerId layer, LayerBlendMode mode)
{ return onThread() ? requestAfterWrite(m_d->document->writeLayerBlendMode(layer, mode)) : Result::InvalidInput; }
Result PaintImage::renameLayer(LayerId layer, const QString &name)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name)) : Result::InvalidInput; }
Result PaintImage::renameLayer(LayerId layer, const QString &name, PropertyHistory history)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name, history)) : Result::InvalidInput; }
Result PaintImage::moveLayer(LayerId layer, qsizetype index)
{ return onThread() ? publish(m_d->document->moveLayer(layer, index)) : Result::InvalidInput; }
Result PaintImage::reorderLayers(const QList<LayerId> &order)
{ return onThread() ? publish(m_d->document->reorderLayers(order)) : Result::InvalidInput; }
Result PaintImage::moveLayers(const QList<LayerId> &layers, const QUuid &parentUuid, const QUuid &aboveUuid,
                              LayerId activeRoot)
{
    if (!onThread()) return Result::InvalidInput;
    const auto before = m_d->document->snapshot();
    const auto result = m_d->document->moveLayers(layers, parentUuid, aboveUuid, activeRoot);
    return m_d->document->snapshot() == before ? result : publish(result);
}
Result PaintImage::groupLayers(const QList<LayerId> &layers, LayerId activeRoot, const QString &groupName,
                              const QString &clippingLayerName)
{ return onThread() ? publish(m_d->document->groupLayers(layers, activeRoot, groupName, clippingLayerName)) : Result::InvalidInput; }
Result PaintImage::ungroupLayers(const QList<LayerId> &layers, LayerId activeRoot, const QString &replacementName)
{ return onThread() ? publish(m_d->document->ungroupLayers(layers, activeRoot, replacementName)) : Result::InvalidInput; }
Result PaintImage::mergeLayers(const LayerMergeRequest &request)
{ return onThread() ? publish(m_d->document->mergeLayers(request)) : Result::InvalidInput; }
Result PaintImage::removeLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->removeLayer(layer)) : Result::InvalidInput; }
Result PaintImage::removeLayers(const QList<LayerId> &layers, const QString &replacementName)
{ return onThread() ? publish(m_d->document->removeLayers(layers, replacementName)) : Result::InvalidInput; }
Result PaintImage::undo() { return onThread() ? publish(m_d->document->undo()) : Result::InvalidInput; }
Result PaintImage::redo() { return onThread() ? publish(m_d->document->redo()) : Result::InvalidInput; }
Result PaintImage::setHistoryIndex(int index)
{ return onThread() ? publish(m_d->document->setHistoryIndex(index)) : Result::InvalidInput; }
void PaintImage::close()
{
    if (!onThread() || !m_d->handle) return;
    if (m_d->maskStroke) {
        std::unique_ptr<MaskStroke> session = std::move(m_d->maskStroke);
        session->image->cancelStroke(session->id);
        m_d->document->endExternalStroke(session->id);
    }
    m_d->execution.close(m_d->handle);
    m_d->handle = 0;
    m_d->document->close();
    refreshLayerPixelReferences();
    Q_EMIT changed();
}
Result PaintImage::markSaved(const Snapshot &version)
{
    if (!onThread()) return Result::InvalidInput;
    const auto result = m_d->document->markSaved(version);
    if (result == Result::Success) Q_EMIT savedStateChanged();
    return result;
}

}
