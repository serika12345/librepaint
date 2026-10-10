/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GpuWindowSurface_p.h"
#include "SurfaceRenderer.h"
#include <QGuiApplication>
#include <QWindow>
#include <qguiapplication_platform.h>
#include <stdexcept>
#include <X11/Xlib.h>
namespace Krita::Canvas {
struct GpuWindowSurface::Private {
    WGPUSurface surface = nullptr;
    Display *display = nullptr;
    WId windowId = 0;
    Private(KisGpuDevice &device, QWindow &window) {
        if (QGuiApplication::platformName() != QStringLiteral("xcb"))
            throw std::runtime_error("GPU window requires the X11 Qt platform");
        const auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
        if (!native || !native->display()) throw std::runtime_error("GPU window requires an X11 display");
        WGPUSurfaceSourceXlibWindow xlib{};
        xlib.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        display = native->display();
        windowId = window.winId();
        xlib.display = display;
        xlib.window = windowId;
        WGPUSurfaceDescriptor descriptor{};
        descriptor.nextInChain = &xlib.chain;
        surface = wgpuInstanceCreateSurface(device.instance(), &descriptor);
        if (!surface) throw std::runtime_error("Cannot create GPU window surface");
    }
    ~Private() {
        wgpuSurfaceUnconfigure(surface);
        wgpuSurfaceRelease(surface);
    }
};
void SurfaceRenderer::prepareWindow(QWindow &) {}
GpuWindowSurface::GpuWindowSurface(KisGpuDevice &device, QWindow &window) : d(new Private(device, window)) {}
GpuWindowSurface::~GpuWindowSurface() = default;
WGPUSurface GpuWindowSurface::surface() const { return d->surface; }
bool GpuWindowSurface::resize(QSize size) {
    XWindowAttributes attributes{};
    return XGetWindowAttributes(d->display, d->windowId, &attributes)
        && attributes.width == size.width() && attributes.height == size.height();
}
}
