/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImportExportManager.h"
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_default_bounds.h>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <chrono>
#include <exception>
#include <stdexcept>

namespace {
constexpr int ExportBlockSide = LibrePaint::Image::Gpu::TileRenderer::TileSize;
}

KisPaintDeviceSP KisImportExportManager::prepareExportPixels(KisImageSP image, qsizetype memoryBudgetBytes)
{
    if (!image) throw std::invalid_argument("Export requires an image");
    const auto owner = image->gpuImage();
    if (!owner) return image->projection();
    if (owner->thread() != QThread::currentThread()) throw std::invalid_argument("Export pixels require the GPU thread");
    const auto pinned = owner->operations().snapshot();
    const auto bounds = image->bounds();
    const qint64 columns = (qint64(bounds.width()) + ExportBlockSide - 1) / ExportBlockSide;
    const qint64 rows = (qint64(bounds.height()) + ExportBlockSide - 1) / ExportBlockSide;
    constexpr qint64 blockBytes = ExportBlockSide * ExportBlockSide * 4;
    constexpr qint64 scratchBytes = 8 * 1024 * 1024;
    if (memoryBudgetBytes < scratchBytes || columns > (memoryBudgetBytes - scratchBytes) / blockBytes / 4 / rows)
        throw std::length_error("GPU export working memory budget exceeded");
    KisPaintDeviceSP output = new KisPaintDevice(image->colorSpace());
    output->setDefaultBounds(new KisDefaultBounds(image));
    QByteArray bytes;
    bytes.reserve(blockBytes);
    QEventLoop loop;
    QTimer step;
    step.setInterval(0);
    std::future<LibrePaint::Image::Gpu::PixelReadbackOutcome> pending;
    std::exception_ptr error;
    bool completed = false;
    int x = bounds.left();
    int y = bounds.top();
    connect(&step, &QTimer::timeout, &loop, [&] {
        try {
            const auto region = QRect(x, y, ExportBlockSide, ExportBlockSide).intersected(bounds);
            if (!pending.valid())
                pending = owner->execution().readPixelsAsync(owner->handle(), pinned, region);
            const auto polled = owner->execution().pollReadbacks();
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                if (polled.code != LibrePaint::Image::Gpu::ExecutionCode::Success &&
                    polled.code != LibrePaint::Image::Gpu::ExecutionCode::Busy)
                    throw std::runtime_error(polled.diagnostic.toStdString());
                step.setInterval(1);
                return;
            }
            const auto read = pending.get();
            if (read.code == LibrePaint::Image::Gpu::ExecutionCode::Busy) {
                step.setInterval(1);
                return;
            }
            if (read.code != LibrePaint::Image::Gpu::ExecutionCode::Success)
                throw std::runtime_error(read.diagnostic.toStdString());
            bytes.fill('\0', qsizetype(region.width()) * region.height() * 4);
            auto *destination = reinterpret_cast<quint8 *>(bytes.data());
            for (const auto &tile : read.tiles) {
                const QPoint origin = tile.coordinate * ExportBlockSide;
                const auto clipped = QRect(origin, QSize(ExportBlockSide, ExportBlockSide)).intersected(region);
                for (int row = clipped.top(); row <= clipped.bottom(); ++row) {
                    for (int column = clipped.left(); column <= clipped.right(); ++column) {
                        const auto from = ((row - origin.y()) * ExportBlockSide + column - origin.x()) * 4;
                        const auto to = ((row - region.y()) * region.width() + column - region.x()) * 4;
                        destination[to] = quint8(tile.rgba[from + 2]);
                        destination[to + 1] = quint8(tile.rgba[from + 1]);
                        destination[to + 2] = quint8(tile.rgba[from]);
                        destination[to + 3] = quint8(tile.rgba[from + 3]);
                    }
                }
            }
            output->writeBytes(reinterpret_cast<const quint8 *>(bytes.constData()), region);
            x += ExportBlockSide;
            if (x > bounds.right()) {
                x = bounds.left();
                y += ExportBlockSide;
            }
            if (y > bounds.bottom()) {
                completed = true;
                step.stop();
                loop.quit();
            } else step.setInterval(0);
        } catch (...) {
            error = std::current_exception();
            step.stop();
            loop.quit();
        }
    });
    step.start();
    loop.exec();
    if (error) std::rethrow_exception(error);
    if (!completed) throw std::runtime_error("GPU export interrupted before pixel preparation completed");
    return output;
}

