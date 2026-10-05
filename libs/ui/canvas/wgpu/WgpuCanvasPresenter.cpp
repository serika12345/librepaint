/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuCanvasPresenter.h"
#include "WgpuWindowSurface.h"
#include "../kis_canvas_performance_measurement_p.h"
#include <WgpuImageRenderer.h>

#include <QPointer>
#include <QElapsedTimer>
#include <QPainter>
#include <QPlatformSurfaceEvent>
#include <QTimer>
#include <QWindow>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

namespace Krita::Canvas
{
struct WgpuCanvasPresenter::Private
{
    explicit Private(QWindow &window) : window(&window) {}
    QPointer<QWindow> window;
    QTimer frameTimer;
    QImage image;
    QString error;
    quint64 frames = 0;
    QSize surfaceSize;
    WGPUPresentMode presentMode = WGPUPresentMode_Fifo;
    bool settingsValid = true;
    int frameInterval = 0;
    QElapsedTimer lastFrame;
    bool threaded = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_THREADED");
    QThread gpuThread;
    QObject *worker = nullptr;
    bool busy = false;
    QRect pendingDirty;
    quint64 completedUploadBytes = 0;
    bool lateUpload = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_LATE_UPLOAD");
    bool borrowSnapshot = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_BORROW_SNAPSHOT");
    const QImage *borrowedImage = nullptr;
    QMutex latestMutex;
    QImage latestImage;
    QRect latestDirty;
    qint64 latestInput = 0;
    std::array<float,16> geometry{};
    bool rawProjection = qEnvironmentVariableIntValue("LIBREPAINT_WGPU_RAW_PROJECTION");
    QVector<QPair<QImage,QRect>> patches;
    quint64 generation = 0, latestGeneration = 0;
    // The renderer must release its GPU surface before the native layer.
    std::unique_ptr<WgpuWindowSurface> native;
    std::unique_ptr<WgpuImageRenderer> renderer;
};

WgpuCanvasPresenter::WgpuCanvasPresenter(QWindow &window) : m_d(new Private(window))
{
    const QString mode = qEnvironmentVariable("LIBREPAINT_WGPU_PRESENT_MODE", QStringLiteral("fifo"));
    if (mode == QStringLiteral("immediate")) m_d->presentMode = WGPUPresentMode_Immediate;
    else if (mode != QStringLiteral("fifo")) {
        m_d->settingsValid = false;
        m_d->error = QStringLiteral("Unknown experimental GPU presentation mode: %1").arg(mode);
    }
    const QString intervalText = qEnvironmentVariable("LIBREPAINT_WGPU_FRAME_INTERVAL_MS", QStringLiteral("0"));
    bool intervalValid = false;
    const int interval = intervalText.toInt(&intervalValid);
    if (!intervalValid || interval < 0 || interval > 1000) {
        m_d->settingsValid = false;
        m_d->error = QStringLiteral("Experimental GPU frame interval requires 0..1000 ms");
    }
    window.installEventFilter(this);
    m_d->frameTimer.setSingleShot(true);
    m_d->frameInterval = m_d->settingsValid ? interval : 0;
    m_d->frameTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_d->frameTimer, &QTimer::timeout, this, [this] { render(); });
    if (m_d->threaded) {
        m_d->worker = new QObject;
        m_d->worker->moveToThread(&m_d->gpuThread);
        connect(&m_d->gpuThread, &QThread::finished, m_d->worker, &QObject::deleteLater);
        m_d->gpuThread.start();
    }
}
WgpuCanvasPresenter::~WgpuCanvasPresenter()
{
    if (m_d->window) m_d->window->removeEventFilter(this);
    if (m_d->threaded) {
        QMetaObject::invokeMethod(m_d->worker, [this] { m_d->renderer.reset(); }, Qt::BlockingQueuedConnection);
        m_d->gpuThread.quit();
        m_d->gpuThread.wait();
    }
}
void WgpuCanvasPresenter::setProjectionGeometry(const std::array<float,16> &geometry)
{
    QMutexLocker lock(&m_d->latestMutex);
    m_d->geometry = geometry;
}
void WgpuCanvasPresenter::queueProjectionPatch(const QImage &patch, const QRect &destination)
{
    QMutexLocker lock(&m_d->latestMutex);
    m_d->patches.append(qMakePair(patch,destination));
}
void WgpuCanvasPresenter::setProjectionGeneration(quint64 generation) { m_d->generation=generation; }

void WgpuCanvasPresenter::requestFrame()
{
    if (!m_d->settingsValid) return;
    if (!m_d->frameTimer.isActive()) {
        const int delay = m_d->lastFrame.isValid()
            ? qMax(0, m_d->frameInterval - int(qMin(m_d->lastFrame.elapsed(), qint64(m_d->frameInterval)))) : 0;
        m_d->frameTimer.start(delay);
    }
}

bool WgpuCanvasPresenter::setImage(const QImage &image, const QRect &dirty)
{
    if (!m_d->window || !m_d->settingsValid) return false;
    if (m_d->threaded) {
        Performance::Measurement measurement(*m_d->window, "snapshot");
        if (m_d->borrowSnapshot) {
            if (m_d->image.isNull()) m_d->image = image;
            m_d->borrowedImage = &image;
        } else {
            m_d->image = image;
            if (!m_d->lateUpload) m_d->image.setDevicePixelRatio(1.0);
        }
        if (m_d->lateUpload) {
            QMutexLocker lock(&m_d->latestMutex);
            m_d->latestImage = image;
            m_d->latestDirty |= dirty;
            m_d->latestInput = m_d->window->property(Performance::inputProperty).toLongLong();
            m_d->latestGeneration = m_d->generation;
        }
        m_d->pendingDirty |= dirty;
        requestFrame();
        return true;
    }
    Performance::Measurement measurement(*m_d->window, "upload");
    if (image.isNull() || (!dirty.isEmpty() && !image.rect().contains(dirty))
        || (image.size() != m_d->image.size() && dirty != image.rect())) {
        m_d->error = QStringLiteral("Canvas image requires an in-bounds update and complete initialization");
        return false;
    }
    if (dirty.isEmpty()) return true;
    if (m_d->renderer && !m_d->renderer->upload(image, dirty)) {
        m_d->error = m_d->renderer->error();
        return false;
    }
    if (dirty == image.rect()) {
        m_d->image = image;
        // The GPU and cached patches address physical pixels.
        m_d->image.setDevicePixelRatio(1.0);
    } else {
        QPainter painter(&m_d->image);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.drawImage(dirty, image, dirty);
    }
    m_d->error.clear();
    requestFrame();
    return true;
}

void WgpuCanvasPresenter::render()
{
    if (!m_d->window || !m_d->window->isExposed() || m_d->window->size().isEmpty() || m_d->image.isNull()) return;
    if (m_d->threaded) {
        if (m_d->busy) return;
        if (!m_d->native) m_d->native = std::make_unique<WgpuWindowSurface>(*m_d->window);
        if (!m_d->native->source()) { m_d->error = m_d->native->error(); return; }
        m_d->native->resize();
        const auto *source = m_d->native->source();
        const QSize pixels = m_d->window->size() * m_d->window->devicePixelRatio();
        const QImage image = m_d->borrowedImage ? *m_d->borrowedImage : m_d->image;
        const QRect dirty = m_d->pendingDirty;
        m_d->pendingDirty = {};
        const qint64 input = m_d->window->property(Performance::inputProperty).toLongLong();
        const bool enabled = m_d->window->property(Performance::enabledProperty).toBool();
        const auto geometry = m_d->geometry;
        const quint64 generation = m_d->generation;
        m_d->busy = true;
        QMetaObject::invokeMethod(m_d->worker, [this, source, pixels, image, dirty, input, enabled, geometry, generation] {
            const qint64 start = Performance::nowNs();
            bool ok = true;
            if (!m_d->renderer) {
                m_d->renderer = std::make_unique<WgpuImageRenderer>();
                ok = m_d->renderer->initialize(source) && m_d->renderer->upload(image, image.rect());
            } else if (!dirty.isEmpty() && !m_d->rawProjection) ok = m_d->renderer->upload(image, dirty);
            qint64 uploaded = Performance::nowNs();
            qint64 usedInput = input;
            quint64 usedGeneration = generation;
            qint64 uploadCpu = uploaded-start;
            if (ok && m_d->surfaceSize != pixels) {
                ok = m_d->renderer->configureSurface(pixels, m_d->presentMode);
                m_d->surfaceSize = pixels;
            }
            WgpuImageRenderer::PresentationTiming timing;
            m_d->renderer->setProjectionGeometry(geometry);
            const auto prepareLatest = [&] {
                QImage latest;
                QRect latestDirty;
                std::array<float,16> latestGeometry;
                QVector<QPair<QImage,QRect>> patches;
                {
                    QMutexLocker lock(&m_d->latestMutex);
                    latest = m_d->latestImage;
                    latestDirty = m_d->latestDirty;
                    m_d->latestDirty = {};
                    usedInput = m_d->latestInput;
                    usedGeneration = m_d->latestGeneration;
                    latestGeometry = m_d->geometry;
                    patches.swap(m_d->patches);
                }
                const qint64 uploadStart = Performance::nowNs();
                bool result = m_d->rawProjection || latest.isNull() || latestDirty.isEmpty() || m_d->renderer->upload(latest, latestDirty);
                for (const auto &patch : patches) result = m_d->renderer->uploadPatch(patch.first,patch.second) && result;
                m_d->renderer->setProjectionGeometry(latestGeometry);
                uploaded = Performance::nowNs();
                uploadCpu += uploaded-uploadStart;
                return result;
            };
            if (ok) ok = m_d->renderer->present(enabled ? &timing : nullptr,
                                               m_d->lateUpload ? std::function<bool()>(prepareLatest) : std::function<bool()>());
            const qint64 finish = Performance::nowNs();
            const QString error = ok ? QString() : m_d->renderer->error();
            const quint64 bytes = m_d->renderer->uploadedBytes();
            QMetaObject::invokeMethod(this, [this, start, uploaded, uploadCpu, finish, timing, usedInput, usedGeneration, enabled, error, bytes] {
                m_d->busy = false;
                m_d->error = error;
                m_d->completedUploadBytes = bytes;
                if (error.isEmpty()) { ++m_d->frames; m_d->lastFrame.start(); }
                if (enabled && m_d->window) {
                    m_d->window->setProperty("librepaintCanvasPresentedGeneration",qulonglong(usedGeneration));
                    Performance::record(*m_d->window, QStringLiteral("upload"), uploadCpu, usedInput, uploaded);
                    if (timing.presentEndNs) {
                        Performance::record(*m_d->window, QStringLiteral("surface_acquire"), timing.acquireEndNs-timing.acquireStartNs, usedInput, timing.acquireEndNs);
                        Performance::record(*m_d->window, QStringLiteral("gpu_encode_submit"), timing.submitEndNs-timing.beforeDrawEndNs, usedInput, timing.submitEndNs);
                        Performance::record(*m_d->window, QStringLiteral("surface_present"), timing.presentEndNs-timing.submitEndNs, usedInput, timing.presentEndNs);
                        if(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_GPU_COMPLETION"))
                            Performance::record(*m_d->window,QStringLiteral("gpu_completion_wait"),timing.gpuCompletionEndNs-timing.presentEndNs,usedInput,timing.gpuCompletionEndNs);
                    }
                    Performance::record(*m_d->window, QStringLiteral("submit"), finish-start, usedInput, finish);
                }
                if (!m_d->pendingDirty.isEmpty()) requestFrame();
            });
        });
        return;
    }
    Performance::Measurement measurement(*m_d->window, "submit");
    m_d->window->setProperty("librepaintCanvasPresentedGeneration",qulonglong(m_d->generation));
    if (!m_d->renderer) {
        m_d->native = std::make_unique<WgpuWindowSurface>(*m_d->window);
        if (!m_d->native->source()) {
            m_d->error = m_d->native->error();
            return;
        }
        m_d->renderer = std::make_unique<WgpuImageRenderer>();
        if (!m_d->renderer->initialize(m_d->native->source())
            || !m_d->renderer->upload(m_d->image, m_d->image.rect())) {
            m_d->error = m_d->renderer->error();
            return;
        }
    }
    m_d->native->resize();
    const QSize pixels = m_d->window->size() * m_d->window->devicePixelRatio();
    if (pixels != m_d->surfaceSize) {
        if (!m_d->renderer->configureSurface(pixels, m_d->presentMode)) {
            m_d->error = m_d->renderer->error();
            return;
        }
        m_d->surfaceSize = pixels;
    }
    const auto present = [&] {
        m_d->renderer->setProjectionGeometry(m_d->geometry);
        const bool enabled = m_d->window->property(Performance::enabledProperty).toBool();
        const qint64 input = m_d->window->property(Performance::inputProperty).toLongLong();
        WgpuImageRenderer::PresentationTiming timing;
        const bool result = m_d->renderer->present(enabled ? &timing : nullptr);
        if (enabled && timing.presentEndNs) {
            Performance::record(*m_d->window, QStringLiteral("surface_acquire"),
                                timing.acquireEndNs - timing.acquireStartNs, input, timing.acquireEndNs);
            Performance::record(*m_d->window, QStringLiteral("gpu_encode_submit"),
                                timing.submitEndNs - timing.beforeDrawEndNs, input, timing.submitEndNs);
            Performance::record(*m_d->window, QStringLiteral("surface_present"),
                                timing.presentEndNs - timing.submitEndNs, input, timing.presentEndNs);
            if(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_GPU_COMPLETION"))
                Performance::record(*m_d->window,QStringLiteral("gpu_completion_wait"),timing.gpuCompletionEndNs-timing.presentEndNs,input,timing.gpuCompletionEndNs);
        }
        return result;
    };
    if (!present()) {
        // Reacquire once after a compositor invalidates the presentation images.
        if (!m_d->renderer->configureSurface(pixels, m_d->presentMode) || !present()) {
            m_d->error = m_d->renderer->error();
            return;
        }
    }
    m_d->error.clear();
    m_d->lastFrame.start();
    ++m_d->frames;
}

bool WgpuCanvasPresenter::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_d->window.data()) return false;
    if (event->type() == QEvent::PlatformSurface) {
        auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            if (m_d->threaded) QMetaObject::invokeMethod(m_d->worker, [this] { m_d->renderer.reset(); }, Qt::BlockingQueuedConnection);
            else m_d->renderer.reset();
            m_d->native.reset();
            m_d->surfaceSize = {};
            m_d->lastFrame.invalidate();
        } else {
            requestFrame();
        }
    } else if (event->type() == QEvent::Expose || event->type() == QEvent::Resize) {
        requestFrame();
    }
    return false;
}
QImage WgpuCanvasPresenter::readback()
{
    if (m_d->threaded) {
        QImage image;
        QMetaObject::invokeMethod(m_d->worker, [this, &image] {
            if (m_d->renderer) image = m_d->renderer->readbackSurface();
        }, Qt::BlockingQueuedConnection);
        return image;
    }
    if (!m_d->renderer || !m_d->window || !m_d->window->isExposed()) return {};
    QImage image = m_d->renderer->readbackSurface();
    if (image.isNull()) m_d->error = m_d->renderer->error();
    else ++m_d->frames;
    return image;
}
quint64 WgpuCanvasPresenter::submittedFrames() const { return m_d->frames; }
quint64 WgpuCanvasPresenter::uploadedBytes() const { return m_d->threaded ? m_d->completedUploadBytes : m_d->renderer ? m_d->renderer->uploadedBytes() : 0; }
QString WgpuCanvasPresenter::error() const { return m_d->error; }
}
