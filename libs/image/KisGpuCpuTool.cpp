/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCpuTool.h"
#include "KisGpuPaintDeviceTransfer_p.h"
#include "gpu/KisGpuExecution.h"
#include "kis_default_bounds.h"
#include "kis_image.h"
#include "kis_paint_layer.h"
#include "kis_paint_device.h"
#include "kis_pixel_selection.h"
#include "kis_selection.h"
#include "kis_selection_mask.h"
#include "kis_transaction.h"
#include <KoColor.h>
#include <QScopeGuard>
#include <QThread>
#include <algorithm>
#include <stdexcept>

bool kisGpuApplyCpuTool(KisImageSP image, KisNodeSP node, QRect area,
    LibrePaint::Image::Gpu::HistoryAction action,
    const std::function<bool(KisPaintDeviceSP, KisSelectionSP)> &process)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    if (!owner || !node || node->image().data() != image.data() || !node->parent() || !node->isEditable() || !process ||
        QThread::currentThread() != owner->thread() || owner->operations().hasActiveStroke()) return false;
    const auto device = node->paintDevice();
    if (!device) return false;
    const auto mask = qobject_cast<KisSelectionMask *>(node.data());
    const bool globalSelection = mask && mask->selection() == image->globalSelection();
    const bool alpha = device->pixelSize() == 1 && (image->isGpuEditableMaskView(node) || globalSelection);
    if (!alpha && !qobject_cast<KisPaintLayer *>(node.data())) return false;
    const auto version = owner->operations().snapshot();
    const auto target = std::find_if(version->layers.cbegin(), version->layers.cend(), [&](const auto &layer) {
        return alpha ? bool(Gpu::editableMaskCoverage(layer, node->uuid())) :
            layer.uuid == node->uuid() && layer.kind == Gpu::LayerKind::Paint;
    });
    if (!globalSelection && target == version->layers.cend()) return false;
    if (!alpha && int(device->pixelSize()) != Gpu::pixelByteCount(version->format)) return false;
    const Gpu::LayerId layerId = globalSelection ? 0 : target->id;
    const auto layer = qobject_cast<KisLayer *>(node.data());
    const auto selected = globalSelection ? KisSelectionSP{} : layer ? layer->selection() : image->globalSelection();
    try {
        if (selected) area &= selected->selectedExactRect();
        if (area.isEmpty()) return true;
        constexpr qint64 capacity = 256 * 1024 * 1024;
        const qint64 pixels = qint64(area.width()) * area.height();
        if (pixels <= 0 || pixels > capacity / device->pixelSize()) return false;
        const qint64 count = pixels * device->pixelSize();
        QByteArray input;
        if (alpha) {
            input.resize(count);
            device->readBytes(reinterpret_cast<quint8 *>(input.data()), area);
        } else {
            const auto read = owner->execution().readRgbaPixels(owner->handle(), version, area, layerId, count);
            if (read.code != Gpu::ExecutionCode::Success) return false;
            input = kisGpuPixelTransfer(read.bytes, version->format, false);
        }
        const KisDefaultBoundsBaseSP bounds = globalSelection ?
            KisDefaultBoundsBaseSP(new KisDefaultBounds(image)) : device->defaultBounds();
        // Alpha8 uses its native GrayA composition device for painting.
        KisPaintDeviceSP working = alpha ? KisPaintDeviceSP(new KisPixelSelection) :
            KisPaintDeviceSP(new KisPaintDevice(device->colorSpace()));
        working->setDefaultBounds(bounds);
        working->setDefaultPixel(device->defaultPixel());
        working->writeBytes(reinterpret_cast<const quint8 *>(input.constData()), area);
        KisSelectionSP selection;
        if (selected) {
            const KisPaintDeviceSP source = selected->projection();
            const auto extent = source->extent();
            const qint64 selectionBytes = qint64(extent.width()) * extent.height();
            if (selectionBytes < 0 || selectionBytes > capacity - count) return false;
            selection = new KisSelection;
            KisPixelSelectionSP pixels = selection->pixelSelection();
            pixels->setDefaultBounds(source->defaultBounds());
            pixels->setDefaultPixel(source->defaultPixel());
            if (selectionBytes) {
                QByteArray bytes(selectionBytes, Qt::Uninitialized);
                source->readBytes(reinterpret_cast<quint8 *>(bytes.data()), extent);
                pixels->writeBytes(reinterpret_cast<const quint8 *>(bytes.constData()), extent);
            }
            if (selected->outlineCacheValid()) pixels->setOutlineCache(selected->outlineCache());
        }
        const auto operation = owner->beginPixelEdit(action);
        if (operation.isNull()) return false;
        auto cancel = qScopeGuard([&] { owner->cancelPixelEdit(operation); });
        {
            KisTransaction transaction(working);
            if (!process(working, selection)) return false;
        }
        if (image->gpuImage() != owner || owner->operations().snapshot() != version ||
            owner->operations().activeOperation() != operation) return false;
        QByteArray output(count, Qt::Uninitialized);
        working->readBytes(reinterpret_cast<quint8 *>(output.data()), area);
        if (output == input) return true;
        const auto bytes = alpha ? output : kisGpuPixelTransfer(output, version->format, true);
        if (owner->updatePixelEdit(operation, [&] {
                return (alpha ? owner->replaceMaskPixels(layerId, globalSelection ? QUuid{} : node->uuid(), area, bytes, {}) :
                    owner->replaceRgbaPixels(layerId, area, bytes)) == Gpu::Result::Success;
            }) != Gpu::Result::Success) return false;
        if (owner->commitPixelEdit(operation) != Gpu::Result::Success) return false;
        cancel.dismiss();
        return true;
    } catch (const std::exception &) {
        return false;
    }
}
