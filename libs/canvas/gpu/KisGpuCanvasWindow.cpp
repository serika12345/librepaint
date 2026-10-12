/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuCanvasWindow.h"
#include "KisGpuTilePresentation.h"
#include "KisGpuPresentationResources_p.h"
#include "KisGpuViewTransform_p.h"
#include <QPlatformSurfaceEvent>
#include <QThread>
#include <stdexcept>

namespace Krita::Canvas::Gpu {
using namespace Image::Gpu;

struct CanvasWindow::Private
{
    DevicePtr device;
    Execution &execution;
    quint64 generation;
    DocumentHandle handle = 0;
    QTransform transform;
    bool recording = false;
    Detail::Resource<QRhiRenderPassDescriptor> pass;
    Detail::Resource<QRhiSwapChain> surface;
    std::unique_ptr<TilePresentation> presentation;

    explicit Private(Execution &execution)
        : device(execution.device()), execution(execution), generation(execution.generation()),
          pass(Detail::own<QRhiRenderPassDescriptor>(device->rhi(), nullptr)),
          surface(Detail::own<QRhiSwapChain>(device->rhi(), nullptr)) {}

    void releaseSurface()
    {
        presentation.reset();
        surface.reset();
        pass.reset();
    }
    QTransform physicalTransform(qreal ratio) const
    {
        return {transform.m11() * ratio, transform.m12() * ratio,
                transform.m21() * ratio, transform.m22() * ratio,
                transform.dx() * ratio, transform.dy() * ratio};
    }
};

CanvasWindow::CanvasWindow(Execution &execution, QWindow *parent) : QWindow(parent)
{
    if (!execution.device() || execution.device()->rhi().thread() != QThread::currentThread()) {
        throw std::invalid_argument("GPU canvas requires the shared execution thread");
    }
    m_d = std::make_unique<Private>(execution);
    connectSurface();
}

CanvasWindow::~CanvasWindow()
{
    m_d->releaseSurface();
    destroy();
#if QT_CONFIG(vulkan)
    setVulkanInstance(nullptr);
#endif
}

void CanvasWindow::connectSurface()
{
    setSurfaceType(m_d->device->vulkanInstance() ? QSurface::VulkanSurface : QSurface::MetalSurface);
#if QT_CONFIG(vulkan)
    setVulkanInstance(m_d->device->vulkanInstance());
#endif
}

bool CanvasWindow::setDocument(DocumentHandle handle)
{
    if (handle && !m_d->execution.requested(handle)) return false;
    m_d->handle = handle;
    refresh();
    return true;
}

void CanvasWindow::refresh() { requestUpdate(); }
bool CanvasWindow::setDocumentToWindow(const QTransform &transform)
{
    if (!Detail::validDocumentTransform(transform)) return false;
    const auto ratio = devicePixelRatio();
    const QTransform physical(transform.m11() * ratio, transform.m12() * ratio,
                              transform.m21() * ratio, transform.m22() * ratio,
                              transform.dx() * ratio, transform.dy() * ratio);
    if (!Detail::validDocumentTransform(physical)) return false;
    m_d->transform = transform;
    refresh();
    return true;
}
QTransform CanvasWindow::documentToWindow() const { return m_d->transform; }
QRect CanvasWindow::documentViewport() const
{
    const auto document = m_d->execution.requested(m_d->handle);
    if (!document || size().isEmpty()) return {};
    return m_d->transform.inverted().mapRect(QRectF(QPointF(0, 0), size()))
        .intersected(QRectF(QPointF(0, 0), document->size)).toAlignedRect();
}

bool CanvasWindow::resetSurface()
{
    auto &d = *m_d;
    const auto next = d.execution.device();
    if (d.recording || !next || d.device->rhi().isRecordingFrame() || next->rhi().isRecordingFrame()) return false;
    const bool visible = isVisible();
    d.releaseSurface();
    destroy();
    // The native window is destroyed while its original Vulkan instance is alive.
    d.device = next;
    d.generation = d.execution.generation();
    connectSurface();
    if (visible) show();
    return true;
}

bool CanvasWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderFrame();
        return true;
    }
    if (event->type() == QEvent::PlatformSurface &&
        static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
        m_d->releaseSurface();
    }
    return QWindow::event(event);
}
void CanvasWindow::exposeEvent(QExposeEvent *event)
{
    QWindow::exposeEvent(event);
    if (isExposed()) refresh();
}
void CanvasWindow::resizeEvent(QResizeEvent *event)
{
    QWindow::resizeEvent(event);
    refresh();
}

void CanvasWindow::renderFrame()
{
    auto &d = *m_d;
    if (d.recording || !isExposed() || !d.execution.requested(d.handle)) return;
    if (d.device != d.execution.device() || d.generation != d.execution.generation()) {
        resetSurface();
        return;
    }
    d.recording = true;
    bool begun = false;
    const auto outcome = [&]() -> ExecutionOutcome {
        try {
            auto &rhi = d.device->rhi();
            if (rhi.isRecordingFrame()) return {ExecutionCode::Busy, {}};
            if (!Detail::validDocumentTransform(d.physicalTransform(devicePixelRatio()))) {
                return {ExecutionCode::InvalidRequest, QStringLiteral("GPU canvas transform is outside the supported finite range")};
            }
            if (!d.surface) {
                d.surface = Detail::own(rhi, rhi.newSwapChain());
                d.surface->setWindow(this);
                d.surface->setFlags(QRhiSwapChain::SurfaceHasPreMulAlpha);
                d.pass = Detail::own(rhi, d.surface->newCompatibleRenderPassDescriptor());
                d.surface->setRenderPassDescriptor(d.pass.get());
                d.presentation = std::make_unique<TilePresentation>(d.device);
            }
            if (d.surface->currentPixelSize() != d.surface->surfacePixelSize() && !d.surface->createOrResize()) {
                return {ExecutionCode::DeviceFailure, QStringLiteral("GPU canvas surface could not be created or resized")};
            }
            auto frame = d.execution.beginPresentationFrame(*d.surface);
            if (frame.code == ExecutionCode::SurfaceOutOfDate && d.surface->createOrResize()) {
                frame = d.execution.beginPresentationFrame(*d.surface);
            }
            if (frame.code != ExecutionCode::Success) return {frame.code, frame.diagnostic};
            begun = true;
            const auto projected = d.execution.project(d.handle, documentViewport());
            if (projected.code != RenderOutcome::Recorded) {
                d.execution.cancelFrame();
                begun = false;
                const auto code = projected.code == RenderOutcome::CapacityExceeded ? ExecutionCode::CapacityExceeded :
                    projected.code == RenderOutcome::InvalidVersion ? ExecutionCode::InvalidRequest : ExecutionCode::DeviceFailure;
                return {code, projected.diagnostic};
            }
            const auto drawn = d.presentation->record(*frame.commands, projected.version, *frame.target,
                                                      d.physicalTransform(devicePixelRatio()), Qt::white);
            if (!drawn.recorded) {
                d.execution.cancelFrame();
                begun = false;
                return {ExecutionCode::DeviceFailure, drawn.diagnostic};
            }
            const auto submitted = d.execution.submitFrame();
            begun = false;
            return submitted;
        } catch (const std::exception &exception) {
            if (begun) d.execution.cancelFrame();
            return {ExecutionCode::DeviceFailure, QString::fromUtf8(exception.what())};
        }
    }();
    d.recording = false;
    const auto handle = d.handle;
    const auto generation = d.generation;
    if (outcome.code == ExecutionCode::Success) {
        const auto document = d.execution.published(handle);
        if (document) Q_EMIT frameSubmitted(handle, document->revision(), generation);
    } else if (outcome.code != ExecutionCode::Busy) {
        Q_EMIT renderingFailed(handle, generation, outcome.code, outcome.diagnostic);
    }
}
}
