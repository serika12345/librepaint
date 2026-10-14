/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuWindowSurface_p.h"
#include <webgpu/wgpu.h>
#include <QGuiApplication>
#include <QtGui/qguiapplication_platform.h>
#include <stdexcept>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#ifdef Q_OS_LINUX
#include <qpa/qplatformnativeinterface.h>
#endif
namespace LibrePaint::Canvas::Gpu::Detail {
#ifndef Q_OS_DARWIN
void setNativeSurfaceType(QWindow &window) { window.setSurfaceType(QSurface::RasterSurface); }
std::unique_ptr<Image::Gpu::PresentationSurface> createWindowSurface(Image::Gpu::DevicePtr device, QWindow &window)
{
    WGPUSurfaceDescriptor descriptor{};
#ifdef Q_OS_WIN
    WGPUSurfaceSourceWindowsHWND source{};
    source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    source.hinstance = GetModuleHandle(nullptr);
    source.hwnd = reinterpret_cast<HWND>(window.winId());
    descriptor.nextInChain = &source.chain;
    return std::make_unique<Image::Gpu::PresentationSurface>(std::move(device), descriptor);
#elif defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#if QT_CONFIG(xcb)
    if (auto native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        WGPUSurfaceSourceXlibWindow source{};
        source.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        source.display = native->display();
        source.window = window.winId();
        descriptor.nextInChain = &source.chain;
        return std::make_unique<Image::Gpu::PresentationSurface>(std::move(device), descriptor);
    }
#endif
#if QT_CONFIG(wayland)
    if (auto native = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
        WGPUSurfaceSourceWaylandSurface source{};
        source.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
        source.display = native->display();
        source.surface = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", &window);
        if (!source.surface) throw std::runtime_error("The Wayland window has no native surface");
        descriptor.nextInChain = &source.chain;
        return std::make_unique<Image::Gpu::PresentationSurface>(std::move(device), descriptor);
    }
#endif
#endif
    throw std::runtime_error("The window platform has no native GPU surface connection");
}
#endif
}
