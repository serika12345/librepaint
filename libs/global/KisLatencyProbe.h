/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef KIS_LATENCY_PROBE_H
#define KIS_LATENCY_PROBE_H

#include "kritaglobal_export.h"
#include <QtGlobal>

class QCoreApplication;

/**
 * Timestamped markers for measuring the time from pen input to display.
 *
 * The probe is enabled when the LIBREPAINT_LATENCY_LOG environment variable
 * names an output file at the first call. Otherwise every function returns
 * after one check and nothing is recorded.
 *
 * The output is CSV: a "# frequency=<ticks per second>" line, the header
 * "time,event,x,y,extra,thread", then one row per marker. Times use
 * QueryPerformanceCounter on Windows, the clock that PresentMon reports with
 * --qpc_time, and std::chrono::steady_clock nanoseconds elsewhere. A
 * background thread writes the rows every 500 ms and when the process exits
 * normally.
 *
 * scripts/perf/pen-latency/analyze.py consumes the file. The events are:
 * native_pointer (Windows WM_POINTER message; extra = message << 32 | pointer
 * flags), native_wintab, native_mouse, qt_tablet_move, qt_mouse_move,
 * move_compressed, compressed_dispatch, outline_request, stroke_begin,
 * stroke_paint, stroke_dab (the stroke job has painted its dab into the paint
 * device), projection_update, canvas_update_dispatch, canvas_busy_defer,
 * canvas_update_issued, outline_state, canvas_outline_doc,
 * canvas_outline_wdg, canvas_paint_event, canvas_initialize_gl,
 * canvas_resize_gl, paintgl_begin, paintgl_end and frame_swapped.
 *
 * The canvas update path records where an update stopped before a paint.
 * outline_state (one per outline request) uses extra bit 1 = the tool supports
 * an outline, bit 2 = a paint op preset exists, bit 4 = the brush outline rect
 * is not empty, bit 8 = the previous outline rect is still set and bit 16 = a
 * color preview rect exists. canvas_outline_doc carries the widget rect after
 * clipping in x/y and extra bit 1 = the document rect was empty.
 * canvas_outline_wdg carries the canvas widget size in x/y and extra bit 1 =
 * the clipped rect is empty, bit 2 = the widget is visible.
 * canvas_update_issued carries the canvas widget size and extra bit 1 = the
 * combined update rect was empty. canvas_paint_event carries the paint event
 * rect and extra bit 1 = the widget is visible, bit 2 = it is minimized,
 * bit 4 = the previous updateRect was still set. canvas_initialize_gl (extra =
 * 1 when a context was current) and canvas_resize_gl (x/y = the GL size) show
 * whether the canvas was recreated or resized during a run.
 *
 * The renderer choice and its first frame check are recorded because
 * selectSurfaceConfig() runs before the usage logger is initialized, so a
 * measurement launch has no other place for them. l1_renderer_selection
 * carries x = the chosen renderer id, y = the number of candidates and
 * extra bit 1 = the choice was explicit; l1_renderer_skipped carries x = a
 * renderer id skipped by a failure mark; l1_first_frame_check carries x =
 * the checked renderer id and y = 1 when the check passed.
 *
 * The L1 experiments add last_frame_completed_on_gpu (extra = 1 when the
 * previous frame finished on the GPU), qt_update_deferred (Qt's frame
 * compressor deferred a paint), qt_update_deferred_skipped (H6 (frame update
 * throttle) painted despite the previous frame), gpu_wait_begin and
 * gpu_wait_end (H2 (waiting for the display handoff); extra = 1 when the
 * fence completed within the timeout) and l1_h2_state (H2's
 * decision for one paint call: extra bit 1 = the switch is on, bit 2 = the
 * previous frame's fence exists, bit 4 = the fence had already completed,
 * bit 8 = the fence was waited for, bit 16 = update compression deferred the
 * paint). The H2 wait runs before the update-compression early return, so a
 * deferred paint records its decision instead of skipping the wait.
 *
 * H1 (frames queued ahead) records whether its D3D11 call took effect:
 * l1_h1_egl_context (extra = 1 when Qt exposes the EGL context),
 * l1_h1_client_extensions and l1_h1_display_extensions (extra = bit 1 for
 * EGL_EXT_device_query, bit 2 for EGL_ANGLE_device_d3d),
 * l1_h1_device_query_ext (the display bits, kept for older analyses),
 * l1_h1_entry_points (bit 1 for eglQueryDisplayAttribEXT, bit 2 for
 * eglQueryDeviceAttribEXT), l1_h1_display_attrib (extra = EGLBoolean,
 * x = 1 when a device was returned), l1_h1_device_attrib (extra =
 * EGLBoolean, x = 1 when a D3D11 device was returned), l1_h1_device (extra =
 * 1 when the ANGLE D3D11 device was found), l1_h1_query_interface (extra =
 * HRESULT for IDXGIDevice1), l1_h1_set_max_frame_latency (x = requested
 * latency, extra = HRESULT) and l1_h1_get_max_frame_latency (x = requested
 * latency, extra = read-back latency, -1 when the read failed). The raw client
 * and display extension lists are written once to probe.egl.txt beside the
 * probe log.
 *
 * The context and GPU object markers count driver work per frame:
 * gl_make_current (extra bit 1 = the target context was already current,
 * bit 2 = a canvas context lock, bit 4 = restoring the previous context)
 * and gl_done_current (bit 1 = a doneCurrent call with no previous context).
 * gpu_texture_create/destroy (glGenTextures/glDeleteTextures),
 * gpu_texture_image (glTexImage2D), gpu_texture_reuse (a full-tile update
 * reused the existing storage), gpu_buffer_create/destroy (QOpenGLBuffer
 * create/destroy) and gpu_buffer_allocate (QOpenGLBuffer allocate or
 * glBufferData), gpu_fbo_recreate (QOpenGLWidget recreated its framebuffer
 * object), gpu_sync_create/destroy (KisOpenGLSync's glFenceSync/glDeleteSync),
 * color_selector_update and color_selector_paint. The analyzer adds one
 * QOpenGLWidget paint activation per paintgl_begin and one composition
 * activation per canvas_about_to_compose, because those two calls happen
 * inside Qt and cannot be marked from the application.
 *
 * The focus and tool markers separate a pen press that reached the tool from
 * one that was dropped before it (2026-11-11: official Qt Windows builds
 * stopped repainting after the first press in 2 of 8 runs). qt_event records
 * FocusIn (x = 1), FocusOut (2), TabletPress (3), TabletRelease (4),
 * MouseButtonPress (5), MouseButtonRelease (6), Enter (7) and Leave (8);
 * y = the active tool mode (-1 without a tool) and extra bit 1 = the pointer
 * is inside the canvas, bit 2 = mouse events are blocked, bit 4 = the
 * shortcut matcher has seen the pointer enter, bit 8 = all actions are
 * suppressed, bit 16 = a popup widget is registered. A TabletPress records
 * one row when it is seen and, when it survives the popup check, a second
 * row just before the shortcut matcher is called with extra bit 32 set; a
 * press without that second row never reached the matcher. outline_gate
 * records one outline request: x = the view zoom, y = the effective physical
 * zoom, extra bit 1 = the tool
 * supports an outline, bit 2 = the outline is enabled, bit 4 = it is visible,
 * bit 8 = the configured outline style is visible, bit 16 = hover mode,
 * bit 32 = paint mode, bit 64 = the outline is shown while painting,
 * bit 128 = gesture mode. tool_action records a tool action becoming active
 * (extra = 1) or inactive (0); x = the KisTool::ToolAction value and y = 1
 * when a tool was active.
 */
namespace KisLatencyProbe
{
KRITAGLOBAL_EXPORT bool enabled();
/// Records @p event, which must be a string literal or otherwise outlive the process.
KRITAGLOBAL_EXPORT void mark(const char *event, double x = 0.0, double y = 0.0, qint64 extra = 0);
/// Records Windows pointer, Wintab and mouse messages when the probe is enabled.
KRITAGLOBAL_EXPORT void installNativeFilter(QCoreApplication *app);
}

#endif
