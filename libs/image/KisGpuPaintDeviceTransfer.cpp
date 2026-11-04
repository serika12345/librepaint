/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintDeviceTransfer_p.h"
#include "kis_paint_device.h"
#include "kis_default_bounds.h"
#include "gpu/KisGpuExecution.h"
#include "gpu/KisGpuPaintGeometry_p.h"
#include <KisRegion.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorModelStandardIds.h>
#include <KoColor.h>
#include <QSet>
#include <algorithm>
#include <QtEndian>
#include <cstring>
#include <stdexcept>

// Public pixels use native-endian BGRA; GPU pixels use little-endian RGBA.
QByteArray kisGpuPixelTransfer(QByteArrayView source, LibrePaint::Image::Gpu::PixelFormat format, bool toGpu)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    QByteArray output(source.size(), Qt::Uninitialized);
    const int componentBytes = Gpu::pixelByteCount(format) / 4;
    for (qsizetype i = 0; i < source.size(); i += componentBytes * 4) {
        for (int c = 0; c < 4; ++c) {
            const int other = c == 0 ? 2 : c == 2 ? 0 : c;
            const auto input = source.data() + i + other * componentBytes;
            auto destination = output.data() + i + c * componentBytes;
            if (componentBytes == 1) *destination = *input;
            else if (toGpu) {
                quint16 value;
                std::memcpy(&value, input, sizeof(value));
                qToLittleEndian(value, destination);
            } else {
                const quint16 value = qFromLittleEndian<quint16>(input);
                std::memcpy(destination, &value, sizeof(value));
            }
        }
    }
    return output;
}

bool kisGpuRasterLayerFromPaintDevice(KisPaintDeviceSP source,
    LibrePaint::Image::Gpu::PixelFormat format, qsizetype maximumBytes,
    LibrePaint::Image::Gpu::RasterLayer &layer)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!source || maximumBytes < 0 || !Gpu::pixelByteCount(format) ||
        source->colorSpace()->colorModelId() != RGBAColorModelID ||
        source->colorSpace()->colorDepthId() != (format == Gpu::PixelFormat::Rgba16 ?
            Integer16BitsColorDepthID : Integer8BitsColorDepthID)) return false;
    constexpr int side = Gpu::RgbaTile::Size;
    constexpr int limit = Gpu::RgbaTile::CoordinateLimit;
    const qsizetype tileBytes = Gpu::RgbaTile::byteCount(format);
    const auto tileIndex = [](int value) { return int(value >= 0 ? qint64(value) / side : (qint64(value) - side + 1) / side); };
    try {
        // Read the full-resolution snapshot independently of LoD and wraparound.
        KisPaintDevice fixed(*source);
        fixed.setDefaultBounds(new KisDefaultBounds);
        const auto region = fixed.region().rects();
        QSet<QPoint> coordinates;
        for (const auto &area : region) {
            if (area.left() < -limit || area.top() < -limit || area.right() >= limit || area.bottom() >= limit) return false;
            for (int y = tileIndex(area.top()); y <= tileIndex(area.bottom()); ++y) {
                for (int x = tileIndex(area.left()); x <= tileIndex(area.right()); ++x) {
                    coordinates.insert({x, y});
                    if (coordinates.size() > maximumBytes / tileBytes) return false;
                }
            }
        }
        auto ordered = coordinates.values();
        std::sort(ordered.begin(), ordered.end(), [](QPoint left, QPoint right) {
            return left.y() == right.y() ? left.x() < right.x() : left.y() < right.y();
        });
        QList<Gpu::RgbaTile> tiles;
        for (const auto point : ordered) {
            QByteArray bytes(tileBytes, Qt::Uninitialized);
            fixed.readBytes(reinterpret_cast<quint8 *>(bytes.data()), point.x() * side, point.y() * side, side, side);
            tiles.append({point, kisGpuPixelTransfer(bytes, format, true), format});
        }
        const auto background = fixed.defaultPixel();
        const auto rgba = kisGpuPixelTransfer(QByteArrayView(reinterpret_cast<const char *>(background.data()),
            Gpu::pixelByteCount(format)), format, true);
        Gpu::RgbaPixel defaultPixel{};
        for (int c = 0; c < 4; ++c)
            defaultPixel[c] = format == Gpu::PixelFormat::Rgba16 ?
                qFromLittleEndian<quint16>(rgba.constData() + c * 2) : quint8(rgba[c]);
        QList<QRect> writtenAreas;
        for (const auto &area : region) {
            for (int y = area.top(); y <= area.bottom(); y += Gpu::Detail::PixelHistoryBlockSize) {
                for (int x = area.left(); x <= area.right(); x += Gpu::Detail::PixelHistoryBlockSize) {
                    const QRect block = area.intersected(QRect(x, y,
                        Gpu::Detail::PixelHistoryBlockSize, Gpu::Detail::PixelHistoryBlockSize));
                    // A shifted native block can straddle up to four GPU tiles.
                    for (int ty = tileIndex(block.top()); ty <= tileIndex(block.bottom()); ++ty)
                        for (int tx = tileIndex(block.left()); tx <= tileIndex(block.right()); ++tx)
                            writtenAreas.append(block.intersected(QRect(tx * side, ty * side, side, side)));
                }
            }
        }
        layer.tiles = std::move(tiles);
        layer.defaultPixel = defaultPixel;
        layer.writtenAreas = std::move(writtenAreas);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

KisPaintDeviceSP kisGpuPaintDeviceTransfer(const LibrePaint::Image::Gpu::PaintImageSP &image,
                                         quint64 layer, KisDefaultBoundsBaseSP bounds,
                                         const LibrePaint::Image::Gpu::Snapshot &version,
                                         const KoColorSpace *colorSpace, qsizetype maximumBytes)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!image || !version || maximumBytes < 0)
        throw std::invalid_argument("invalid GPU pixel transfer source or capacity");
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
        const auto pixels = image->execution().readLayerPixels(image->handle(), version, layer, maximumBytes);
        if (pixels.code != Gpu::ExecutionCode::Success)
            throw std::runtime_error(pixels.diagnostic.toStdString());
        setDefault(version->projectionDefaultPixels().value(layer));
        for (const auto &tile : pixels.tiles)
            write(tile.rgba, QRect(tile.coordinate * Gpu::RgbaTile::Size, QSize(Gpu::RgbaTile::Size, Gpu::RgbaTile::Size)));
    } else {
        const auto &rgba = version->projectionBackground;
        setDefault(rgba);
        bool empty = rgba[3] == 0;
        for (const auto &value : version->layers)
            if (value.visible && (value.pixels || !value.operations.isEmpty() || value.defaultPixel[3])) empty = false;
        if (!empty) {
            const auto pixels = image->execution().readProjectionImage(image->handle(), version, maximumBytes);
            if (pixels.code != Gpu::ExecutionCode::Success)
                throw std::runtime_error(pixels.diagnostic.toStdString());
            for (int y = 0; y < pixels.image.height(); ++y)
                write(QByteArray(reinterpret_cast<const char *>(pixels.image.constScanLine(y)), pixels.image.width() * Gpu::pixelByteCount(version->format)),
                      QRect(0, y, pixels.image.width(), 1), false);
        }
    }
    return result;
}
