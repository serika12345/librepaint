// SPDX-License-Identifier: GPL-3.0-or-later
//
// Capture-based pointer latency measurement for macOS.
//
//   macos_capture --window-pid PID --out DIR --snapshot
//       Saves first-frame.png and canvas.json (the white canvas rectangle in
//       captured pixels) for the largest on-screen window of process PID.
//   macos_capture --window-pid PID --out DIR --plan plan.csv
//       Takes the snapshot, applies plan.csv (trajectory.py, relative to the
//       canvas) to the detected canvas, replays it with Quartz mouse events
//       while ScreenCaptureKit captures that one window, and writes inject.csv
//       and capture.csv in the format of mutter_capture.py.
//
// The tool captures one application window only; it has no display capture
// mode, so nothing else on the screen is recorded. Captured pixels, canvas
// rectangles and injected positions are relative to the window's content.
//
// Times are mach_absolute_time converted to nanoseconds. frame_ns is the
// frame's SCStreamFrameInfoDisplayTime. Posting events needs the
// Accessibility permission and capturing needs Screen Recording permission
// for the process that starts this tool.

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreImage/CoreImage.h>
#import <CoreMedia/CoreMedia.h>
#import <ImageIO/ImageIO.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <mach/mach_time.h>
#include <stdio.h>
#include <vector>
#include <string>

static mach_timebase_info_data_t timebase;
static uint64_t toNs(uint64_t ticks) { return ticks * timebase.numer / timebase.denom; }
static uint64_t nowNs() { return toNs(mach_absolute_time()); }

struct Band { std::string name; int x0, x1, y0, y1; };
struct Sample { double t; std::string phase, band; double x, y; int pressed; };

@interface Output : NSObject <SCStreamOutput>
@property(nonatomic) std::vector<Band> bands;
@property(nonatomic) BOOL detecting;
@property(nonatomic) int dark;
@property(nonatomic) int dense;
@property(nonatomic) dispatch_semaphore_t firstFrame;
@property(nonatomic) CVPixelBufferRef latest;
@property(nonatomic) NSMutableString *rows;
@end

@implementation Output
- (void)stream:(SCStream *)stream didOutputSampleBuffer:(CMSampleBufferRef)sample ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen) return;
    uint64_t arrival = nowNs();
    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sample, false);
    if (!attachments || CFArrayGetCount(attachments) == 0) return;
    NSDictionary *info = (__bridge NSDictionary *)CFArrayGetValueAtIndex(attachments, 0);
    if ([info[SCStreamFrameInfoStatus] integerValue] != SCFrameStatusComplete) return;
    uint64_t frameNs = toNs([info[SCStreamFrameInfoDisplayTime] unsignedLongLongValue]);
    CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
    if (!pixels) return;
    @synchronized(self) {
        if (!self.detecting) {
            BOOL first = self.latest == NULL;
            if (self.latest) CVPixelBufferRelease(self.latest);
            self.latest = CVPixelBufferRetain(pixels);
            if (first) dispatch_semaphore_signal(self.firstFrame);
            return;
        }
        CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
        const uint8_t *base = (const uint8_t *)CVPixelBufferGetBaseAddress(pixels);
        size_t stride = CVPixelBufferGetBytesPerRow(pixels);
        for (const Band &b : self.bands) {
            int xmin = -1, xmax = -1, dxmin = -1, dxmax = -1, ymin = -1, ymax = -1;
            std::vector<int> columns(b.x1 - b.x0, 0);
            for (int y = b.y0; y < b.y1; ++y) {
                const uint8_t *row = base + y * stride;
                bool any = false;
                for (int x = b.x0; x < b.x1; ++x) {
                    const uint8_t *p = row + 4 * x; // BGRA
                    if (p[0] < self.dark && p[1] < self.dark && p[2] < self.dark) { columns[x - b.x0]++; any = true; }
                }
                if (any) { if (ymin < 0) ymin = y; ymax = y; }
            }
            for (int i = 0; i < (int)columns.size(); ++i) {
                if (columns[i] > 0) { if (xmin < 0) xmin = b.x0 + i; xmax = b.x0 + i; }
                if (columns[i] >= self.dense) { if (dxmin < 0) dxmin = b.x0 + i; dxmax = b.x0 + i; }
            }
            if (xmin < 0) {
                [self.rows appendFormat:@"%llu,%llu,%s,,,,,,\n", frameNs, arrival, b.name.c_str()];
            } else {
                NSString *dense = dxmin < 0 ? @"," : [NSString stringWithFormat:@"%d,%d", dxmin, dxmax];
                [self.rows appendFormat:@"%llu,%llu,%s,%d,%d,%@,%d,%d\n", frameNs, arrival, b.name.c_str(), xmin, xmax, dense, ymin, ymax];
            }
        }
        CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    }
}
@end

static void savePng(CVPixelBufferRef pixels, NSString *path) {
    CIImage *image = [CIImage imageWithCVPixelBuffer:pixels];
    CIContext *context = [CIContext context];
    CGImageRef cg = [context createCGImage:image fromRect:image.extent];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:path], CFSTR("public.png"), 1, NULL);
    CGImageDestinationAddImage(dest, cg, NULL);
    CGImageDestinationFinalize(dest);
    CFRelease(dest);
    CGImageRelease(cg);
}

// Brings the application to the front with the Accessibility API, so that the
// mouse events posted below reach its window.
static void raiseApplication(pid_t pid) {
    AXUIElementRef application = AXUIElementCreateApplication(pid);
    if (!application) return;
    AXError frontmostError = AXUIElementSetAttributeValue(application, kAXFrontmostAttribute, kCFBooleanTrue);
    if (frontmostError != kAXErrorSuccess)
        fprintf(stderr, "raise: setting the frontmost attribute failed with AXError %d (Accessibility trusted: %d)\n", (int)frontmostError, (int)AXIsProcessTrusted());
    CFTypeRef windows = NULL;
    if (AXUIElementCopyAttributeValue(application, kAXWindowsAttribute, &windows) == kAXErrorSuccess && windows) {
        if (CFArrayGetCount((CFArrayRef)windows) > 0)
            AXUIElementPerformAction((AXUIElementRef)CFArrayGetValueAtIndex((CFArrayRef)windows, 0), kAXRaiseAction);
        CFRelease(windows);
    }
    CFRelease(application);
}

// True when the frontmost visible window at the global point belongs to pid.
// The tool posts mouse presses and drags, so it posts nothing while another
// application's window covers the target point.
// Describes the frontmost visible window at the point for error messages: the
// owner's process name and id, the window layer, and its bounds. Titles are not read.
static NSString *frontWindowDescriptionAtPoint(CGPoint point) {
    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!list) return @"no window list";
    NSString *description = @"no window at the point";
    for (NSDictionary *info in (__bridge NSArray *)list) {
        CGRect frame;
        if (!CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)info[(id)kCGWindowBounds], &frame)) continue;
        if (!CGRectContainsPoint(frame, point)) continue;
        if ([info[(id)kCGWindowAlpha] doubleValue] <= 0.01) continue;
        description = [NSString stringWithFormat:@"front window: owner %@ (pid %d), layer %d, bounds [%g, %g, %g, %g]",
                       info[(id)kCGWindowOwnerName], [info[(id)kCGWindowOwnerPID] intValue], [info[(id)kCGWindowLayer] intValue],
                       frame.origin.x, frame.origin.y, frame.size.width, frame.size.height];
        break;
    }
    CFRelease(list);
    return description;
}

static bool frontWindowAtPointBelongsTo(pid_t pid, CGPoint point) {
    CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!list) return false;
    bool belongs = false;
    for (NSDictionary *info in (__bridge NSArray *)list) {
        CGRect frame;
        if (!CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)info[(id)kCGWindowBounds], &frame)) continue;
        if (!CGRectContainsPoint(frame, point)) continue;
        if ([info[(id)kCGWindowAlpha] doubleValue] <= 0.01) continue;
        belongs = [info[(id)kCGWindowOwnerPID] intValue] == pid;
        break;
    }
    CFRelease(list);
    return belongs;
}

// Same rule as mutter_capture.find_canvas: the white rectangle around the
// white pixel nearest the frame center, inset by 8 pixels.
static bool findCanvas(CVPixelBufferRef pixels, int rect[4]) {
    CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    const uint8_t *base = (const uint8_t *)CVPixelBufferGetBaseAddress(pixels);
    size_t stride = CVPixelBufferGetBytesPerRow(pixels);
    int w = (int)CVPixelBufferGetWidth(pixels), h = (int)CVPixelBufferGetHeight(pixels);
    auto white = [&](int x, int y) {
        const uint8_t *p = base + y * stride + 4 * x;
        return p[0] >= 245 && p[1] >= 245 && p[2] >= 245;
    };
    long best = -1; int cx = 0, cy = 0;
    for (int y = 0; y < h; y += 2)
        for (int x = 0; x < w; x += 2)
            if (white(x, y)) {
                long d = (long)(x - w / 2) * (x - w / 2) + (long)(y - h / 2) * (y - h / 2);
                if (best < 0 || d < best) { best = d; cx = x; cy = y; }
            }
    if (best < 0) { CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly); return false; }
    int left = cx, right = cx, top = cy, bottom = cy;
    while (left > 0 && white(left - 1, cy)) left--;
    while (right < w - 1 && white(right + 1, cy)) right++;
    while (top > 0 && white(cx, top - 1)) top--;
    while (bottom < h - 1 && white(cx, bottom + 1)) bottom++;
    CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    rect[0] = left + 8; rect[1] = top + 8; rect[2] = right - left + 1 - 16; rect[3] = bottom - top + 1 - 16;
    return true;
}

// Reads a plan relative to the canvas and maps it to the canvas rectangle in pixels.
static std::vector<Sample> readPlan(NSString *path, std::vector<Band> &bands, const int rect[4]) {
    auto px = [&](double f) { return rect[0] + f * rect[2]; };
    auto py = [&](double f) { return rect[1] + f * rect[3]; };
    std::vector<Sample> samples;
    NSString *text = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    for (NSString *line in [text componentsSeparatedByString:@"\n"]) {
        if ([line hasPrefix:@"# bands="]) {
            for (NSString *entry in [[line substringFromIndex:8] componentsSeparatedByString:@";"]) {
                NSArray *f = [entry componentsSeparatedByString:@":"];
                bands.push_back({[f[0] UTF8String], (int)px([f[1] doubleValue]), (int)px([f[2] doubleValue]),
                                 (int)py([f[3] doubleValue]), (int)py([f[4] doubleValue]) + 1});
            }
            continue;
        }
        NSArray *f = [line componentsSeparatedByString:@","];
        if (f.count != 6 || [f[0] isEqualToString:@"time_s"]) continue;
        samples.push_back({[f[0] doubleValue], [f[1] UTF8String], [f[2] UTF8String], px([f[3] doubleValue]), py([f[4] doubleValue]), [f[5] intValue]});
    }
    return samples;
}

int main(int argc, const char **argv) {
    @autoreleasepool {
        mach_timebase_info(&timebase);
        [NSApplication sharedApplication]; // connects to the window server for the CoreGraphics calls
        int windowPid = 0; BOOL snapshot = NO; NSString *out = nil, *planPath = nil;
        int dark = 128, dense = 6;
        for (int i = 1; i < argc; ++i) {
            NSString *a = @(argv[i]);
            if ([a isEqualToString:@"--window-pid"] && i + 1 < argc) windowPid = atoi(argv[++i]);
            else if ([a isEqualToString:@"--out"] && i + 1 < argc) out = @(argv[++i]);
            else if ([a isEqualToString:@"--plan"] && i + 1 < argc) planPath = @(argv[++i]);
            else if ([a isEqualToString:@"--dark"] && i + 1 < argc) dark = atoi(argv[++i]);
            else if ([a isEqualToString:@"--dense"] && i + 1 < argc) dense = atoi(argv[++i]);
            else if ([a isEqualToString:@"--snapshot"]) snapshot = YES;
        }
        if (windowPid <= 0 || !out || (!snapshot && !planPath)) {
            fprintf(stderr, "usage: macos_capture --window-pid PID --out DIR (--snapshot | --plan plan.csv)\n");
            return 2;
        }
        [[NSFileManager defaultManager] createDirectoryAtPath:out withIntermediateDirectories:YES attributes:nil error:nil];
        // Posted mouse events are dropped without the Accessibility permission, so a run
        // without it would capture a canvas that never receives input.
        if (!snapshot && !AXIsProcessTrusted()) {
            fprintf(stderr, "Accessibility is not granted to the process that runs this tool; posted mouse events would be dropped\n");
            return 1;
        }

        // Find the largest on-screen window of the process. The application may
        // still be starting, so look again for up to 30 seconds.
        __block SCShareableContent *content = nil;
        dispatch_semaphore_t got = dispatch_semaphore_create(0);
        SCWindow *window = nil;
        for (int attempt = 0; attempt < 60 && !window; ++attempt) {
            [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent *c, NSError *e) {
                if (e && attempt == 0) fprintf(stderr, "shareable content: %s\n", e.localizedDescription.UTF8String);
                content = c;
                dispatch_semaphore_signal(got);
            }];
            dispatch_semaphore_wait(got, DISPATCH_TIME_FOREVER);
            if (!content) { fprintf(stderr, "no shareable content (Screen Recording permission?)\n"); return 1; }
            double bestArea = 0;
            for (SCWindow *candidate in content.windows) {
                if (candidate.owningApplication.processID != windowPid || !candidate.isOnScreen || candidate.windowLayer != 0) continue;
                double area = candidate.frame.size.width * candidate.frame.size.height;
                if (candidate.frame.size.width >= 100 && candidate.frame.size.height >= 100 && area > bestArea) { window = candidate; bestArea = area; }
            }
            if (!window) usleep(500000);
        }
        if (!window) {
            fprintf(stderr, "no on-screen window of process %d\n", windowPid);
            return 1;
        }
        CGRect bounds = window.frame; // points, global coordinates (top-left origin)
        SCContentFilter *filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
        SCShareableContentInfo *info = [SCShareableContent infoForFilter:filter];
        double scale = info.pointPixelScale;
        size_t pixelWidth = (size_t)(info.contentRect.size.width * scale + 0.5);
        size_t pixelHeight = (size_t)(info.contentRect.size.height * scale + 0.5);
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(CGMainDisplayID());
        double refresh = CGDisplayModeGetRefreshRate(mode);
        CGDisplayModeRelease(mode);

        SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
        config.width = pixelWidth;
        config.height = pixelHeight;
        config.pixelFormat = kCVPixelFormatType_32BGRA;
        config.showsCursor = NO;
        config.minimumFrameInterval = CMTimeMake(1, 240);
        config.queueDepth = 6;

        Output *output = [[Output alloc] init];
        output.firstFrame = dispatch_semaphore_create(0);
        output.rows = [NSMutableString stringWithString:@"frame_ns,arrival_ns,band,xmin,xmax,dense_xmin,dense_xmax,ymin,ymax\n"];
        output.dark = dark; output.dense = dense;
        SCStream *stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:nil];
        dispatch_queue_t queue = dispatch_queue_create("capture", DISPATCH_QUEUE_SERIAL);
        NSError *error = nil;
        [stream addStreamOutput:output type:SCStreamOutputTypeScreen sampleHandlerQueue:queue error:&error];
        __block NSError *startError = nil;
        [stream startCaptureWithCompletionHandler:^(NSError *e) { startError = e; dispatch_semaphore_signal(got); }];
        dispatch_semaphore_wait(got, DISPATCH_TIME_FOREVER);
        if (startError) { fprintf(stderr, "start capture: %s\n", startError.localizedDescription.UTF8String); return 1; }

        raiseApplication(windowPid);
        usleep(1000000);
        // Park the pointer on the title bar of the target window so that the application's
        // hover outline does not cover the canvas in the first frame. A display corner is
        // avoided: a hot corner there opens Mission Control and covers the window.
        CGPoint corner = CGPointMake(bounds.origin.x + bounds.size.width / 2, bounds.origin.y + 10);
        CGEventRef park = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, corner, kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, park);
        CFRelease(park);
        if (dispatch_semaphore_wait(output.firstFrame, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)) != 0) {
            fprintf(stderr, "no frame captured\n");
            return 1;
        }

        NSString *meta = [NSString stringWithFormat:
            @"{\"window_pid\": %d, \"frame\": [%zu, %zu], \"points\": [%g, %g, %g, %g], \"scale\": %g, \"refresh\": %g",
            windowPid, pixelWidth, pixelHeight, bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height, scale, refresh];
        // Wait until the application shows a canvas of a usable size.
        const int minCanvas = 300;
        int rect[4] = {0, 0, 0, 0};
        bool found = false;
        for (int i = 0; i < 240 && !found; ++i) {
            @synchronized(output) { found = findCanvas(output.latest, rect) && rect[2] >= minCanvas && rect[3] >= minCanvas; }
            if (!found) usleep(500000);
        }
        @synchronized(output) { savePng(output.latest, [out stringByAppendingPathComponent:@"first-frame.png"]); }
        if (!found) { fprintf(stderr, "no white canvas of at least %d px visible\n", minCanvas); return 1; }
        NSString *canvas = [meta stringByAppendingFormat:@", \"canvas\": [%d, %d, %d, %d]}\n", rect[0], rect[1], rect[2], rect[3]];
        [canvas writeToFile:[out stringByAppendingPathComponent:@"canvas.json"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
        printf("%s", canvas.UTF8String);
        if (snapshot) {
            [stream stopCaptureWithCompletionHandler:^(NSError *e) { dispatch_semaphore_signal(got); }];
            dispatch_semaphore_wait(got, DISPATCH_TIME_FOREVER);
            return 0;
        }

        CGPoint canvasCenter = CGPointMake(bounds.origin.x + (rect[0] + rect[2] / 2) / scale, bounds.origin.y + (rect[1] + rect[3] / 2) / scale);
        if (!frontWindowAtPointBelongsTo(windowPid, canvasCenter)) {
            fprintf(stderr, "the window of process %d is not in front at the canvas; no input was posted (%s)\n", windowPid, frontWindowDescriptionAtPoint(canvasCenter).UTF8String);
            [stream stopCaptureWithCompletionHandler:^(NSError *e) { dispatch_semaphore_signal(got); }];
            dispatch_semaphore_wait(got, DISPATCH_TIME_FOREVER);
            return 1;
        }

        std::vector<Band> bands;
        std::vector<Sample> samples = readPlan(planPath, bands, rect);
        @synchronized(output) { output.bands = bands; output.detecting = YES; }
        NSMutableString *injected = [NSMutableString stringWithString:@"time_ns,phase,band,x,y,pressed\n"];
        int pressed = 0;
        uint64_t start = nowNs() + 300000000ull;
        for (const Sample &s : samples) {
            uint64_t target = start + (uint64_t)(s.t * 1e9);
            while (true) {
                int64_t remaining = (int64_t)(target - nowNs());
                if (remaining <= 0) break;
                if (remaining > 2000000) usleep((useconds_t)((remaining - 1500000) / 1000));
            }
            CGPoint point = CGPointMake(bounds.origin.x + s.x / scale, bounds.origin.y + s.y / scale);
            if ((&s - &samples[0]) % 100 == 0 && !frontWindowAtPointBelongsTo(windowPid, point)) {
                if (pressed) {
                    CGEventRef release = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, point, kCGMouseButtonLeft);
                    CGEventPost(kCGHIDEventTap, release);
                    CFRelease(release);
                }
                fprintf(stderr, "the window of process %d lost the front; input stopped (%s)\n", windowPid, frontWindowDescriptionAtPoint(point).UTF8String);
                return 1;
            }
            CGEventType type;
            if (s.pressed && !pressed) type = kCGEventLeftMouseDown;
            else if (!s.pressed && pressed) type = kCGEventLeftMouseUp;
            else type = s.pressed ? kCGEventLeftMouseDragged : kCGEventMouseMoved;
            uint64_t t = nowNs();
            CGEventRef event = CGEventCreateMouseEvent(NULL, type, point, kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
            pressed = s.pressed;
            [injected appendFormat:@"%llu,%s,%s,%.2f,%.2f,%d\n", t, s.phase.c_str(), s.band.c_str(), s.x, s.y, s.pressed];
        }
        usleep(500000);
        [stream stopCaptureWithCompletionHandler:^(NSError *e) { dispatch_semaphore_signal(got); }];
        dispatch_semaphore_wait(got, DISPATCH_TIME_FOREVER);
        @synchronized(output) {
            [output.rows writeToFile:[out stringByAppendingPathComponent:@"capture.csv"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
        }
        [injected writeToFile:[out stringByAppendingPathComponent:@"inject.csv"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
        meta = [meta stringByAppendingString:@"}\n"];
        [meta writeToFile:[out stringByAppendingPathComponent:@"capture-meta.json"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
        printf("samples=%zu\n", samples.size());
    }
    return 0;
}
