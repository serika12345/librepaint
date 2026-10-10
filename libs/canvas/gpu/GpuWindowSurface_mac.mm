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
#import <QuartzCore/CATransaction.h>
#import <Metal/MTLDrawable.h>
#include <cmath>
#include <limits>

@interface LibrePaintGpuPresentationLayer : CAMetalLayer {
@public
    std::shared_ptr<Krita::Canvas::GpuPresentationData> latestPresentation;
    BOOL commitConfiguration;
}
@end
@implementation LibrePaintGpuPresentationLayer
- (id<CAMetalDrawable>)nextDrawable {
    if (commitConfiguration) {
        [CATransaction flush];
        commitConfiguration = NO;
    }
    auto drawable = [super nextDrawable];
    if (drawable) {
        latestPresentation = std::make_shared<Krita::Canvas::GpuPresentationData>();
        auto state = latestPresentation;
        [drawable addPresentedHandler:^(id<MTLDrawable> presented) {
            const auto time = presented.presentedTime;
            const bool valid = std::isfinite(time) && time > 0 && time < double(std::numeric_limits<quint64>::max()) / 1e9;
            state->finish(valid ? quint64(time * 1e9) : 0);
        }];
    }
    return drawable;
}
@end

namespace Krita::Canvas {
struct GpuWindowSurface::Private {
    NSView *view = nil;
    LibrePaintGpuPresentationLayer *layer = nil;
    WGPUSurface surface = nullptr;
    Private(KisGpuDevice &device, QWindow &window) {
        if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
            throw std::runtime_error("GPU window requires the Cocoa Qt platform");
        if (window.surfaceType() != QSurface::MetalSurface) throw std::runtime_error("GPU window requires a Metal surface");
        view = reinterpret_cast<NSView *>(window.winId());
        layer = [[LibrePaintGpuPresentationLayer alloc] init];
        layer->commitConfiguration = YES;
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
        if (layer->latestPresentation) layer->latestPresentation->abandon();
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
SurfaceRenderer::Presentation GpuWindowSurface::presentation() const {
    return SurfaceRenderer::Presentation(d->layer->latestPresentation);
}
std::optional<quint64> GpuWindowSurface::presentationClockNanoseconds() {
    return quint64(CACurrentMediaTime() * 1e9);
}
bool GpuWindowSurface::resize(QSize size) {
    d->layer.frame = d->view.bounds;
    d->layer.drawableSize = CGSizeMake(size.width(), size.height());
    d->layer->commitConfiguration = YES;
    return true;
}
}
