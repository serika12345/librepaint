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
    const auto version = m_d->document->snapshot();
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
    const auto version = m_d->document->snapshot();
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
Result PaintImage::updateShape(OperationId id, const Shape &shape)
{ return onThread() ? publish(m_d->document->updateShape(id, shape)) : Result::InvalidInput; }
Result PaintImage::commitStroke(OperationId id)
{ return onThread() ? publish(m_d->document->commitStroke(id)) : Result::InvalidInput; }
Result PaintImage::cancelStroke(OperationId id)
{ return onThread() ? publish(m_d->document->cancelStroke(id)) : Result::InvalidInput; }
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
Result PaintImage::duplicateLayers(const QList<LayerId> &layers, const QList<QString> &names, LayerId activeSource)
{ return onThread() ? publish(m_d->document->duplicateLayers(layers, names, activeSource)) : Result::InvalidInput; }
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
