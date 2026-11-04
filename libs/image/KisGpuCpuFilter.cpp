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
    const auto layer = qobject_cast<KisPaintLayer *>(node.data());
    if (!owner || !layer || layer->image() != image || !layer->isEditable() || !configuration ||
        QThread::currentThread() != owner->thread() || owner->operations().hasActiveStroke()) return false;
    const auto filter = KisFilterRegistry::instance()->value(configuration->name());
    if (!filter) return false;
    const auto version = owner->operations().snapshot();
    const auto target = std::find_if(version->layers.cbegin(), version->layers.cend(),
        [&](const auto &value) { return value.uuid == layer->uuid() && value.kind == Gpu::LayerKind::Paint; });
    if (target == version->layers.cend()) return false;
    const auto layerId = target->id;
    const auto device = layer->paintDevice();
    if (!device || int(device->pixelSize()) != Gpu::pixelByteCount(version->format)) return false;
    try {
        const auto config = configuration->cloneWithResourcesSnapshot();
        auto affected = device->exactBounds();
        if (filter->needsTransparentPixels(config, device->colorSpace())) affected |= image->bounds();
        const auto processRect = filter->changedRect(affected, config, 0).intersected(image->bounds());
        auto output = processRect;
        const auto selection = layer->selection();
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
        auto read = owner->execution().readRgbaPixels(owner->handle(), version, input, layerId, inputBytes);
        if (read.code != Gpu::ExecutionCode::Success) return false;
        const auto native = kisGpuPixelTransfer(read.bytes, version->format, false);
        read.bytes.clear();
        KisPaintDeviceSP working = new KisPaintDevice(device->colorSpace());
        working->setDefaultBounds(new KisDefaultBounds(image));
        const auto defaultBytes = kisGpuPixelTransfer(Gpu::rgbaBytes(target->defaultPixel, version->format), version->format, false);
        working->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(defaultBytes.constData()), device->colorSpace()));
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
        return owner->replaceRgbaPixels(layerId, output, kisGpuPixelTransfer(result, version->format, true)) == Gpu::Result::Success;
    } catch (const std::exception &) {
        return false;
    }
}
