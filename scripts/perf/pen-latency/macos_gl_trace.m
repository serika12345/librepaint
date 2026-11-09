// SPDX-License-Identifier: GPL-3.0-or-later
//
// DYLD_INSERT_LIBRARIES library that records how a macOS OpenGL application
// hands frames to the window server, the macOS counterpart of
// gl_swap_trace.c. With GL_SWAP_TRACE naming an output file it writes:
//   swap,<api>,<begin_ns>,<end_ns>,<thread>,<context>
//   interval,<api>,<value>,<time_ns>,<thread>,<context>
//   input,<time_ns>,<type>          (mouse move and drag events)
// for -[NSOpenGLContext flushBuffer], CGLFlushDrawable, the swap-interval
// parameter of -[NSOpenGLContext setValues:forParameter:] and
// CGLSetParameter. Times are mach_absolute_time in nanoseconds, the clock
// of macos_capture.mm.
//
// Hardened-runtime applications ignore DYLD_INSERT_LIBRARIES; measure an
// ad-hoc re-signed copy of such an application.
//
// Build: clang -fobjc-arc -dynamiclib -O2 macos_gl_trace.m -framework AppKit
//        -framework OpenGL -o macos_gl_trace.dylib

#define GL_SILENCE_DEPRECATION
#import <AppKit/AppKit.h>
#import <OpenGL/OpenGL.h>
#include <mach/mach_time.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdio.h>

static mach_timebase_info_data_t timebase;
static FILE *out;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t nowNs(void) { return mach_absolute_time() * timebase.numer / timebase.denom; }
static uint64_t tid(void) { uint64_t t; pthread_threadid_np(NULL, &t); return t; }

static void emit(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void emit(const char *fmt, ...)
{
    pthread_mutex_lock(&lock);
    if (out) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(out, fmt, ap);
        va_end(ap);
    }
    pthread_mutex_unlock(&lock);
}

// Mouse events as the application receives them.
static IMP originalSendEvent;
static void tracedSendEvent(id self, SEL cmd, NSEvent *event)
{
    NSEventType type = event.type;
    if (type == NSEventTypeMouseMoved || type == NSEventTypeLeftMouseDragged || type == NSEventTypeLeftMouseDown)
        emit("input,%llu,%lu\n", (unsigned long long)nowNs(), (unsigned long)type);
    ((void (*)(id, SEL, NSEvent *))originalSendEvent)(self, cmd, event);
}

// CGL entry points, replaced through the dyld interpose section.
static CGLError traced_CGLFlushDrawable(CGLContextObj ctx)
{
    uint64_t b = nowNs();
    CGLError r = CGLFlushDrawable(ctx);
    emit("swap,cgl,%llu,%llu,%llu,%p\n", (unsigned long long)b, (unsigned long long)nowNs(), (unsigned long long)tid(), ctx);
    return r;
}
static CGLError traced_CGLSetParameter(CGLContextObj ctx, CGLContextParameter p, const GLint *v)
{
    if (p == kCGLCPSwapInterval && v)
        emit("interval,cgl,%d,%llu,%llu,%p\n", *v, (unsigned long long)nowNs(), (unsigned long long)tid(), ctx);
    return CGLSetParameter(ctx, p, v);
}

__attribute__((used, section("__DATA,__interpose"))) static struct { const void *replacement, *original; } interposers[] = {
    {(const void *)traced_CGLFlushDrawable, (const void *)CGLFlushDrawable},
    {(const void *)traced_CGLSetParameter, (const void *)CGLSetParameter},
};

// NSOpenGLContext methods, swizzled at load time.
static IMP originalFlush, originalSetValues;
static void tracedFlush(id self, SEL cmd)
{
    uint64_t b = nowNs();
    ((void (*)(id, SEL))originalFlush)(self, cmd);
    emit("swap,nsgl,%llu,%llu,%llu,%p\n", (unsigned long long)b, (unsigned long long)nowNs(), (unsigned long long)tid(), (__bridge void *)self);
}
static void tracedSetValues(id self, SEL cmd, const GLint *values, NSOpenGLContextParameter parameter)
{
    if (parameter == NSOpenGLContextParameterSwapInterval && values)
        emit("interval,nsgl,%d,%llu,%llu,%p\n", *values, (unsigned long long)nowNs(), (unsigned long long)tid(), (__bridge void *)self);
    ((void (*)(id, SEL, const GLint *, NSOpenGLContextParameter))originalSetValues)(self, cmd, values, parameter);
}

__attribute__((constructor)) static void install(void)
{
    mach_timebase_info(&timebase);
    const char *path = getenv("GL_SWAP_TRACE");
    if (path) {
        out = fopen(path, "a");
        if (out) setvbuf(out, NULL, _IOLBF, 0);
    }
    Class cls = objc_getClass("NSOpenGLContext");
    Method flush = class_getInstanceMethod(cls, @selector(flushBuffer));
    Method setValues = class_getInstanceMethod(cls, @selector(setValues:forParameter:));
    originalFlush = method_setImplementation(flush, (IMP)tracedFlush);
    originalSetValues = method_setImplementation(setValues, (IMP)tracedSetValues);
    Method sendEvent = class_getInstanceMethod(objc_getClass("NSApplication"), @selector(sendEvent:));
    originalSendEvent = method_setImplementation(sendEvent, (IMP)tracedSendEvent);
}
