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
#include "kis_transparency_mask.h"
#include "KisImageResolutionProxy.h"
#include "kis_pointer_utils.h"
#include <kis_meta_data_store.h>
#include <klocalizedstring.h>
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
    /** Last mask coverage written into each layer's layer-panel view node. */
    QHash<QUuid, LibrePaint::Image::Gpu::SelectionMaskSP> transparencyMasks;
};

namespace {
LibrePaint::Image::Gpu::LayerId gpuPaintLayerId(const LibrePaint::Image::Gpu::Snapshot &version,
                                                const KisNodeSP &node)
{
    if (!version || !node || node->uuid().isNull()) return 0;
    for (const auto &layer : version->layers)
        if (layer.uuid == node->uuid() && layer.kind == LibrePaint::Image::Gpu::LayerKind::Paint)
            return layer.id;
    return 0;
}

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

/** True when this node is the layer-panel view of a GPU paint layer's
 * transparency mask. The coverage itself stays owned by the GPU document.
 */
bool isGpuTransparencyMaskViewNode(KisNodeSP node)
{
    if (!node || !node->inherits("KisTransparencyMask")) return false;
    const KisNodeSP parent = node->parent();
    const KisImageSP image = parent ? parent->image().toStrongRef() : KisImageSP();
    if (!image || !image->gpuImage()) return false;
    const auto version = image->gpuImage()->operations().snapshot();
    return std::any_of(version->layers.cbegin(), version->layers.cend(),
        [&](const auto &layer) {
            return layer.uuid == parent->uuid() &&
                layer.kind == LibrePaint::Image::Gpu::LayerKind::Paint;
        });
}

KisTransparencyMaskSP findTransparencyMaskChild(KisNodeSP parent)
{
    if (!parent) return {};
    for (auto child = parent->firstChild(); child; child = child->nextSibling())
        if (child->inherits("KisTransparencyMask"))
            return qobject_cast<KisTransparencyMask *>(child.data());
    return {};
}

bool hasNonMaskChild(KisNodeSP node)
{
    if (!node) return false;
    for (auto child = node->firstChild(); child; child = child->nextSibling())
        if (!isGpuTransparencyMaskViewNode(child)) return true;
    return false;
}

/** Writes retained GPU coverage into a view-only alpha selection. The mask node
 * is refreshed only when the document version's coverage changes.
 */
void writeGpuTransparencyMaskPixels(KisTransparencyMask *mask,
                                    const LibrePaint::Image::Gpu::SelectionMaskSP &coverage,
                                    const QRect &previousBounds, const QRect &clip)
{
    if (!mask) return;
    KisSelectionSP selection = mask->selection();
    if (!selection || !selection->pixelSelection()) return;
    KisPixelSelectionSP pixels = selection->pixelSelection();
    // The retained document version owns the coverage; this node is only a
    // view. Extent-based dirty propagation would ask the GPU parent layer for
    // native pixel bounds, so the previous and new coverage rectangles are
    // propagated directly.
    QRect touched = previousBounds.intersected(clip);
    pixels->setDefaultPixel(KoColor(Qt::transparent, pixels->colorSpace()));
    pixels->clear();
    if (coverage) {
        const QRect covered = coverage->bounds().intersected(clip);
        if (!covered.isEmpty()) {
            QByteArray values(covered.width() * covered.height(), '\0');
            for (int y = covered.top(); y <= covered.bottom(); ++y)
                for (int x = covered.left(); x <= covered.right(); ++x)
                    values[(y - covered.top()) * covered.width() + x - covered.left()] =
                        char(coverage->coverage(x, y));
            pixels->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), covered);
            touched = touched.united(covered);
        }
    }
    if (!touched.isEmpty()) pixels->setDirty(touched);
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
    return node && !hasNonMaskChild(node) && insertGpuLayers({node}, index, parent);
}

bool KisImage::insertGpuLayers(const KisNodeList &roots, qsizetype index, KisNodeSP parent,
                               bool clearPixelSelection)
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
        for (auto child = node->firstChild(); child; child = child->nextSibling())
            if (!isGpuTransparencyMaskViewNode(child)) nodes.append(child);
    }
    QList<Gpu::LayerInsertion> prepared;
    qsizetype preparedBytes = 0;
    for (KisNodeSP node : nodes) {
        auto *layer = qobject_cast<KisLayer*>(node.data());
        auto *paint = qobject_cast<KisPaintLayer*>(node.data());
        auto *group = qobject_cast<KisGroupLayer*>(node.data());
        if (!layer || (!paint && !group) || (paint && hasNonMaskChild(node)) ||
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
    QScopedValueRollback<QRect> previousSelection(m_lastGpuSelection);
    QScopedValueRollback<bool> previousInverted(m_lastGpuSelectionInverted);
    QScopedValueRollback<quint8> previousKind(m_lastGpuSelectionKind);
    QScopedValueRollback<Gpu::SelectionMaskSP> previousMask(m_lastGpuSelectionMask);
    QScopedValueRollback<bool> previousRemembered(m_hasLastGpuSelection);
    if (clearPixelSelection && (!owner->selectionRect().isEmpty() || owner->selectionInverted())) {
        m_lastGpuSelection = owner->selectionRect();
        m_lastGpuSelectionInverted = owner->selectionInverted();
        m_lastGpuSelectionKind = quint8(owner->selectionKind());
        m_lastGpuSelectionMask = owner->selectionMask();
        m_hasLastGpuSelection = true;
    }
    for (const auto &node : nodes) state->nodes.insert(node->uuid(), node);
    const auto result = owner->insertLayers(prepared, insertion, true, clearPixelSelection);
    if (result != Gpu::Result::Success) {
        for (const auto &node : nodes) state->nodes.remove(node->uuid());
    } else {
        previousSelection.commit();
        previousInverted.commit();
        previousKind.commit();
        previousMask.commit();
        previousRemembered.commit();
    }
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
        if (!source || !layer || (!paint && !group) || (paint && hasNonMaskChild(node)) ||
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
            if (!isGpuTransparencyMaskViewNode(child))
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
        for (auto child = node->firstChild(); child;) {
            auto next = child->nextSibling();
            if (isGpuTransparencyMaskViewNode(child)) view->removeNode(child);
            child = next;
        }
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

bool KisImage::setGpuSelectionEllipse(const QRect &rect, bool inverted)
{
    const auto owner = gpuImage();
    return owner && owner->setSelectionEllipse(rect, inverted) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::setGpuSelectionMask(const LibrePaint::Image::Gpu::SelectionMaskSP &mask, bool inverted)
{
    const auto owner = gpuImage();
    return owner && owner->setSelectionMask(mask, inverted) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::setGpuSelectionFreehand(const QList<QPointF> &points, bool inverted)
{
    const auto owner = gpuImage();
    if (!owner) return false;
    const auto mask = LibrePaint::Image::Gpu::SelectionMask::fromPolygon(points, QRect(QPoint(), size()));
    if (!mask) return false;
    return owner->setSelectionMask(mask, inverted) == LibrePaint::Image::Gpu::Result::Success;
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

bool KisImage::gpuSelectionFreehand() const
{
    const auto owner = gpuImage();
    return owner && owner->selectionKind() == LibrePaint::Image::Gpu::SelectionKind::Freehand;
}

bool KisImage::moveGpuSelectionBy(QPoint delta)
{
    const auto owner = gpuImage();
    if (!owner) return false;
    const auto mask = owner->selectionMask();
    const auto moved = mask ? mask->translated(delta, QRect(QPoint(), size())) : nullptr;
    return moved && owner->setSelectionMask(moved, owner->selectionInverted()) ==
        LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::invertGpuSelectionMask()
{
    const auto owner = gpuImage();
    if (!owner || !owner->selectionMask()) return false;
    if (owner->selectionMask()->isAlpha()) {
        // Soft coverage complements through the frozen flag without allocating
        // an image-sized alpha mask.
        return owner->setSelectionMask(owner->selectionMask(), !owner->selectionInverted()) ==
            LibrePaint::Image::Gpu::Result::Success;
    }
    const auto inverted = owner->selectionMask()->inverted(QRect(QPoint(), size()));
    return inverted ? owner->setSelectionMask(inverted, false) == LibrePaint::Image::Gpu::Result::Success
                    : owner->clearSelection() == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::growGpuSelectionMask(int xRadius, int yRadius)
{
    const auto owner = gpuImage();
    if (!owner || !owner->selectionMask()) return false;
    const auto grown = owner->selectionMask()->grown(xRadius, yRadius, QRect(QPoint(), size()));
    if (!grown && owner->selectionMask()->isAlpha()) return false;
    return grown ? owner->setSelectionMask(grown, false) == LibrePaint::Image::Gpu::Result::Success
                 : owner->clearSelection() == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::shrinkGpuSelectionMask(int xRadius, int yRadius)
{
    const auto owner = gpuImage();
    if (!owner || !owner->selectionMask()) return false;
    const auto shrunk = owner->selectionMask()->shrunk(xRadius, yRadius, QRect(QPoint(), size()));
    if (!shrunk && owner->selectionMask()->isAlpha()) return false;
    return shrunk ? owner->setSelectionMask(shrunk, false) == LibrePaint::Image::Gpu::Result::Success
                  : owner->clearSelection() == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::borderGpuSelectionMask(int xRadius, int yRadius)
{
    const auto owner = gpuImage();
    if (!owner || !owner->selectionMask()) return false;
    const auto border = owner->selectionMask()->border(xRadius, yRadius, QRect(QPoint(), size()));
    if (!border && owner->selectionMask()->isAlpha()) return false;
    return border ? owner->setSelectionMask(border, false) == LibrePaint::Image::Gpu::Result::Success
                  : owner->clearSelection() == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::featherGpuSelectionMask(int radius)
{
    const auto owner = gpuImage();
    if (!owner || radius <= 0) return false;
    const auto mask = owner->selectionMask();
    if (!mask) return false;
    const auto feathered = mask->feathered(radius, QRect(QPoint(), size()));
    if (!feathered) return false;
    return owner->setSelectionMask(feathered, owner->selectionInverted()) ==
        LibrePaint::Image::Gpu::Result::Success;
}

void KisImage::previewGpuSelectionMoveBy(QPoint delta)
{
    const auto owner = gpuImage();
    if (!owner || !owner->selectionMask()) return;
    const auto moved = owner->selectionMask()->translated(delta, QRect(QPoint(), size()));
    updateGpuSelectionMask(moved ? moved->bounds() : QRect(), owner->selectionInverted(),
                           LibrePaint::Image::Gpu::SelectionKind::Freehand, moved);
}

bool KisImage::reselectGpuSelection()
{
    if (!m_hasLastGpuSelection || m_lastGpuSelection.isEmpty()) return false;
    if (m_lastGpuSelectionKind == quint8(LibrePaint::Image::Gpu::SelectionKind::Freehand))
        return setGpuSelectionMask(m_lastGpuSelectionMask, m_lastGpuSelectionInverted);
    if (m_lastGpuSelectionKind == quint8(LibrePaint::Image::Gpu::SelectionKind::Ellipse))
        return setGpuSelectionEllipse(m_lastGpuSelection, m_lastGpuSelectionInverted);
    return setGpuSelectionRect(m_lastGpuSelection, m_lastGpuSelectionInverted);
}

bool KisImage::clearGpuSelection()
{
    const auto owner = gpuImage();
    if (!owner) return false;
    const QRect rect = owner->selectionRect();
    const bool inverted = owner->selectionInverted();
    if (!rect.isEmpty() || inverted) {
        m_lastGpuSelection = rect;
        m_lastGpuSelectionInverted = inverted;
        m_lastGpuSelectionKind = quint8(owner->selectionKind());
        m_lastGpuSelectionMask = owner->selectionMask();
        m_hasLastGpuSelection = true;
    }
    return owner->clearSelection() == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::invertGpuLayerColors(KisNodeSP layer)
{
    const auto owner = gpuImage();
    if (!owner || !layer || layer->image() != this ||
        QThread::currentThread() != owner->thread()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->invertColors(layerId) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::grayscaleGpuLayerColors(KisNodeSP layer)
{
    const auto owner = gpuImage();
    if (!owner || !layer || layer->image() != this ||
        QThread::currentThread() != owner->thread()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->grayscale(layerId) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::hsvAdjustmentGpuLayerColors(KisNodeSP layer,
                                           const LibrePaint::Image::Gpu::HsvAdjustment &settings)
{
    const auto owner = gpuImage();
    if (!owner || !layer || layer->image() != this ||
        QThread::currentThread() != owner->thread() || !settings.isValid()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->hsvAdjustment(layerId, settings) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::addGpuTransparencyMaskFromSelection(KisNodeSP layer)
{
    const auto owner = gpuImage();
    if (!owner || QThread::currentThread() != owner->thread()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->addTransparencyMaskFromSelection(layerId) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::setGpuTransparencyMaskVisible(KisNodeSP layer, bool visible)
{
    const auto owner = gpuImage();
    if (!owner || QThread::currentThread() != owner->thread()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->setTransparencyMaskVisible(layerId, visible) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::removeGpuTransparencyMask(KisNodeSP layer)
{
    const auto owner = gpuImage();
    if (!owner || QThread::currentThread() != owner->thread()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->removeTransparencyMask(layerId) == LibrePaint::Image::Gpu::Result::Success;
}

bool KisImage::gpuTransparencyMaskVisible(KisNodeSP layer) const
{
    const auto owner = gpuImage();
    if (!owner || QThread::currentThread() != owner->thread()) return false;
    const auto layerId = gpuPaintLayerId(owner->operations().snapshot(), layer);
    return layerId && owner->layerTransparencyMaskVisible(layerId);
}

bool KisImage::isGpuTransparencyMaskView(KisNodeSP node) const
{
    const auto owner = gpuImage();
    if (!owner || !node || !node->inherits("KisTransparencyMask")) return false;
    const KisNodeSP parent = node->parent();
    if (!parent || parent->image() != this) return false;
    const auto version = owner->operations().snapshot();
    return std::any_of(version->layers.cbegin(), version->layers.cend(),
        [&](const auto &layer) {
            return layer.uuid == parent->uuid() &&
                layer.kind == LibrePaint::Image::Gpu::LayerKind::Paint;
        });
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
    const auto mask = owner->selectionMask();
    if (rootLayer() && rootLayer()->selectionMask() && m_hasGpuSelectionMirror &&
        m_gpuSelectionMirrorRect == rect && m_gpuSelectionMirrorInverted == inverted &&
        m_gpuSelectionMirrorKind == quint8(kind) && m_gpuSelectionMirrorMask == mask) return;
    updateGpuSelectionMask(rect, inverted, kind, mask);
}

void KisImage::previewGpuSelectionRect(const QRect &rect, bool ellipse, bool inverted)
{
    if (!gpuImage()) return;
    updateGpuSelectionMask(rect, inverted,
                           ellipse ? LibrePaint::Image::Gpu::SelectionKind::Ellipse
                                   : LibrePaint::Image::Gpu::SelectionKind::Rectangle);
}

void KisImage::previewGpuSelectionMask(const LibrePaint::Image::Gpu::SelectionMaskSP &mask, bool inverted)
{
    if (!gpuImage()) return;
    updateGpuSelectionMask(mask ? mask->bounds() : QRect(), inverted,
                           LibrePaint::Image::Gpu::SelectionKind::Freehand, mask);
}

void KisImage::updateGpuSelectionMask(const QRect &rect, bool inverted, LibrePaint::Image::Gpu::SelectionKind kind,
                                      const LibrePaint::Image::Gpu::SelectionMaskSP &coverage)
{
    KisImageSP retained = this;
    KisSelectionMaskSP mask = rootLayer() ? rootLayer()->selectionMask() : KisSelectionMaskSP();
    if (rect.isEmpty() && !inverted) {
        if (!mask) return;
        m_hasGpuSelectionMirror = false;
        removeNode(mask);
        retained->notifySelectionChanged();
        return;
    }
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
        m_gpuSelectionMirrorInverted == inverted && m_gpuSelectionMirrorKind == quint8(kind) &&
        m_gpuSelectionMirrorMask == coverage) return;
    pixels->clear();
    QPainterPath outline;
    if (!rect.isEmpty() && kind == LibrePaint::Image::Gpu::SelectionKind::Freehand && coverage &&
        coverage->isAlpha()) {
        // Alpha coverage is written into the CPU mirror so the display keeps the
        // fractional selectedness. The outline follows its half-selected contour.
        const QRect clip(QPoint(), bounds().size());
        const QRect covered = coverage->bounds().intersected(clip);
        QByteArray values(covered.width() * covered.height(), '\0');
        QRegion region;
        for (int y = covered.top(); y <= covered.bottom(); ++y) {
            int runStart = -1;
            for (int x = covered.left(); x <= covered.right() + 1; ++x) {
                const quint8 stored = x <= covered.right() ? coverage->coverage(x, y) : 0;
                const quint8 effective = inverted ? quint8(255u - stored) : stored;
                if (x <= covered.right())
                    values[(y - covered.top()) * covered.width() + x - covered.left()] = char(effective);
                const bool outlined = x <= covered.right() && effective >= 128;
                if (outlined && runStart < 0) runStart = x;
                else if (!outlined && runStart >= 0) {
                    region += QRect(runStart, y, x - runStart, 1);
                    runStart = -1;
                }
            }
        }
        if (inverted) pixels->select(clip);
        if (!covered.isEmpty())
            pixels->writeBytes(reinterpret_cast<const quint8 *>(values.constData()), covered);
        if (inverted) {
            QRegion visible(clip);
            visible -= region;
            outline.addRegion(visible);
        } else {
            outline.addRegion(region);
        }
        if (outline.isEmpty()) outline.addRect(QRect());
    } else if (!rect.isEmpty() && kind == LibrePaint::Image::Gpu::SelectionKind::Freehand && coverage) {
        const QRect clip(QPoint(), bounds().size());
        const QRect covered = coverage->bounds().intersected(clip);
        QRegion region;
        for (int y = covered.top(); y <= covered.bottom(); ++y) {
            int runStart = -1;
            for (int x = covered.left(); x <= covered.right() + 1; ++x) {
                const bool selected = x <= covered.right() && coverage->contains(x, y);
                if (selected && runStart < 0) runStart = x;
                else if (!selected && runStart >= 0) {
                    region += QRect(runStart, y, x - runStart, 1);
                    runStart = -1;
                }
            }
        }
        if (inverted) {
            pixels->select(clip);
            for (auto run = region.begin(); run != region.end(); ++run) pixels->clear(*run);
            QRegion visible(clip);
            visible -= region;
            outline.addRegion(visible);
        } else {
            for (auto run = region.begin(); run != region.end(); ++run) pixels->select(*run);
            outline.addRegion(region);
        }
        if (outline.isEmpty()) outline.addRect(QRect());
    } else if (!rect.isEmpty()) {
        if (inverted) {
            pixels->select(bounds());
            if (kind == LibrePaint::Image::Gpu::SelectionKind::Ellipse) {
                const qreal cx = rect.x() + rect.width() / 2.0, cy = rect.y() + rect.height() / 2.0;
                const qreal rx = rect.width() / 2.0, ry = rect.height() / 2.0;
                for (int y = rect.top(); y <= rect.bottom(); ++y) {
                    const qreal dy = (y + 0.5 - cy) / ry;
                    if (qAbs(dy) > 1.0) continue;
                    const qreal dx = rx * std::sqrt(1.0 - dy * dy);
                    const int left = qCeil(cx - dx - 0.5), right = qFloor(cx + dx - 0.5);
                    if (right >= left) pixels->clear(QRect(left, y, right - left + 1, 1));
                }
                outline.addEllipse(rect);
            } else {
                pixels->clear(rect);
                outline.addRect(rect);
            }
            outline.addRect(bounds());
        } else if (kind == LibrePaint::Image::Gpu::SelectionKind::Ellipse) {
            QByteArray mask(rect.width() * rect.height(), '\0');
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
    m_gpuSelectionMirrorMask = coverage;
    m_hasGpuSelectionMirror = true;
    retained->notifySelectionChanged();
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
            const bool retainedMaskView = isGpuTransparencyMaskViewNode(node) && node->parent() &&
                desired.contains(node->parent()->uuid());
            if (!node->inherits("KisSelectionMask") && !retainedMaskView && !desired.contains(node->uuid()) && node->parent() &&
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
        for (const auto &layer : version->layers) {
            if (layer.kind != LibrePaint::Image::Gpu::LayerKind::Paint) continue;
            KisNodeSP layerNode = existing.value(layer.uuid);
            if (!layerNode) continue;
            auto maskView = findTransparencyMaskChild(layerNode);
            if (!layer.transparencyMask) {
                if (maskView) {
                    view->removeNode(maskView);
                    if (state) state->transparencyMasks.remove(layer.uuid);
                    if (view->gpuImage() != owner) return;
                    if (changed()) break;
                }
                continue;
            }
            const bool contentChanged = !state ||
                state->transparencyMasks.value(layer.uuid) != layer.transparencyMask;
            if (!maskView) {
                KisLayerSP layerView = qobject_cast<KisLayer *>(layerNode.data());
                if (!layerView) continue;
                maskView = new KisTransparencyMask(view, i18n("Transparency Mask"));
                maskView->initSelection(layerView);
                writeGpuTransparencyMaskPixels(maskView.data(), layer.transparencyMask, QRect(),
                                               QRect(QPoint(), version->size));
                view->addNode(maskView, layerNode, layerNode->lastChild(),
                              KisNodeAdditionFlags(KisNodeAdditionFlag::DontActivateNode));
                if (view->gpuImage() != owner) return;
                if (changed()) break;
            } else if (contentChanged) {
                const auto previousMask = state ? state->transparencyMasks.value(layer.uuid)
                                                : LibrePaint::Image::Gpu::SelectionMaskSP{};
                const QRect previousBounds = previousMask
                    ? previousMask->bounds() : QRect(QPoint(), version->size);
                writeGpuTransparencyMaskPixels(maskView.data(), layer.transparencyMask, previousBounds,
                                               QRect(QPoint(), version->size));
            }
            if (state) state->transparencyMasks.insert(layer.uuid, layer.transparencyMask);
            if (maskView->visible() != layer.transparencyMaskVisible)
                maskView->setVisible(layer.transparencyMaskVisible);
            if (view->gpuImage() != owner) return;
            if (changed()) break;
        }
        if (view->gpuImage() != owner) return;
        if (changed()) continue;
        if (state) {
            for (auto it = state->transparencyMasks.begin(); it != state->transparencyMasks.end();) {
                if (!desired.contains(it.key())) it = state->transparencyMasks.erase(it);
                else ++it;
            }
        }
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
