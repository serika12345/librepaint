// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal Cocoa pointer drawing window, the macOS reference for
// capture-based latency (see reference_app.py for the rule). On every mouse
// event the view redraws: a black 21 px square at the hover position, and a
// 20 px black line while the button is held, on white.
//
// Build: clang -fobjc-arc macos_reference_app.m -framework AppKit -o macos_reference_app

#import <AppKit/AppKit.h>

@interface DrawView : NSView
@property(nonatomic) NSBezierPath *ink;
@property(nonatomic) NSPoint hover;
@property(nonatomic) BOOL hovering;
@end

@implementation DrawView
- (BOOL)isFlipped { return NO; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }
- (void)drawRect:(NSRect)dirty {
    [[NSColor whiteColor] setFill];
    NSRectFill(dirty);
    [[NSColor blackColor] set];
    if (self.ink) [self.ink stroke];
    if (self.hovering) NSRectFill(NSMakeRect(self.hover.x - 10, self.hover.y - 10, 21, 21));
}
// Receive mouse moves whether or not the window is key.
- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    for (NSTrackingArea *area in self.trackingAreas) [self removeTrackingArea:area];
    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:self.bounds
                                                       options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect
                                                         owner:self userInfo:nil]];
}
- (NSPoint)at:(NSEvent *)event { return [self convertPoint:event.locationInWindow fromView:nil]; }
- (void)mouseMoved:(NSEvent *)event { self.hover = [self at:event]; self.hovering = YES; [self setNeedsDisplay:YES]; }
- (void)mouseDown:(NSEvent *)event {
    if (!self.ink) { self.ink = [NSBezierPath bezierPath]; self.ink.lineWidth = 20; self.ink.lineCapStyle = NSLineCapStyleRound; }
    self.hovering = NO;
    NSPoint p = [self at:event];
    [self.ink moveToPoint:p];
    [self.ink lineToPoint:p];
    [self setNeedsDisplay:YES];
}
- (void)mouseDragged:(NSEvent *)event { [self.ink lineToPoint:[self at:event]]; [self setNeedsDisplay:YES]; }
- (void)mouseUp:(NSEvent *)event { [self mouseMoved:event]; }
@end

int main(void) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSRect screen = [NSScreen mainScreen].visibleFrame;
        NSRect frame = NSInsetRect(screen, screen.size.width * 0.1, screen.size.height * 0.1);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                         backing:NSBackingStoreBuffered defer:NO];
        DrawView *view = [[DrawView alloc] initWithFrame:frame];
        window.contentView = view;
        window.acceptsMouseMovedEvents = YES;
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}
