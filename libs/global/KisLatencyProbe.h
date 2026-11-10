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
 * gpu_wait_end (H2 (waiting for the display handoff)).
 *
 * H1 (frames queued ahead) records whether its D3D11 call took effect:
 * l1_h1_egl_context (extra = 1 when Qt exposes the EGL context),
 * l1_h1_device (extra = 1 when the ANGLE D3D11 device was found),
 * l1_h1_query_interface (extra = HRESULT for IDXGIDevice1),
 * l1_h1_set_max_frame_latency (x = requested latency, extra = HRESULT) and
 * l1_h1_get_max_frame_latency (x = requested latency, extra = read-back
 * latency, -1 when the read failed).
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
