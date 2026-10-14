/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_image.h"
#include "kis_paint_layer.h"
#include "kis_paint_device.h"
#include "gpu/KisGpuPaintImage.h"
#include <KoColorSpaceRegistry.h>
#include <QHash>
#include <QSet>
#include <QThread>
#include <stdexcept>

KisImageSP KisImage::fromGpuImage(LibrePaint::Image::Gpu::PaintImageSP image, const QString &name)
{ return fromGpuImage(image, name, KoColorSpaceRegistry::instance()->rgb8()); }

KisImageSP KisImage::fromGpuImage(LibrePaint::Image::Gpu::PaintImageSP image, const QString &name, const KoColorSpace *colorSpace)
{
    if (!image || !image->handle() || QThread::currentThread() != image->thread())
        throw std::invalid_argument("GPU image construction requires an open image on its owning thread");
    if (!colorSpace || colorSpace->colorModelId().id() != QLatin1String("RGBA") ||
        colorSpace->colorDepthId().id() != QLatin1String("U8"))
        throw std::invalid_argument("GPU image views require the validated RGBA8/sRGB encoding");
    KisImageSP result = new KisImage(image, name, colorSpace);
    result->synchronizeGpuNodes();
    result->connectGpuImageUpdates();
    return result;
}

QSharedPointer<LibrePaint::Image::Gpu::PaintImage> KisImage::cloneGpuImage(bool exactCopy) const
{
    const auto owner = gpuImage();
    if (!owner) return {};
    auto result = owner->clone(exactCopy);
    if (!result) throw std::invalid_argument("GPU image cloning requires the owning thread");
    return result;
}

void KisImage::connectGpuImageUpdates()
{
    const auto image = gpuImage();
    connect(image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this,
            [view = this, synchronized = image->operations().snapshot()]() mutable {
        KisImageSP retainedView = view;
        const auto version = retainedView->gpuImage()->operations().snapshot();
        if (version != synchronized) {
            retainedView->synchronizeGpuNodes();
            synchronized = version;
            Q_EMIT retainedView->sigImageModified();
        }
        retainedView->notifyProjectionUpdated(retainedView->bounds());
    });
}

void KisImage::synchronizeGpuNodes()
{
    const auto owner = gpuImage();
    const auto version = owner->operations().snapshot();
    QHash<QUuid, KisNodeSP> existing;
    QSet<QUuid> desired;
    for (const auto &layer : version->layers) desired.insert(layer.uuid);
    for (auto node = root()->firstChild(); node;) {
        auto next = node->nextSibling();
        if (desired.contains(node->uuid())) existing.insert(node->uuid(), node);
        else removeNode(node);
        node = next;
    }
    for (qsizetype index = 0; index < version->layers.size(); ++index) {
        const auto &layer = version->layers[index];
        KisNodeSP node = existing.value(layer.uuid);
        if (!node) {
            node = new KisPaintLayer(this, layer.name, layer.opacity,
                                     KisPaintDevice::fromGpuImage(owner, layer.id, {}, colorSpace()));
            node->setUuid(layer.uuid);
        }
        if (node->name() != layer.name) node->setName(layer.name);
        node->setVisible(layer.visible);
        if (node->opacity() != layer.opacity) node->setOpacity(layer.opacity);
        if (node->userLocked() != layer.userLocked) node->setUserLocked(layer.userLocked);
        if (root()->at(index) != node) {
            if (node->parent()) removeNode(node);
            addNode(node, root(), quint32(index));
        }
    }
}
