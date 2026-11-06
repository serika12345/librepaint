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
