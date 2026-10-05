/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "WgpuWindowSurface.h"

#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/CATransaction.h>
#ifdef Q_OS_IOS
#include <UIKit/UIKit.h>
#else
#include <AppKit/AppKit.h>
#endif

namespace Krita::Canvas
{
WgpuWindowSurface::WgpuWindowSurface(QWindow &window) : m_window(window)
{
    @autoreleasepool {
#ifdef Q_OS_IOS
        UIView *view = reinterpret_cast<UIView *>(window.winId());
#else
        NSView *view = reinterpret_cast<NSView *>(window.winId());
        view.wantsLayer = YES;
#endif
        CAMetalLayer *layer = [[CAMetalLayer alloc] init];
        layer.presentsWithTransaction = NO;
        layer.framebufferOnly = NO;
        [view.layer addSublayer:layer];
        m_source.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        m_source.layer = layer;
        resize();
    }
}

WgpuWindowSurface::~WgpuWindowSurface()
{
    @autoreleasepool {
        CAMetalLayer *layer = static_cast<CAMetalLayer *>(m_source.layer);
        [layer removeFromSuperlayer];
        [layer release];
    }
}

const WGPUChainedStruct *WgpuWindowSurface::source() const { return &m_source.chain; }

void WgpuWindowSurface::resize()
{
    @autoreleasepool {
        CAMetalLayer *layer = static_cast<CAMetalLayer *>(m_source.layer);
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        layer.frame = CGRectMake(0, 0, m_window.width(), m_window.height());
        layer.contentsScale = m_window.devicePixelRatio();
        layer.drawableSize = CGSizeMake(m_window.width() * m_window.devicePixelRatio(),
                                        m_window.height() * m_window.devicePixelRatio());
        [CATransaction commit];
    }
}
}
