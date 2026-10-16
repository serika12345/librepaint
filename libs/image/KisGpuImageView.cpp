/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_image.h"
#include "KisNodeAdditionFlags.h"
#include "kis_paint_layer.h"
#include "kis_paint_device.h"
#include "gpu/KisGpuPaintImage.h"
#include <KoColorSpaceRegistry.h>
#include <KoChannelInfo.h>
#include <QHash>
#include <QSet>
#include <QThread>
#include <stdexcept>
#include <utility>

namespace {
void synchronizeGpuLayerViews(KisImage *view, QHash<QUuid, KisNodeSP> *retainedNodes);
}

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
    QHash<QUuid, KisNodeSP> nodes;
    for (auto node = root()->firstChild(); node; node = node->nextSibling())
        nodes.insert(node->uuid(), node);
    connect(image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this,
            [view = this, synchronized = image->operations().snapshot(),
             historyIndex = image->operations().undoCount(), nodes = std::move(nodes)]() mutable {
        KisImageSP retainedView = view;
        const auto owner = retainedView->gpuImage();
        const auto version = owner->operations().snapshot();
        if (version != synchronized) {
            bool undoneRemoval = false;
            if (version->documentId() == synchronized->documentId()) {
                for (int index = owner->operations().undoCount() + 1; index <= historyIndex; ++index)
                    undoneRemoval |= owner->operations().historyAction(index) == LibrePaint::Image::Gpu::HistoryAction::RemoveLayer;
            }
            QSet<QUuid> previousLayers;
            if (undoneRemoval)
                for (const auto &layer : synchronized->layers) previousLayers.insert(layer.uuid);
            synchronizeGpuLayerViews(retainedView.data(), &nodes);
            synchronized = retainedView->gpuImage()->operations().snapshot();
            historyIndex = retainedView->gpuImage()->operations().undoCount();
            if (undoneRemoval && synchronized == version) {
                KisNodeList restored;
                KisNodeSP active;
                for (const auto &layer : synchronized->layers) {
                    if (!previousLayers.contains(layer.uuid)) restored.append(nodes.value(layer.uuid));
                    if (layer.id == synchronized->activeLayer) active = nodes.value(layer.uuid);
                }
                if (!restored.isEmpty()) Q_EMIT retainedView->sigRequestNodeReselection(active, restored);
            }
            Q_EMIT retainedView->sigImageModified();
        }
        retainedView->notifyProjectionUpdated(retainedView->bounds());
    });
}

void KisImage::synchronizeGpuNodes()
{ synchronizeGpuLayerViews(this, nullptr); }

namespace {
void synchronizeGpuLayerViews(KisImage *view, QHash<QUuid, KisNodeSP> *retainedNodes)
{
    for (;;) {
        const auto owner = view->gpuImage();
        const auto version = owner->operations().snapshot();
        const auto changed = [&] { return view->gpuImage() != owner || owner->operations().snapshot() != version; };
        QHash<QUuid, KisNodeSP> existing = retainedNodes ? *retainedNodes : QHash<QUuid, KisNodeSP>();
        QSet<QUuid> desired;
        for (const auto &layer : version->layers) desired.insert(layer.uuid);
        for (auto node = view->root()->firstChild(); node;) {
            auto next = node->nextSibling();
            if (desired.contains(node->uuid())) existing.insert(node->uuid(), node);
            else view->removeNode(node);
            node = next;
        }
        if (view->gpuImage() != owner) return;
        if (changed()) continue;
        for (qsizetype index = 0; index < version->layers.size(); ++index) {
            const auto &layer = version->layers[index];
            KisNodeSP node = existing.value(layer.uuid);
            if (!node) {
                node = new KisPaintLayer(view, layer.name, layer.opacity,
                                         KisPaintDevice::fromGpuImage(owner, layer.id, {}, view->colorSpace()));
                node->setUuid(layer.uuid);
            }
            existing.insert(layer.uuid, node);
            if (node->name() != layer.name) node->setName(layer.name);
            if (changed()) break;
            node->setVisible(layer.visible);
            if (changed()) break;
            if (node->opacity() != layer.opacity) node->setOpacity(layer.opacity);
            if (changed()) break;
            if (node->userLocked() != layer.userLocked) node->setUserLocked(layer.userLocked);
            if (changed()) break;
            auto *paintLayer = qobject_cast<KisPaintLayer*>(node.data());
            QBitArray paintFlags;
            if (layer.channelWriteMask != 15) {
                const auto channels = paintLayer->colorSpace()->channels();
                paintFlags.resize(4);
                for (int channel = 0; channel < 4; ++channel)
                    paintFlags.setBit(channel, layer.channelWriteMask & (1u << channels[channel]->displayPosition()));
            }
            if (paintLayer->channelLockFlags() != paintFlags) paintLayer->setChannelLockFlags(paintFlags);
            if (changed()) break;
            if (node->isPinnedToTimeline() != layer.pinnedToTimeline) node->setPinnedToTimeline(layer.pinnedToTimeline);
            if (changed()) break;
            if (view->root()->at(index) != node) {
                if (node->parent()) view->removeNode(node);
                const auto above = index ? view->root()->at(index - 1) : KisNodeSP();
                const auto flags = layer.id == version->activeLayer
                    ? KisNodeAdditionFlags(KisNodeAdditionFlag::None)
                    : KisNodeAdditionFlags(KisNodeAdditionFlag::DontActivateNode);
                view->addNode(node, view->root(), above, flags);
            }
        }
        if (view->gpuImage() != owner) return;
        if (changed()) continue;
        if (retainedNodes) {
            QHash<QUuid, KisNodeSP> retained;
            for (const auto &state : owner->operations().historyVersions()) {
                for (const auto &layer : state->layers) {
                    const auto node = existing.value(layer.uuid);
                    if (node) retained.insert(layer.uuid, node);
                }
            }
            retainedNodes->swap(retained);
        }
        return;
    }
}
}
