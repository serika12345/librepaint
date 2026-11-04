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
#include <kis_selection.h>
#include <kis_pixel_selection.h>
#include <kis_selection_mask.h>
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

class CpuBrushLayer final : public KisPaintLayer
{
public:
    using KisPaintLayer::setDirty;
    CpuBrushLayer(KisImageWSP image, KisPaintDeviceSP device)
        : KisPaintLayer(image, QStringLiteral("CPU brush pixels"), 255, device) {}

    void setDirty(const QVector<QRect> &rects) override
    {
        {
            QMutexLocker lock(&m_mutex);
            for (const auto &rect : rects) m_dirty |= rect;
            ++m_serial;
        }
        KisPaintLayer::setDirty(rects);
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

    QByteArray previewPixels(QRect area) const
    {
        const qint64 count = qint64(area.width()) * area.height() * colorSpace()->pixelSize();
        if (count <= 0 || count > 256 * 1024 * 1024) throw std::length_error("CPU brush preview exceeds capacity");
        QByteArray bytes(count, Qt::Uninitialized);
        KisPaintDeviceSP pixels = paintDevice();
        if (hasTemporaryTarget()) {
            pixels = new KisPaintDevice(colorSpace());
            pixels->setDefaultBounds(paintDevice()->defaultBounds());
            copyOriginalToProjection(paintDevice(), pixels, area);
        }
        pixels->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        return bytes;
    }

private:
    mutable QMutex m_mutex;
    QRect m_dirty;
    quint64 m_serial = 0;
};

struct NativePreview
{
    QMutex mutex;
    QRect area;
    QByteArray bytes;
    quint64 serial = 0;
    bool pending = false;
    bool failed = false;
};
}

struct KisGpuCpuBrushStroke::Private
{
    KisImageWSP source;
    Gpu::PaintImageSP owner;
    Gpu::Snapshot before;
    Gpu::OperationId operation;
    Gpu::LayerId layer = 0;
    KisImageSP working;
    KisSharedPtr<CpuBrushLayer> target;
    std::shared_ptr<NativePreview> preview = std::make_shared<NativePreview>();
    quint64 publishedSerial = 0;
};

KisGpuCpuBrushStroke::KisGpuCpuBrushStroke() : m_d(std::make_unique<Private>()) {}

std::unique_ptr<KisGpuCpuBrushStroke> KisGpuCpuBrushStroke::create(KisImageSP image, KisNodeSP node, QString &diagnostic,
                                                               bool sampleProjection)
{
    diagnostic = i18n("A brush stroke could not be started on this paint layer.");
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    const auto paint = qobject_cast<KisPaintLayer *>(node.data());
    if (!owner || !paint || node->image().data() != image.data() || !node->parent() || !node->isEditable() ||
        owner->operations().hasActiveStroke()) return {};
    const auto before = owner->operations().snapshot();
    const auto layer = std::find_if(before->layers.cbegin(), before->layers.cend(),
        [&](const auto &value) { return value.uuid == node->uuid() && value.kind == Gpu::LayerKind::Paint; });
    if (layer == before->layers.cend()) return {};
    try {
        auto result = std::unique_ptr<KisGpuCpuBrushStroke>(new KisGpuCpuBrushStroke);
        auto &d = *result->m_d;
        d.source = image; d.owner = owner; d.before = before; d.layer = layer->id;
        d.working = new KisImage(new KisDumbUndoStore, image->width(), image->height(), image->colorSpace(), "CPU brush calculation");
        d.working->setResolution(image->xRes(), image->yRes());
        if (sampleProjection) {
            const auto projection = copyNativePixels(image->projection(), new KisDefaultBounds(d.working));
            d.working->setRootLayer(new CpuBrushRoot(d.working, projection));
        }
        const auto pixels = kisGpuPaintDeviceTransfer(owner, layer->id, new KisDefaultBounds(d.working), before, image->colorSpace());
        d.target = new CpuBrushLayer(d.working, pixels);
        d.target->setChannelLockFlags(paint->channelLockFlags());
        if (!d.working->addNode(d.target)) return {};
        d.working->setMirrorAxesCenter(image->mirrorAxesCenter());
        if (const auto selection = paint->selection()) {
            // Native selection devices own ordinary CPU pixels, including the
            // default value of an inverted or empty selection.
            KisSelectionSP coverage = new KisSelection;
            coverage->pixelSelection()->setDefaultBounds(new KisDefaultBounds(d.working));
            const KisPaintDeviceSP source = selection->projection();
            coverage->pixelSelection()->setDefaultPixel(source->defaultPixel());
            const QRect area = source->extent();
            const qint64 count = qint64(area.width()) * area.height();
            if (count > 256 * 1024 * 1024) return {};
            if (count > 0) {
                QByteArray bytes(count, Qt::Uninitialized);
                source->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
                coverage->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(bytes.constData()), area);
            }
            KisSelectionMaskSP mask = new KisSelectionMask(d.working);
            mask->setSelection(coverage); mask->setActive(true);
            if (!d.working->addNode(mask, d.working->root())) return {};
        }
        d.working->refreshGraphAsync();
        d.working->waitForDone();
        d.target->resetDirty();
        d.publishedSerial = d.target->serial();
        d.operation = owner->beginPixelEdit(Gpu::HistoryAction::PaintStroke);
        if (d.operation.isNull()) return {};
        diagnostic.clear();
        return result;
    } catch (const std::exception &error) {
        diagnostic = i18n("The brush pixels could not be prepared: %1", QString::fromUtf8(error.what()));
        return {};
    }
}

KisGpuCpuBrushStroke::~KisGpuCpuBrushStroke()
{
    if (!m_d->operation.isNull()) m_d->owner->cancelPixelEdit(m_d->operation);
}

KisImageSP KisGpuCpuBrushStroke::image() const { return m_d->working; }
KisNodeSP KisGpuCpuBrushStroke::node() const { return m_d->target; }

bool KisGpuCpuBrushStroke::publish(QRect area, const QByteArray &native)
{
    auto &d = *m_d;
    if (!d.source || d.source->gpuImage() != d.owner || d.owner->operations().snapshot() != d.before ||
        d.owner->operations().activeOperation() != d.operation) return false;
    try {
        if (area.isEmpty()) return true;
        const auto bytes = kisGpuPixelTransfer(native, d.before->format, true);
        const auto result = d.owner->updatePixelEdit(d.operation, [&] {
            return d.owner->replaceRgbaPixels(d.layer, area, bytes) == Gpu::Result::Success;
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
                const auto serial = target->serial();
                const auto complete = qScopeGuard([&] {
                    QMutexLocker lock(&preview->mutex);
                    preview->pending = false;
                });
                {
                    QMutexLocker lock(&preview->mutex);
                    if (serial == preview->serial) return;
                }
                try {
                    const auto area = target->dirtyRect();
                    if (area.isEmpty()) return;
                    const auto bytes = target->previewPixels(area);
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
        const auto area = m_d->target->dirtyRect();
        if (!area.isEmpty() && !publish(area, m_d->target->previewPixels(area))) return false;
    } catch (const std::exception &) {
        return false;
    }
    if (m_d->owner->commitPixelEdit(m_d->operation) != Gpu::Result::Success) return false;
    m_d->operation = {};
    return true;
}
