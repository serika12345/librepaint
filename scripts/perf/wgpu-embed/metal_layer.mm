// SPDX-License-Identifier: GPL-3.0-or-later
//
// macOS part of wgpu_embed_check: gives the canvas's NSView a CAMetalLayer
// for the wgpu surface. The view belongs to a native child widget with
// Qt::WA_PaintOnScreen, so Qt paints nothing into it.

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

void *wgpuEmbedMetalLayer(void *nsView)
{
    NSView *view = (__bridge NSView *)nsView;
    CAMetalLayer *layer = [CAMetalLayer layer];
    layer.contentsScale = view.window.backingScaleFactor;
    layer.opaque = YES;
    view.wantsLayer = YES;
    view.layer = layer;
    return (__bridge void *)layer;
}
