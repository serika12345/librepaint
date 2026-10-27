/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuExportRasterRows.h"
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <KoColorSpace.h>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <QtEndian>
#include <cstring>
#include <chrono>
#include <deque>
#include <future>
#include <mutex>
#include <stdexcept>

namespace {
namespace Gpu = LibrePaint::Image::Gpu;
constexpr int Side = Gpu::TileRenderer::TileSize;

struct Stripe {
    KisImportExportErrorCode status = ImportExportCodes::Cancelled;
    int firstRow = 0;
    int rowCount = 0;
    QByteArray bytes;
};
struct Request {
    explicit Request(int row) : row(row) {}
    int row;
    std::promise<Stripe> result;
    bool completed = false; // guarded by State::mutex
};
struct State {
    std::mutex mutex;
    QObject *gui = nullptr; // guarded by mutex; destroyed on the GUI thread
    bool closed = false;
    std::function<void(std::shared_ptr<Request>)> dispatch;
    std::vector<std::weak_ptr<Request>> pending;

    void complete(const std::shared_ptr<Request> &request, Stripe result)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (request->completed) return;
        request->completed = true;
        request->result.set_value(std::move(result));
    }
    void close()
    {
        std::lock_guard<std::mutex> lock(mutex);
        closed = true;
        gui = nullptr;
        dispatch = {};
        for (const auto &weak : pending) {
            if (const auto request = weak.lock(); request && !request->completed) {
                request->completed = true;
                request->result.set_value({});
            }
        }
        pending.clear();
    }
};

struct Reader {
    std::shared_ptr<State> state;
    QSize size;
    int pixelSize = 4;
    Stripe cached;

    KisImportExportErrorCode read(int row, QByteArray &output)
    {
        if (row < 0 || row >= size.height()) return ImportExportCodes::InternalError;
        bool guiThread = false;
        std::shared_ptr<Request> request;
        std::future<Stripe> future;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->closed) return ImportExportCodes::Cancelled;
            if (!cached.status.isOk() || row < cached.firstRow || row >= cached.firstRow + cached.rowCount) {
                request = std::make_shared<Request>(row);
                future = request->result.get_future();
                state->pending.erase(std::remove_if(state->pending.begin(), state->pending.end(),
                    [](const auto &weak) { return weak.expired(); }), state->pending.end());
                state->pending.push_back(request);
                guiThread = state->gui->thread() == QThread::currentThread();
                // The CPU state owns queued work. It resolves the GUI owner only
                // during delivery, so cancellation never captures a freed owner.
                const auto shared = state;
                QMetaObject::invokeMethod(state->gui, [shared, request] {
                    std::function<void(std::shared_ptr<Request>)> dispatch;
                    {
                        std::lock_guard<std::mutex> lock(shared->mutex);
                        if (!shared->closed) dispatch = shared->dispatch;
                    }
                    if (dispatch) dispatch(request);
                }, Qt::QueuedConnection);
            }
        }
        if (future.valid()) {
            if (guiThread) {
                QEventLoop loop;
                QTimer poll;
                poll.setInterval(1);
                QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
                    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) loop.quit();
                });
                poll.start();
                if (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) loop.exec();
            }
            cached = future.get();
            if (!cached.status.isOk()) return cached.status;
        }
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->closed) return ImportExportCodes::Cancelled;
        }
        output = cached.bytes.mid(qsizetype(row - cached.firstRow) * size.width() * pixelSize, qsizetype(size.width()) * pixelSize);
        return ImportExportCodes::OK;
    }
};
}

struct KisGpuExportRasterRows::Private {
    QSharedPointer<Gpu::PaintImage> owner;
    Gpu::Snapshot pinned;
    int pixelSize = 4;
    QSize size;
    std::shared_ptr<State> state = std::make_shared<State>();
    KisExportRasterRowsSP rows;
    QTimer step;
    std::future<Gpu::PixelReadbackOutcome> pending;
    std::deque<std::shared_ptr<Request>> requests;
    std::shared_ptr<Request> current;
    Stripe stripe;
    int x = 0;
    int y = 0;
    bool metadata = true;
    bool hasTransparency = false;
    bool metadataComplete = false;
    QString error;

    void advance()
    {
        x += Side;
        if (x >= size.width()) { x = 0; y += Side; }
    }
    void finishStripe()
    {
        stripe.status = ImportExportCodes::OK;
        state->complete(current, std::move(stripe));
        current.reset();
    }
    void tick()
    {
        try {
            if (!metadata && !current) {
                if (requests.empty()) { step.stop(); return; }
                current = requests.front();
                requests.pop_front();
                stripe.firstRow = current->row / Side * Side;
                stripe.rowCount = std::min(Side, size.height() - stripe.firstRow);
                stripe.bytes.fill('\0', qsizetype(size.width()) * stripe.rowCount * pixelSize);
                x = 0;
                y = stripe.firstRow;
            }
            const QRect region(x, y, std::min(Side, size.width() - x), std::min(Side, size.height() - y));
            if (!pending.valid()) {
                pending = metadata ? owner->execution().readTransparencyAsync(owner->handle(), pinned, region) :
                                     owner->execution().readPixelsAsync(owner->handle(), pinned, region);
            }
            const auto polled = owner->execution().pollReadbacks();
            if (pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                if (polled.code != Gpu::ExecutionCode::Success && polled.code != Gpu::ExecutionCode::Busy)
                    throw std::runtime_error(polled.diagnostic.toStdString());
                step.setInterval(1);
                return;
            }
            const auto read = pending.get();
            if (read.code == Gpu::ExecutionCode::Busy) { step.setInterval(1); return; }
            if (read.code != Gpu::ExecutionCode::Success) throw std::runtime_error(read.diagnostic.toStdString());
            if (metadata) {
                if (!read.containsTransparency) throw std::runtime_error("GPU transparency result is missing");
                hasTransparency |= *read.containsTransparency;
                advance();
                if (hasTransparency || y >= size.height()) {
                    metadataComplete = true;
                    metadata = false;
                    step.stop();
                }
            } else {
                auto *destination = reinterpret_cast<quint8 *>(stripe.bytes.data());
                for (const auto &tile : read.tiles) {
                    const QPoint origin = tile.coordinate * Side;
                    const auto clipped = QRect(origin, QSize(Side, Side)).intersected(region);
                    for (int row = clipped.top(); row <= clipped.bottom(); ++row) {
                        for (int column = clipped.left(); column <= clipped.right(); ++column) {
                            const auto from = ((row - origin.y()) * Side + column - origin.x()) * pixelSize;
                            const qsizetype to = (qsizetype(row - stripe.firstRow) * size.width() + column) * pixelSize;
                            if (pixelSize == 8) {
                                quint16 bgra[4];
                                for (int c = 0; c < 4; ++c)
                                    bgra[c] = qFromLittleEndian<quint16>(tile.rgba.constData() + from + (c == 0 ? 2 : c == 2 ? 0 : c) * 2);
                                std::memcpy(destination + to, bgra, sizeof(bgra));
                            } else {
                                destination[to] = quint8(tile.rgba[from + 2]);
                                destination[to + 1] = quint8(tile.rgba[from + 1]);
                                destination[to + 2] = quint8(tile.rgba[from]);
                                destination[to + 3] = quint8(tile.rgba[from + 3]);
                            }
                        }
                    }
                }
                advance();
                if (y != stripe.firstRow) finishStripe();
            }
            step.setInterval(0);
        } catch (const std::exception &failure) {
            error = QString::fromUtf8(failure.what());
            if (current) {
                Stripe failed;
                failed.status = KisImportExportErrorCode(error);
                state->complete(current, std::move(failed));
                current.reset();
            }
            state->close();
            step.stop();
        }
    }
};

KisGpuExportRasterRows::KisGpuExportRasterRows(KisImageSP image, qsizetype memoryBudgetBytes)
    : m_d(std::make_unique<Private>())
{
    auto &d = *m_d;
    if (!image || !image->gpuImage()) throw std::invalid_argument("Export rows require a GPU image");
    d.owner = image->gpuImage();
    if (d.owner->thread() != QThread::currentThread()) throw std::invalid_argument("Export rows require the GPU thread");
    d.size = image->size();
    d.pinned = d.owner->operations().snapshot();
    d.pixelSize = Gpu::pixelByteCount(d.pinned->format);
    if (image->colorSpace()->pixelSize() != d.pixelSize)
        throw std::invalid_argument("Export precision and document color encoding must match");
    constexpr qint64 scratchBytes = 8 * 1024 * 1024;
    const qint64 rowBytes = qint64(d.size.width()) * d.pixelSize;
    const qint64 workingRows = 2 * std::min(Side, d.size.height()) + 16;
    if (memoryBudgetBytes < scratchBytes || rowBytes > (memoryBudgetBytes - scratchBytes) / workingRows)
        throw std::length_error("GPU export working memory budget exceeded");
    d.pinned = d.owner->operations().snapshot();
    d.state->gui = this;
    d.state->dispatch = [&d](std::shared_ptr<Request> request) {
        d.requests.push_back(std::move(request));
        d.step.start(0);
    };
    connect(&d.step, &QTimer::timeout, this, [&d] { d.tick(); });
    QEventLoop loop;
    QTimer poll;
    poll.setInterval(1);
    connect(&poll, &QTimer::timeout, &loop, [&] {
        if (d.metadataComplete || !d.error.isEmpty()) loop.quit();
    });
    poll.start();
    d.step.start(0);
    loop.exec();
    if (!d.error.isEmpty()) throw std::runtime_error(d.error.toStdString());
    if (!d.metadataComplete) throw std::runtime_error("GPU export metadata preparation interrupted");
    auto reader = std::make_shared<Reader>();
    reader->state = d.state;
    reader->size = d.size;
    reader->pixelSize = d.pixelSize;
    auto rows = QSharedPointer<KisExportRasterRows>::create();
    rows->colorSpace = image->colorSpace();
    rows->size = d.size;
    rows->readRow = [reader](int row, QByteArray &bytes) { return reader->read(row, bytes); };
    d.rows = rows;
}

KisGpuExportRasterRows::~KisGpuExportRasterRows()
{
    m_d->step.stop();
    m_d->state->close();
}
KisExportRasterRowsSP KisGpuExportRasterRows::rows() const { return m_d->rows; }
bool KisGpuExportRasterRows::containsTransparency() const { return m_d->hasTransparency; }
