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
 * move_compressed, compressed_dispatch, outline_request, stroke_paint,
 * projection_update, canvas_update_dispatch, canvas_busy_defer,
 * paintgl_begin, paintgl_end and frame_swapped.
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
