/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintImage_p.h"
#include <QThread>
#include <new>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
PaintImage::PaintImage(Execution &execution, QSize size, Limits limits)
    : PaintImage(execution, std::make_unique<OperationDocument>(size, limits), limits) {}
PaintImage::PaintImage(Execution &execution, const RasterImage &image, Limits limits)
    : PaintImage(execution, std::make_unique<OperationDocument>(image, limits), limits) {}
PaintImage::PaintImage(Execution &execution, std::unique_ptr<OperationDocument> document, Limits limits)
    : m_d(new Private{execution, std::move(document), limits})
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
{ return cloneTo(m_d->execution, preserveLayerUuids); }

PaintImageSP PaintImage::cloneTo(Execution &destination, bool preserveLayerUuids) const
{
    if (!onThread()) return {};
    return PaintImageSP(new PaintImage(destination, m_d->document->cloneCurrent(preserveLayerUuids), m_d->limits));
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
void PaintImage::requestCurrentVersion()
{
    try {
        m_d->lastRequest = m_d->execution.request(m_d->handle, m_d->document->preview());
    } catch (const std::bad_alloc &) {
        m_d->lastRequest = {ExecutionCode::CapacityExceeded, QStringLiteral("GPU request allocation exceeded capacity")};
    }
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
    const auto id = m_d->document->addLayer(name);
    if (id) publish(Result::Success);
    return id;
}
Result PaintImage::selectLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->selectLayer(layer)) : Result::InvalidInput; }
Result PaintImage::setLayerVisible(LayerId layer, bool visible)
{ return onThread() ? publish(m_d->document->setLayerVisible(layer, visible)) : Result::InvalidInput; }
Result PaintImage::renameLayer(LayerId layer, const QString &name)
{ return onThread() ? publish(m_d->document->renameLayer(layer, name)) : Result::InvalidInput; }
Result PaintImage::moveLayer(LayerId layer, qsizetype index)
{ return onThread() ? publish(m_d->document->moveLayer(layer, index)) : Result::InvalidInput; }
Result PaintImage::removeLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->removeLayer(layer)) : Result::InvalidInput; }
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
    Q_EMIT changed();
    return Result::Success;
}

}
