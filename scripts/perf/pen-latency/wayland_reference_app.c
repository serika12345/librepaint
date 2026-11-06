// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wayland pointer drawing window with presentation feedback: the reference
// that checks the capture-based measurement against the time the compositor
// actually showed each frame (wp_presentation, from the kernel page flip).
//
// Like reference_app.py, it draws a black 21 px square at the hover position
// and a 20 px line while the button is held, on white, and commits a new
// frame on every pointer event. With REFERENCE_LOG set it writes:
//   input,<time_ns>,<x>,<y>,<pressed>       for every pointer event
//   commit,<time_ns>,<frame>,<input_ns>       for every committed frame
//   presented,<frame>,<time_ns>,<refresh_ns>,<flags>
//   discarded,<frame>
// Times are in the presentation clock (CLOCK_MONOTONIC on mutter).
//
// Build (wayland-scanner generates the protocol code):
//   nix-shell -p wayland wayland-protocols wayland-scanner pkg-config gcc --run '
//     P=$(pkg-config --variable=pkgdatadir wayland-protocols)
//     for x in stable/xdg-shell/xdg-shell stable/presentation-time/presentation-time; do
//       n=$(basename $x)
//       wayland-scanner client-header $P/$x.xml $n-client-protocol.h
//       wayland-scanner private-code $P/$x.xml $n-protocol.c
//     done
//     gcc -O2 -I. wayland_reference_app.c xdg-shell-protocol.c presentation-time-protocol.c \
//       $(pkg-config --cflags --libs wayland-client) -lm -o wayland_reference_app'
//
// Measured with run-linux.sh, the run directory receives reference.log;
// presentation_check.py compares it with the capture measurement.

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>

#include "presentation-time-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#define BUFFERS 4

struct buffer {
    struct wl_buffer *wl;
    uint32_t *pixels;
    int busy;
};

static struct {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *wm;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wp_presentation *presentation;
    clockid_t clock;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *toplevel;
    int width, height, configured, running;
    struct buffer buffers[BUFFERS];
    uint32_t *ink; // persistent strokes
    double x, y, last_x, last_y;
    int pressed, have_last, dirty;
    uint64_t input_ns;
    uint32_t frame;
    FILE *log;
} app = {.width = 1200, .height = 700, .running = 1, .clock = CLOCK_MONOTONIC};

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(app.clock, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

static void fill_disc(uint32_t *pixels, double cx, double cy, double r)
{
    int x0 = (int)(cx - r), x1 = (int)(cx + r + 1), y0 = (int)(cy - r), y1 = (int)(cy + r + 1);
    for (int y = y0; y <= y1; ++y) {
        if (y < 0 || y >= app.height) continue;
        for (int x = x0; x <= x1; ++x) {
            if (x < 0 || x >= app.width) continue;
            double dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= r * r) pixels[y * app.width + x] = 0xff000000;
        }
    }
}

static void stroke_to(double x, double y)
{
    double dx = x - app.last_x, dy = y - app.last_y;
    int steps = (int)(sqrt(dx * dx + dy * dy)) + 1;
    for (int i = 0; i <= steps; ++i)
        fill_disc(app.ink, app.last_x + dx * i / steps, app.last_y + dy * i / steps, 10);
    app.last_x = x;
    app.last_y = y;
}

static void buffer_release(void *data, struct wl_buffer *wl) { ((struct buffer *)data)->busy = 0; }
static const struct wl_buffer_listener buffer_listener = {buffer_release};

static void create_buffers(void)
{
    int stride = app.width * 4, size = stride * app.height;
    int fd = memfd_create("reference", MFD_CLOEXEC);
    if (fd < 0 || ftruncate(fd, (off_t)size * BUFFERS) < 0) { perror("memfd"); exit(1); }
    uint8_t *data = mmap(NULL, (size_t)size * BUFFERS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct wl_shm_pool *pool = wl_shm_create_pool(app.shm, fd, size * BUFFERS);
    for (int i = 0; i < BUFFERS; ++i) {
        app.buffers[i].pixels = (uint32_t *)(data + (size_t)size * i);
        app.buffers[i].wl = wl_shm_pool_create_buffer(pool, size * i, app.width, app.height, stride, WL_SHM_FORMAT_XRGB8888);
        wl_buffer_add_listener(app.buffers[i].wl, &buffer_listener, &app.buffers[i]);
    }
    wl_shm_pool_destroy(pool);
    close(fd);
    app.ink = malloc((size_t)size);
    for (int i = 0; i < app.width * app.height; ++i) app.ink[i] = 0xffffffff;
}

static void feedback_sync(void *data, struct wp_presentation_feedback *f, struct wl_output *o) {}
static void feedback_presented(void *data, struct wp_presentation_feedback *f, uint32_t sec_hi, uint32_t sec_lo,
                               uint32_t nsec, uint32_t refresh, uint32_t seq_hi, uint32_t seq_lo, uint32_t flags)
{
    uint64_t t = (((uint64_t)sec_hi << 32) | sec_lo) * 1000000000ull + nsec;
    if (app.log) fprintf(app.log, "presented,%u,%llu,%u,%u\n", (uint32_t)(uintptr_t)data, (unsigned long long)t, refresh, flags);
    wp_presentation_feedback_destroy(f);
}
static void feedback_discarded(void *data, struct wp_presentation_feedback *f)
{
    if (app.log) fprintf(app.log, "discarded,%u\n", (uint32_t)(uintptr_t)data);
    wp_presentation_feedback_destroy(f);
}
static const struct wp_presentation_feedback_listener feedback_listener = {feedback_sync, feedback_presented, feedback_discarded};

static void draw(void)
{
    if (!app.configured || !app.dirty) return;
    struct buffer *b = NULL;
    for (int i = 0; i < BUFFERS; ++i)
        if (!app.buffers[i].busy) { b = &app.buffers[i]; break; }
    if (!b) return; // drawn again when a buffer is released
    memcpy(b->pixels, app.ink, (size_t)app.width * app.height * 4);
    if (!app.pressed) {
        for (int y = (int)app.y - 10; y <= (int)app.y + 10; ++y)
            for (int x = (int)app.x - 10; x <= (int)app.x + 10; ++x)
                if (x >= 0 && y >= 0 && x < app.width && y < app.height) b->pixels[y * app.width + x] = 0xff000000;
    }
    b->busy = 1;
    app.dirty = 0;
    uint32_t frame = ++app.frame;
    wl_surface_attach(app.surface, b->wl, 0, 0);
    wl_surface_damage_buffer(app.surface, 0, 0, app.width, app.height);
    if (app.presentation) {
        struct wp_presentation_feedback *f = wp_presentation_feedback(app.presentation, app.surface);
        wp_presentation_feedback_add_listener(f, &feedback_listener, (void *)(uintptr_t)frame);
    }
    wl_surface_commit(app.surface);
    if (app.log) fprintf(app.log, "commit,%llu,%u,%llu\n", (unsigned long long)now_ns(), frame, (unsigned long long)app.input_ns);
}

static void pointer_enter(void *d, struct wl_pointer *p, uint32_t serial, struct wl_surface *s, wl_fixed_t x, wl_fixed_t y)
{
    wl_pointer_set_cursor(p, serial, NULL, 0, 0); // hide the cursor
    app.x = wl_fixed_to_double(x);
    app.y = wl_fixed_to_double(y);
}
static void pointer_leave(void *d, struct wl_pointer *p, uint32_t serial, struct wl_surface *s) {}
static void pointer_motion(void *d, struct wl_pointer *p, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
    app.input_ns = now_ns();
    app.x = wl_fixed_to_double(x);
    app.y = wl_fixed_to_double(y);
    if (app.pressed) stroke_to(app.x, app.y);
    if (app.log) fprintf(app.log, "input,%llu,%.2f,%.2f,%d\n", (unsigned long long)app.input_ns, app.x, app.y, app.pressed);
    app.dirty = 1;
    draw();
}
static void pointer_button(void *d, struct wl_pointer *p, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    if (button != BTN_LEFT) return;
    app.input_ns = now_ns();
    app.pressed = state == WL_POINTER_BUTTON_STATE_PRESSED;
    if (app.pressed) {
        app.last_x = app.x;
        app.last_y = app.y;
        stroke_to(app.x, app.y);
    }
    app.dirty = 1;
    draw();
}
static void pointer_axis(void *d, struct wl_pointer *p, uint32_t t, uint32_t a, wl_fixed_t v) {}
static void pointer_frame(void *d, struct wl_pointer *p) {}
static void pointer_axis_source(void *d, struct wl_pointer *p, uint32_t s) {}
static void pointer_axis_stop(void *d, struct wl_pointer *p, uint32_t t, uint32_t a) {}
static void pointer_axis_discrete(void *d, struct wl_pointer *p, uint32_t a, int32_t v) {}
static const struct wl_pointer_listener pointer_listener = {pointer_enter, pointer_leave, pointer_motion, pointer_button,
                                                            pointer_axis, pointer_frame, pointer_axis_source,
                                                            pointer_axis_stop, pointer_axis_discrete};

static void seat_capabilities(void *d, struct wl_seat *seat, uint32_t caps)
{
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !app.pointer) {
        app.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(app.pointer, &pointer_listener, NULL);
    }
}
static void seat_name(void *d, struct wl_seat *s, const char *n) {}
static const struct wl_seat_listener seat_listener = {seat_capabilities, seat_name};

static void presentation_clock(void *d, struct wp_presentation *p, uint32_t clock) { app.clock = (clockid_t)clock; }
static const struct wp_presentation_listener presentation_listener = {presentation_clock};

static void wm_ping(void *d, struct xdg_wm_base *wm, uint32_t serial) { xdg_wm_base_pong(wm, serial); }
static const struct xdg_wm_base_listener wm_listener = {wm_ping};

static void xdg_surface_configure(void *d, struct xdg_surface *s, uint32_t serial)
{
    xdg_surface_ack_configure(s, serial);
    if (!app.configured) {
        app.configured = 1;
        app.dirty = 1;
        draw();
    }
}
static const struct xdg_surface_listener xdg_surface_listener = {xdg_surface_configure};
static void toplevel_configure(void *d, struct xdg_toplevel *t, int32_t w, int32_t h, struct wl_array *states) {}
static void toplevel_close(void *d, struct xdg_toplevel *t) { app.running = 0; }
static const struct xdg_toplevel_listener toplevel_listener = {toplevel_configure, toplevel_close};

static void registry_global(void *d, struct wl_registry *r, uint32_t name, const char *iface, uint32_t version)
{
    if (!strcmp(iface, wl_compositor_interface.name)) app.compositor = wl_registry_bind(r, name, &wl_compositor_interface, 4);
    else if (!strcmp(iface, wl_shm_interface.name)) app.shm = wl_registry_bind(r, name, &wl_shm_interface, 1);
    else if (!strcmp(iface, xdg_wm_base_interface.name)) {
        app.wm = wl_registry_bind(r, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(app.wm, &wm_listener, NULL);
    } else if (!strcmp(iface, wl_seat_interface.name)) {
        app.seat = wl_registry_bind(r, name, &wl_seat_interface, 5);
        wl_seat_add_listener(app.seat, &seat_listener, NULL);
    } else if (!strcmp(iface, wp_presentation_interface.name)) {
        app.presentation = wl_registry_bind(r, name, &wp_presentation_interface, 1);
        wp_presentation_add_listener(app.presentation, &presentation_listener, NULL);
    }
}
static void registry_remove(void *d, struct wl_registry *r, uint32_t name) {}
static const struct wl_registry_listener registry_listener = {registry_global, registry_remove};

int main(void)
{
    const char *log = getenv("REFERENCE_LOG");
    if (log) {
        app.log = fopen(log, "w");
        setvbuf(app.log, NULL, _IOLBF, 0);
    }
    app.display = wl_display_connect(NULL);
    if (!app.display) { fprintf(stderr, "no Wayland display\n"); return 1; }
    struct wl_registry *registry = wl_display_get_registry(app.display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(app.display);
    wl_display_roundtrip(app.display);
    if (!app.compositor || !app.shm || !app.wm) { fprintf(stderr, "missing Wayland globals\n"); return 1; }
    if (!app.presentation) fprintf(stderr, "wp_presentation unavailable\n");
    create_buffers();
    app.surface = wl_compositor_create_surface(app.compositor);
    app.xdg_surface = xdg_wm_base_get_xdg_surface(app.wm, app.surface);
    xdg_surface_add_listener(app.xdg_surface, &xdg_surface_listener, NULL);
    app.toplevel = xdg_surface_get_toplevel(app.xdg_surface);
    xdg_toplevel_add_listener(app.toplevel, &toplevel_listener, NULL);
    xdg_toplevel_set_title(app.toplevel, "pen-latency wayland reference");
    xdg_toplevel_set_min_size(app.toplevel, app.width, app.height);
    xdg_toplevel_set_max_size(app.toplevel, app.width, app.height);
    wl_surface_commit(app.surface);
    while (app.running && wl_display_dispatch(app.display) != -1) {
        if (app.dirty) draw();
    }
    return 0;
}
