/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GpuWindowSurface_p.h"
#include "SurfaceRenderer.h"
#include <QGuiApplication>
#include <QWindow>
#include <stdexcept>
#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>
namespace Krita::Canvas {
struct GpuWindowSurface::Private {
    NSView *view = nil;
    CAMetalLayer *layer = nil;
    WGPUSurface surface = nullptr;
    Private(KisGpuDevice &device, QWindow &window) {
        if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
            throw std::runtime_error("GPU window requires the Cocoa Qt platform");
        if (window.surfaceType() != QSurface::MetalSurface) throw std::runtime_error("GPU window requires a Metal surface");
        view = reinterpret_cast<NSView *>(window.winId());
        layer = [[CAMetalLayer alloc] init];
        layer.frame = view.bounds;
        layer.contentsScale = window.devicePixelRatio();
        const auto colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        layer.colorspace = colorSpace;
        CGColorSpaceRelease(colorSpace);
        [view.layer addSublayer:layer];
        WGPUSurfaceSourceMetalLayer metal{};
        metal.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        metal.layer = layer;
        WGPUSurfaceDescriptor descriptor{};
        descriptor.nextInChain = &metal.chain;
        surface = wgpuInstanceCreateSurface(device.instance(), &descriptor);
        if (!surface) {
            [layer removeFromSuperlayer];
            [layer release];
            layer = nil;
            throw std::runtime_error("Cannot create GPU window surface");
        }
    }
    ~Private() {
        wgpuSurfaceUnconfigure(surface);
        wgpuSurfaceRelease(surface);
        [layer removeFromSuperlayer];
        [layer release];
    }
};
void SurfaceRenderer::prepareWindow(QWindow &window) { window.setSurfaceType(QSurface::MetalSurface); }
GpuWindowSurface::GpuWindowSurface(KisGpuDevice &device, QWindow &window) : d(new Private(device, window)) {}
GpuWindowSurface::~GpuWindowSurface() = default;
WGPUSurface GpuWindowSurface::surface() const { return d->surface; }
bool GpuWindowSurface::resize(QSize size) {
    d->layer.frame = d->view.bounds;
    d->layer.drawableSize = CGSizeMake(size.width(), size.height());
    return true;
}
}
