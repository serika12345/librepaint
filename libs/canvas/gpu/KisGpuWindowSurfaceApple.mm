/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuWindowSurface_p.h"
#include <webgpu/wgpu.h>
#import <QuartzCore/CAMetalLayer.h>
#ifdef Q_OS_IOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif
#include <stdexcept>
namespace LibrePaint::Canvas::Gpu::Detail {
namespace {
CAMetalLayer *metalLayer(CALayer *layer)
{
    if ([layer isKindOfClass:[CAMetalLayer class]]) return static_cast<CAMetalLayer *>(layer);
    for (CALayer *child in layer.sublayers) if (auto metal = metalLayer(child)) return metal;
    return nil;
}
}
void setNativeSurfaceType(QWindow &window) { window.setSurfaceType(QSurface::MetalSurface); }
std::unique_ptr<Image::Gpu::PresentationSurface> createWindowSurface(Image::Gpu::DevicePtr device, QWindow &window)
{
#ifdef Q_OS_IOS
    UIView *view = reinterpret_cast<UIView *>(window.winId());
#else
    NSView *view = reinterpret_cast<NSView *>(window.winId());
    view.wantsLayer = YES;
#endif
    CAMetalLayer *layer = metalLayer(view.layer);
    if (!layer) throw std::runtime_error("The Qt window does not own a Metal layer");
    layer.contentsScale = window.devicePixelRatio();
    layer.drawableSize = CGSizeMake(window.width() * window.devicePixelRatio(), window.height() * window.devicePixelRatio());
    WGPUSurfaceSourceMetalLayer source{};
    source.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    source.layer = layer;
    WGPUSurfaceDescriptor descriptor{};
    descriptor.nextInChain = &source.chain;
    return std::make_unique<Image::Gpu::PresentationSurface>(std::move(device), descriptor);
}
}
