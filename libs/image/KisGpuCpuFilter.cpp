/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCpuFilter.h"
#include "kis_image.h"
#include "KisGpuPaintDeviceTransfer_p.h"
#include "filter/kis_filter.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter_registry.h"
#include "gpu/KisGpuExecution.h"
#include "kis_default_bounds.h"
#include "kis_paint_layer.h"
#include "kis_selection_mask.h"
#include "kis_paint_device.h"
#include "kis_pixel_selection.h"
#include "kis_selection.h"
#include "kis_transaction.h"
#include <KoColor.h>
#include <QThread>
#include <algorithm>
#include <stdexcept>

bool kisGpuApplyCpuFilter(KisImage *image, KisNodeSP node, KisFilterConfigurationSP configuration)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP();
    if (!owner || !node || node->image() != image || !node->parent() || !node->isEditable() || !configuration ||
        QThread::currentThread() != owner->thread() || owner->operations().hasActiveStroke()) return false;
    const auto filter = KisFilterRegistry::instance()->value(configuration->name());
    if (!filter) return false;
    const auto version = owner->operations().snapshot();
    const auto device = node->paintDevice();
    const auto layer = qobject_cast<KisPaintLayer *>(node.data());
    const auto selectionLayer = qobject_cast<KisLayer *>(node.data());
    const auto selectionMask = qobject_cast<KisSelectionMask *>(node.data());
    const bool globalSelection = selectionMask && selectionMask->selection() == image->globalSelection();
    const bool alpha = device && device->pixelSize() == 1 && (image->isGpuEditableMaskView(node) || globalSelection);
    if (!device || (!alpha && !layer)) return false;
    const auto target = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [&](const auto &value) { return alpha ? bool(Gpu::editableMaskCoverage(value, node->uuid())) :
            value.uuid == node->uuid() && value.kind == Gpu::LayerKind::Paint; });
    if (!globalSelection && target == version->layers.cend()) return false;
    const auto layerId = globalSelection ? 0 : target->id;
    if (!alpha && int(device->pixelSize()) != Gpu::pixelByteCount(version->format)) return false;
    try {
        const auto config = configuration->cloneWithResourcesSnapshot();
        auto affected = device->exactBounds();
        // The global selection's editing domain is the canvas; its display
        // view can carry empty default bounds.
        const KisDefaultBoundsBaseSP defaultBounds = globalSelection ?
            KisDefaultBoundsBaseSP(new KisDefaultBounds(image)) : device->defaultBounds();
        const auto bounds = defaultBounds->bounds();
        if (filter->needsTransparentPixels(config, device->colorSpace())) affected |= bounds;
        const auto processRect = filter->changedRect(affected, config, 0).intersected(bounds);
        auto output = processRect;
        const auto selection = globalSelection ? KisSelectionSP{} : selectionLayer ? selectionLayer->selection() : image->globalSelection();
        if (selection) output &= selection->selectedRect();
        if (output.isEmpty()) return true;
        // The native stroke processes its full changed rectangle before
        // merging the selection, including filters that derive global values.
        const auto input = filter->neededRect(processRect, config, 0).united(processRect);
        // Bound external transfers before allocating their buffers.
        constexpr qint64 maximumBytes = 256 * 1024 * 1024;
        const auto byteCount = [&](QRect area) {
            return qint64(area.width()) * area.height() * device->pixelSize();
        };
        const auto inputBytes = byteCount(input);
        const auto outputBytes = byteCount(output);
        if (inputBytes <= 0 || outputBytes <= 0 || inputBytes > maximumBytes ||
            outputBytes > maximumBytes - inputBytes) return false;
        QByteArray native;
        if (alpha) {
            native.resize(inputBytes);
            device->readBytes(reinterpret_cast<quint8 *>(native.data()), input);
        } else {
            const auto read = owner->execution().readRgbaPixels(owner->handle(), version, input, layerId, inputBytes);
            if (read.code != Gpu::ExecutionCode::Success) return false;
            native = kisGpuPixelTransfer(read.bytes, version->format, false);
        }
        // Pixel selections filter through their native GrayA composition
        // device, then convert back to Alpha8 when merging the result.
        KisPaintDeviceSP working = alpha ? KisPaintDeviceSP(new KisPixelSelection) :
            KisPaintDeviceSP(new KisPaintDevice(device->colorSpace()));
        working->setDefaultBounds(defaultBounds);
        working->setDefaultPixel(device->defaultPixel());
        working->writeBytes(reinterpret_cast<const quint8 *>(native.constData()), input);
        KisSelectionSP coverage;
        if (selection) {
            QByteArray alpha(qsizetype(output.width()) * output.height(), Qt::Uninitialized);
            selection->projection()->readBytes(reinterpret_cast<quint8 *>(alpha.data()), output);
            coverage = new KisSelection;
            coverage->pixelSelection()->writeBytes(reinterpret_cast<const quint8 *>(alpha.constData()), output);
        }
        {
            // Neighbour reads use oldRawData throughout the native filter.
            KisTransaction transaction(working);
            filter->process(working, working, coverage, processRect, config);
        }
        QByteArray result(outputBytes, Qt::Uninitialized);
        working->readBytes(reinterpret_cast<quint8 *>(result.data()), output);
        if (image->gpuImage() != owner || owner->operations().snapshot() != version) return false;
        return (alpha ? owner->replaceMaskPixels(layerId, globalSelection ? QUuid{} : node->uuid(), output, result, config->name()) :
            owner->replaceRgbaPixels(layerId, output, kisGpuPixelTransfer(result, version->format, true), config->name())) == Gpu::Result::Success;
    } catch (const std::exception &) {
        return false;
    }
}

KisPaintDeviceSP kisGpuFilterConfigurationDevice(KisImage *image, KisNodeSP node)
{
    const auto owner = image ? image->gpuImage() : LibrePaint::Image::Gpu::PaintImageSP{};
    const auto device = node ? node->paintDevice() : KisPaintDeviceSP{};
    if (!owner || !device || node->image() != image || QThread::currentThread() != owner->thread()) return {};
    const auto version = owner->operations().snapshot();
    if (device->pixelSize() != 1) {
        for (const auto &layer : version->layers) if (layer.uuid == node->uuid())
            return kisGpuPaintDeviceTransfer(owner, layer.id, device->defaultBounds(), version, device->colorSpace());
        return {};
    }
    const auto area = device->extent();
    const qint64 count = qint64(area.width()) * area.height();
    if (count < 0 || count > 256 * 1024 * 1024) return {};
    KisPaintDeviceSP native = new KisPixelSelection;
    const auto selection = qobject_cast<KisSelectionMask *>(node.data());
    native->setDefaultBounds(selection && selection->selection() == image->globalSelection() ?
        KisDefaultBoundsBaseSP(new KisDefaultBounds(image)) : device->defaultBounds());
    native->setDefaultPixel(device->defaultPixel());
    if (!area.isEmpty()) {
        QByteArray bytes(count, Qt::Uninitialized);
        device->readBytes(reinterpret_cast<quint8 *>(bytes.data()), area);
        native->writeBytes(reinterpret_cast<const quint8 *>(bytes.constData()), area);
    }
    return native;
}
