/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPresentationSurface.h"
#include "KisGpuNativeResources_p.h"
#include <QThread>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {
using namespace Detail;
struct PresentationSurface::Private
{
    DevicePtr device;
    NativeSurface surface;
    NativeTexture texture;
    NativeTextureView view;
    QSize size;
    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
};
PresentationSurface::PresentationSurface(DevicePtr device, const WGPUSurfaceDescriptor &descriptor)
{
    if (!device || device->thread() != QThread::currentThread())
        throw std::invalid_argument("A presentation surface requires its owning GPU thread");
    m_d = std::make_unique<Private>();
    m_d->device = std::move(device);
    m_d->surface.reset(wgpuInstanceCreateSurface(m_d->device->instance(), &descriptor));
    if (!m_d->surface) throw std::runtime_error("The native window surface could not be created");
}
PresentationSurface::~PresentationSurface()
{
    Q_ASSERT(m_d->device->thread() == QThread::currentThread());
    discard();
    if (!m_d->size.isEmpty()) wgpuSurfaceUnconfigure(m_d->surface.get());
}
DevicePtr PresentationSurface::device() const { return m_d->device; }
QSize PresentationSurface::size() const { return m_d->size; }
quint32 PresentationSurface::format() const { return m_d->format; }
WGPUTextureView PresentationSurface::target() const { return m_d->view.get(); }
bool PresentationSurface::configure(QSize size)
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || size.isEmpty() || d.texture || d.device->isRecordingFrame()) return false;
    WGPULimits limits{};
    if (wgpuDeviceGetLimits(d.device->nativeDevice(), &limits) != WGPUStatus_Success ||
        quint32(size.width()) > limits.maxTextureDimension2D ||
        quint32(size.height()) > limits.maxTextureDimension2D) return false;
    WGPUSurfaceCapabilities capabilities{};
    if (wgpuSurfaceGetCapabilities(d.surface.get(), d.device->adapter(), &capabilities) != WGPUStatus_Success) return false;
    if (!capabilities.formatCount || !capabilities.alphaModeCount) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        return false;
    }
    d.format = capabilities.formats[0];
    for (size_t i = 0; i < capabilities.formatCount; ++i) {
        if (capabilities.formats[i] == WGPUTextureFormat_BGRA8Unorm ||
            capabilities.formats[i] == WGPUTextureFormat_RGBA8Unorm) {
            d.format = capabilities.formats[i];
            break;
        }
    }
    WGPUSurfaceConfiguration configuration{};
    configuration.device = d.device->nativeDevice();
    configuration.format = d.format;
    configuration.usage = WGPUTextureUsage_RenderAttachment;
    configuration.width = size.width();
    configuration.height = size.height();
    configuration.presentMode = WGPUPresentMode_Fifo;
    configuration.alphaMode = capabilities.alphaModes[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    wgpuSurfaceConfigure(d.surface.get(), &configuration);
    if (!d.device->diagnostic().isEmpty()) return false;
    d.size = size;
    return true;
}
ExecutionOutcome PresentationSurface::acquire()
{
    auto &d = *m_d;
    if (d.device->thread() != QThread::currentThread() || d.texture)
        return {ExecutionCode::InvalidRequest, QStringLiteral("The native surface is already acquired or belongs to another thread")};
    if (d.size.isEmpty()) return {ExecutionCode::SurfaceOutOfDate, QStringLiteral("The native surface requires configuration")};
    WGPUSurfaceTexture current{};
    wgpuSurfaceGetCurrentTexture(d.surface.get(), &current);
    d.texture.reset(current.texture);
    if (current.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        current.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        discard();
        const bool outdated = current.status == WGPUSurfaceGetCurrentTextureStatus_Outdated ||
            current.status == WGPUSurfaceGetCurrentTextureStatus_Lost;
        return {outdated ? ExecutionCode::SurfaceOutOfDate : ExecutionCode::DeviceFailure,
                QStringLiteral("The native surface texture could not be acquired (%1)").arg(current.status)};
    }
    d.view.reset(wgpuTextureCreateView(d.texture.get(), nullptr));
    if (!d.view) { discard(); return {ExecutionCode::DeviceFailure, QStringLiteral("The native surface texture view could not be created")}; }
    return {ExecutionCode::Success, {}};
}
bool PresentationSurface::present()
{
    if (!m_d->texture || m_d->device->thread() != QThread::currentThread()) return false;
    const bool success = wgpuSurfacePresent(m_d->surface.get()) == WGPUStatus_Success;
    discard();
    return success;
}
void PresentationSurface::discard()
{
    m_d->view.reset();
    // Releasing an unpresented surface texture returns it to the native surface.
    m_d->texture.reset();
}
}
