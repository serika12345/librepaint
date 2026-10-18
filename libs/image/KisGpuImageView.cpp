/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_image.h"
#include "KisNodeAdditionFlags.h"
#include "kis_paint_layer.h"
#include "kis_paint_device.h"
#include "kis_default_bounds.h"
#include "gpu/KisGpuPaintImage.h"
#include <KoColorSpaceRegistry.h>
#include <KoChannelInfo.h>
#include <KoColor.h>
#include <KoCompositeOpRegistry.h>
#include <KisRegion.h>
#include <QHash>
#include <QSet>
#include <QScopedValueRollback>
#include <QThread>
#include <stdexcept>
#include <utility>
#include <algorithm>

struct KisGpuImageViewState
{
    LibrePaint::Image::Gpu::Snapshot synchronized;
    QHash<QUuid, KisNodeSP> nodes;
};

bool KisImage::insertGpuLayer(KisNodeSP node, qsizetype index)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto owner = gpuImage();
    const auto state = gpuViewState();
    auto *layer = qobject_cast<KisPaintLayer*>(node.data());
    if (!owner || !state || QThread::currentThread() != owner->thread() || !owner->handle() || owner->operations().hasActiveStroke() ||
        !layer || node->parent() || node->childCount() || index < 0 ||
        layer->compositeOpId() != COMPOSITE_OVER ||
        node->isAnimated() || !node->keyframeChannels().isEmpty() || layer->layerStyle() ||
        layer->temporary() || layer->hasTemporaryTarget() ||
        index > owner->operations().snapshot()->layers.size() ||
        *layer->colorSpace() != *colorSpace() || node->uuid().isNull() || state->nodes.contains(node->uuid())) return false;
    const auto projectionFlags = layer->channelFlags();
    if (!projectionFlags.isEmpty() && (projectionFlags.size() != 4 || projectionFlags.count(true) != 4)) return false;
    const auto pixels = layer->paintDevice();
    Gpu::RasterLayer properties;
    properties.name = node->name();
    properties.uuid = node->uuid();
    properties.visible = node->visible();
    properties.opacity = node->opacity();
    properties.userLocked = node->userLocked();
    properties.pinnedToTimeline = node->isPinnedToTimeline();
    properties.position = {node->x(), node->y()};
    const auto flags = layer->channelLockFlags();
    if (!flags.isEmpty()) {
        properties.channelWriteMask = 0;
        const auto channels = layer->colorSpace()->channels();
        for (int channel = 0; channel < 4; ++channel)
            if (flags.testBit(channel)) properties.channelWriteMask |= 1u << channels[channel]->displayPosition();
    }
    const auto background = pixels->defaultPixel();
    const auto *bgra = background.data();
    properties.defaultPixel = {bgra[2], bgra[1], bgra[0], bgra[3]};
    const auto source = pixels->gpuPixelImage();
    const auto reference = pixels->gpuPixelReference();
    std::unique_ptr<Gpu::OperationDocument> imported;
    if (!source) {
        const auto budget = owner->limits().rasterBytes - owner->operations().snapshot()->rasterBytes;
        QSet<QPoint> coordinates;
        const auto tileIndex = [](int value) { return int(value >= 0 ? qint64(value) / 256 : (qint64(value) - 255) / 256); };
        const auto collect = [&](const KisRegion &region) {
            for (const auto &rect : region.rects()) {
                for (int y = tileIndex(rect.top()); y <= tileIndex(rect.bottom()); ++y) {
                    for (int x = tileIndex(rect.left()); x <= tileIndex(rect.right()); ++x) {
                        coordinates.insert({x, y});
                        if (coordinates.size() > budget / Gpu::RgbaTile::ByteCount) return false;
                    }
                }
            }
            return true;
        };
        if (!collect(pixels->regionForLodSyncing())) return false;
        // Pin full-resolution storage and read raw pixels independently of view modes.
        KisPaintDevice fixed(*pixels);
        fixed.setDefaultBounds(new KisDefaultBounds());
        if (*fixed.colorSpace() != *colorSpace()) return false;
        coordinates.clear();
        if (!collect(fixed.region())) return false;
        const auto fixedBackground = fixed.defaultPixel();
        const auto *fixedBgra = fixedBackground.data();
        properties.defaultPixel = {fixedBgra[2], fixedBgra[1], fixedBgra[0], fixedBgra[3]};
        auto ordered = coordinates.values();
        std::sort(ordered.begin(), ordered.end(), [](QPoint left, QPoint right) {
            return left.y() == right.y() ? left.x() < right.x() : left.y() < right.y();
        });
        Gpu::RasterImage input;
        input.size = owner->operations().snapshot()->size;
        input.layers = {properties};
        try {
            imported = std::make_unique<Gpu::OperationDocument>(input, owner->limits());
        } catch (const std::invalid_argument &) {
            return false;
        }
        QList<Gpu::RgbaTile> batch;
        const auto flush = [&] {
            if (batch.isEmpty()) return true;
            const bool accepted = imported->replaceTiles(imported->activeLayer(), batch) == Gpu::Result::Success;
            batch.clear();
            return accepted;
        };
        for (const auto &coordinate : ordered) {
            QByteArray bytes(Gpu::RgbaTile::ByteCount, Qt::Uninitialized);
            fixed.readBytes(reinterpret_cast<quint8*>(bytes.data()), coordinate.x() * 256, coordinate.y() * 256, 256, 256);
            for (qsizetype offset = 0; offset < bytes.size(); offset += 4) std::swap(bytes[offset], bytes[offset + 2]);
            batch.append({coordinate, std::move(bytes)});
            if (batch.size() == 64 && !flush()) return false;
        }
        if (!flush()) return false;
    }
    KisImageSP retained = this;
    state->nodes.insert(node->uuid(), node);
    const auto id = source ? owner->insertLayer(*source, reference, properties, index)
                           : owner->insertLayer(*imported, imported->activeLayer(), properties, index);
    if (!id) state->nodes.remove(node->uuid());
    return id != 0;
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
    auto state = QSharedPointer<KisGpuImageViewState>::create();
    state->synchronized = image->operations().snapshot();
    for (auto node = root()->firstChild(); node; node = node->nextSibling())
        state->nodes.insert(node->uuid(), node);
    gpuViewState() = state;
    connect(image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this,
            [view = this, state]() {
        auto &synchronized = state->synchronized;
        auto &nodes = state->nodes;
        KisImageSP retainedView = view;
        const auto owner = retainedView->gpuImage();
        const auto version = owner->operations().snapshot();
        if (version != synchronized) {
            const bool selectionChanged = version->activeLayer != synchronized->activeLayer ||
                version->selectedLayers != synchronized->selectedLayers;
            {
                QScopedValueRollback<bool> syncing(retainedView->m_synchronizingGpuNodes, true);
                retainedView->synchronizeGpuLayerViews(state.data());
            }
            synchronized = retainedView->gpuImage()->operations().snapshot();
            if (selectionChanged && synchronized == version) {
                KisNodeList selected;
                KisNodeSP active;
                for (const auto &layer : synchronized->layers) {
                    if (synchronized->selectedLayers.contains(layer.id)) selected.append(nodes.value(layer.uuid));
                    if (layer.id == synchronized->activeLayer) active = nodes.value(layer.uuid);
                }
                if (active) Q_EMIT retainedView->sigRequestNodeReselection(active, selected);
            }
            Q_EMIT retainedView->sigImageModified();
        }
        retainedView->notifyProjectionUpdated(retainedView->bounds());
    });
}

void KisImage::synchronizeGpuNodes()
{
    QScopedValueRollback<bool> syncing(m_synchronizingGpuNodes, true);
    synchronizeGpuLayerViews(nullptr);
}

void KisImage::synchronizeGpuLayerViews(KisGpuImageViewState *state)
{
    auto *view = this;
    auto *retainedNodes = state ? &state->nodes : nullptr;
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
            node->paintDevice()->bindGpuLayer(owner, layer.id);
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
