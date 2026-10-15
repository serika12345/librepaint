/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintDeviceTransfer_p.h"
#include "kis_paint_device.h"
#include "gpu/KisGpuExecution.h"
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <algorithm>
#include <stdexcept>

KisPaintDeviceSP kisGpuPaintDeviceTransfer(const LibrePaint::Image::Gpu::PaintImageSP &image,
                                         quint64 layer, KisDefaultBoundsBaseSP bounds,
                                         const LibrePaint::Image::Gpu::Snapshot &version)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    constexpr qsizetype capacity = 256 * 1024 * 1024;
    if (!layer) layer = version->directProjectionLayer();
    const auto validated = image->execution().readRgbaPixels(image->handle(), version, {}, layer, 0);
    if (validated.code != Gpu::ExecutionCode::Success)
        throw std::runtime_error(validated.diagnostic.toStdString());
    KisPaintDeviceSP result = new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8());
    result->setDefaultBounds(bounds);
    const auto write = [&](QByteArray rgba, QRect rect) {
        for (qsizetype i = 0; i < rgba.size(); i += 4) std::swap(rgba[i], rgba[i + 2]);
        result->writeBytes(reinterpret_cast<const quint8 *>(rgba.constData()), rect);
    };
    if (layer) {
        for (const auto &value : version->layers) {
            if (value.id != layer) continue;
            const quint8 bgra[] = {value.defaultPixel[2], value.defaultPixel[1], value.defaultPixel[0], value.defaultPixel[3]};
            result->setDefaultPixel(KoColor(bgra, result->colorSpace()));
        }
        const auto pixels = image->execution().readLayerPixels(image->handle(), version, layer, capacity);
        if (pixels.code != Gpu::ExecutionCode::Success)
            throw std::runtime_error(pixels.diagnostic.toStdString());
        for (const auto &tile : pixels.tiles)
            write(tile.rgba, QRect(tile.coordinate * Gpu::RgbaTile::Size, QSize(Gpu::RgbaTile::Size, Gpu::RgbaTile::Size)));
    } else {
        const auto &rgba = version->projectionBackground;
        const quint8 bgra[] = {rgba[2], rgba[1], rgba[0], rgba[3]};
        result->setDefaultPixel(KoColor(bgra, result->colorSpace()));
        bool empty = rgba[3] == 0;
        for (const auto &value : version->layers)
            if (value.visible && (!value.operations.isEmpty() || value.defaultPixel[3])) empty = false;
        if (!empty) {
            const auto pixels = image->execution().readProjectionImage(image->handle(), version, capacity);
            if (pixels.code != Gpu::ExecutionCode::Success)
                throw std::runtime_error(pixels.diagnostic.toStdString());
            for (int y = 0; y < pixels.image.height(); ++y)
                write(QByteArray(reinterpret_cast<const char *>(pixels.image.constScanLine(y)), pixels.image.width() * 4),
                      QRect(0, y, pixels.image.width(), 1));
        }
    }
    return result;
}
