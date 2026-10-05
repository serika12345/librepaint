/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KRITA_CANVAS_WGPU_WINDOW_SURFACE_H
#define KRITA_CANVAS_WGPU_WINDOW_SURFACE_H

#include <QString>
#include <QWindow>
#include <webgpu/webgpu.h>

namespace Krita::Canvas
{
/** Native surface connection. Destroy the GPU renderer before this connection
 * and this connection before its borrowed QWindow.
 */
class WgpuWindowSurface
{
public:
    explicit WgpuWindowSurface(QWindow &window);
    ~WgpuWindowSurface();
    WgpuWindowSurface(const WgpuWindowSurface &) = delete;
    WgpuWindowSurface &operator=(const WgpuWindowSurface &) = delete;
    const WGPUChainedStruct *source() const;
    void resize();
    QString error() const { return m_error; }

private:
    QWindow &m_window;
    QString m_error;
#ifdef Q_OS_DARWIN
    WGPUSurfaceSourceMetalLayer m_source = {};
#elif defined(Q_OS_WIN)
    WGPUSurfaceSourceWindowsHWND m_source = {};
#else
    WGPUSurfaceSourceXlibWindow m_source = {};
#endif
};
}
#endif
