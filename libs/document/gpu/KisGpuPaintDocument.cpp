/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintDocument_p.h"
#include <QThread>

namespace Krita::Document::Gpu {
using namespace Image::Gpu;
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
}
