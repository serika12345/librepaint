/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintImage_p.h"
#include <QThread>
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
Result PaintImage::writeProjectionBackground(const std::array<quint8, 4> &rgba)
{
    return onThread() ? requestAfterWrite(m_d->document->writeProjectionBackground(rgba)) : Result::InvalidInput;
}
Result PaintImage::requestAfterWrite(Result result)
{
    if (result == Result::Success) requestCurrentVersion();
    return result;
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
LayerPixelReferenceSP PaintImage::retainLayerPixels(LayerId layer)
{
    if (!onThread() || !m_d->handle) return {};
    const auto version = layerPixelVersion();
    const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [layer](const auto &value) { return value.id == layer; });
    if (found == version->layers.cend()) return {};
    auto reference = m_d->layerPixels.value(found->uuid).toStrongRef();
    if (!reference) {
        reference.reset(new LayerPixelReference(layer, found->uuid, version));
        m_d->layerPixels.insert(found->uuid, reference);
    }
    return reference;
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
        if (found != version->layers.cend()) {
            reference->m_layer = found->id;
            reference->m_version = version;
        }
        ++it;
    }
}
Snapshot PaintImage::layerPixelVersion() const
{
    const auto preview = m_d->document->preview();
    if (!preview->pendingOperation().isNull()) {
        for (const auto &layer : preview->layers) {
            if (layer.operations.isEmpty()) continue;
            const auto translation = std::get_if<TranslationSnapshot>(&layer.operations.last());
            if (translation && (*translation)->id == preview->pendingOperation()) return preview;
        }
    }
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
Result PaintImage::appendSamples(OperationId id, const QList<InputSample> &samples)
{ return onThread() ? publish(m_d->document->appendSamples(id, samples)) : Result::InvalidInput; }
Result PaintImage::flushSmoothing(OperationId id)
{
    if (!onThread()) return Result::InvalidInput;
    const auto before = m_d->document->preview();
    const auto result = m_d->document->flushSmoothing(id);
    return m_d->document->preview() == before ? result : publish(result);
}
Result PaintImage::updateShape(OperationId id, const Shape &shape)
{ return onThread() ? publish(m_d->document->updateShape(id, shape)) : Result::InvalidInput; }
Result PaintImage::updateFigure(OperationId id, const Shape &shape, const FigureStyle &style)
{ return onThread() ? publish(m_d->document->updateFigure(id, shape, style)) : Result::InvalidInput; }
Result PaintImage::paintFigure(LayerId layer, const Brush &brush, const QList<InputSample> &samples,
                               const Shape &shape, const FigureStyle &style, quint64 randomSeed)
{ return onThread() ? publish(m_d->document->paintFigure(layer, brush, samples, shape, style, randomSeed)) : Result::InvalidInput; }
Result PaintImage::commitStroke(OperationId id)
{ return onThread() ? publish(m_d->document->commitStroke(id)) : Result::InvalidInput; }
Result PaintImage::cancelStroke(OperationId id)
{ return onThread() ? publish(m_d->document->cancelStroke(id)) : Result::InvalidInput; }
OperationId PaintImage::beginLayerTranslation(LayerId layer)
{ return onThread() ? m_d->document->beginLayerTranslation(layer) : OperationId{}; }
Result PaintImage::updateLayerTranslation(OperationId id, QPoint offset)
{
    if (!onThread()) return Result::InvalidInput;
    const auto &document = *m_d->document;
    if (document.m_closed) return Result::Closed;
    if (!document.m_translation || document.m_translation->id != id) return Result::StaleOperation;
    const auto admitted = checkLayerTranslation(document.snapshot(), document.m_translation->layer, offset);
    return admitted == Result::Success ? publish(m_d->document->updateLayerTranslation(id, offset)) : admitted;
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

LayerId PaintImage::addLayer(const QString &name, qsizetype index, bool pinnedToTimeline)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->addLayer(name, index, pinnedToTimeline);
    if (id) publish(Result::Success);
    return id;
}
LayerId PaintImage::insertLayer(const OperationDocument &source, LayerId layer,
                               const RasterLayer &properties, qsizetype index)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->insertLayer(source, layer, properties, index);
    if (id) publish(Result::Success);
    return id;
}

LayerId PaintImage::insertLayer(const PaintImage &source, const LayerPixelReferenceSP &pixels,
                               const RasterLayer &properties, qsizetype index)
{
    if (!onThread() || !source.onThread() || !pixels || !pixels->version() ||
        pixels->version()->documentId() != source.operations().snapshot()->documentId()) return 0;
    const auto id = m_d->document->insertVersion(pixels->version(), pixels->layer(), properties, index);
    if (id) publish(Result::Success);
    return id;
}

Result PaintImage::duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource)
{ return onThread() ? publish(m_d->document->duplicateLayers(layers, names, activeSource)) : Result::InvalidInput; }
Result PaintImage::clearLayers(const QList<LayerId> &layers)
{ return onThread() ? publish(m_d->document->clearLayers(layers)) : Result::InvalidInput; }
Result PaintImage::cropLayer(LayerId layer, const QRect &rect)
{ return onThread() ? publish(m_d->document->cropLayer(layer, rect)) : Result::InvalidInput; }
Result PaintImage::fillLayer(LayerId layer, const std::array<quint8, 3> &rgb,
                             quint8 opacity, bool erase, bool alphaLocked)
{ return onThread() ? publish(m_d->document->fillLayer(layer, rgb, opacity, erase, alphaLocked)) : Result::InvalidInput; }
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
Result PaintImage::renameLayer(LayerId layer, const QString &name)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name)) : Result::InvalidInput; }
Result PaintImage::renameLayer(LayerId layer, const QString &name, PropertyHistory history)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name, history)) : Result::InvalidInput; }
Result PaintImage::moveLayer(LayerId layer, qsizetype index)
{ return onThread() ? publish(m_d->document->moveLayer(layer, index)) : Result::InvalidInput; }
Result PaintImage::reorderLayers(const QList<LayerId> &order)
{ return onThread() ? publish(m_d->document->reorderLayers(order)) : Result::InvalidInput; }
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

Result PaintImage::restoreRecovery(const QByteArray &data)
{
    auto &d = *m_d;
    if (!onThread()) return Result::InvalidRecovery;
    if (!d.handle) return Result::Closed;
    auto document = std::make_unique<OperationDocument>(QSize(1, 1), d.limits);
    if (document->restoreRecovery(data) != Result::Success) return Result::InvalidRecovery;
    const auto replaced = d.execution.replaceDocument(d.handle, document->preview());
    if (replaced.code != ExecutionCode::Success)
        return replaced.code == ExecutionCode::CapacityExceeded ? Result::CapacityExceeded : Result::InvalidRecovery;
    d.document.swap(document);
    d.lastRequest = replaced;
    refreshLayerPixelReferences();
    Q_EMIT changed();
    return Result::Success;
}

}
