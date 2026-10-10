/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "SurfaceRenderer.h"
#include "GpuWindowSurface_p.h"
#include <QWindow>
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace Krita::Canvas {
SurfaceRenderer::Presentation::Status SurfaceRenderer::Presentation::status() const {
    return d ? d->status.load(std::memory_order_acquire) : Status::Unavailable;
}
std::optional<quint64> SurfaceRenderer::Presentation::hostTimeNanoseconds() const {
    if (status() != Status::Presented) return {};
    return d->hostTime.load(std::memory_order_relaxed);
}
void SurfaceRenderer::Presentation::abandon() {
    if (d) d->abandon();
}
std::optional<quint64> SurfaceRenderer::presentationClockNanoseconds() {
    return GpuWindowSurface::presentationClockNanoseconds();
}
struct SurfaceRenderer::Private {
    KisGpuDevice &owner;
    QWindow &window;
    WId windowId;
    quint64 budget;
    quint32 maximumPending;
    GpuWindowSurface surface;
    std::unique_ptr<GpuRenderer> renderer;
    WGPUTextureFormat format;
    quint32 maximumDimension = 0;
    QSize size{0, 0};
    bool needsConfigure = true;
    quint64 requests = 0;
    QVector<Presentation> presentations;
    quint64 displayed = 0, skipped = 0, abandoned = 0;
    ~Private() { for (auto &presentation : presentations) presentation.abandon(); }
    void collectPresentations() {
        for (auto it = presentations.begin(); it != presentations.end();) {
            if (!owner.available()) it->abandon();
            const auto status = it->status();
            if (status == Presentation::Status::Pending) { ++it; continue; }
            if (status == Presentation::Status::Presented) ++displayed;
            else if (status == Presentation::Status::Skipped) ++skipped;
            else if (status == Presentation::Status::Abandoned) ++abandoned;
            it = presentations.erase(it);
        }
    }
    Private(KisGpuDevice &device, QWindow &target, quint64 limit, quint32 maximum)
        : owner(device), window(target), windowId(target.winId()), budget(limit), maximumPending(maximum), surface(device, target) {
        WGPUSurfaceCapabilities capabilities{};
        const auto result = wgpuSurfaceGetCapabilities(surface.surface(), owner.adapter(), &capabilities);
        const auto supports = [](const auto *begin, size_t count, auto value) {
            return count && std::find(begin, begin + count, value) != begin + count;
        };
        format = supports(capabilities.formats, capabilities.formatCount, WGPUTextureFormat_BGRA8Unorm)
            ? WGPUTextureFormat_BGRA8Unorm : WGPUTextureFormat_RGBA8Unorm;
        const bool compatible = result == WGPUStatus_Success
            && supports(capabilities.formats, capabilities.formatCount, format)
            && supports(capabilities.presentModes, capabilities.presentModeCount, WGPUPresentMode_Fifo)
            && supports(capabilities.alphaModes, capabilities.alphaModeCount, WGPUCompositeAlphaMode_Opaque)
            && (capabilities.usages & WGPUTextureUsage_RenderAttachment);
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        if (!compatible) throw std::runtime_error("GPU window has no supported presentation format");
        WGPULimits limits{};
        if (wgpuDeviceGetLimits(owner.device(), &limits) != WGPUStatus_Success) throw std::runtime_error("Cannot read GPU window limits");
        maximumDimension = limits.maxTextureDimension2D;
        renderer = std::make_unique<GpuRenderer>(owner, format, maximum);
    }
    Error configure(QSize wanted) {
        if (quint32(wanted.width()) > maximumDimension || quint32(wanted.height()) > maximumDimension
            || quint64(wanted.width()) * wanted.height() > budget / 12) return Error::BudgetExceeded;
        if (renderer->statistics().pendingFrames || !presentations.isEmpty()) return Error::ResizePending;
        if (!surface.resize(wanted)) return Error::ResizePending;
        WGPUSurfaceConfigurationExtras extras{};
        extras.chain.sType = WGPUSType(WGPUSType_SurfaceConfigurationExtras);
        extras.desiredMaximumFrameLatency = 2;
        WGPUSurfaceConfiguration configuration{};
        configuration.nextInChain = &extras.chain;
        configuration.device = owner.device();
        configuration.format = format;
        configuration.usage = WGPUTextureUsage_RenderAttachment;
        configuration.width = wanted.width();
        configuration.height = wanted.height();
        configuration.presentMode = WGPUPresentMode_Fifo;
        configuration.alphaMode = WGPUCompositeAlphaMode_Opaque;
        const auto errors = owner.errorCount();
        wgpuSurfaceConfigure(surface.surface(), &configuration);
        if (!owner.available()) return Error::DeviceLost;
        if (owner.errorCount() != errors) return Error::SurfaceUnavailable;
        size = wanted;
        needsConfigure = false;
        return Error::None;
    }
};
SurfaceRenderer::SurfaceRenderer(KisGpuDevice &device, QWindow &window, quint64 budget, quint32 maximum) {
    if (!device.available() || !window.isVisible() || !window.isExposed() || !budget || !maximum || maximum > 2)
        throw std::runtime_error("Invalid GPU presentation configuration");
    d = std::make_unique<Private>(device, window, budget, maximum);
}
SurfaceRenderer::~SurfaceRenderer() = default;
SurfaceRenderer::Frame SurfaceRenderer::present(const KisGpuTileStore::TextureSnapshot &image, const GpuRenderer::View &view) {
    Frame frame;
    if (!d->owner.available()) { frame.error = Error::DeviceLost; return frame; }
    const auto ratio = d->window.devicePixelRatio();
    const auto width = d->window.width() * ratio, height = d->window.height() * ratio;
    if (!d->window.isVisible() || !d->window.isExposed() || d->window.winId() != d->windowId || width <= 0 || height <= 0
        || width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max()) {
        frame.error = Error::WindowUnavailable; return frame;
    }
    if (!image.texture() || !image.usesDevice(d->owner) || image.completion.status() != GpuRenderer::Status::Succeeded) {
        frame.error = Error::ImageRejected; return frame;
    }
    const QSize wanted(qRound(width), qRound(height));
    if (wanted != d->size || d->needsConfigure) {
        frame.error = d->configure(wanted);
        if (frame.error != Error::None) return frame;
    }
    if (d->renderer->statistics().pendingFrames >= d->maximumPending
        || d->presentations.size() >= d->maximumPending) { frame.error = Error::QueueFull; return frame; }
    WGPUSurfaceTexture current{};
    wgpuSurfaceGetCurrentTexture(d->surface.surface(), &current);
    if (!current.texture) {
        if (current.status == WGPUSurfaceGetCurrentTextureStatus_Outdated
            || current.status == WGPUSurfaceGetCurrentTextureStatus_Lost) d->needsConfigure = true;
        frame.error = d->owner.available() ? Error::SurfaceUnavailable : Error::DeviceLost;
        return frame;
    }
    frame.presentation = d->surface.presentation();
    frame.rendering = d->renderer->render(image, current.texture, view);
    if (frame.rendering.error != GpuRenderer::Error::None) {
        frame.error = frame.rendering.error == GpuRenderer::Error::DeviceLost ? Error::DeviceLost : Error::ImageRejected;
    } else if (wgpuSurfacePresent(d->surface.surface()) != WGPUStatus_Success) {
        frame.error = Error::PresentFailed;
        d->needsConfigure = true;
    } else {
        ++d->requests;
        if (frame.presentation.status() != Presentation::Status::Unavailable) d->presentations.push_back(frame.presentation);
    }
    if (frame.error != Error::None) frame.presentation.abandon();
    wgpuTextureRelease(current.texture);
    return frame;
}
void SurfaceRenderer::poll() { d->renderer->poll(); d->collectPresentations(); }
SurfaceRenderer::Statistics SurfaceRenderer::statistics() const {
    return {d->requests, quint64(d->size.width()) * d->size.height() * 12, d->size, d->renderer->statistics(),
            d->displayed, d->skipped, d->abandoned, quint32(d->presentations.size())};
}
}
