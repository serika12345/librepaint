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
#include <QThread>
#include <algorithm>
#include <QtEndian>
#include <cstring>
#include <stdexcept>

qsizetype kisNativePixelByteCount(QRect region, int pixelSize, qsizetype maximumBytes)
{
    if (region.width() < 0 || region.height() < 0 || pixelSize <= 0 || maximumBytes < 0 ||
        qint64(region.x()) + region.width() > qint64(std::numeric_limits<int>::max()) + 1 ||
        qint64(region.y()) + region.height() > qint64(std::numeric_limits<int>::max()) + 1)
        throw std::invalid_argument("Invalid native pixel transfer dimensions");
    if (region.isEmpty()) return 0;
    if (region.width() > maximumBytes / pixelSize / region.height())
        throw std::length_error("Native pixel transfer capacity exceeded");
    return qsizetype(region.width()) * region.height() * pixelSize;
}

// Public pixels use native-endian BGRA; GPU pixels use little-endian RGBA.
QByteArray kisGpuPixelTransfer(QByteArrayView source, LibrePaint::Image::Gpu::PixelFormat format, bool toGpu)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const int pixelSize = Gpu::pixelByteCount(format);
    if (!pixelSize || source.size() % pixelSize)
        throw std::invalid_argument("GPU pixel transfer requires complete pixels at a supported precision");
    QByteArray output(source.size(), Qt::Uninitialized);
    const int componentBytes = pixelSize / 4;
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

KisGpuRasterImport::KisGpuRasterImport(QSize size, const KoColorSpace *colorSpace, qsizetype maximumBytes)
    : m_size(size)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!colorSpace || colorSpace->colorModelId() != RGBAColorModelID ||
        (colorSpace->colorDepthId() != Integer8BitsColorDepthID && colorSpace->colorDepthId() != Integer16BitsColorDepthID) ||
        size.isEmpty() || size.width() > Gpu::RgbaTile::CoordinateLimit || size.height() > Gpu::RgbaTile::CoordinateLimit)
        throw std::invalid_argument("Native raster import requires supported RGBA dimensions and precision");
    m_format = colorSpace->colorDepthId() == Integer16BitsColorDepthID ? Gpu::PixelFormat::Rgba16 : Gpu::PixelFormat::Rgba8;
    const int columns = (size.width() + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
    const int rows = (size.height() + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
    const auto tileBytes = Gpu::RgbaTile::byteCount(m_format);
    if (maximumBytes < 0 || qint64(columns) * rows > maximumBytes / tileBytes)
        throw std::length_error("Native raster import tile capacity exceeded");
    m_tiles.reserve(columns * rows);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns; ++x)
            m_tiles.append({QPoint(x, y), QByteArray(tileBytes, '\0'), m_format});
}

void KisGpuRasterImport::appendRow(QByteArrayView nativePixels)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    const int pixelBytes = Gpu::pixelByteCount(m_format);
    if (m_tiles.isEmpty() || m_nextRow >= m_size.height() || nativePixels.size() != qsizetype(m_size.width()) * pixelBytes)
        throw std::invalid_argument("Native raster import requires one complete consecutive row");
    const auto rgba = kisGpuPixelTransfer(nativePixels, m_format, true);
    const int side = Gpu::RgbaTile::Size;
    const int columns = (m_size.width() + side - 1) / side;
    for (int x = 0; x < m_size.width(); x += side) {
        auto &tile = m_tiles[(m_nextRow / side) * columns + x / side];
        std::memcpy(tile.rgba.data() + (m_nextRow % side) * side * pixelBytes,
            rgba.constData() + qsizetype(x) * pixelBytes, size_t(std::min(side, m_size.width() - x)) * pixelBytes);
    }
    ++m_nextRow;
}

LibrePaint::Image::Gpu::RasterLayer KisGpuRasterImport::takeLayer()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (m_tiles.isEmpty() || m_nextRow != m_size.height())
        throw std::invalid_argument("Native raster import is incomplete or already consumed");
    Gpu::RasterLayer layer;
    constexpr int blockSize = Gpu::Detail::PixelHistoryBlockSize;
    for (int y = 0; y < m_size.height(); y += blockSize)
        for (int x = 0; x < m_size.width(); x += blockSize)
            layer.writtenAreas.append(QRect(x, y, std::min(blockSize, m_size.width() - x), std::min(blockSize, m_size.height() - y)));
    layer.tiles = std::move(m_tiles);
    return layer;
}

QByteArray kisGpuReadNativePixels(const LibrePaint::Image::Gpu::PaintImageSP &image,
    const LibrePaint::Image::Gpu::Snapshot &version, quint64 layer, QRect region,
    qsizetype maximumBytes, KisGpuPixelReadSource source)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!image || !version)
        throw std::invalid_argument("Native pixel reads require an image and an explicit version");
    const auto count = kisNativePixelByteCount(region, Gpu::pixelByteCount(version->format), maximumBytes);
    Gpu::PixelBytesOutcome read;
    switch (source) {
    case KisGpuPixelReadSource::Layer:
        if (!layer) throw std::invalid_argument("Native layer reads require a layer");
        read = image->execution().readRgbaPixels(image->handle(), version, region, layer, count);
        break;
    case KisGpuPixelReadSource::LayerProjection:
        if (!layer) throw std::invalid_argument("Native layer projection reads require a layer");
        read = image->execution().readLayerProjectionPixels(image->handle(), version, layer, region, count);
        break;
    case KisGpuPixelReadSource::CanvasProjection:
        read = image->execution().readRgbaPixels(image->handle(), version, region, 0, count);
        break;
    case KisGpuPixelReadSource::NativeProjection:
        read = image->execution().readNativeProjectionPixels(image->handle(), version, region, count);
        break;
    }
    if (read.code != Gpu::ExecutionCode::Success)
        throw std::runtime_error(read.diagnostic.toStdString());
    if (read.bytes.size() != count)
        throw std::runtime_error("GPU native pixel read returned incomplete pixels");
    return kisGpuPixelTransfer(read.bytes, version->format, false);
}

LibrePaint::Image::Gpu::Result kisGpuWriteNativePixels(const LibrePaint::Image::Gpu::PaintImageSP &image,
    const LibrePaint::Image::Gpu::LayerPixelReferenceSP &pixels, QRect region, QByteArrayView bytes)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!image || QThread::currentThread() != image->thread() || !pixels || !pixels->version())
        return Gpu::Result::InvalidInput;
    try {
        const auto format = pixels->version()->format;
        const auto count = kisNativePixelByteCount(region, Gpu::pixelByteCount(format), bytes.size());
        if (count != bytes.size()) return Gpu::Result::InvalidInput;
        constexpr qint64 limit = Gpu::RgbaTile::CoordinateLimit;
        if (region.x() < -limit || region.y() < -limit ||
            qint64(region.x()) + region.width() > limit || qint64(region.y()) + region.height() > limit)
            return Gpu::Result::InvalidInput;
        if (count > image->limits().rasterBytes) return Gpu::Result::CapacityExceeded;
        return image->writeRgbaPixels(pixels, region, kisGpuPixelTransfer(bytes, format, true));
    } catch (const std::length_error &) {
        return Gpu::Result::CapacityExceeded;
    } catch (const std::invalid_argument &) {
        return Gpu::Result::InvalidInput;
    } catch (const std::bad_alloc &) {
        return Gpu::Result::CapacityExceeded;
    }
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
    // QImage RGBA64 rows use native endian; sparse tile bytes are canonical.
    const auto write = [&](QByteArray rgba, QRect rect, bool canonical = true) {
        if (!canonical && version->format == Gpu::PixelFormat::Rgba16)
            qToLittleEndian<quint16>(rgba.constData(), rgba.size() / 2, rgba.data());
        const auto native = kisGpuPixelTransfer(rgba, version->format, false);
        result->writeBytes(reinterpret_cast<const quint8 *>(native.constData()), rect);
    };
    const auto setDefault = [&](const Gpu::RgbaPixel &value) {
        const auto native = kisGpuPixelTransfer(Gpu::rgbaBytes(value, version->format), version->format, false);
        result->setDefaultPixel(KoColor(reinterpret_cast<const quint8 *>(native.constData()), colorSpace));
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
