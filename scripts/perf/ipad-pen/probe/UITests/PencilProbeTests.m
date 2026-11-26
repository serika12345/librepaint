// SPDX-License-Identifier: GPL-3.0-or-later
//
// UI tests that send synthesized Apple Pencil strokes on the iPad for the pen
// latency measurement (run-ipad.sh).
//
// XCTest synthesizes only finger touches. The strokes are built with Recap,
// the private event stream framework of the system, from its pencil
// digitizer sender, and handed to testmanagerd through the runner's private
// daemon proxy (_XCT_playBackHIDEventRecordingFromData:reply:). The public
// wrappers of that call (XCUIDevice and XCUIElement) refuse it on devices
// without Apple's internal security policies; testmanagerd itself plays the
// stream.
//
// testmanagerd plays the stream through a unit frame and then divides the
// locations by the panel's size in its native portrait orientation, while
// Recap writes locations as fractions of that size. stretchedForPlayback:
// multiplies the locations by the size first. Locations here are therefore
// native portrait points; testCalibrate maps them to window points.
//
// Pressure is set per sample on the stylus child event (field 0xB000A) on
// the device's scale of 0 to 700; UIKit reports force = pressure / 120.
#import <XCTest/XCTest.h>
#import <objc/message.h>
#import <objc/runtime.h>
#include <dlfcn.h>

typedef struct __IOHIDEvent *IOHIDEventRef;
// IOKit's HID event accessors, looked up at run time so the test bundle links
// only XCTest.
static void (*IOHIDEventSetFloatValue)(IOHIDEventRef, uint32_t, double);
static int64_t (*IOHIDEventGetIntegerValue)(IOHIDEventRef, uint32_t);
static CFArrayRef (*IOHIDEventGetChildren)(IOHIDEventRef);
enum { kFieldTouch = 0xB0009, kFieldPressure = 0xB000A };

static void loadFrameworks(void)
{
    void *iokit = dlopen("/System/Library/Frameworks/IOKit.framework/IOKit", RTLD_NOW);
    IOHIDEventSetFloatValue = dlsym(iokit, "IOHIDEventSetFloatValue");
    IOHIDEventGetIntegerValue = dlsym(iokit, "IOHIDEventGetIntegerValue");
    IOHIDEventGetChildren = dlsym(iokit, "IOHIDEventGetChildren");
    dlopen("/System/Library/PrivateFrameworks/Recap.framework/Recap", RTLD_NOW);
}

@interface PencilProbeTests : XCTestCase
@property XCUIApplication *app;
@end

@implementation PencilProbeTests

- (void)setUp
{
    self.continueAfterFailure = NO;
    loadFrameworks();
    XCTAssertNotEqual(IOHIDEventSetFloatValue, NULL);
    XCTAssertNotNil(NSClassFromString(@"RCPSyntheticEventStream"));
    // testPlayPlan draws into the measured application, not this one.
    if ([self.name containsString:@"testPlayPlan"]) return;
    self.app = [XCUIApplication new];
    [self.app launch];
    XCTAssertTrue([self.app.staticTexts[@"status"] waitForExistenceWithTimeout:10]);
}

- (NSString *)status
{
    XCUIElement *label = self.app.staticTexts[@"status"];
    return [NSString stringWithFormat:@"%@ | %@", label.label, label.value];
}

- (id)daemonProxy
{
    id session = [NSClassFromString(@"XCTRunnerDaemonSession") performSelector:@selector(sharedSession)];
    return [session performSelector:@selector(daemonProxy)];
}

- (id)pencilSender
{
    return [NSClassFromString(@"RCPEventSenderProperties") performSelector:@selector(pencilDigitizerSender)];
}

- (CGSize)nativeSize
{
    id environment = [NSClassFromString(@"RCPEventEnvironment") performSelector:@selector(currentEnvironment)];
    id screen = [[environment performSelector:@selector(screens)] firstObject];
    return ((CGSize(*)(id, SEL))objc_msgSend)(screen, @selector(pointSize));
}

- (NSData *)stretchedForPlayback:(id)stream
{
    const CGSize native = [self nativeSize];
    const CGAffineTransform stretch = {native.width, 0, 0, native.height, 0, 0};
    ((void(*)(id, SEL, CGAffineTransform))objc_msgSend)(stream, @selector(transformDigitizerEventLocationsWithTransform:), stretch);
    return [stream performSelector:@selector(dataRepresentation)];
}

- (void)play:(id)stream
{
    XCTestExpectation *played = [self expectationWithDescription:@"playback"];
    ((void(*)(id, SEL, id, id))objc_msgSend)([self daemonProxy], NSSelectorFromString(@"_XCT_playBackHIDEventRecordingFromData:reply:"),
        [self stretchedForPlayback:stream], ^(NSError *error) {
            if (error) NSLog(@"PencilProbe playback error %@", error);
            [played fulfill];
        });
    [self waitForExpectations:@[played] timeout:120];
}

// Taps three native points into the probe application and logs where its
// window receives them: ipad_measure.py calibration reads the CAL lines.
- (void)testCalibrate
{
    const CGSize size = self.app.frame.size;
    NSLog(@"PencilProbe app frame %gx%g orientation %ld", size.width, size.height, (long)[XCUIDevice sharedDevice].orientation);
    const CGSize native = [self nativeSize];
    const double points[3][2] = {{0.25, 0.25}, {0.75, 0.25}, {0.25, 0.75}};
    id pencil = [self pencilSender];
    for (int i = 0; i < 3; i++) {
        const CGPoint point = CGPointMake(native.width * points[i][0], native.height * points[i][1]);
        id stream = ((id(*)(id, SEL, id))objc_msgSend)(NSClassFromString(@"RCPSyntheticEventStream"), @selector(eventStreamWithEventActions:), ^(id composer) {
            ((void(*)(id, SEL, id, id))objc_msgSend)(composer, @selector(composeWithSender:actionBlock:), pencil, ^(id p) {
                ((void(*)(id, SEL, CGPoint, CGPoint, double, double))objc_msgSend)(p, @selector(dragWithStartPoint:endPoint:duration:pressure:), point, point, 0.1, 0.5);
            });
        });
        [self play:stream];
        [NSThread sleepForTimeInterval:1];
        NSLog(@"PencilProbe CAL point %g,%g -> %@", point.x, point.y, [self status]);
    }
}

// Plays the strokes of tmp/plan.json (ipad_measure.py plan) into the
// application named there, one playback per stroke:
// {"bundle": "...", "rate": 240, "gap": 1.0, "pressure": 350,
//  "strokes": [{"from": [x, y], "to": [x, y], "seconds": s, "dwell": d}]}
// Points are native portrait points. Each stroke touches down, rests for
// dwell, moves at constant speed for seconds, rests again and lifts.
- (void)testPlayPlan
{
    NSData *json = [NSData dataWithContentsOfFile:[NSTemporaryDirectory() stringByAppendingPathComponent:@"plan.json"]];
    XCTAssertNotNil(json);
    NSDictionary *plan = [NSJSONSerialization JSONObjectWithData:json options:0 error:NULL];
    XCUIApplication *target = [[XCUIApplication alloc] initWithBundleIdentifier:plan[@"bundle"]];
    [target activate];
    [NSThread sleepForTimeInterval:2];
    const long rate = [plan[@"rate"] longValue];
    const double pressure = [plan[@"pressure"] doubleValue], gap = [plan[@"gap"] doubleValue];
    id pencil = [self pencilSender];
    int index = 0;
    for (NSDictionary *stroke in plan[@"strokes"]) {
        const CGPoint a = CGPointMake([stroke[@"from"][0] doubleValue], [stroke[@"from"][1] doubleValue]);
        const CGPoint b = CGPointMake([stroke[@"to"][0] doubleValue], [stroke[@"to"][1] doubleValue]);
        const double seconds = [stroke[@"seconds"] doubleValue], dwell = [stroke[@"dwell"] doubleValue];
        id stream = ((id(*)(id, SEL, id))objc_msgSend)(NSClassFromString(@"RCPSyntheticEventStream"), @selector(eventStreamWithEventActions:), ^(id composer) {
            ((void(*)(id, SEL, id, id))objc_msgSend)(composer, @selector(composeWithSender:actionBlock:), pencil, ^(id p) {
                ((void(*)(id, SEL, long))objc_msgSend)(p, @selector(setTouchFrequency:), rate);
                CGPoint start[1] = {a};
                ((void(*)(id, SEL, CGPoint *, unsigned long))objc_msgSend)(p, @selector(touchDownAtPoints:touchCount:), start, 1);
                ((void(*)(id, SEL, CGPoint, double))objc_msgSend)(p, @selector(moveToPoint:duration:), a, dwell);
                ((void(*)(id, SEL, CGPoint, double))objc_msgSend)(p, @selector(moveToPoint:duration:), b, seconds);
                ((void(*)(id, SEL, CGPoint, double))objc_msgSend)(p, @selector(moveToPoint:duration:), b, dwell);
                ((void(*)(id, SEL))objc_msgSend)(p, @selector(liftUpAtAllActivePoints));
            });
        });
        NSArray *events = [stream performSelector:@selector(events)];
        int touching = 0;
        for (id event in events) {
            IOHIDEventRef hid = (IOHIDEventRef)((void *(*)(id, SEL))objc_msgSend)(event, @selector(hidEvent));
            CFArrayRef children = IOHIDEventGetChildren(hid);
            if (!children || !CFArrayGetCount(children)) continue;
            IOHIDEventRef stylus = (IOHIDEventRef)CFArrayGetValueAtIndex(children, 0);
            if (IOHIDEventGetIntegerValue(stylus, kFieldTouch)) {
                IOHIDEventSetFloatValue(stylus, kFieldPressure, pressure);
                touching++;
            }
        }
        [self play:stream];
        NSLog(@"PencilProbe PLAN stroke %d events %lu touching %d", index++, (unsigned long)events.count, touching);
        [NSThread sleepForTimeInterval:gap];
    }
}

@end
