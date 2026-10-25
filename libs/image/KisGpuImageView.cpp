/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_image.h"
#include "KisNodeAdditionFlags.h"
#include "KisImageSignals.h"
#include "kis_paint_layer.h"
#include "kis_group_layer.h"
#include "kis_paint_device.h"
#include "kis_default_bounds.h"
#include "kis_selection.h"
#include "kis_selection_mask.h"
#include "kis_pixel_selection.h"
#include "KisImageResolutionProxy.h"
#include "kis_pointer_utils.h"
#include <kis_meta_data_store.h>
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
#include <cstdlib>

struct KisGpuImageViewState
{
    LibrePaint::Image::Gpu::Snapshot synchronized;
    quint64 previewRevision = 0;
    QHash<QUuid, KisNodeSP> nodes;
};

namespace {
qsizetype gpuInsertionIndex(KisImage *image, const LibrePaint::Image::Gpu::Snapshot &version,
                           KisNodeSP parent, qsizetype index)
{
    if (!parent || parent->image() != image || index < 0 || index > parent->childCount()) return -1;
    if (parent != image->root()) {
        const auto group = std::find_if(version->layers.cbegin(), version->layers.cend(),
            [&](const auto &layer) { return layer.uuid == parent->uuid() &&
                layer.kind == LibrePaint::Image::Gpu::LayerKind::Group; });
        if (!parent->parent() || group == version->layers.cend()) return -1;
    }
    KisNodeSP anchor = index < parent->childCount() ? parent->at(index) : parent->lastChild();
    const bool after = anchor && index == parent->childCount();
    if (!anchor) {
        if (parent == image->root()) return version->layers.size();
        anchor = parent;
    }
    const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [&](const auto &layer) { return layer.uuid == anchor->uuid(); });
    return found == version->layers.cend() ? -1 : std::distance(version->layers.cbegin(), found) + after;
}
}

KisNodeSP KisImage::addGpuPaintLayer(const QString &name, KisNodeSP parent, qsizetype index, bool pinnedToTimeline)
{
    const auto owner = gpuImage();
    const auto state = gpuViewState();
    if (!owner || !state || QThread::currentThread() != owner->thread()) return {};
    const auto insertion = gpuInsertionIndex(this, owner->operations().snapshot(), parent, index);
    if (insertion < 0) return {};
    KisImageSP retained = this;
    const auto id = owner->addLayer(name, insertion, pinnedToTimeline, parent == root() ? QUuid() : parent->uuid());
    if (!id || retained->gpuImage() != owner) return {};
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.id == id) return state->nodes.value(layer.uuid);
    return {};
}

bool KisImage::insertGpuLayer(KisNodeSP node, qsizetype index, KisNodeSP parent)
{
    return node && !node->childCount() && insertGpuLayers({node}, index, parent);
}

bool KisImage::insertGpuLayers(const KisNodeList &roots, qsizetype index, KisNodeSP parent)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto owner = gpuImage();
    const auto state = gpuViewState();
    if (!parent) parent = root();
    if (!owner || !state || roots.isEmpty() || QThread::currentThread() != owner->thread() ||
        !owner->handle() || owner->operations().hasActiveStroke()) return false;
    const auto insertion = gpuInsertionIndex(this, owner->operations().snapshot(), parent, index);
    if (insertion < 0) return false;
    KisNodeList nodes = roots;
    for (const auto &node : roots) if (!node || node->parent()) return false;
    QSet<QUuid> identities;
    for (qsizetype i = 0; i < nodes.size(); ++i) {
        const auto node = nodes[i];
        if (node->uuid().isNull() || state->nodes.contains(node->uuid()) || identities.contains(node->uuid()) ||
            nodes.size() > owner->limits().layerCount - owner->operations().snapshot()->layers.size()) return false;
        identities.insert(node->uuid());
        for (auto child = node->firstChild(); child; child = child->nextSibling()) nodes.append(child);
    }
    QList<Gpu::LayerInsertion> prepared;
    qsizetype preparedBytes = 0;
    for (KisNodeSP node : nodes) {
        auto *layer = qobject_cast<KisLayer*>(node.data());
        auto *paint = qobject_cast<KisPaintLayer*>(node.data());
        auto *group = qobject_cast<KisGroupLayer*>(node.data());
        if (!layer || (!paint && !group) || (paint && node->childCount()) ||
            (layer->compositeOpId() != COMPOSITE_OVER && layer->compositeOpId() != COMPOSITE_MULT && layer->compositeOpId() != COMPOSITE_SCREEN &&
             layer->compositeOpId() != COMPOSITE_DARKEN && layer->compositeOpId() != COMPOSITE_LIGHTEN &&
             layer->compositeOpId() != COMPOSITE_ADD && layer->compositeOpId() != COMPOSITE_LINEAR_DODGE) ||
            node->isAnimated() || !node->keyframeChannels().isEmpty() || layer->layerStyle() ||
            layer->temporary() || (paint && paint->hasTemporaryTarget()) ||
            *layer->colorSpace() != *colorSpace()) return false;
        const auto projectionFlags = layer->channelFlags();
        if (!projectionFlags.isEmpty() && projectionFlags.size() != 4) return false;
        const auto pixels = group ? group->original() : paint->paintDevice();
        Gpu::RasterLayer properties;
        properties.kind = group ? Gpu::LayerKind::Group : Gpu::LayerKind::Paint;
        properties.passThrough = group && group->passThroughMode();
        properties.blendMode = layer->compositeOpId() == COMPOSITE_MULT ? Gpu::LayerBlendMode::Multiply :
                    layer->compositeOpId() == COMPOSITE_SCREEN ? Gpu::LayerBlendMode::Screen :
                    layer->compositeOpId() == COMPOSITE_DARKEN ? Gpu::LayerBlendMode::Darken :
                    layer->compositeOpId() == COMPOSITE_LIGHTEN ? Gpu::LayerBlendMode::Lighten :
                    layer->compositeOpId() == COMPOSITE_ADD ? Gpu::LayerBlendMode::Addition :
                    layer->compositeOpId() == COMPOSITE_LINEAR_DODGE ? Gpu::LayerBlendMode::LinearDodge : Gpu::LayerBlendMode::Normal;
        properties.name = node->name();
        properties.uuid = node->uuid();
        properties.parentUuid = roots.contains(node) ? (parent == root() ? QUuid() : parent->uuid()) : node->parent()->uuid();
        properties.visible = node->visible();
        properties.opacity = node->opacity();
        properties.userLocked = node->userLocked();
        properties.pinnedToTimeline = node->isPinnedToTimeline();
        properties.colorLabelIndex = node->colorLabelIndex();
        if (!projectionFlags.isEmpty()) {
            properties.projectionChannelMask = 0;
            const auto channels = layer->colorSpace()->channels();
            for (int channel = 0; channel < 4; ++channel)
                if (projectionFlags.testBit(channel)) properties.projectionChannelMask |= 1u << channels[channel]->displayPosition();
        }
        properties.position = {node->x(), node->y()};
        const auto flags = paint ? paint->channelLockFlags() : QBitArray();
        if (!flags.isEmpty()) {
            if (flags.size() != 4) return false;
            properties.channelWriteMask = 0;
            const auto channels = layer->colorSpace()->channels();
            for (int channel = 0; channel < 4; ++channel)
                if (flags.testBit(channel)) properties.channelWriteMask |= 1u << channels[channel]->displayPosition();
        }
        const auto background = pixels->defaultPixel();
        const auto *bgra = background.data();
        properties.defaultPixel = {bgra[2], bgra[1], bgra[0], bgra[3]};
        if (group) properties.defaultPixel = {};
        const auto source = pixels->gpuPixelImage();
        const auto reference = pixels->gpuPixelReference();
        std::unique_ptr<Gpu::OperationDocument> imported;
        if (!source && group) {
            if (!node->childCount() && ((bgra[0] || bgra[1] || bgra[2] || bgra[3]) ||
                !pixels->regionForLodSyncing().isEmpty())) return false;
            Gpu::RasterImage input;
            input.size = owner->operations().snapshot()->size;
            input.layers = {properties};
            input.layers.first().parentUuid = QUuid();
            try {
                imported = std::make_unique<Gpu::OperationDocument>(input, owner->limits());
            } catch (const std::invalid_argument &) {
                return false;
            }
        } else if (!source) {
            const auto budget = owner->limits().rasterBytes - owner->operations().snapshot()->rasterBytes - preparedBytes;
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
            input.layers.first().parentUuid = QUuid();
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
        const auto version = imported ? imported->snapshot() : reference ? reference->version() : Gpu::Snapshot();
        const auto sourceId = imported ? imported->activeLayer() : reference ? reference->layer() : Gpu::LayerId(0);
        if (!version || (!imported && (!source || QThread::currentThread() != source->thread() ||
            version->documentId() != source->operations().snapshot()->documentId()))) return false;
        const auto found = std::find_if(version->layers.cbegin(), version->layers.cend(),
            [&](const auto &layer) { return layer.id == sourceId; });
        if (found == version->layers.cend()) return false;
        for (const auto &operation : found->operations) {
            if (const auto tiles = std::get_if<Gpu::TileWriteSnapshot>(&operation)) {
                const auto bytes = (*tiles)->tiles.size() * Gpu::RgbaTile::ByteCount;
                if (bytes > owner->limits().rasterBytes - owner->operations().snapshot()->rasterBytes - preparedBytes) return false;
                preparedBytes += bytes;
            }
        }
        prepared.append({version, sourceId, properties});
    }
    KisImageSP retained = this;
    for (const auto &node : nodes) state->nodes.insert(node->uuid(), node);
    const auto result = owner->insertLayers(prepared, insertion);
    if (result != Gpu::Result::Success)
        for (const auto &node : nodes) state->nodes.remove(node->uuid());
    return result == Gpu::Result::Success;
}

KisImageSP KisImage::cloneGpuNodeImage(const KisNodeList &roots) const
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto owner = gpuImage();
    if (!owner || !owner->handle() || roots.isEmpty() || QThread::currentThread() != owner->thread()) return {};
    const auto version = owner->operations().snapshot();
    QHash<QUuid, const Gpu::Layer *> sourceLayers;
    for (const auto &layer : version->layers) sourceLayers.insert(layer.uuid, &layer);
    QList<QPair<KisNodeSP, QUuid>> pending;
    for (const auto &node : roots) {
        if (!node || node == root() || node->image() != this || !node->parent()) return {};
        pending.append({node, {}});
    }
    QList<Gpu::LayerInsertion> prepared;
    for (qsizetype i = 0; i < pending.size(); ++i) {
        KisNodeSP node = pending[i].first;
        const auto source = sourceLayers.value(node->uuid());
        auto *layer = qobject_cast<KisLayer*>(node.data());
        auto *paint = qobject_cast<KisPaintLayer*>(node.data());
        auto *group = qobject_cast<KisGroupLayer*>(node.data());
        if (!source || !layer || (!paint && !group) || (paint && node->childCount()) ||
            (layer->compositeOpId() != COMPOSITE_OVER && layer->compositeOpId() != COMPOSITE_MULT && layer->compositeOpId() != COMPOSITE_SCREEN &&
             layer->compositeOpId() != COMPOSITE_DARKEN && layer->compositeOpId() != COMPOSITE_LIGHTEN &&
             layer->compositeOpId() != COMPOSITE_ADD && layer->compositeOpId() != COMPOSITE_LINEAR_DODGE) || node->isAnimated() ||
            !node->keyframeChannels().isEmpty() || layer->layerStyle() || layer->temporary() ||
            (paint && paint->hasTemporaryTarget()) ||
            *layer->colorSpace() != *colorSpace() || pending.size() > owner->limits().layerCount) return {};
        const auto flags = layer->channelFlags();
        if (!flags.isEmpty() && flags.size() != 4) return {};
        Gpu::RasterLayer properties;
        properties.name = source->name;
        properties.visible = source->visible;
        properties.uuid = QUuid::createUuid();
        properties.defaultPixel = source->defaultPixel;
        properties.opacity = source->opacity;
        properties.userLocked = source->userLocked;
        properties.pinnedToTimeline = source->pinnedToTimeline;
        properties.colorLabelIndex = source->colorLabelIndex;
        properties.channelWriteMask = source->channelWriteMask;
        properties.projectionChannelMask = source->projectionChannelMask;
        properties.passThrough = source->passThrough;
        properties.blendMode = source->blendMode;
        properties.position = source->position;
        properties.kind = source->kind;
        properties.parentUuid = pending[i].second;
        prepared.append({version, source->id, properties});
        for (auto child = node->firstChild(); child; child = child->nextSibling())
            pending.append({child, properties.uuid});
    }
    const auto copied = owner->cloneLayerForest(prepared);
    if (!copied) return {};
    auto view = KisImage::fromGpuImage(copied, QStringLiteral("Copied layers"), colorSpace());
    view->setResolution(xRes(), yRes());
    QScopedValueRollback<bool> syncing(view->m_synchronizingGpuNodes, true);
    QHash<QUuid, KisNodeSP> copies;
    KisNodeList tree{view->root()};
    for (qsizetype i = 0; i < tree.size(); ++i)
        for (auto node = tree[i]->firstChild(); node; node = node->nextSibling()) {
            copies.insert(node->uuid(), node);
            tree.append(node);
        }
    for (qsizetype i = 0; i < pending.size(); ++i) {
        KisNodeSP node = copies.value(prepared[i].properties.uuid);
        node->mergeNodeProperties(pending[i].first->nodeProperties());
        node->setCollapsed(pending[i].first->collapsed());
        auto *layer = qobject_cast<KisLayer*>(node.data());
        auto *source = qobject_cast<KisLayer*>(pending[i].first.data());
        layer->setChannelFlags(source->channelFlags());
        layer->metaData()->copyFrom(source->metaData());
    }
    return view;
}

KisNodeList KisImage::cloneGpuNodes(const KisNodeList &roots) const
{
    auto view = cloneGpuNodeImage(roots);
    if (!view) return {};
    QScopedValueRollback<bool> syncing(view->m_synchronizingGpuNodes, true);
    KisNodeList result;
    while (const auto node = view->root()->firstChild()) {
        if (!view->removeNode(node)) return {};
        result.append(node);
    }
    return result;
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
    result->adoptGpuSelectionMask();
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

bool KisImage::setGpuSelectionRect(const QRect &rect, bool inverted)
{
    const auto owner = gpuImage();
    return owner && owner->setSelectionRect(rect, inverted) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::setGpuSelectionEllipse(const QRect &rect)
{
    const auto owner = gpuImage();
    return owner && owner->setSelectionEllipse(rect) == LibrePaint::Image::Gpu::Result::Success;
}

QRect KisImage::gpuSelectionRect() const
{
    const auto owner = gpuImage();
    return owner ? owner->selectionRect() : QRect();
}

bool KisImage::gpuSelectionInverted() const
{
    const auto owner = gpuImage();
    return owner && owner->selectionInverted();
}

LibrePaint::Image::Gpu::SelectionKind KisImage::gpuSelectionKind() const
{
    const auto owner = gpuImage();
    return owner ? owner->selectionKind() : LibrePaint::Image::Gpu::SelectionKind::Rectangle;
}

bool KisImage::gpuSelectionEllipse() const
{
    const auto owner = gpuImage();
    return owner && owner->selectionKind() == LibrePaint::Image::Gpu::SelectionKind::Ellipse;
}

bool KisImage::clearGpuSelection()
{
    const auto owner = gpuImage();
    return owner && owner->clearSelection() == LibrePaint::Image::Gpu::Result::Success;
}

void KisImage::adoptGpuSelectionMask()
{ synchronizeGpuSelectionToMask(); }

void KisImage::synchronizeGpuSelectionToMask()
{
    const auto owner = gpuImage();
    if (!owner) return;
    const QRect rect = owner->selectionRect();
    const bool inverted = owner->selectionInverted();
    const auto kind = owner->selectionKind();
    if (rootLayer() && rootLayer()->selectionMask() && m_hasGpuSelectionMirror &&
        m_gpuSelectionMirrorRect == rect && m_gpuSelectionMirrorInverted == inverted &&
        m_gpuSelectionMirrorKind == quint8(kind)) return;
    updateGpuSelectionMask(rect, inverted, kind);
}

void KisImage::previewGpuSelectionRect(const QRect &rect, bool ellipse)
{
    if (!gpuImage()) return;
    updateGpuSelectionMask(rect, false,
                           ellipse ? LibrePaint::Image::Gpu::SelectionKind::Ellipse
                                   : LibrePaint::Image::Gpu::SelectionKind::Rectangle);
}

void KisImage::updateGpuSelectionMask(const QRect &rect, bool inverted, LibrePaint::Image::Gpu::SelectionKind kind)
{
    KisSelectionMaskSP mask = rootLayer() ? rootLayer()->selectionMask() : KisSelectionMaskSP();
    if (!mask && rect.isEmpty()) return;
    if (!mask) {
        mask = new KisSelectionMask(this);
        mask->initSelection(rootLayer());
        KisSelectionSP selection = new KisSelection(new KisSelectionEmptyBounds(this),
                                                   toQShared(new KisImageResolutionProxy(this)));
        mask->setSelection(selection);
        mask->setActive(true);
        if (!addNode(mask, rootLayer(), rootLayer()->lastChild(), KisNodeAdditionFlag::DontActivateNode)) return;
    }
    KisSelectionSP selection = mask->selection();
    if (!selection || !selection->pixelSelection()) return;
    KisPixelSelectionSP pixels = selection->pixelSelection();
    const QRect previousRect = pixels->selectedExactRect();
    if (previousRect == rect && m_hasGpuSelectionMirror && m_gpuSelectionMirrorRect == rect &&
        m_gpuSelectionMirrorInverted == inverted && m_gpuSelectionMirrorKind == quint8(kind)) return;
    pixels->clear();
    QPainterPath outline;
    if (!rect.isEmpty()) {
        if (inverted) {
            pixels->select(bounds());
            pixels->clear(rect);
            outline.addRect(bounds());
            if (kind == LibrePaint::Image::Gpu::SelectionKind::Ellipse) outline.addEllipse(rect);
            else outline.addRect(rect);
        } else if (kind == LibrePaint::Image::Gpu::SelectionKind::Ellipse) {
            QByteArray mask(rect.width() * rect.height(), ' ');
            const qreal cx = rect.x() + rect.width() / 2.0, cy = rect.y() + rect.height() / 2.0;
            const qreal rx = rect.width() / 2.0, ry = rect.height() / 2.0;
            for (int y = 0; y < rect.height(); ++y)
                for (int x = 0; x < rect.width(); ++x) {
                    const qreal dx = (rect.x() + x + 0.5 - cx) / rx;
                    const qreal dy = (rect.y() + y + 0.5 - cy) / ry;
                    if (dx * dx + dy * dy <= 1.0) mask[y * rect.width() + x] = char(255);
                }
            pixels->writeBytes(reinterpret_cast<const quint8 *>(mask.constData()), rect.x(), rect.y(),
                               rect.width(), rect.height());
            outline.addEllipse(rect);
        } else {
            pixels->select(rect);
            outline.addRect(rect);
        }
    }
    pixels->setOutlineCache(outline);
    selection->updateProjection(inverted ? bounds() : (rect.isEmpty() ? bounds() : rect));
    mask->setDirty(inverted ? bounds() : previousRect.united(rect));
    m_gpuSelectionMirrorRect = rect;
    m_gpuSelectionMirrorInverted = inverted;
    m_gpuSelectionMirrorKind = quint8(kind);
    m_hasGpuSelectionMirror = true;
}

void KisImage::connectGpuImageUpdates()
{
    const auto image = gpuImage();
    auto state = QSharedPointer<KisGpuImageViewState>::create();
    state->synchronized = image->operations().snapshot();
    state->previewRevision = image->operations().preview()->revision();
    QList<KisNodeSP> parents{root()};
    for (qsizetype i = 0; i < parents.size(); ++i)
        for (auto node = parents[i]->firstChild(); node; node = node->nextSibling()) {
            state->nodes.insert(node->uuid(), node);
            parents.append(node);
        }
    gpuViewState() = state;
    connect(image.data(), &LibrePaint::Image::Gpu::PaintImage::changed, this,
            [view = this, state]() {
        auto &synchronized = state->synchronized;
        auto &nodes = state->nodes;
        KisImageSP retainedView = view;
        const auto owner = retainedView->gpuImage();
        const auto version = owner->operations().snapshot();
        const auto previewRevision = owner->operations().preview()->revision();
        const bool projectionChanged = version->documentId() != synchronized->documentId() ||
            version->revision() != synchronized->revision() || previewRevision != state->previewRevision;
        state->previewRevision = previewRevision;
        if (version != synchronized) {
            const auto previous = synchronized;
            const auto previousSize = retainedView->size();
            const bool selectionChanged = version->activeLayer != synchronized->activeLayer ||
                version->selectedLayers != synchronized->selectedLayers;
            const bool hierarchyChanged = version->layers.size() != previous->layers.size() ||
                !std::equal(version->layers.cbegin(), version->layers.cend(), previous->layers.cbegin(),
                            [](const auto &left, const auto &right) {
                    return left.id == right.id && left.uuid == right.uuid && left.parentUuid == right.parentUuid;
                });
            retainedView->setSize(version->size);
            {
                QScopedValueRollback<bool> syncing(retainedView->m_synchronizingGpuNodes, true);
                retainedView->synchronizeGpuLayerViews(state.data());
            }
            synchronized = retainedView->gpuImage()->operations().snapshot();
            retainedView->setSize(synchronized->size);
            retainedView->synchronizeGpuSelectionToMask();
            const auto &operations = retainedView->gpuImage()->operations();
            const auto versions = operations.historyVersions();
            int previousIndex = -1;
            for (int i = 0; i < versions.size(); ++i) {
                if (versions[i]->documentId() == previous->documentId() &&
                    versions[i]->historyRevision() == previous->historyRevision()) previousIndex = i;
            }
            std::optional<ComplexSizeChangedSignal> resize;
            const int currentIndex = operations.undoCount();
            if (previousIndex >= 0 && previousIndex != currentIndex) {
                QPointF offset;
                bool resized = false;
                for (int i = std::min(previousIndex, currentIndex) + 1; i <= std::max(previousIndex, currentIndex); ++i) {
                    if (const auto change = operations.historyCanvasResize(i)) {
                        resized = true;
                        offset += change->rect.topLeft();
                        if (std::abs(previousIndex - currentIndex) == 1) {
                            const ComplexSizeChangedSignal signal(change->rect, change->rect.size());
                            resize = previousIndex < currentIndex ? signal : signal.inverted();
                        }
                    }
                }
                if (resized && !resize) {
                    if (previousIndex > currentIndex) offset = -offset;
                    const QPointF centre = QRectF(QPointF(), QSizeF(synchronized->size)).center();
                    resize = ComplexSizeChangedSignal(centre + offset, centre);
                }
            }
            if (!resize && previousSize != synchronized->size)
                resize = ComplexSizeChangedSignal(QRect(QPoint(), previousSize), synchronized->size);
            if (resize) Q_EMIT retainedView->sigSizeChanged(resize->oldStillPoint, resize->newStillPoint);
            if ((selectionChanged || hierarchyChanged) && synchronized == version) {
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
        if (projectionChanged) retainedView->notifyProjectionUpdated(retainedView->bounds());
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
        QList<KisNodeSP> attached{view->root()};
        for (qsizetype i = 0; i < attached.size(); ++i)
            for (auto node = attached[i]->firstChild(); node; node = node->nextSibling()) attached.append(node);
        for (qsizetype i = 1; i < attached.size(); ++i) {
            const auto node = attached[i];
            existing.insert(node->uuid(), node);
            if (!node->inherits("KisSelectionMask") && !desired.contains(node->uuid()) && node->parent() &&
                (node->parent() == view->root() || desired.contains(node->parent()->uuid()))) view->removeNode(node);
            if (changed()) break;
        }
        if (view->gpuImage() != owner) return;
        if (changed()) continue;
        for (qsizetype index = 0; index < version->layers.size(); ++index) {
            const auto &layer = version->layers[index];
            KisNodeSP node = existing.value(layer.uuid);
            if (!node) {
                if (layer.kind == LibrePaint::Image::Gpu::LayerKind::Group)
                    node = new KisGroupLayer(view, layer.name, layer.opacity, view->colorSpace(),
                                             KisPaintDevice::fromGpuImage(owner, layer.id, {}, view->colorSpace()));
                else node = new KisPaintLayer(view, layer.name, layer.opacity,
                                             KisPaintDevice::fromGpuImage(owner, layer.id, {}, view->colorSpace()));
                node->setUuid(layer.uuid);
            }
            existing.insert(layer.uuid, node);
            if (auto *group = qobject_cast<KisGroupLayer *>(node.data())) group->original()->bindGpuLayer(owner, layer.id);
            else node->paintDevice()->bindGpuLayer(owner, layer.id);
            if (node->name() != layer.name) node->setName(layer.name);
            if (changed()) break;
            node->setCompositeOpId(layer.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::Multiply ? COMPOSITE_MULT :
                layer.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::Screen ? COMPOSITE_SCREEN :
                layer.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::Darken ? COMPOSITE_DARKEN :
                layer.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::Lighten ? COMPOSITE_LIGHTEN :
                layer.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::Addition ? COMPOSITE_ADD :
                layer.blendMode == LibrePaint::Image::Gpu::LayerBlendMode::LinearDodge ? COMPOSITE_LINEAR_DODGE : COMPOSITE_OVER);
            if (changed()) break;
            node->setVisible(layer.visible);
            if (changed()) break;
            if (node->opacity() != layer.opacity) node->setOpacity(layer.opacity);
            if (changed()) break;
            if (node->userLocked() != layer.userLocked) node->setUserLocked(layer.userLocked);
            if (changed()) break;
            if (auto *group = qobject_cast<KisGroupLayer *>(node.data()))
                if (group->passThroughMode() != layer.passThrough) group->setPassThroughMode(layer.passThrough);
            if (changed()) break;
            auto *layerView = qobject_cast<KisLayer*>(node.data());
            QBitArray projectionFlags;
            if (layer.projectionChannelMask != 15) {
                const auto channels = layerView->colorSpace()->channels();
                projectionFlags.resize(4);
                for (int channel = 0; channel < 4; ++channel)
                    projectionFlags.setBit(channel, layer.projectionChannelMask & (1u << channels[channel]->displayPosition()));
            }
            if (layerView->channelFlags() != projectionFlags) layerView->setChannelFlags(projectionFlags);
            if (changed()) break;
            if (auto *paintLayer = qobject_cast<KisPaintLayer*>(node.data())) {
                QBitArray paintFlags;
                if (layer.channelWriteMask != 15) {
                    const auto channels = paintLayer->colorSpace()->channels();
                    paintFlags.resize(4);
                    for (int channel = 0; channel < 4; ++channel)
                        paintFlags.setBit(channel, layer.channelWriteMask & (1u << channels[channel]->displayPosition()));
                }
                if (paintLayer->channelLockFlags() != paintFlags) paintLayer->setChannelLockFlags(paintFlags);
            }
            if (changed()) break;
            if (node->isPinnedToTimeline() != layer.pinnedToTimeline) node->setPinnedToTimeline(layer.pinnedToTimeline);
            if (changed()) break;
            if (node->colorLabelIndex() != layer.colorLabelIndex) node->setColorLabelIndex(layer.colorLabelIndex);
            if (changed()) break;
        }
        if (view->gpuImage() != owner) return;
        if (changed()) continue;
        QHash<QUuid, QList<const LibrePaint::Image::Gpu::Layer *>> children;
        for (const auto &layer : version->layers) children[layer.parentUuid].append(&layer);
        QList<QUuid> parents{QUuid()};
        for (qsizetype i = 0; i < parents.size() && !changed(); ++i) {
            auto parent = parents[i].isNull() ? view->root() : existing.value(parents[i]);
            KisNodeSP above;
            qsizetype index = 0;
            for (const auto layer : children.value(parents[i])) {
                const auto node = existing.value(layer->uuid);
                if (node->parent() != parent || parent->at(index) != node) {
                    if (node->parent()) view->removeNode(node);
                    if (changed()) break;
                    const auto flags = layer->id == version->activeLayer
                        ? KisNodeAdditionFlags(KisNodeAdditionFlag::None)
                        : KisNodeAdditionFlags(KisNodeAdditionFlag::DontActivateNode);
                    view->addNode(node, parent, above, flags);
                    if (changed()) break;
                }
                if (layer->kind == LibrePaint::Image::Gpu::LayerKind::Group) parents.append(layer->uuid);
                above = node;
                ++index;
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
