/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintDocument_p.h"
#include <QThread>
#include <stdexcept>

namespace Krita::Document::Gpu {
using namespace Image::Gpu;
PaintDocument::PaintDocument(Execution &execution, QSize size, Limits limits)
    : PaintDocument(execution, std::make_unique<OperationDocument>(size, limits), limits) {}
PaintDocument::PaintDocument(Execution &execution, const RasterImage &image, Limits limits)
    : PaintDocument(execution, std::make_unique<OperationDocument>(image, limits), limits) {}
PaintDocument::PaintDocument(Execution &execution, std::unique_ptr<OperationDocument> document, Limits limits)
    : m_d(new Private{execution, std::move(document), limits, 0, {}, {}, {}})
{
    const auto admitted = execution.open(m_d->document->snapshot());
    if (admitted.code == ExecutionCode::CapacityExceeded) throw std::length_error(admitted.diagnostic.toStdString());
    if (admitted.code != ExecutionCode::Success) throw std::invalid_argument(admitted.diagnostic.toStdString());
    m_d->handle = admitted.handle;
}
PaintDocument::~PaintDocument()
{
    if (m_d->handle) m_d->execution.close(m_d->handle);
}
const OperationDocument &PaintDocument::operations() const { return *m_d->document; }
quint64 PaintDocument::handle() const { return m_d->handle; }
QString PaintDocument::filePath() const { return m_d->path; }
ExecutionOutcome PaintDocument::lastRequestOutcome() const { return m_d->lastRequest; }
bool PaintDocument::onThread() const { return QThread::currentThread() == thread(); }
Result PaintDocument::publish(Result result)
{
    if (result == Result::Success) {
        m_d->lastRequest = m_d->execution.request(m_d->handle, m_d->document->preview());
        Q_EMIT changed();
    }
    return result;
}
OperationId PaintDocument::beginStroke(LayerId layer, const Brush &brush, quint64 seed)
{ return onThread() ? m_d->document->beginStroke(layer, brush, seed) : OperationId{}; }
Result PaintDocument::appendSamples(OperationId id, const QList<InputSample> &samples)
{ return onThread() ? publish(m_d->document->appendSamples(id, samples)) : Result::InvalidInput; }
Result PaintDocument::updateShape(OperationId id, const Shape &shape)
{ return onThread() ? publish(m_d->document->updateShape(id, shape)) : Result::InvalidInput; }
Result PaintDocument::commitStroke(OperationId id)
{ return onThread() ? publish(m_d->document->commitStroke(id)) : Result::InvalidInput; }
Result PaintDocument::cancelStroke(OperationId id)
{ return onThread() ? publish(m_d->document->cancelStroke(id)) : Result::InvalidInput; }
Result PaintDocument::replaceTiles(LayerId layer, const QList<RgbaTile> &tiles)
{ return onThread() ? publish(m_d->document->replaceTiles(layer, tiles)) : Result::InvalidInput; }
LayerId PaintDocument::addLayer(const QString &name)
{
    if (!onThread()) return 0;
    const auto id = m_d->document->addLayer(name);
    if (id) publish(Result::Success);
    return id;
}
Result PaintDocument::selectLayer(LayerId layer)
{ return onThread() ? publish(m_d->document->selectLayer(layer)) : Result::InvalidInput; }
Result PaintDocument::setLayerVisible(LayerId layer, bool visible)
{ return onThread() ? publish(m_d->document->setLayerVisible(layer, visible)) : Result::InvalidInput; }
Result PaintDocument::undo() { return onThread() ? publish(m_d->document->undo()) : Result::InvalidInput; }
Result PaintDocument::redo() { return onThread() ? publish(m_d->document->redo()) : Result::InvalidInput; }
void PaintDocument::close()
{
    if (!onThread() || !m_d->handle) return;
    m_d->execution.close(m_d->handle);
    m_d->handle = 0;
    m_d->document->close();
    Q_EMIT changed();
}
}
