/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCpuLayer.h"
#include "KisGpuPaintDeviceTransfer_p.h"
#include "kis_default_bounds.h"
#include "kis_paint_device.h"
#include <KoColorSpace.h>
#include <KoColorModelStandardIds.h>
#include <QThread>
#include <exception>

namespace {
class RenderBounds : public KisDefaultBounds
{
public:
    explicit RenderBounds(QSize size) : m_bounds(QPoint(), size) {}
    QRect bounds() const override { return m_bounds; }
private:
    QRect m_bounds;
};
}

bool kisGpuRenderCpuLayer(LibrePaint::Image::Gpu::LayerFeature feature,
    LibrePaint::Image::Gpu::RasterLayer &layer, QSize size, const KoColorSpace *space,
    qsizetype maximumBytes, const std::function<KisPaintDeviceSP(KisPaintDeviceSP)> &render,
    const KisGpuCpuLayerContext &context)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (Gpu::layerExecutionLocation(feature) != Gpu::ExecutionLocation::Cpu ||
        (layer.cpuDefinition && layer.cpuDefinition->feature != feature) ||
        !render || !space || maximumBytes < 0 || size.width() <= 0 || size.height() <= 0 ||
        size.width() > Gpu::OperationDocument::CoordinateLimit || size.height() > Gpu::OperationDocument::CoordinateLimit ||
        space->colorModelId() != RGBAColorModelID ||
        (space->colorDepthId() != Integer8BitsColorDepthID && space->colorDepthId() != Integer16BitsColorDepthID)) return false;
    const auto format = space->colorDepthId() == Integer16BitsColorDepthID ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const auto current = [&] {
        if (!context.image) return !context.version && !context.inputLayer;
        return context.version && QThread::currentThread() == context.image->thread() && context.image->handle() &&
            context.image->operations().preview() == context.version && context.version->format == format;
    };
    if (!current()) return false;
    try {
        KisPaintDeviceSP input;
        if (context.inputLayer)
            input = kisGpuPaintDeviceTransfer(context.image, *context.inputLayer,
                new RenderBounds(context.version->size), context.version, space, maximumBytes);
        if (!current()) return false;
        const auto result = render(input);
        if (!result || *result->colorSpace() != *space || !current()) return false;
        auto prepared = layer;
        if (!kisGpuRasterLayerFromPaintDevice(result, format, maximumBytes, prepared) || !current()) return false;
        if (prepared.cpuDefinition) prepared.cpuDefinition->resolved = true;
        layer = std::move(prepared);
        return true;
    } catch (const std::exception &) {
        return false;
    }
}
