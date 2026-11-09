// SPDX-License-Identifier: GPL-3.0-or-later
//
// LD_PRELOAD library that records buffer swaps and swap-interval requests of
// any OpenGL application on Linux, so that applications can be compared
// without source changes. With GL_SWAP_TRACE naming an output file it writes:
//   swap,<api>,<begin_ns>,<end_ns>,<thread>,<drawable>
//   interval,<api>,<value>,<time_ns>,<thread>,<drawable>
//   make_current,<api>,<time_ns>,<thread>,<drawable>   (when the drawable changes)
// <drawable> is the GLX drawable or EGL surface; for glXSwapIntervalMESA and
// glXSwapIntervalSGI, which act on the current drawable, it is the drawable
// most recently made current on the thread.
// Times are CLOCK_MONOTONIC nanoseconds, the clock of mutter_capture.py and
// of the LibrePaint latency probe on Linux.
//
// Functions fetched through glXGetProcAddress(ARB) and eglGetProcAddress are
// wrapped as well, since Qt resolves swap-interval entry points that way.
//
// Build: gcc -O2 -shared -fPIC gl_swap_trace.c -ldl -o gl_swap_trace.so
// Use:   GL_SWAP_TRACE=/path/trace.csv LD_PRELOAD=/path/gl_swap_trace.so app

#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

typedef void *Display;
typedef unsigned long GLXDrawable;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef int32_t EGLint;
typedef unsigned int EGLBoolean;
typedef void (*Proc)(void);

static FILE *out;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static __thread unsigned long current_drawable;

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static long tid(void) { return syscall(SYS_gettid); }

static void emit(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void emit(const char *fmt, ...)
{
    pthread_mutex_lock(&lock);
    if (!out) {
        const char *path = getenv("GL_SWAP_TRACE");
        if (path) {
            out = fopen(path, "a");
            if (out) setvbuf(out, NULL, _IOLBF, 0);
        }
    }
    if (out) {
        __builtin_va_list ap;
        __builtin_va_start(ap, fmt);
        vfprintf(out, fmt, ap);
        __builtin_va_end(ap);
    }
    pthread_mutex_unlock(&lock);
}

static void *next(const char *name)
{
    void *f = dlsym(RTLD_NEXT, name);
    if (!f) fprintf(stderr, "gl_swap_trace: %s not found\n", name);
    return f;
}

// GLX
static void (*real_glXSwapBuffers)(Display *, GLXDrawable);
void glXSwapBuffers(Display *d, GLXDrawable w)
{
    if (!real_glXSwapBuffers) real_glXSwapBuffers = next("glXSwapBuffers");
    uint64_t b = now_ns();
    real_glXSwapBuffers(d, w);
    emit("swap,glx,%llu,%llu,%ld,%lx\n", (unsigned long long)b, (unsigned long long)now_ns(), tid(), w);
}

static void (*real_glXSwapIntervalEXT)(Display *, GLXDrawable, int);
static void wrap_glXSwapIntervalEXT(Display *d, GLXDrawable w, int v)
{
    emit("interval,glx_ext,%d,%llu,%ld,%lx\n", v, (unsigned long long)now_ns(), tid(), w);
    real_glXSwapIntervalEXT(d, w, v);
}
static int (*real_glXSwapIntervalMESA)(unsigned int);
static int wrap_glXSwapIntervalMESA(unsigned int v)
{
    emit("interval,glx_mesa,%u,%llu,%ld,%lx\n", v, (unsigned long long)now_ns(), tid(), current_drawable);
    return real_glXSwapIntervalMESA(v);
}
static int (*real_glXSwapIntervalSGI)(int);
static int wrap_glXSwapIntervalSGI(int v)
{
    emit("interval,glx_sgi,%d,%llu,%ld,%lx\n", v, (unsigned long long)now_ns(), tid(), current_drawable);
    return real_glXSwapIntervalSGI(v);
}

static Proc wrap_glx_proc(const char *name, Proc f)
{
    if (!f || !name) return f;
    if (!strcmp(name, "glXSwapIntervalEXT")) { real_glXSwapIntervalEXT = (void *)f; return (Proc)wrap_glXSwapIntervalEXT; }
    if (!strcmp(name, "glXSwapIntervalMESA")) { real_glXSwapIntervalMESA = (void *)f; return (Proc)wrap_glXSwapIntervalMESA; }
    if (!strcmp(name, "glXSwapIntervalSGI")) { real_glXSwapIntervalSGI = (void *)f; return (Proc)wrap_glXSwapIntervalSGI; }
    if (!strcmp(name, "glXSwapBuffers")) { real_glXSwapBuffers = (void *)f; return (Proc)glXSwapBuffers; }
    return f;
}

static Proc (*real_glXGetProcAddressARB)(const unsigned char *);
Proc glXGetProcAddressARB(const unsigned char *name)
{
    if (!real_glXGetProcAddressARB) real_glXGetProcAddressARB = next("glXGetProcAddressARB");
    return wrap_glx_proc((const char *)name, real_glXGetProcAddressARB(name));
}
static Proc (*real_glXGetProcAddress)(const unsigned char *);
Proc glXGetProcAddress(const unsigned char *name)
{
    if (!real_glXGetProcAddress) real_glXGetProcAddress = next("glXGetProcAddress");
    return wrap_glx_proc((const char *)name, real_glXGetProcAddress(name));
}

// EGL
static EGLBoolean (*real_eglSwapBuffers)(EGLDisplay, EGLSurface);
EGLBoolean eglSwapBuffers(EGLDisplay d, EGLSurface s)
{
    if (!real_eglSwapBuffers) real_eglSwapBuffers = next("eglSwapBuffers");
    uint64_t b = now_ns();
    EGLBoolean r = real_eglSwapBuffers(d, s);
    emit("swap,egl,%llu,%llu,%ld,%lx\n", (unsigned long long)b, (unsigned long long)now_ns(), tid(), (unsigned long)s);
    return r;
}
static EGLBoolean (*real_eglSwapBuffersWithDamage)(EGLDisplay, EGLSurface, const EGLint *, EGLint);
static EGLBoolean wrap_eglSwapBuffersWithDamage(EGLDisplay d, EGLSurface s, const EGLint *r, EGLint n)
{
    uint64_t b = now_ns();
    EGLBoolean v = real_eglSwapBuffersWithDamage(d, s, r, n);
    emit("swap,egl_damage,%llu,%llu,%ld,%lx\n", (unsigned long long)b, (unsigned long long)now_ns(), tid(), (unsigned long)s);
    return v;
}
static EGLBoolean (*real_eglSwapInterval)(EGLDisplay, EGLint);
EGLBoolean eglSwapInterval(EGLDisplay d, EGLint v)
{
    if (!real_eglSwapInterval) real_eglSwapInterval = next("eglSwapInterval");
    emit("interval,egl,%d,%llu,%ld,%lx\n", v, (unsigned long long)now_ns(), tid(), current_drawable);
    return real_eglSwapInterval(d, v);
}
static Proc (*real_eglGetProcAddress)(const char *);
Proc eglGetProcAddress(const char *name)
{
    if (!real_eglGetProcAddress) real_eglGetProcAddress = next("eglGetProcAddress");
    Proc f = real_eglGetProcAddress(name);
    if (!f || !name) return f;
    if (!strcmp(name, "eglSwapBuffersWithDamageKHR") || !strcmp(name, "eglSwapBuffersWithDamageEXT")) {
        real_eglSwapBuffersWithDamage = (void *)f;
        return (Proc)wrap_eglSwapBuffersWithDamage;
    }
    if (!strcmp(name, "eglSwapBuffers")) { real_eglSwapBuffers = (void *)f; return (Proc)eglSwapBuffers; }
    if (!strcmp(name, "eglSwapInterval")) { real_eglSwapInterval = (void *)f; return (Proc)eglSwapInterval; }
    return f;
}

// Context binding, to see which API and thread renders.
static int (*real_glXMakeCurrent)(Display *, GLXDrawable, void *);
int glXMakeCurrent(Display *d, GLXDrawable w, void *c)
{
    if (!real_glXMakeCurrent) real_glXMakeCurrent = next("glXMakeCurrent");
    if (w != current_drawable) { current_drawable = w; emit("make_current,glx,%llu,%ld,%lx\n", (unsigned long long)now_ns(), tid(), w); }
    return real_glXMakeCurrent(d, w, c);
}
static EGLBoolean (*real_eglMakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, void *);
EGLBoolean eglMakeCurrent(EGLDisplay d, EGLSurface draw, EGLSurface read, void *c)
{
    if (!real_eglMakeCurrent) real_eglMakeCurrent = next("eglMakeCurrent");
    if ((unsigned long)draw != current_drawable) { current_drawable = (unsigned long)draw; emit("make_current,egl,%llu,%ld,%lx\n", (unsigned long long)now_ns(), tid(), current_drawable); }
    return real_eglMakeCurrent(d, draw, read, c);
}
