/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuWindowSurface.h"
#include <QGuiApplication>
#include <QtGui/qguiapplication_platform.h>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Krita::Canvas
{
WgpuWindowSurface::WgpuWindowSurface(QWindow &window) : m_window(window)
{
#ifdef Q_OS_WIN
    m_source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    m_source.hinstance = GetModuleHandle(nullptr);
    m_source.hwnd = reinterpret_cast<void *>(window.winId());
#elif QT_CONFIG(xcb)
    auto *native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (native) {
        m_source.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        m_source.display = native->display();
        m_source.window = uint64_t(window.winId());
    } else {
        m_error = QStringLiteral("The wgpu canvas probe requires the Qt xcb platform on Linux");
    }
#else
    m_error = QStringLiteral("This Qt platform has no native surface connection for the wgpu canvas probe");
#endif
}
WgpuWindowSurface::~WgpuWindowSurface() = default;
const WGPUChainedStruct *WgpuWindowSurface::source() const { return m_error.isEmpty() ? &m_source.chain : nullptr; }
void WgpuWindowSurface::resize() {}
}
