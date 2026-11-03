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
#include <QtEndian>
#include <cstring>
#include <stdexcept>

KisPaintDeviceSP kisGpuPaintDeviceTransfer(const LibrePaint::Image::Gpu::PaintImageSP &image,
                                         quint64 layer, KisDefaultBoundsBaseSP bounds,
                                         const LibrePaint::Image::Gpu::Snapshot &version,
                                         const KoColorSpace *colorSpace)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    constexpr qsizetype capacity = 256 * 1024 * 1024;
    if (!layer) layer = version->directProjectionLayer();
    const auto validated = image->execution().readRgbaPixels(image->handle(), version, {}, layer, 0);
    if (validated.code != Gpu::ExecutionCode::Success)
        throw std::runtime_error(validated.diagnostic.toStdString());
    if (!colorSpace || colorSpace->pixelSize() != Gpu::pixelByteCount(version->format) ||
        colorSpace->colorModelId().id() != QLatin1String("RGBA"))
        throw std::invalid_argument("GPU preview precision and document color encoding must match");
    KisPaintDeviceSP result = new KisPaintDevice(colorSpace);
    result->setDefaultBounds(bounds);
    // Tile bytes are canonical little-endian; QImage RGBA64 rows use native endian.
    const auto write = [&](QByteArray rgba, QRect rect, bool canonical = true) {
        if (version->format == Gpu::PixelFormat::Rgba16) {
            for (qsizetype i = 0; i < rgba.size(); i += 8) {
                quint16 bgra[4];
                for (int c = 0; c < 4; ++c) {
                    const auto source = rgba.constData() + i + (c == 0 ? 2 : c == 2 ? 0 : c) * 2;
                    if (canonical) bgra[c] = qFromLittleEndian<quint16>(source);
                    else std::memcpy(&bgra[c], source, sizeof(quint16));
                }
                std::memcpy(rgba.data() + i, bgra, sizeof(bgra));
            }
        } else for (qsizetype i = 0; i < rgba.size(); i += 4) std::swap(rgba[i], rgba[i + 2]);
        result->writeBytes(reinterpret_cast<const quint8 *>(rgba.constData()), rect);
    };
    const auto setDefault = [&](const Gpu::RgbaPixel &value) {
        if (version->format == Gpu::PixelFormat::Rgba16) {
            const quint16 bgra[] = {value[2], value[1], value[0], value[3]};
            result->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(bgra), colorSpace));
        } else {
            const quint8 bgra[] = {quint8(value[2]), quint8(value[1]), quint8(value[0]), quint8(value[3])};
            result->setDefaultPixel(KoColor(bgra, colorSpace));
        }
    };
    if (layer) {
        setDefault(version->projectionDefaultPixels().value(layer));
        const auto pixels = image->execution().readLayerPixels(image->handle(), version, layer, capacity);
        if (pixels.code != Gpu::ExecutionCode::Success)
            throw std::runtime_error(pixels.diagnostic.toStdString());
        for (const auto &tile : pixels.tiles)
            write(tile.rgba, QRect(tile.coordinate * Gpu::RgbaTile::Size, QSize(Gpu::RgbaTile::Size, Gpu::RgbaTile::Size)));
    } else {
        const auto &rgba = version->projectionBackground;
        setDefault(rgba);
        bool empty = rgba[3] == 0;
        for (const auto &value : version->layers)
            if (value.visible && (!value.operations.isEmpty() || value.defaultPixel[3])) empty = false;
        if (!empty) {
            const auto pixels = image->execution().readProjectionImage(image->handle(), version, capacity);
            if (pixels.code != Gpu::ExecutionCode::Success)
                throw std::runtime_error(pixels.diagnostic.toStdString());
            for (int y = 0; y < pixels.image.height(); ++y)
                write(QByteArray(reinterpret_cast<const char *>(pixels.image.constScanLine(y)), pixels.image.width() * Gpu::pixelByteCount(version->format)),
                      QRect(0, y, pixels.image.width(), 1), false);
        }
    }
    return result;
}
