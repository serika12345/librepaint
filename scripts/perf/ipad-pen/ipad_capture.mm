// SPDX-License-Identifier: GPL-3.0-or-later
//
// Captures the screen of a USB-connected iPad on the Mac through CoreMediaIO
// (the iOS screen capture device) for the iPad pen latency measurement.
//
//   ipad_capture OUT_DIR SECONDS [BANDS_FILE]
//
// Writes into OUT_DIR:
//   frames.csv   frame_ns,arrival_ns,width,height,luma: every frame's
//                presentation time and arrival time (nanoseconds of the Mac's
//                uptime clock) and the mean luminance of the frame's centre,
//                which the flash calibration reads.
//   capture.csv  with BANDS_FILE: the ink of every band in every frame, in the
//                columns of macos_capture.mm, so capture_analyze.py reads it.
//   first-frame.pgm  the first frame as 8-bit grey, for finding the canvas.
//   capture.log  messages; ready appears when the first frame arrives, and
//                done when the files are written.
//
// The capture ends after SECONDS or when a file named stop appears in OUT_DIR.
//
// BANDS_FILE has one band per line: name x0 x1 y0 y1 in captured pixels. A
// pixel is dark when all its channels are below 128; a column of a band is
// dense when it has at least 6 dark pixels (macos_capture.mm's defaults).
//
// The tool runs inside IPadCapture.app (see build-capture-app.sh) because
// macOS asks for the camera permission per signed application, and the
// iOS screen capture device is a camera device.
#import <AVFoundation/AVFoundation.h>
#import <CoreMediaIO/CMIOHardware.h>
#include <mach/mach_time.h>
#include <string>
#include <vector>

static mach_timebase_info_data_t timebase;

struct Band { std::string name; int x0, x1, y0, y1; };

@interface Sink : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property(nonatomic) NSMutableString *frames;
@property(nonatomic) NSMutableString *ink;
@property(nonatomic) NSUInteger count;
@property(nonatomic) NSString *outDir;
@property(nonatomic) std::vector<Band> bands;
@end

@implementation Sink
- (void)captureOutput:(AVCaptureOutput *)output didOutputSampleBuffer:(CMSampleBufferRef)buffer fromConnection:(AVCaptureConnection *)connection
{
    const uint64_t arrival = mach_absolute_time() * timebase.numer / timebase.denom;
    CVPixelBufferRef image = CMSampleBufferGetImageBuffer(buffer);
    if (!image) return;
    const uint64_t frameNs = (uint64_t)llround(CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(buffer)) * 1e9);
    CVPixelBufferLockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    const int width = (int)CVPixelBufferGetWidth(image), height = (int)CVPixelBufferGetHeight(image);
    const size_t stride = CVPixelBufferGetBytesPerRow(image);
    const uint8_t *base = (const uint8_t *)CVPixelBufferGetBaseAddress(image);
    double sum = 0;
    int n = 0;
    for (int y = height / 2 - 50; y < height / 2 + 50; y += 2) {
        for (int x = width / 2 - 50; x < width / 2 + 50; x += 2) {
            const uint8_t *p = base + y * stride + x * 4;
            sum += 0.114 * p[0] + 0.587 * p[1] + 0.299 * p[2];
            n++;
        }
    }
    NSMutableString *inkRows = [NSMutableString string];
    for (const Band &b : self.bands) {
        const int x0 = MAX(0, b.x0), x1 = MIN(width - 1, b.x1), y0 = MAX(0, b.y0), y1 = MIN(height - 1, b.y1);
        std::vector<int> columns(MAX(0, x1 - x0 + 1), 0);
        int xmin = -1, xmax = -1, ymin = -1, ymax = -1;
        for (int y = y0; y <= y1; y++) {
            const uint8_t *row = base + y * stride;
            for (int x = x0; x <= x1; x++) {
                const uint8_t *p = row + x * 4;
                if (p[0] < 128 && p[1] < 128 && p[2] < 128) {
                    columns[x - x0]++;
                    if (xmin < 0 || x < xmin) xmin = x;
                    if (x > xmax) xmax = x;
                    if (ymin < 0) ymin = y;
                    ymax = y;
                }
            }
        }
        int dxmin = -1, dxmax = -1;
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i] >= 6) { if (dxmin < 0) dxmin = x0 + (int)i; dxmax = x0 + (int)i; }
        }
        int gap = 0;
        if (dxmin >= 0) {
            int run = 0;
            for (int i = dxmin - x0; i <= dxmax - x0; i++) {
                if (columns[i] < 6) { if (++run > gap) gap = run; } else { run = 0; }
            }
        }
        if (xmin < 0) {
            [inkRows appendFormat:@"%llu,%llu,%s,,,,,,,\n", frameNs, arrival, b.name.c_str()];
        } else {
            NSString *dense = dxmin < 0 ? @"," : [NSString stringWithFormat:@"%d,%d", dxmin, dxmax];
            NSString *gapText = dxmin < 0 ? @"" : [NSString stringWithFormat:@"%d", gap];
            [inkRows appendFormat:@"%llu,%llu,%s,%d,%d,%@,%d,%d,%@\n", frameNs, arrival, b.name.c_str(), xmin, xmax, dense, ymin, ymax, gapText];
        }
    }
    NSData *grey = nil;
    if (self.count == 0) {
        NSMutableData *pgm = [NSMutableData dataWithData:[[NSString stringWithFormat:@"P5\n%d %d\n255\n", width, height] dataUsingEncoding:NSASCIIStringEncoding]];
        std::vector<uint8_t> line(width);
        for (int y = 0; y < height; y++) {
            const uint8_t *row = base + y * stride;
            for (int x = 0; x < width; x++) {
                const uint8_t *p = row + x * 4;
                line[x] = (uint8_t)lround(0.114 * p[0] + 0.587 * p[1] + 0.299 * p[2]);
            }
            [pgm appendBytes:line.data() length:width];
        }
        grey = pgm;
    }
    CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    @synchronized (self) {
        [self.frames appendFormat:@"%llu,%llu,%d,%d,%.1f\n", frameNs, arrival, width, height, sum / n];
        [self.ink appendString:inkRows];
        if (self.count++ == 0) {
            [grey writeToFile:[self.outDir stringByAppendingPathComponent:@"first-frame.pgm"] atomically:YES];
            [@"" writeToFile:[self.outDir stringByAppendingPathComponent:@"ready"] atomically:YES encoding:NSUTF8StringEncoding error:NULL];
        }
    }
}
@end

static AVCaptureDevice *findDevice(void)
{
    AVCaptureDeviceDiscoverySession *session = [AVCaptureDeviceDiscoverySession
        discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeExternal] mediaType:nil position:AVCaptureDevicePositionUnspecified];
    for (AVCaptureDevice *device in session.devices) {
        if ([device.modelID containsString:@"iOS"]) return device;
    }
    return nil;
}

int main(int argc, char **argv)
{
    @autoreleasepool {
        if (argc < 3) return 2;
        mach_timebase_info(&timebase);
        NSString *out = @(argv[1]);
        const double seconds = atof(argv[2]);
        [[NSFileManager defaultManager] createDirectoryAtPath:out withIntermediateDirectories:YES attributes:nil error:NULL];
        freopen([[out stringByAppendingPathComponent:@"capture.log"] UTF8String], "w", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);

        Sink *sink = [Sink new];
        sink.outDir = out;
        sink.frames = [NSMutableString stringWithString:@"frame_ns,arrival_ns,width,height,luma\n"];
        sink.ink = [NSMutableString stringWithString:@"frame_ns,arrival_ns,band,xmin,xmax,dense_xmin,dense_xmax,ymin,ymax,dense_gap\n"];
        if (argc > 3) {
            std::vector<Band> bands;
            for (NSString *line in [[NSString stringWithContentsOfFile:@(argv[3]) encoding:NSUTF8StringEncoding error:NULL] componentsSeparatedByString:@"\n"]) {
                NSArray<NSString *> *f = [line componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
                if (f.count != 5) continue;
                bands.push_back({f[0].UTF8String, f[1].intValue, f[2].intValue, f[3].intValue, f[4].intValue});
            }
            sink.bands = bands;
        }

        dispatch_semaphore_t answered = dispatch_semaphore_create(0);
        __block BOOL granted = NO;
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL value) { granted = value; dispatch_semaphore_signal(answered); }];
        while (dispatch_semaphore_wait(answered, DISPATCH_TIME_NOW)) [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        if (!granted) { NSLog(@"camera access denied"); return 3; }

        // The iOS screen capture device appears only after this property is set.
        CMIOObjectPropertyAddress property = {kCMIOHardwarePropertyAllowScreenCaptureDevices, kCMIOObjectPropertyScopeGlobal, kCMIOObjectPropertyElementMain};
        UInt32 allow = 1;
        CMIOObjectSetPropertyData(kCMIOObjectSystemObject, &property, 0, NULL, sizeof(allow), &allow);
        AVCaptureDevice *device = nil;
        for (int i = 0; i < 100 && !(device = findDevice()); i++) [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        if (!device) { NSLog(@"no iPad screen device"); return 4; }
        // Starting the stream right after the device appears fails with '!dev'.
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:2]];

        AVCaptureSession *session = [AVCaptureSession new];
        [session addInput:[AVCaptureDeviceInput deviceInputWithDevice:device error:NULL]];
        AVCaptureVideoDataOutput *output = [AVCaptureVideoDataOutput new];
        output.videoSettings = @{(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)};
        output.alwaysDiscardsLateVideoFrames = NO;
        [output setSampleBufferDelegate:sink queue:dispatch_queue_create("capture", DISPATCH_QUEUE_SERIAL)];
        [session addOutput:output];
        [[NSNotificationCenter defaultCenter] addObserverForName:AVCaptureSessionRuntimeErrorNotification object:session queue:nil usingBlock:^(NSNotification *note) {
            NSLog(@"capture error %@", note.userInfo[AVCaptureSessionErrorKey]);
        }];
        [session startRunning];
        NSDate *end = [NSDate dateWithTimeIntervalSinceNow:seconds];
        NSString *stopPath = [out stringByAppendingPathComponent:@"stop"];
        while ([end timeIntervalSinceNow] > 0 && ![[NSFileManager defaultManager] fileExistsAtPath:stopPath]) {
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.2]];
        }
        [session stopRunning];
        @synchronized (sink) {
            [sink.frames writeToFile:[out stringByAppendingPathComponent:@"frames.csv"] atomically:YES encoding:NSUTF8StringEncoding error:NULL];
            if (!sink.bands.empty()) [sink.ink writeToFile:[out stringByAppendingPathComponent:@"capture.csv"] atomically:YES encoding:NSUTF8StringEncoding error:NULL];
            NSLog(@"%lu frames", (unsigned long)sink.count);
            [@"" writeToFile:[out stringByAppendingPathComponent:@"done"] atomically:YES encoding:NSUTF8StringEncoding error:NULL];
        }
        return sink.count ? 0 : 5;
    }
}
