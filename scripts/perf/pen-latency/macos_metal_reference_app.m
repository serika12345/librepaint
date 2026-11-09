// SPDX-License-Identifier: GPL-3.0-or-later
//
// Metal pointer drawing window with presentation times: the macOS reference
// that checks the capture-based measurement against the time each frame was
// actually shown (MTLDrawable presentedTime).
//
// Like reference_app.py, it draws a black 21 px square at the hover position
// and a 20 px line while the button is held, on white, and presents a new
// frame on every mouse event. With REFERENCE_LOG set it writes the format of
// wayland_reference_app.c, which presentation_check.py reads:
//   input,<time_ns>,<x>,<y>,<pressed>
//   commit,<time_ns>,<frame>,<input_ns>
//   presented,<frame>,<time_ns>,0,0
//   discarded,<frame>
// Times are mach_absolute_time in nanoseconds (presentedTime uses the same
// clock as CACurrentMediaTime), the clock of macos_capture.mm.
//
// Build: clang -fobjc-arc -O2 macos_metal_reference_app.m -framework AppKit
//        -framework Metal -framework QuartzCore -o macos_metal_reference_app

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <simd/simd.h>
#include <mach/mach_time.h>
#include <stdio.h>

static mach_timebase_info_data_t timebase;
static uint64_t nowNs(void) { return mach_absolute_time() * timebase.numer / timebase.denom; }
static FILE *logFile;

static NSString *const shaderSource =
    @"#include <metal_stdlib>\n"
     "using namespace metal;\n"
     "vertex float4 v(uint id [[vertex_id]], constant float4 *rects [[buffer(0)]], uint inst [[instance_id]]) {\n"
     "  float4 r = rects[inst];\n"
     "  float2 c[4] = { r.xy, float2(r.z, r.y), float2(r.x, r.w), r.zw };\n"
     "  return float4(c[id], 0, 1);\n"
     "}\n"
     "fragment float4 f() { return float4(0, 0, 0, 1); }\n";

@interface MetalView : NSView
@property(nonatomic) CAMetalLayer *metalLayer;
@property(nonatomic) id<MTLDevice> device;
@property(nonatomic) id<MTLCommandQueue> queue;
@property(nonatomic) id<MTLRenderPipelineState> pipeline;
@property(nonatomic) id<MTLTexture> ink;
@property(nonatomic) NSPoint hover;
@property(nonatomic) NSPoint last;
@property(nonatomic) BOOL pressed;
@property(nonatomic) BOOL hovering;
@property(nonatomic) uint64_t inputNs;
@property(nonatomic) uint32_t frameCounter;
@end

@implementation MetalView
- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    self.device = MTLCreateSystemDefaultDevice();
    self.queue = [self.device newCommandQueue];
    self.wantsLayer = YES;
    self.metalLayer = [CAMetalLayer layer];
    self.metalLayer.device = self.device;
    self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalLayer.framebufferOnly = NO;
    self.layer = self.metalLayer;
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    MTLRenderPipelineDescriptor *desc = [MTLRenderPipelineDescriptor new];
    desc.vertexFunction = [library newFunctionWithName:@"v"];
    desc.fragmentFunction = [library newFunctionWithName:@"f"];
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    self.pipeline = [self.device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (!self.pipeline) NSLog(@"pipeline: %@", error);
    return self;
}
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }
- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    for (NSTrackingArea *area in self.trackingAreas) [self removeTrackingArea:area];
    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:self.bounds
                                                       options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect
                                                         owner:self userInfo:nil]];
}
- (void)viewDidChangeBackingProperties {
    self.metalLayer.contentsScale = self.window.backingScaleFactor;
    [self resize];
}
- (void)setFrameSize:(NSSize)size { [super setFrameSize:size]; [self resize]; }
- (void)resize {
    CGFloat scale = self.window ? self.window.backingScaleFactor : 2;
    CGSize pixels = CGSizeMake(self.bounds.size.width * scale, self.bounds.size.height * scale);
    if (pixels.width < 1 || pixels.height < 1) return;
    self.metalLayer.drawableSize = pixels;
    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                  width:(NSUInteger)pixels.width
                                                                                 height:(NSUInteger)pixels.height
                                                                              mipmapped:NO];
    td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    self.ink = [self.device newTextureWithDescriptor:td];
    [self renderTo:self.ink clear:YES rects:nil count:0];
    [self present];
}
// Points in view coordinates (origin bottom-left) to normalized device rectangles.
- (simd_float4)rectAt:(NSPoint)p half:(CGFloat)half {
    CGFloat w = self.bounds.size.width, h = self.bounds.size.height;
    return (simd_float4){(float)((p.x - half) / w * 2 - 1), (float)((p.y - half) / h * 2 - 1),
                         (float)((p.x + half) / w * 2 - 1), (float)((p.y + half) / h * 2 - 1)};
}
- (void)renderTo:(id<MTLTexture>)texture clear:(BOOL)clear rects:(const simd_float4 *)rects count:(NSUInteger)count {
    id<MTLCommandBuffer> cmd = [self.queue commandBuffer];
    [self encodeTo:texture clear:clear rects:rects count:count in:cmd];
    [cmd commit];
}
- (void)encodeTo:(id<MTLTexture>)texture clear:(BOOL)clear rects:(const simd_float4 *)rects count:(NSUInteger)count
              in:(id<MTLCommandBuffer>)cmd {
    MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = texture;
    pass.colorAttachments[0].loadAction = clear ? MTLLoadActionClear : MTLLoadActionLoad;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(1, 1, 1, 1);
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:pass];
    if (count) {
        [enc setRenderPipelineState:self.pipeline];
        [enc setVertexBytes:rects length:sizeof(simd_float4) * count atIndex:0];
        [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:count];
    }
    [enc endEncoding];
}
- (void)strokeTo:(NSPoint)p {
    CGFloat dx = p.x - self.last.x, dy = p.y - self.last.y;
    NSUInteger steps = (NSUInteger)(sqrt(dx * dx + dy * dy) / 2) + 1;
    simd_float4 *rects = calloc(steps + 1, sizeof(simd_float4));
    for (NSUInteger i = 0; i <= steps; ++i)
        rects[i] = [self rectAt:NSMakePoint(self.last.x + dx * i / steps, self.last.y + dy * i / steps) half:10];
    [self renderTo:self.ink clear:NO rects:rects count:steps + 1];
    free(rects);
    self.last = p;
}
- (void)present {
    id<CAMetalDrawable> drawable = [self.metalLayer nextDrawable];
    if (!drawable || !self.ink) return;
    id<MTLCommandBuffer> cmd = [self.queue commandBuffer];
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    [blit copyFromTexture:self.ink toTexture:drawable.texture];
    [blit endEncoding];
    simd_float4 square = [self rectAt:self.hover half:10.5];
    [self encodeTo:drawable.texture clear:NO rects:&square count:(self.hovering && !self.pressed) ? 1 : 0 in:cmd];
    uint32_t frame = ++self.frameCounter;
    [drawable addPresentedHandler:^(id<MTLDrawable> d) {
        if (!logFile) return;
        if (d.presentedTime > 0)
            fprintf(logFile, "presented,%u,%llu,0,0\n", frame, (unsigned long long)(d.presentedTime * 1e9));
        else
            fprintf(logFile, "discarded,%u\n", frame);
    }];
    [cmd presentDrawable:drawable];
    [cmd commit];
    if (logFile) fprintf(logFile, "commit,%llu,%u,%llu\n", (unsigned long long)nowNs(), frame, (unsigned long long)self.inputNs);
}
- (void)record:(NSPoint)p {
    self.inputNs = nowNs();
    if (logFile) fprintf(logFile, "input,%llu,%.2f,%.2f,%d\n", (unsigned long long)self.inputNs, p.x, p.y, self.pressed);
}
- (NSPoint)at:(NSEvent *)event { return [self convertPoint:event.locationInWindow fromView:nil]; }
- (void)mouseMoved:(NSEvent *)event {
    self.hover = [self at:event]; self.hovering = YES; [self record:self.hover]; [self present];
}
- (void)mouseDown:(NSEvent *)event {
    self.pressed = YES; self.last = [self at:event]; [self record:self.last]; [self strokeTo:self.last]; [self present];
}
- (void)mouseDragged:(NSEvent *)event {
    NSPoint p = [self at:event]; [self record:p]; [self strokeTo:p]; [self present];
}
- (void)mouseUp:(NSEvent *)event { self.pressed = NO; [self mouseMoved:event]; }
@end

int main(void) {
    @autoreleasepool {
        mach_timebase_info(&timebase);
        const char *path = getenv("REFERENCE_LOG");
        if (path) { logFile = fopen(path, "w"); setvbuf(logFile, NULL, _IOLBF, 0); }
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSRect screen = [NSScreen mainScreen].visibleFrame;
        NSRect frame = NSInsetRect(screen, screen.size.width * 0.1, screen.size.height * 0.1);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                         backing:NSBackingStoreBuffered defer:NO];
        MetalView *view = [[MetalView alloc] initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
        window.contentView = view;
        window.acceptsMouseMovedEvents = YES;
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}
