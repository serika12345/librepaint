/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCpuBrushStroke.h"
#include <KisGpuPaintDeviceTransfer_p.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_selection_mask.h>
#include <kis_transparency_mask.h>
#include <kis_filter_mask.h>
#include <kis_adjustment_layer.h>
#include <filter/kis_filter_configuration.h>
#include <brushengine/kis_paintop_preset.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_default_bounds.h>
#include <kis_undo_stores.h>
#include <KisRunnableStrokeJobData.h>
#include <KLocalizedString>
#include <KoColorSpace.h>
#include <QMutex>
#include <QMutexLocker>
#include <QScopeGuard>
#include <algorithm>
#include <stdexcept>

namespace {
namespace Gpu = LibrePaint::Image::Gpu;

KisPaintDeviceSP copyNativePixels(KisPaintDeviceSP source, KisDefaultBoundsBaseSP bounds)
{
    const auto area = source->extent();
    const qint64 count = qint64(area.width()) * area.height() * source->pixelSize();
    if (count > 256 * 1024 * 1024) throw std::length_error("CPU brush source exceeds capacity");
    KisPaintDeviceSP result = new KisPaintDevice(source->colorSpace());
    result->setDefaultBounds(bounds);
    result->setDefaultPixel(source->defaultPixel());
    if (count > 0) {
        QByteArray bytes(count, Qt::Uninitialized);
        source->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        result->writeBytes(reinterpret_cast<const quint8 *>(bytes.constData()), area);
    }
    return result;
}

KisSelectionSP copyNativeSelection(KisPaintDeviceSP source, KisDefaultBoundsBaseSP bounds)
{
    const auto pixels = copyNativePixels(source, bounds);
    KisSelectionSP result = new KisSelection;
    result->pixelSelection()->setDefaultBounds(bounds);
    result->pixelSelection()->setDefaultPixel(pixels->defaultPixel());
    const auto area = pixels->extent();
    KisPainter::copyAreaOptimized(area.topLeft(), pixels, result->pixelSelection(), area);
    return result;
}

// Smudge's native overlay starts from the captured document projection and
// incorporates its own live dabs. Keep that source separate from the working
// layer so native graph updates cannot replace it with just the edited layer.
class CpuBrushRoot final : public KisGroupLayer
{
public:
    CpuBrushRoot(KisImageWSP image, KisPaintDeviceSP pixels)
        : KisGroupLayer(image, QStringLiteral("CPU brush sampling"), 255), m_pixels(pixels) {}
    KisPaintDeviceSP projection() const override { return m_pixels; }
private:
    KisPaintDeviceSP m_pixels;
};

class CpuBrushPixels
{
public:
    virtual ~CpuBrushPixels() = default;
    void markDirty(const QVector<QRect> &rects)
    {
        QMutexLocker lock(&m_mutex);
        for (const auto &rect : rects) m_dirty |= rect;
        ++m_serial;
    }

    QRect dirtyRect() const
    {
        QMutexLocker lock(&m_mutex);
        return m_dirty;
    }

    void resetDirty()
    {
        QMutexLocker lock(&m_mutex);
        m_dirty = {};
    }

    quint64 serial() const
    {
        QMutexLocker lock(&m_mutex);
        return m_serial;
    }

    QByteArray previewPixels(KisNodeSP node, QRect area) const
    {
        KisPaintDeviceSP pixels = node->paintDevice();
        const qint64 count = qint64(area.width()) * area.height() * pixels->pixelSize();
        if (count <= 0 || count > 256 * 1024 * 1024) throw std::length_error("CPU brush preview exceeds capacity");
        QByteArray bytes(count, Qt::Uninitialized);
        const auto indirect = dynamic_cast<KisIndirectPaintingSupport *>(node.data());
        KisIndirectPaintingSupport::ReadLocker lock(indirect);
        if (indirect->hasTemporaryTarget()) {
            const auto original = pixels;
            // Alpha selections compose through their native GrayA source
            // device, just as KisMask does when displaying indirect strokes.
            pixels = original->pixelSize() == 1 ? KisPaintDeviceSP(new KisPixelSelection) :
                KisPaintDeviceSP(new KisPaintDevice(original->colorSpace()));
            pixels->setDefaultBounds(original->defaultBounds());
            pixels->setDefaultPixel(original->defaultPixel());
            KisPainter::copyAreaOptimized(area.topLeft(), original, pixels, area);
            KisPainter painter(pixels);
            indirect->setupTemporaryPainter(&painter);
            painter.bitBlt(area.topLeft(), indirect->temporaryTarget(), area);
        }
        pixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        return bytes;
    }

private:
    mutable QMutex m_mutex;
    QRect m_dirty;
    quint64 m_serial = 0;
};

template<class Base>
class CpuBrushNode final : public Base, public CpuBrushPixels
{
public:
    using Base::Base;
    using Base::setDirty;
    void setDirty(const QVector<QRect> &rects) override
    {
        this->markDirty(rects);
        Base::setDirty(rects);
    }
};
using CpuBrushLayer = CpuBrushNode<KisPaintLayer>;

struct NativePreview
{
    QMutex mutex;
    QRect area;
    QByteArray bytes;
    quint64 serial = 0;
    bool pending = false;
    bool failed = false;
};

void copySamplingMasks(KisNodeSP source, KisLayerSP target, KisImageSP working,
                       KisNodeSP edited = {}, KisNodeSP editedTarget = {})
{
    for (auto child = source->firstChild(); child; child = child->nextSibling()) {
        if (qobject_cast<KisSelectionMask *>(child.data())) continue;
        const auto mask = qobject_cast<KisMask *>(child.data());
        if (!mask) continue;
        KisMaskSP native;
        if (child == edited) {
            native = qobject_cast<KisMask *>(editedTarget.data());
        } else if (const auto filter = qobject_cast<KisFilterMask *>(mask)) {
            auto copy = new KisFilterMask(working, child->name());
            native = copy;
            if (filter->filter()) copy->setFilter(filter->filter()->cloneWithResourcesSnapshot());
        } else if (qobject_cast<KisTransparencyMask *>(mask)) {
            native = new KisTransparencyMask(working, child->name());
        } else {
            throw std::invalid_argument("CPU brush sampling mask is unsupported");
        }
        if (child == edited) {
            if (!native) throw std::invalid_argument("CPU brush mask target is unavailable");
        } else if (mask->selection())
            native->initSelection(copyNativePixels(mask->selection()->projection(), new KisDefaultBounds(working)), target);
        else native->initSelection(target);
        native->setVisible(child->visible());
        native->setOpacity(child->opacity());
        native->setCompositeOpId(child->compositeOpId());
        if (!working->addNode(native, target)) throw std::runtime_error("CPU brush sampling mask could not be added");
    }
}

// Clone sampling reads the native projection's old data while indirect dabs
// update that projection. Keep a temporary graph so those reads observe the
// existing scheduler's updates, including layers above the edited layer.
KisNodeSP copySamplingLayers(KisNodeSP source, KisNodeSP parent, KisNodeSP edited,
                            KisNodeSP target, KisImageSP working, KisNodeSP samplingNode)
{
    KisNodeSP nativeSource;
    for (auto child = source->firstChild(); child; child = child->nextSibling()) {
        const auto layer = qobject_cast<KisLayer *>(child.data());
        if (!layer) continue;
        KisLayerSP native;
        const auto group = qobject_cast<KisGroupLayer *>(layer);
        if (group) {
            auto copy = new KisGroupLayer(working, child->name(), child->opacity());
            copy->setPassThroughMode(group->passThroughMode());
            copy->setDefaultProjectionColor(group->defaultProjectionColor());
            native = copy;
        } else if (child == edited) {
            native = qobject_cast<KisLayer *>(target.data());
        } else {
            native = new KisPaintLayer(working, child->name(), child->opacity(),
                copyNativePixels(child == edited->parent() ? child->original() : child->projection(), new KisDefaultBounds(working)));
        }
        native->setOpacity(child->opacity());
        native->setVisible(child->visible());
        native->setCompositeOpId(child->compositeOpId());
        native->setColorLabelIndex(child->colorLabelIndex());
        native->setChannelFlags(layer->channelFlags());
        if (!working->addNode(native, parent)) throw std::runtime_error("CPU brush sampling layer could not be added");
        if (child == samplingNode) nativeSource = native;
        if (group) {
            const auto nestedSource = copySamplingLayers(child, native, edited, target, working, samplingNode);
            if (nestedSource) nativeSource = nestedSource;
        }
        if (group || child == edited || child == edited->parent()) copySamplingMasks(child, native, working, edited, target);
    }
    return nativeSource;
}
}

struct KisGpuCpuBrushStroke::Private
{
    KisImageWSP source;
    Gpu::PaintImageSP owner;
    Gpu::Snapshot before;
    Gpu::OperationId operation;
    Gpu::LayerId layer = 0;
    bool alpha = false;
    QUuid maskUuid;
    KisImageSP working;
    KisNodeSP target;
    CpuBrushPixels *pixels = nullptr;
    KisNodeSP samplingNode;
    std::shared_ptr<NativePreview> preview = std::make_shared<NativePreview>();
    quint64 publishedSerial = 0;
};

KisGpuCpuBrushStroke::KisGpuCpuBrushStroke() : m_d(std::make_unique<Private>()) {}

std::unique_ptr<KisGpuCpuBrushStroke> KisGpuCpuBrushStroke::create(KisImageSP image, KisNodeSP node, QString &diagnostic,
                                                               Projection projection, KisNodeSP samplingNode, Gpu::HistoryAction action)
{
    diagnostic = i18n("A brush stroke could not be started on this paint layer or mask.");
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    const auto paint = qobject_cast<KisPaintLayer *>(node.data());
    if (!owner || !node || node->image().data() != image.data() || !node->parent() || !node->isEditable() ||
        owner->operations().hasActiveStroke()) return {};
    const auto selectionMask = qobject_cast<KisSelectionMask *>(node.data());
    const bool globalSelection = selectionMask && selectionMask->selection() == image->globalSelection();
    const bool alpha = node->paintDevice() && node->paintDevice()->pixelSize() == 1 &&
        (image->isGpuEditableMaskView(node) || globalSelection);
    if (!paint && !alpha) return {};
    const auto before = owner->operations().snapshot();
    const auto layer = std::find_if(before->layers.cbegin(), before->layers.cend(),
        [&](const auto &value) { return alpha ? bool(Gpu::editableMaskCoverage(value, node->uuid())) :
            value.uuid == node->uuid() && value.kind == Gpu::LayerKind::Paint; });
    if (!globalSelection && layer == before->layers.cend()) return {};
    try {
        auto result = std::unique_ptr<KisGpuCpuBrushStroke>(new KisGpuCpuBrushStroke);
        auto &d = *result->m_d;
        d.source = image; d.owner = owner; d.before = before; d.layer = globalSelection ? 0 : layer->id;
        d.alpha = alpha; d.maskUuid = alpha && !globalSelection ? node->uuid() : QUuid{};
        d.working = new KisImage(new KisDumbUndoStore, image->width(), image->height(), image->colorSpace(), "CPU brush calculation");
        d.working->setResolution(image->xRes(), image->yRes());
        d.working->setWrapAroundModeAxis(image->wrapAroundModeAxis());
        d.working->setWrapAroundModePermitted(image->wrapAroundModePermitted());
        if (projection == Projection::Captured) {
            const auto projection = copyNativePixels(image->projection(), new KisDefaultBounds(d.working));
            d.working->setRootLayer(new CpuBrushRoot(d.working, projection));
        }
        if (alpha) {
            const auto coverage = copyNativeSelection(node->paintDevice(), new KisDefaultBounds(d.working));
            if (globalSelection) {
                auto target = new CpuBrushNode<KisSelectionMask>(d.working, node->name());
                d.target = target;
                target->setSelection(coverage);
                target->setActive(true);
            } else if (const auto adjustment = qobject_cast<KisAdjustmentLayer *>(node.data())) {
                d.target = new CpuBrushNode<KisAdjustmentLayer>(d.working, node->name(),
                    adjustment->filter()->cloneWithResourcesSnapshot(), coverage);
            } else if (const auto filter = qobject_cast<KisFilterMask *>(node.data())) {
                auto target = new CpuBrushNode<KisFilterMask>(d.working, node->name());
                d.target = target;
                target->setSelection(coverage);
                if (filter->filter()) target->setFilter(filter->filter()->cloneWithResourcesSnapshot());
            } else {
                auto target = new CpuBrushNode<KisTransparencyMask>(d.working, node->name());
                d.target = target;
                target->setSelection(coverage);
            }
        } else {
            const auto pixels = kisGpuPaintDeviceTransfer(owner, layer->id, new KisDefaultBounds(d.working), before, image->colorSpace());
            auto target = new CpuBrushLayer(d.working, QStringLiteral("CPU brush pixels"), 255, pixels);
            target->setChannelLockFlags(paint->channelLockFlags());
            d.target = target;
        }
        d.pixels = dynamic_cast<CpuBrushPixels *>(d.target.data());
        if (projection == Projection::Updating) {
            d.working->rootLayer()->setDefaultProjectionColor(image->defaultProjectionColor());
            d.samplingNode = copySamplingLayers(image->root(), d.working->root(), node, d.target, d.working, samplingNode);
            if (samplingNode == image->root()) d.samplingNode = d.working->root();
            if (globalSelection && !d.working->addNode(d.target, d.working->root())) return {};
        } else {
            KisNodeSP parent = d.working->root();
            if (alpha && !globalSelection && !qobject_cast<KisLayer *>(d.target.data())) {
                parent = new KisPaintLayer(d.working, QStringLiteral("CPU mask parent"), 255);
                if (!d.working->addNode(parent)) return {};
            }
            if (!d.working->addNode(d.target, parent)) return {};
        }
        if (!d.target->parent()) return {};
        if (samplingNode == node) {
            d.samplingNode = d.target;
            if (const KisLayerSP target = qobject_cast<KisLayer *>(d.target.data()); target && projection != Projection::Updating)
                copySamplingMasks(node, target, d.working);
        } else if (samplingNode && !d.samplingNode) {
            const auto source = copyNativePixels(samplingNode->projection(), new KisDefaultBounds(d.working));
            d.samplingNode = new KisPaintLayer(d.working, QStringLiteral("CPU brush source"), 255, source);
            d.samplingNode->setVisible(false);
            if (!d.working->addNode(d.samplingNode)) return {};
        }
        d.working->setMirrorAxesCenter(image->mirrorAxesCenter());
        const auto selectionLayer = qobject_cast<KisLayer *>(node.data());
        const auto selection = globalSelection ? KisSelectionSP{} : selectionLayer ? selectionLayer->selection() : image->globalSelection();
        if (selection) {
            // Native selection devices own ordinary CPU pixels, including the
            // default value of an inverted or empty selection.
            const auto coverage = copyNativeSelection(selection->projection(), new KisDefaultBounds(d.working));
            KisSelectionMaskSP mask = new KisSelectionMask(d.working);
            mask->setSelection(coverage); mask->setActive(true);
            if (!d.working->addNode(mask, d.working->root())) return {};
        }
        if (projection == Projection::Updating) {
            const auto area = QRect(QPoint(), before->size) | d.working->rootLayer()->calculateChildrenLooseUserVisibleBounds();
            d.working->refreshGraphAsync(d.working->root(), area, area);
        } else d.working->refreshGraphAsync();
        d.working->waitForDone();
        d.pixels->resetDirty();
        d.publishedSerial = d.pixels->serial();
        d.operation = owner->beginPixelEdit(action);
        if (d.operation.isNull()) return {};
        diagnostic.clear();
        return result;
    } catch (const std::exception &error) {
        diagnostic = i18n("The brush pixels could not be prepared: %1", QString::fromUtf8(error.what()));
        return {};
    }
}

std::unique_ptr<KisGpuCpuBrushStroke> KisGpuCpuBrushStroke::createForPreset(
    KisImageSP image, KisNodeSP node, KisPaintOpPresetSP &preset, QString &diagnostic)
{
    const auto id = preset->paintOp().id();
    const bool duplicate = id == QLatin1String("duplicate");
    const bool sampleProjection = (id == QLatin1String("colorsmudge") && preset->settings()->getBool("MergedPaint", false)) ||
        (duplicate && preset->settings()->getBool("Duplicateop/CloneFromProjection", false));
    KisNodeSP samplingNode;
    if (duplicate && !sampleProjection) {
        samplingNode = preset->settings()->sourceNode();
        if (!samplingNode || !samplingNode->graphListener()) samplingNode = node;
    }
    bool samplingParent = false;
    if (samplingNode && node) for (auto ancestor = node->parent(); ancestor; ancestor = ancestor->parent())
        samplingParent |= ancestor == samplingNode;
    const auto projection = duplicate && (sampleProjection || samplingParent) ? Projection::Updating :
        sampleProjection ? Projection::Captured : Projection::None;
    auto result = create(image, node, diagnostic, projection, samplingNode);
    if (result && samplingNode) {
        preset = preset->clone().dynamicCast<KisPaintOpPreset>();
        preset->settings()->setSourceNode(result->samplingNode());
    }
    return result;
}

KisGpuCpuBrushStroke::~KisGpuCpuBrushStroke()
{
    if (!m_d->operation.isNull()) m_d->owner->cancelPixelEdit(m_d->operation);
}

KisImageSP KisGpuCpuBrushStroke::image() const { return m_d->working; }
KisNodeSP KisGpuCpuBrushStroke::node() const { return m_d->target; }
KisNodeSP KisGpuCpuBrushStroke::samplingNode() const { return m_d->samplingNode; }

bool KisGpuCpuBrushStroke::publish(QRect area, const QByteArray &native)
{
    auto &d = *m_d;
    if (!d.source || d.source->gpuImage() != d.owner || d.owner->operations().snapshot() != d.before ||
        d.owner->operations().activeOperation() != d.operation) return false;
    try {
        if (area.isEmpty()) return true;
        const auto bytes = d.alpha ? native : kisGpuPixelTransfer(native, d.before->format, true);
        const auto result = d.owner->updatePixelEdit(d.operation, [&] {
            return (d.alpha ? d.owner->replaceMaskPixels(d.layer, d.maskUuid, area, bytes, {}) :
                d.owner->replaceRgbaPixels(d.layer, area, bytes)) == Gpu::Result::Success;
        });
        return result == Gpu::Result::Success;
    } catch (const std::exception &) {
        return false;
    }
}

bool KisGpuCpuBrushStroke::updatePreview(KisStrokeId stroke)
{
    try {
        auto &d = *m_d;
        const auto preview = d.preview;
        QRect area;
        QByteArray bytes;
        quint64 serial;
        bool enqueue = false;
        {
            QMutexLocker lock(&preview->mutex);
            if (preview->failed) return false;
            area = preview->area; bytes = preview->bytes; serial = preview->serial;
            enqueue = !preview->pending;
            if (enqueue) preview->pending = true;
        }
        if (enqueue) {
            const auto target = d.target;
            // The exclusive job copies a completed native dab batch, including
            // indirect opacity/selection merging, while the stroke remains open.
            d.working->addJob(stroke, new KisRunnableStrokeJobData([target, preview] {
                const auto pixels = dynamic_cast<const CpuBrushPixels *>(target.data());
                const auto serial = pixels->serial();
                const auto complete = qScopeGuard([&] {
                    QMutexLocker lock(&preview->mutex);
                    preview->pending = false;
                });
                {
                    QMutexLocker lock(&preview->mutex);
                    if (serial == preview->serial) return;
                }
                try {
                    const auto area = pixels->dirtyRect();
                    if (area.isEmpty()) return;
                    const auto bytes = pixels->previewPixels(target, area);
                    QMutexLocker lock(&preview->mutex);
                    preview->bytes = bytes;
                    preview->area = area;
                    preview->serial = serial;
                } catch (const std::exception &) {
                    QMutexLocker lock(&preview->mutex);
                    preview->failed = true;
                }
            }, KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE));
        }
        if (bytes.isEmpty() || serial == d.publishedSerial) return true;
        if (!publish(area, bytes)) return false;
        d.publishedSerial = serial;
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

bool KisGpuCpuBrushStroke::commit()
{
    m_d->working->waitForDone();
    try {
        const auto area = m_d->pixels->dirtyRect();
        if (!area.isEmpty() && !publish(area, m_d->pixels->previewPixels(m_d->target, area))) return false;
    } catch (const std::exception &) {
        return false;
    }
    if (m_d->owner->commitPixelEdit(m_d->operation) != Gpu::Result::Success) return false;
    m_d->operation = {};
    return true;
}
