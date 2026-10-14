/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuWindowSurface_p.h"
#include <webgpu/wgpu.h>
#include <QGuiApplication>
#include <QtGui/qguiapplication_platform.h>
#include <stdexcept>
#ifdef Q_OS_ANDROID
#include <QScopeGuard>
#include <QVariant>
#include <android/native_window.h>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#if defined(Q_OS_LINUX)
#include <qpa/qplatformnativeinterface.h>
#endif
namespace LibrePaint::Canvas::Gpu::Detail {
#ifndef Q_OS_DARWIN
void setNativeSurfaceType(QWindow &window)
{
#ifdef Q_OS_ANDROID
    window.setProperty("_librepaint_gpu_surface", true);
    window.setSurfaceType(QSurface::VulkanSurface);
#else
    window.setSurfaceType(QSurface::RasterSurface);
#endif
}
#ifdef Q_OS_ANDROID
namespace {
ANativeWindow *acquireAndroidWindow(QWindow &window)
{
    auto *platform = QGuiApplication::platformNativeInterface();
    if (!platform || !platform->nativeResourceForIntegration("librepaintAndroidNativeWindow"))
        throw std::runtime_error("The Qt Android platform plugin has no GPU native-window connection");
    window.create();
    return static_cast<ANativeWindow *>(platform->nativeResourceForWindow(
        "librepaintAndroidNativeWindow", &window));
}
}
quintptr currentNativeSurfaceIdentity(QWindow &window)
{
    auto *native = acquireAndroidWindow(window);
    const auto identity = reinterpret_cast<quintptr>(native);
    if (native) ANativeWindow_release(native);
    return identity;
}
#endif
std::unique_ptr<Image::Gpu::PresentationSurface> createWindowSurface(Image::Gpu::DevicePtr device, QWindow &window,
                                                                  quintptr *nativeIdentity)
{
    if (nativeIdentity) *nativeIdentity = 0;
#ifdef Q_OS_WIN
    WGPUSurfaceDescriptor descriptor{};
    WGPUSurfaceSourceWindowsHWND source{};
    source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    source.hinstance = GetModuleHandle(nullptr);
    source.hwnd = reinterpret_cast<HWND>(window.winId());
    descriptor.nextInChain = &source.chain;
    return std::make_unique<Image::Gpu::PresentationSurface>(std::move(device), descriptor);
#elif defined(Q_OS_ANDROID)
    auto *native = acquireAndroidWindow(window);
    if (!native) throw std::runtime_error("The Android window has no native surface");
    const auto release = qScopeGuard([native] { ANativeWindow_release(native); });
    WGPUSurfaceSourceAndroidNativeWindow source{};
    source.chain.sType = WGPUSType_SurfaceSourceAndroidNativeWindow;
    source.window = native;
    WGPUSurfaceDescriptor descriptor{};
    descriptor.nextInChain = &source.chain;
    auto surface = std::make_unique<Image::Gpu::PresentationSurface>(std::move(device), descriptor);
    if (nativeIdentity) *nativeIdentity = reinterpret_cast<quintptr>(native);
    return surface;
#elif defined(Q_OS_LINUX)
    WGPUSurfaceDescriptor descriptor{};
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
#else
    Q_UNUSED(device);
    Q_UNUSED(window);
#endif
    throw std::runtime_error("The window platform has no native GPU surface connection");
}
#endif
}
