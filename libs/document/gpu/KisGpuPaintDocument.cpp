/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintDocument_p.h"
#include <QThread>
#include <QPointer>
#include <stdexcept>

namespace LibrePaint::Document::Gpu {
using namespace Image::Gpu;
namespace {
Execution &executionOfOpenImage(const PaintImageSP &image)
{
    if (!image || !image->handle() || QThread::currentThread() != image->thread())
        throw std::invalid_argument("Document adoption requires an open image on its owning thread");
    return image->execution();
}
}
PaintDocument::PaintDocument(PaintImageSP image, Limits limits)
    : PaintDocument(executionOfOpenImage(image), image, limits) {}
PaintDocument::PaintDocument(Execution &execution, QSize size, Limits limits)
    : PaintDocument(execution, PaintImageSP::create(execution, size, limits), limits) {}
PaintDocument::PaintDocument(Execution &execution, const RasterImage &image, Limits limits)
    : PaintDocument(execution, PaintImageSP::create(execution, image, limits), limits) {}
PaintDocument::PaintDocument(Execution &execution, PaintImageSP image, Limits limits)
    : m_d(new Private{execution, std::move(image), limits, {}, {}, {}})
{
    connect(m_d->image.data(), &PaintImage::changed, this, &PaintDocument::changed);
}
PaintDocument::~PaintDocument() = default;
PaintImageSP PaintDocument::image() const { return m_d->image; }
QString PaintDocument::filePath() const { return m_d->path; }
bool PaintDocument::onThread() const { return QThread::currentThread() == thread(); }
void PaintDocument::close()
{ if (onThread()) m_d->image->close(); }

FileOutcome PaintDocument::adoptSavedFile(const QString &path, const Snapshot &version)
{
    if (!onThread()) return {FileCode::InvalidData, {}, QStringLiteral("Saved files require the document thread")};
    const auto image = m_d->image;
    if (!image->handle()) return {FileCode::Closed, {}, QStringLiteral("The painting document is closed")};
    if (path.isEmpty() || !version || !version->isCommitted() ||
        version->documentId() != image->operations().snapshot()->documentId())
        return {FileCode::InvalidData, {}, QStringLiteral("Saved files require a committed version of this document and a path")};
    m_d->path = path;
    const QPointer<PaintDocument> guard(this);
    image->markSaved(version);
    if (guard) Q_EMIT changed();
    return {FileCode::Success, version, {}};
}
}
