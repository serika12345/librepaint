#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize pen input-to-display latency from one or more measurement runs.

A run directory contains:

- ``presentmon.csv`` (optional): PresentMon 2.x output recorded with ``--qpc_time``.
- ``probe.csv`` (optional): LibrePaint markers written when
  ``LIBREPAINT_LATENCY_LOG`` names the file (``libs/global/KisLatencyProbe``).
- ``inject.csv`` (optional): synthetic pen samples written by ``PenInject.cs``
  or by the capture tools (``windows_capture.cpp``).

All three files use the Windows QueryPerformanceCounter clock. For every
displayed frame the script finds the newest input that the frame can reflect
and reports the time from the OS pointer message to the display, with the
intermediate intervals. Runs without ``probe.csv`` (applications without
markers) report only the presentation intervals.

Usage: analyze.py [--json OUT] RUN_DIR [RUN_DIR ...]
"""

import argparse
import bisect
import collections
import csv
import json
import math
import os
import statistics
import sys

POINTER_FLAG_INCONTACT = 0x4
WM_POINTERUP = 0x0247
MAX_LINK_MS = 200.0
MAX_PAINT_TO_PRESENT_MS = 100.0
MAX_STROKE_QUEUE_MS = 500.0

# Share of the synthetic pen activity that presentmon.csv must cover. A
# PresentMon session can stop early (2026-11-10: 1.2 s of a 33 s run), and
# the remaining rows then describe only the start of the run. Those runs are
# reported as partial and left out of the display breakdown instead of
# looking like a latency change.
PRESENTMON_MIN_COVERAGE = 0.95

# Metrics whose run medians are compared across repeated runs.
PRIMARY_METRICS = ("hover.input_to_display", "contact.ink_input_to_display")


def read_header_frequency(lines):
    for line in lines:
        if line.startswith("# frequency="):
            return int(line.split("=", 1)[1])
    return None


def load_marked_csv(path):
    with open(path, encoding="utf-8-sig") as handle:
        lines = handle.read().splitlines()
    frequency = read_header_frequency(lines)
    rows = list(csv.DictReader(line for line in lines if not line.startswith("#")))
    return frequency, rows


def load_probe(path):
    frequency, rows = load_marked_csv(path)
    events = collections.defaultdict(list)
    for row in rows:
        events[row["event"]].append((int(row["time"]), float(row["x"]), float(row["y"]), int(row["extra"])))
    for series in events.values():
        series.sort()
    return frequency, events


POINTER_FLAG_INRANGE_CONTACT = 0x6  # INRANGE | INCONTACT


def load_inject(path, probe_frequency=None):
    """Read PenInject.cs samples (qpc ticks) or capture-tool samples (time_ns)."""
    frequency, rows = load_marked_csv(path)
    samples = []
    for row in rows:
        if "qpc" in row:
            samples.append({"time": int(row["qpc"]), "phase": row["phase"],
                            "flags": int(row["flags"]), "ok": row["ok"] == "1",
                            "x": float(row.get("x") or 0.0), "y": float(row.get("y") or 0.0)})
        else:
            if probe_frequency is None:
                raise SystemExit("%s: time_ns samples need the probe clock frequency" % path)
            samples.append({"time": int(row["time_ns"]) * probe_frequency // 1_000_000_000, "phase": row["phase"],
                            "flags": POINTER_FLAG_INRANGE_CONTACT if row["pressed"] == "1" else 0x2,
                            "ok": True, "x": float(row["x"]), "y": float(row["y"])})
            frequency = probe_frequency
    return frequency, samples


def load_presents(path):
    """Return (present_qpc, display_qpc_or_None, row) sorted by present time."""
    presents = []
    with open(path, encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            try:
                present = int(row["TimeInQPC"])
            except (KeyError, TypeError, ValueError):
                continue
            presents.append((present, row))
    presents.sort(key=lambda item: item[0])
    return presents


def measurement_window(inject, events):
    """Time span of the synthetic pen activity in probe-clock ticks.

    inject.csv is the trajectory that was sent to the application; without
    it, the probe's own input markers give the span. PresentMon only writes
    rows for frames that were presented, so a static screen before the
    activity is not a gap.
    """
    if inject:
        times = [sample["time"] for sample in inject if sample["ok"]]
        if len(times) >= 2:
            return min(times), max(times)
    if events:
        for name in ("native_pointer", "qt_tablet_move", "outline_request", "paintgl_begin"):
            times = [entry[0] for entry in events.get(name, [])]
            if len(times) >= 2:
                return times[0], times[-1]
    return None


def presentmon_state(path, presents, window):
    """Report whether presentmon.csv covers the measured activity.

    "none" means no file. "partial" means the file exists but has no usable
    rows, or its TimeInQPC span covers less than PRESENTMON_MIN_COVERAGE of
    the measurement window. Only "present" runs belong in the display
    breakdown.
    """
    if not os.path.exists(path):
        return {"present": False, "status": "none", "coverage": None, "n": 0}
    if not presents:
        return {"present": True, "status": "partial", "coverage": 0.0, "n": 0}
    coverage = None
    if window and window[1] > window[0]:
        overlap = min(presents[-1][0], window[1]) - max(presents[0][0], window[0])
        coverage = max(0.0, overlap / (window[1] - window[0]))
    status = "present" if coverage is None or coverage >= PRESENTMON_MIN_COVERAGE else "partial"
    return {"present": True, "status": status, "coverage": coverage, "n": len(presents)}


def presentmon_is_usable(result):
    """Whether a run may be used for the PresentMon display breakdown.

    Older summaries carry only "present"; treat them as usable when their
    file existed. Results without a "presentmon" entry carry no information
    and stay in the comparison.
    """
    if "presentmon" not in result:
        return True
    state = result.get("presentmon") or {}
    status = state.get("status")
    if status is None:
        status = "present" if state.get("present") else "none"
    return status == "present"


def probe_frequency(directory):
    """Clock frequency written in probe.csv, or None without the file."""
    path = os.path.join(directory, "probe.csv")
    if not os.path.exists(path):
        return None
    frequency, _ = load_marked_csv(path)
    return frequency


def l1_max_frame_latency(events):
    """Last outcome of the H1 (frames queued ahead) D3D11 call, if recorded.

    The probe events carry the requested latency in x and the HRESULT or the
    read-back value in extra. The last event of each kind wins: the canvas can
    initialize more than once, and the newest call describes the run.
    """
    egl = events.get("l1_h1_egl_context")
    if not egl:
        return None
    client_extension = events.get("l1_h1_client_extensions")
    display_extension = events.get("l1_h1_display_extensions")
    extension = events.get("l1_h1_device_query_ext")
    entry_points = events.get("l1_h1_entry_points")
    display_attrib = events.get("l1_h1_display_attrib")
    device_attrib = events.get("l1_h1_device_attrib")
    device = events.get("l1_h1_device")
    query = events.get("l1_h1_query_interface")
    setter = events.get("l1_h1_set_max_frame_latency")
    getter = events.get("l1_h1_get_max_frame_latency")
    return {
        "egl_context": int(egl[-1][3]),
        "client_extensions": int(client_extension[-1][3]) if client_extension else None,
        "display_extensions": int(display_extension[-1][3]) if display_extension else None,
        "device_query_ext": int(extension[-1][3]) if extension else None,
        "entry_points": int(entry_points[-1][3]) if entry_points else None,
        "display_attrib_result": display_attrib[-1][3] if display_attrib else None,
        "display_attrib_device": int(display_attrib[-1][1]) if display_attrib else None,
        "device_attrib_result": device_attrib[-1][3] if device_attrib else None,
        "device_attrib_device": int(device_attrib[-1][1]) if device_attrib else None,
        "device": int(device[-1][3]) if device else None,
        "query_interface_hresult": query[-1][3] if query else None,
        "set_hresult": setter[-1][3] if setter else None,
        "requested": int(setter[-1][1]) if setter else None,
        "readback": getter[-1][3] if getter else None,
    }


def percentile(values, fraction):
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int(fraction * len(ordered)))]


def capture_gap_metrics(directory):
    """Pen tip to line end metrics from capture.csv and inject.csv.

    The capture analyzer owns the mapping between a displayed frame and the
    injected pointer. Importing it here keeps one implementation of the
    distance (docs/perf/l2p-prediction.md) and lets analyze.py report the same
    value for runs that also hold the capture columns.
    """
    if not (os.path.exists(os.path.join(directory, "capture.csv"))
            and os.path.exists(os.path.join(directory, "inject.csv"))):
        return {}
    import capture_analyze
    return capture_analyze.display_gap_metrics(directory)


def describe(values):
    if not values:
        return {"n": 0}
    return {
        "n": len(values),
        "p05": percentile(values, 0.05),
        "p50": statistics.median(values),
        "p95": percentile(values, 0.95),
    }


def speed_clusters(entries):
    """Group (speed_px_ms, payload) into clusters of similar pointer speed.

    Speeds that differ by less than a fifth belong together; a stroke that
    changes speed therefore gives one cluster per speed instead of one value
    per sample. Clusters are returned in increasing speed order.
    """
    clusters = []
    for speed, payload in sorted(entries, key=lambda entry: entry[0]):
        if not clusters or speed > clusters[-1]["speeds"][-1] * 1.2:
            clusters.append({"speeds": [], "payloads": []})
        clusters[-1]["speeds"].append(speed)
        clusters[-1]["payloads"].append(payload)
    return [{"speed": statistics.median(cluster["speeds"]), "payloads": cluster["payloads"]}
            for cluster in clusters]


def last_before(times, limit):
    index = bisect.bisect_right(times, limit) - 1
    return times[index] if index >= 0 else None


def float_or_none(text):
    try:
        return float(text)
    except (TypeError, ValueError):
        return None


PAINT_INTERVAL_BINS = (("<4", 4.0), ("<8", 8.0), ("<12", 12.0), ("<16", 16.0))
PHASE_BINS = 10


def interval_histogram(values, bins=PAINT_INTERVAL_BINS):
    """Count intervals into the bands that separate the paint cadences.

    The official Qt Windows runs paint both shortly after an input burst and
    about one display period later, so a median alone hides the two groups.
    """
    counts = collections.OrderedDict((label, 0) for label, _ in bins)
    counts[">=" + bins[-1][0][1:]] = 0
    for value in values:
        for label, limit in bins:
            if value < limit:
                counts[label] += 1
                break
        else:
            counts[">=" + bins[-1][0][1:]] += 1
    return dict(counts)


def display_change_times(presents, ticks_per_ms):
    """Times of the display changes that PresentMon reported.

    PresentMon gives the present time and the wait until the display change,
    so display = TimeInQPC + MsUntilDisplayed. Rows without a display change
    are left out; they carry no phase information.
    """
    times = []
    for present, row in presents:
        until_displayed = float_or_none(row.get("MsUntilDisplayed"))
        if until_displayed is not None:
            times.append(present + until_displayed * ticks_per_ms)
    times.sort()
    return times


def summarize_paint_cycle(presents, events, ticks_per_ms):
    """Paint cadence, presentation cadence and the paint/display phase.

    The L1 Windows measurements need to separate the fast and slow ink states.
    These values name the difference: how often paintGL ran between two
    display changes, how the paint interval is distributed, and where in the
    display period the paint started.
    """
    paints = [entry[0] for entry in events.get("paintgl_begin", [])]
    presents_sorted = [present for present, _ in presents]
    displays = display_change_times(presents, ticks_per_ms)

    window = None
    if displays:
        window = (displays[0], displays[-1])

    def inside(times):
        if window is None:
            return list(times)
        return [time for time in times if window[0] <= time <= window[1]]

    paint_times = inside(paints)
    present_times = inside(presents_sorted)
    paint_intervals = [
        (b - a) / ticks_per_ms for a, b in zip(paint_times, paint_times[1:])
        if (b - a) / ticks_per_ms < MAX_LINK_MS
    ]
    present_intervals = [
        (b - a) / ticks_per_ms for a, b in zip(present_times, present_times[1:])
        if (b - a) / ticks_per_ms < MAX_LINK_MS
    ]

    period = None
    intervals = [
        (b - a) / ticks_per_ms for a, b in zip(displays, displays[1:])
        if (b - a) / ticks_per_ms < MAX_LINK_MS
    ]
    if intervals:
        period = statistics.median(intervals)

    result = {
        "paint_interval": describe(paint_intervals),
        "paint_interval_bins": interval_histogram(paint_intervals),
        "present_interval": describe(present_intervals),
        "paint": len(paint_times),
        "present": len(present_times),
        "display": len(displays),
        "display_period_ms": period,
        "paint_per_display": (len(paint_times) / len(displays)) if displays else None,
        "present_per_display": (len(present_times) / len(displays)) if displays else None,
    }

    modes = collections.Counter(
        row.get("PresentMode") for _, row in presents if row.get("PresentMode"))
    if modes:
        result["present_modes"] = dict(sorted(modes.items()))

    until_displayed = [
        value for value in (float_or_none(row.get("MsUntilDisplayed")) for _, row in presents)
        if value is not None
    ]
    if until_displayed:
        result["ms_until_displayed"] = describe(until_displayed)

    if period and period > 0:
        phases = []
        for time in paints:
            index = bisect.bisect_right(displays, time) - 1
            if index < 0:
                continue
            elapsed = (time - displays[index]) / ticks_per_ms
            if 0.0 <= elapsed < period:
                phases.append(elapsed / period)
        if phases:
            angles = [phase * 2.0 * math.pi for phase in phases]
            x = sum(math.cos(angle) for angle in angles) / len(angles)
            y = sum(math.sin(angle) for angle in angles) / len(angles)
            result["phase"] = describe(phases)
            result["phase"]["concentration"] = math.hypot(x, y)
            result["phase"]["mean"] = (math.atan2(y, x) / (2.0 * math.pi)) % 1.0
            hist = collections.Counter(min(PHASE_BINS - 1, int(phase * PHASE_BINS)) for phase in phases)
            result["phase"]["bins"] = [hist.get(index, 0) for index in range(PHASE_BINS)]

    return result


class Run:
    def __init__(self, directory):
        self.directory = directory
        self.name = os.path.basename(os.path.normpath(directory))
        probe_path = os.path.join(directory, "probe.csv")
        inject_path = os.path.join(directory, "inject.csv")
        self.frequency = None
        self.events = None
        self.inject = None
        self.inject_times = []
        if os.path.exists(probe_path):
            self.frequency, self.events = load_probe(probe_path)
        if os.path.exists(inject_path):
            inject_frequency, self.inject = load_inject(inject_path, self.frequency)
            self.frequency = self.frequency or inject_frequency
            self.inject_times = [sample["time"] for sample in self.inject]
        if self.frequency is None:
            raise SystemExit("%s: probe.csv or inject.csv with a '# frequency=' header is required" % directory)
        presentmon_path = os.path.join(directory, "presentmon.csv")
        self.presentmon_present = os.path.exists(presentmon_path)
        self.presents = load_presents(presentmon_path) if self.presentmon_present else []
        self.ticks_per_ms = self.frequency / 1000.0
        self.stroke_queue_by_speed = None
        self.stroke_queue_counts = None

    def ms(self, ticks):
        return ticks / self.ticks_per_ms

    def times(self, event):
        return [entry[0] for entry in self.events.get(event, [])]

    def contact_lookup(self):
        """Return a function telling whether the pen touched the surface at a time."""
        if self.events and self.events.get("native_pointer"):
            native = self.events["native_pointer"]
            times = [entry[0] for entry in native]
            # extra = (message << 32) | HIWORD(wParam) pointer flags.
            states = [bool(entry[3] & POINTER_FLAG_INCONTACT) and (entry[3] >> 32) != WM_POINTERUP for entry in native]
        elif self.inject:
            times = [sample["time"] for sample in self.inject]
            states = [bool(sample["flags"] & POINTER_FLAG_INCONTACT) for sample in self.inject]
        else:
            return lambda time: None

        def in_contact(time):
            index = bisect.bisect_right(times, time) - 1
            return index >= 0 and states[index]

        return in_contact

    def inject_speed(self, time):
        """Pointer speed in px/ms near a probe time, from injected samples."""
        if not self.inject or len(self.inject) < 2:
            return None
        times = self.inject_times
        index = bisect.bisect_right(times, time)
        index = min(max(index, 1), len(self.inject) - 1)
        first, second = self.inject[index - 1], self.inject[index]
        duration = self.ms(second["time"] - first["time"])
        if duration <= 0:
            return None
        return math.hypot(second["x"] - first["x"], second["y"] - first["y"]) / duration

    def stroke_queue(self, stats):
        """Time from queueing a stroke job to painting its dab.

        stroke_paint records the point entering the stroke queue;
        stroke_dab records the same job after the paint device has drawn it.
        The two series are paired in order because both run first in, first
        out. The delay is grouped by the pointer speed at the queueing time.
        """
        paints = self.events.get("stroke_paint", [])
        dabs = self.events.get("stroke_dab", [])
        if not paints or not dabs:
            return
        self.stroke_queue_counts = {"paint": len(paints), "dab": len(dabs), "paired": 0}
        durations = []
        speed_entries = []
        index = 0
        for paint in paints:
            while index < len(dabs) and dabs[index][0] < paint[0]:
                index += 1
            if index >= len(dabs):
                break
            duration = self.ms(dabs[index][0] - paint[0])
            index += 1
            if not 0.0 <= duration <= MAX_STROKE_QUEUE_MS:
                continue
            durations.append(duration)
            speed = self.inject_speed(paint[0])
            if speed is not None:
                speed_entries.append((speed, duration))
        if durations:
            stats["contact.stroke_paint_to_dab"] = durations
            self.stroke_queue_counts["paired"] = len(durations)
        if speed_entries:
            self.stroke_queue_by_speed = [
                {
                    "speed_px_ms": cluster["speed"],
                    "n": len(cluster["payloads"]),
                    "p50": statistics.median(cluster["payloads"]),
                    "p95": percentile(cluster["payloads"], 0.95),
                    "max": max(cluster["payloads"]),
                }
                for cluster in speed_clusters(speed_entries)
            ]

    def analyze(self):
        stats = collections.defaultdict(list)
        in_contact = self.contact_lookup()
        link = MAX_LINK_MS * self.ticks_per_ms

        displayed = 0
        for present, row in self.presents:
            until_displayed = float_or_none(row.get("MsUntilDisplayed"))
            mode = in_contact(present)
            mode_name = "contact" if mode else ("hover" if mode is not None else "all")
            if until_displayed is not None:
                displayed += 1
                stats[mode_name + ".present_to_display"].append(until_displayed)
            cpu_start = float_or_none(row.get("CPUStartQPC"))
            gpu_latency = float_or_none(row.get("MsGPULatency"))
            if cpu_start is not None and gpu_latency is not None:
                stats[mode_name + ".present_to_gpu_start"].append(self.ms(cpu_start - present) + gpu_latency)
            gpu_busy = float_or_none(row.get("MsGPUBusy"))
            if gpu_busy is not None:
                stats[mode_name + ".gpu_busy"].append(gpu_busy)
            gpu_time = float_or_none(row.get("MsGPUTime"))
            if gpu_time is not None:
                stats[mode_name + ".gpu_time"].append(gpu_time)

        present_times = [present for present, _ in self.presents]
        stats["all.present_interval"] = [
            self.ms(b - a) for a, b in zip(present_times, present_times[1:]) if self.ms(b - a) < MAX_LINK_MS
        ]

        if self.inject:
            injected = [sample["time"] for sample in self.inject if sample["ok"]]
            if self.events and self.events.get("native_pointer"):
                for native_time in self.times("native_pointer"):
                    source = last_before(injected, native_time)
                    if source is not None and native_time - source < link:
                        stats["all.inject_to_native"].append(self.ms(native_time - source))

        if self.events:
            self.analyze_probe(stats, in_contact, link)

        result = {key: describe(values) for key, values in sorted(stats.items())}
        result["presents"] = {"n": len(self.presents), "displayed": displayed}
        result["presentmon"] = presentmon_state(
            os.path.join(self.directory, "presentmon.csv"), self.presents,
            measurement_window(self.inject, self.events))
        if self.events:
            result["paint_cycle"] = summarize_paint_cycle(
                self.presents, self.events, self.ticks_per_ms)
            swap = summarize_swap(self.events, self.ticks_per_ms)
            if swap:
                result["canvas_swap"] = swap
            render_path = summarize_render_path(self.events)
            if render_path:
                result["render_path"] = render_path
            renderer_selection = summarize_renderer_selection(self.events)
            if renderer_selection:
                result["renderer_selection"] = renderer_selection
            result["input_path"] = {
                event: len(self.events.get(event, []))
                for event in ("native_pointer", "native_wintab", "native_mouse", "qt_tablet_move")
            }
            result["canvas_path"] = summarize_canvas_path(self.events)
            result["focus_gate"] = summarize_focus_gate(self.events)
            result["l1_events"] = {
                event: len(self.events.get(event, []))
                for event in ("last_frame_completed_on_gpu", "qt_update_deferred",
                              "qt_update_deferred_skipped", "canvas_busy_defer",
                              "gpu_wait_begin", "gpu_wait_end")
            }
            completed = self.events.get("last_frame_completed_on_gpu", [])
            if completed:
                result["l1_last_frame_completed"] = {
                    "n": len(completed),
                    "completed": sum(1 for entry in completed if entry[3] == 1),
                }
            h2_states = self.events.get("l1_h2_state", [])
            if h2_states:
                wait_end = self.events.get("gpu_wait_end", [])
                result["l1_h2"] = {
                    "n": len(h2_states),
                    "fence": sum(1 for entry in h2_states if int(entry[3]) & 2),
                    "complete": sum(1 for entry in h2_states if int(entry[3]) & 4),
                    "waited": sum(1 for entry in h2_states if int(entry[3]) & 8),
                    "deferred": sum(1 for entry in h2_states if int(entry[3]) & 16),
                    "wait_completed": sum(1 for entry in wait_end if int(entry[3]) == 1),
                }
            h1 = l1_max_frame_latency(self.events)
            if h1:
                result["l1_h1"] = h1
            if self.stroke_queue_by_speed:
                result["contact.stroke_paint_to_dab_speed"] = self.stroke_queue_by_speed
            if self.stroke_queue_counts:
                result["stroke_queue"] = self.stroke_queue_counts
        if self.inject:
            result["inject"] = {
                "n": len(self.inject),
                "failed": sum(1 for sample in self.inject if not sample["ok"]),
            }
        result.update(capture_gap_metrics(self.directory))
        if os.path.exists(os.path.join(self.directory, "threads.csv")):
            import thread_cpu
            result["thread_cpu"] = thread_cpu.summarize(self.directory)
        return result

    def analyze_probe(self, stats, in_contact, link):
        native = self.times("native_pointer")
        outline = self.times("outline_request")
        paint_begin = self.times("paintgl_begin")
        paint_end = self.times("paintgl_end")
        projection = self.times("projection_update")
        stroke = self.times("stroke_paint")
        paint_to_present = MAX_PAINT_TO_PRESENT_MS * self.ticks_per_ms

        for present, row in self.presents:
            until_displayed = float_or_none(row.get("MsUntilDisplayed"))
            if until_displayed is None:
                continue
            display = present + until_displayed * self.ticks_per_ms
            begin = last_before(paint_begin, present)
            if begin is None or present - begin > paint_to_present:
                continue
            mode = "contact" if in_contact(begin) else "hover"
            request = last_before(outline, begin)
            if request is None or begin - request > link:
                continue
            source = last_before(native, request)
            if source is None or request - source > link:
                continue
            end_index = bisect.bisect_left(paint_end, begin)
            end = paint_end[end_index] if end_index < len(paint_end) else None

            prefix = mode + "."
            stats[prefix + "input_to_display"].append(self.ms(display - source))
            stats[prefix + "native_to_outline_request"].append(self.ms(request - source))
            stats[prefix + "outline_request_to_paint_begin"].append(self.ms(begin - request))
            if end is not None and end <= present:
                stats[prefix + "paint_duration"].append(self.ms(end - begin))
                stats[prefix + "paint_end_to_present"].append(self.ms(present - end))
            stats[prefix + "matched_present_to_display"].append(until_displayed)

            if mode == "contact":
                update = last_before(projection, begin)
                if update is None or begin - update > link:
                    continue
                job = last_before(stroke, update)
                if job is None:
                    continue
                job_source = last_before(native, job)
                if job_source is not None and job - job_source < link:
                    stats["contact.ink_input_to_display"].append(self.ms(display - job_source))

        for event in ("native_pointer", "qt_tablet_move", "compressed_dispatch", "outline_request", "paintgl_begin"):
            times = self.times(event)
            gaps = [self.ms(b - a) for a, b in zip(times, times[1:]) if self.ms(b - a) < MAX_LINK_MS]
            stats["rate." + event + "_interval"] = gaps

        # H2 (waiting for the display handoff): pair each recorded wait with
        # the end marker that follows it, so the analysis shows how long the
        # GUI thread actually blocked on the previous frame.
        wait_begin = self.times("gpu_wait_begin")
        wait_end = self.times("gpu_wait_end")
        if wait_begin:
            durations = []
            for begin in wait_begin:
                index = bisect.bisect_left(wait_end, begin)
                if index < len(wait_end):
                    durations.append(self.ms(wait_end[index] - begin))
            stats["gpu_wait.duration"] = durations

        self.stroke_queue(stats)


def compare(results, metrics=PRIMARY_METRICS, require_presentmon=True):
    """Spread of run medians: (max - min) / median of the run medians.

    The breakdown needs PresentMon; a run without presentmon.csv, or with a
    partial recording, is left out so that a missing or short log does not
    look like a latency change.
    """
    summary = {}
    for metric in metrics:
        medians = [
            result[metric]["p50"] for result in results.values()
            if result.get(metric, {}).get("n")
            and not (require_presentmon and not presentmon_is_usable(result))
        ]
        if len(medians) < 2:
            continue
        center = statistics.median(medians)
        summary[metric] = {
            "runs": len(medians),
            "median_of_medians": center,
            "min": min(medians),
            "max": max(medians),
            "spread": (max(medians) - min(medians)) / center if center else None,
        }
    return summary


def format_stat(stat):
    if not stat.get("n"):
        return "n=0"
    return "p05=%6.1f p50=%6.1f p95=%6.1f n=%d" % (stat["p05"], stat["p50"], stat["p95"], stat["n"])


def format_l2p_speed_groups(groups):
    """Pen tip to line end distance per pointer speed from the capture."""
    if not groups:
        return "none"
    return " ; ".join(
        "%.2f px/ms n=%d gap_px=%s gap_ms=%s residual=%s overshoot=%s" % (
            group["speed_px_ms"], group["n"],
            format_stat(group.get("l2p_signed_gap_px", {})),
            format_stat(group.get("l2p_signed_gap_ms", {})),
            format_stat(group.get("l2p_residual_px", {})),
            format_stat(group.get("l2p_overshoot_px", {})))
        for group in groups)


def format_l2p_speed_changes(changes):
    """Distance just after each speed change from the capture."""
    if not changes:
        return "none"
    return " ; ".join(
        "at %.0fms %.2f->%.2f px/ms gap_px=%s gap_ms=%s residual %.0fpx overshoot %.0fpx n=%d" % (
            change["at_ms"],
            change["before_speed_px_ms"] if change["before_speed_px_ms"] is not None else 0.0,
            change["after_speed_px_ms"],
            format_stat(change.get("l2p_signed_gap_px", {})),
            format_stat(change.get("l2p_signed_gap_ms", {})),
            change.get("l2p_speed_change_residual_px", 0.0),
            change.get("l2p_speed_change_overshoot_px", 0.0),
            change["n"])
        for change in changes)


def format_paint_cycle(entry):
    """One line per run: paint cadence, display cadence and phase."""
    if not entry or not entry.get("paint"):
        return "none"
    parts = [
        "paint %s bins{%s}" % (
            format_stat(entry.get("paint_interval", {})),
            ",".join("%s:%d" % (label, count)
                     for label, count in entry.get("paint_interval_bins", {}).items())),
        "present %s" % format_stat(entry.get("present_interval", {})),
    ]
    period = entry.get("display_period_ms")
    paint_per_display = entry.get("paint_per_display")
    present_per_display = entry.get("present_per_display")
    parts.append("display=%s ms paint/display=%s present/display=%s" % (
        "n/a" if period is None else "%.2f" % period,
        "n/a" if paint_per_display is None else "%.2f" % paint_per_display,
        "n/a" if present_per_display is None else "%.2f" % present_per_display))
    if entry.get("ms_until_displayed"):
        parts.append("until_displayed %s" % format_stat(entry["ms_until_displayed"]))
    if entry.get("present_modes"):
        parts.append("modes{%s}" % ",".join(
            "%s=%d" % (name, count) for name, count in entry["present_modes"].items()))
    phase = entry.get("phase")
    if phase:
        parts.append("phase %s R=%.2f mean=%.2f bins=%s" % (
            format_stat(phase), phase.get("concentration", 0.0), phase.get("mean", 0.0),
            ",".join(str(count) for count in phase.get("bins", []))))
    return " ".join(parts)


def format_l1_events(events):
    """Event counts from the probe log (no distribution, so format_stat does not apply)."""
    if not events:
        return "none"
    return " ".join("%s=%d" % (name, count) for name, count in events.items())


CANVAS_PATH_EVENTS = (
    "outline_request",
    "outline_state",
    "canvas_outline_doc",
    "canvas_outline_wdg",
    "canvas_update_dispatch",
    "canvas_update_issued",
    "canvas_widget_update_request",
    "canvas_busy_defer",
    "canvas_paint_event",
    "canvas_initialize_gl",
    "canvas_resize_gl",
    "canvas_about_to_compose",
)

CANVAS_PATH_STATES = (
    "outline_state",
    "canvas_outline_doc",
    "canvas_outline_wdg",
    "canvas_update_issued",
    "canvas_paint_event",
)


def summarize_canvas_path(events):
    """Where the canvas update path stopped, from the probe markers.

    The counts alone separate "no update was requested" from "the request was
    dropped before the canvas", and the extra bit histograms name the state
    that stopped it (no paint op preset, an empty widget rect, a dropped paint
    event).
    """
    summary = {name: len(events.get(name, [])) for name in CANVAS_PATH_EVENTS}
    for name in CANVAS_PATH_STATES:
        values = collections.Counter(int(entry[3]) for entry in events.get(name, []))
        if values:
            summary[name + "_extra"] = dict(sorted(values.items()))
    return summary


def summarize_swap(events, ticks_per_ms):
    """Split the time after paintGL into Qt composition and the platform swap.

    QOpenGLWidget emits aboutToCompose after paintGL and before the backing
    store is composited; frameSwapped follows the platform swap. On desktop
    OpenGL the swap is WGL SwapBuffers, on ANGLE it is the EGL exchange.
    Runs before the marker have no aboutToCompose entries and return an empty
    summary rather than a misleading zero.
    """
    compose = [entry[0] for entry in events.get("canvas_about_to_compose", [])]
    if not compose:
        return {}
    paint_end = [entry[0] for entry in events.get("paintgl_end", [])]
    swapped = [entry[0] for entry in events.get("frame_swapped", [])]
    compose_delay, swap_delay, total_delay = [], [], []
    for moment in compose:
        end = last_before(paint_end, moment)
        if end is not None and moment - end <= MAX_PAINT_TO_PRESENT_MS * ticks_per_ms:
            compose_delay.append((moment - end) / ticks_per_ms)
    for moment in swapped:
        begin = last_before(compose, moment)
        if begin is not None and moment - begin <= MAX_PAINT_TO_PRESENT_MS * ticks_per_ms:
            swap_delay.append((moment - begin) / ticks_per_ms)
        end = last_before(paint_end, moment)
        if end is not None and moment - end <= MAX_PAINT_TO_PRESENT_MS * ticks_per_ms:
            total_delay.append((moment - end) / ticks_per_ms)
    return {
        "paint_end_to_compose": describe(compose_delay),
        "compose_to_frame_swapped": describe(swap_delay),
        "paint_end_to_frame_swapped": describe(total_delay),
        "compose_events": len(compose),
    }


def format_canvas_swap(entry):
    """One line per run: where the time after paintGL goes."""
    if not entry:
        return "none"
    return "paint_end_to_compose %s ; compose_to_swap %s ; paint_end_to_swap %s" % (
        format_stat(entry["paint_end_to_compose"]),
        format_stat(entry["compose_to_frame_swapped"]),
        format_stat(entry["paint_end_to_frame_swapped"]))


RENDERER_NAMES = {1: "auto", 2: "desktop", 4: "angle", 8: "software"}


def summarize_render_path(events):
    """Return the renderer, swap interval and L2 CPU switches of one run."""
    entries = events.get("l1_render_path", [])
    if not entries:
        return None
    renderer, swap_interval, flags = entries[-1][1], entries[-1][2], int(entries[-1][3])
    result = {
        "renderer": int(renderer),
        "renderer_name": RENDERER_NAMES.get(int(renderer), "unknown"),
        "swap_interval": int(swap_interval),
        "angle": bool(flags & 1),
        "force_disable_texture_buffers": bool(flags & 2),
        "texture_buffers_off": bool(flags & 4),
        "texture_buffers_on": bool(flags & 8),
    }
    texture = events.get("l1_texture_buffers", [])
    if texture:
        result["texture_buffers"] = bool(texture[-1][1])
        result["texture_buffers_switch"] = int(texture[-1][3]) - 1
    return result


def format_render_path(entry):
    """One line per run: which renderer and upload/composition path ran."""
    if not entry:
        return "none"
    flags = []
    for key, label in (("angle", "angle"), ("force_disable_texture_buffers", "force-no-pbo"),
                       ("texture_buffers_off", "pbo-off"), ("texture_buffers_on", "pbo-on")):
        if entry.get(key):
            flags.append(label)
    text = "renderer=%s swap_interval=%d texture_buffers=%s" % (
        entry.get("renderer_name", "unknown"), entry.get("swap_interval", -1),
        "unknown" if "texture_buffers" not in entry else ("pbo" if entry["texture_buffers"] else "direct"))
    if flags:
        text += " flags{%s}" % ",".join(flags)
    return text


def summarize_renderer_selection(events):
    """Return the chosen renderer, the skipped candidates and the check.

    selectSurfaceConfig() runs before the usage logger is initialized, so the
    measurement launch keeps these facts only in the probe log and on stderr.
    """
    entries = events.get("l1_renderer_selection", [])
    if not entries:
        return None
    renderer, candidates, flags = int(entries[-1][1]), int(entries[-1][2]), int(entries[-1][3])
    skipped = sorted({int(entry[1]) for entry in events.get("l1_renderer_skipped", [])})
    result = {
        "renderer": renderer,
        "renderer_name": RENDERER_NAMES.get(renderer, "unknown"),
        "candidates": candidates,
        "skipped": [RENDERER_NAMES.get(value, str(value)) for value in skipped],
        "forced": bool(flags & 1),
    }
    checks = events.get("l1_first_frame_check", [])
    if checks:
        check_renderer, passed = int(checks[-1][1]), int(checks[-1][2])
        result["first_frame_check"] = {
            "renderer": RENDERER_NAMES.get(check_renderer, "unknown"),
            "passed": bool(passed),
        }
    return result


def format_renderer_selection(entry):
    """One line per run: the chosen renderer, skipped marks and the check."""
    if not entry:
        return "none"
    text = "chosen=%s candidates=%d skipped=%s forced=%s" % (
        entry.get("renderer_name", "unknown"), entry.get("candidates", 0),
        ",".join(entry.get("skipped") or []) or "none",
        "yes" if entry.get("forced") else "no")
    check = entry.get("first_frame_check")
    if check:
        text += " first_frame=%s/%s" % (
            check.get("renderer", "unknown"), "passed" if check.get("passed") else "failed")
    else:
        text += " first_frame=not-run"
    return text


def format_thread_cpu(entry):
    """One line per run: per-thread CPU inside the injected input window."""
    if not entry:
        return "none"
    parts = ["window=%.1fs cpu=%.2fs (%.1f%%) threads=%d" % (
        entry.get("window_s", 0.0), entry.get("cpu_seconds", 0.0),
        entry.get("percent", 0.0), entry.get("threads", 0))]
    top = entry.get("top") or []
    if top:
        first = top[0]
        parts.append("top=%.2fs#%d" % (first.get("seconds", 0.0), first.get("id", 0)))
    roles = entry.get("roles") or {}
    if roles.get("main_thread") is not None:
        parts.append("main=%.2fs" % roles.get("main_seconds", 0.0))
    if roles.get("paintgl_thread") is not None:
        parts.append("paint=%.2fs" % roles.get("paint_seconds", 0.0))
    if entry.get("process_seconds") is not None:
        parts.append("process=%.2fs" % entry["process_seconds"])
    if entry.get("unaccounted_seconds") is not None:
        parts.append("unaccounted=%.2fs" % entry["unaccounted_seconds"])
    return " ".join(parts)


def format_canvas_path(entry):
    """One line per run: counts and extra-bit histograms of the update path."""
    parts = []
    for name in CANVAS_PATH_EVENTS:
        text = "%s=%d" % (name, entry.get(name, 0))
        values = entry.get(name + "_extra")
        if values:
            text += "{%s}" % ",".join(
                "%d:%d" % (key, count) for key, count in values.items())
        parts.append(text)
    return " ".join(parts)


FOCUS_GATE_EVENTS = ("qt_event", "outline_gate", "tool_action")

QT_EVENT_CODES = {
    1: "focus_in",
    2: "focus_out",
    3: "tablet_press",
    4: "tablet_release",
    5: "mouse_press",
    6: "mouse_release",
    7: "enter",
    8: "leave",
}

TOOL_MODES = {
    0: "hover",
    1: "paint",
    2: "secondary_paint",
    3: "mirror_axis_setup",
    4: "gesture",
    5: "pan",
    6: "other",
    7: "other_1",
}

TOOL_ACTIONS = {
    0: "primary",
    1: "alternate_change_size",
    2: "alternate_change_size_snap",
    3: "alternate_sample_fg_node",
    4: "alternate_sample_bg_node",
    5: "alternate_sample_fg_image",
    6: "alternate_sample_bg_image",
    7: "secondary",
    8: "third",
    9: "fourth",
    10: "fifth",
}


def summarize_focus_gate(events):
    """Focus and tool-action state around the presses, from the probe markers.

    They separate a press that reached the tool from one that was dropped
    before it, and an empty outline caused by a deactivated action from one
    caused by a degenerate zoom.
    """
    summary = {name: len(events.get(name, [])) for name in FOCUS_GATE_EVENTS}

    qt = events.get("qt_event", [])
    # A press records one row when it is seen and one when it reaches the
    # shortcut matcher (extra bit 32). Keep the delivered rows apart so the
    # press count stays one per press and a missing delivery is visible.
    delivered = [entry for entry in qt if int(entry[3]) & 32]
    qt = [entry for entry in qt if not int(entry[3]) & 32]
    summary["qt_event"] = len(qt)
    if qt:
        summary["qt_event_codes"] = dict(sorted(collections.Counter(int(entry[1]) for entry in qt).items()))
        summary["qt_event_modes"] = dict(sorted(collections.Counter(int(entry[2]) for entry in qt).items()))
        summary["qt_event_extra"] = dict(sorted(collections.Counter(int(entry[3]) for entry in qt).items()))
    if delivered:
        summary["qt_event_delivered"] = len(delivered)
        summary["qt_event_delivered_extra"] = dict(sorted(collections.Counter(int(entry[3]) for entry in delivered).items()))

    gate = events.get("outline_gate", [])
    if gate:
        summary["outline_gate_extra"] = dict(sorted(collections.Counter(int(entry[3]) for entry in gate).items()))
        summary["outline_gate_view_zoom"] = {"min": min(entry[1] for entry in gate),
                                             "max": max(entry[1] for entry in gate)}
        summary["outline_gate_physical_zoom"] = {"min": min(entry[2] for entry in gate),
                                                 "max": max(entry[2] for entry in gate)}

    actions = events.get("tool_action", [])
    if actions:
        summary["tool_action_actions"] = dict(sorted(collections.Counter(int(entry[1]) for entry in actions).items()))
        summary["tool_action_extra"] = dict(sorted(collections.Counter(int(entry[3]) for entry in actions).items()))

    return summary


def _pair_list(values):
    return ",".join("%d:%d" % (key, count) for key, count in values.items())


def _named_list(values, names):
    return ",".join("%s:%d" % (names.get(key, str(key)), count) for key, count in values.items())


def _zoom_range(values):
    low, high = values["min"], values["max"]
    if abs(high - low) < 1e-9:
        return "%.2f" % low
    return "%.2f-%.2f" % (low, high)


def format_focus_gate(entry):
    """One line per run: focus/press events, outline gates and tool actions."""
    if not entry or not any(entry.get(name, 0) for name in FOCUS_GATE_EVENTS):
        return "none"
    parts = []

    count = entry.get("qt_event", 0)
    if count:
        text = "qt_event=%d{%s}" % (count, _named_list(entry.get("qt_event_codes", {}), QT_EVENT_CODES))
        if entry.get("qt_event_modes"):
            text += " modes{%s}" % _named_list(entry["qt_event_modes"], TOOL_MODES)
        if entry.get("qt_event_extra"):
            text += " extra{%s}" % _pair_list(entry["qt_event_extra"])
        parts.append(text)

    delivered = entry.get("qt_event_delivered")
    if delivered:
        text = "qt_event_delivered=%d" % delivered
        if entry.get("qt_event_delivered_extra"):
            text += "{extra %s}" % _pair_list(entry["qt_event_delivered_extra"])
        parts.append(text)

    count = entry.get("outline_gate", 0)
    if count:
        text = "outline_gate=%d" % count
        if entry.get("outline_gate_extra"):
            text += "{extra %s}" % _pair_list(entry["outline_gate_extra"])
        view = entry.get("outline_gate_view_zoom")
        physical = entry.get("outline_gate_physical_zoom")
        if view and physical:
            text += " zoom=%s/%s" % (_zoom_range(view), _zoom_range(physical))
        parts.append(text)

    count = entry.get("tool_action", 0)
    if count:
        text = "tool_action=%d{%s}" % (count, _named_list(entry.get("tool_action_actions", {}), TOOL_ACTIONS))
        if entry.get("tool_action_extra"):
            text += " extra{%s}" % _pair_list(entry["tool_action_extra"])
        parts.append(text)

    return " ; ".join(parts)


def format_l1_last_frame_completed(entry):
    """Share of frames whose predecessor had finished on the GPU."""
    n = entry.get("n", 0)
    if not n:
        return "n=0"
    return "completed=%d/%d (%.1f%%)" % (entry.get("completed", 0), n, 100.0 * entry.get("completed", 0) / n)


def format_l1_h2(entry):
    """H2 (waiting for the display handoff) decisions from the probe log."""
    n = entry.get("n", 0)
    if not n:
        return "n=0"
    return "n=%d fence=%d complete=%d waited=%d deferred=%d wait_done=%d" % (
        n, entry.get("fence", 0), entry.get("complete", 0),
        entry.get("waited", 0), entry.get("deferred", 0),
        entry.get("wait_completed", 0))


def format_l1_h1(entry):
    """Outcome of the H1 (frames queued ahead) D3D11 call from the probe log."""
    def flag(value):
        return "none" if value is None else str(int(value))

    def hresult(value):
        return "none" if value is None else "0x%08X" % (value & 0xFFFFFFFF)

    return ("egl=%s client_ext=%s display_ext=%s device_query=%s entry=%s "
            "display_attrib=%s/%s device_attrib=%s/%s device=%s query_interface=%s "
            "set=%s requested=%s readback=%s"
            % (flag(entry.get("egl_context")), flag(entry.get("client_extensions")),
               flag(entry.get("display_extensions")), flag(entry.get("device_query_ext")),
               flag(entry.get("entry_points")),
               flag(entry.get("display_attrib_result")), flag(entry.get("display_attrib_device")),
               flag(entry.get("device_attrib_result")), flag(entry.get("device_attrib_device")),
               flag(entry.get("device")),
               hresult(entry.get("query_interface_hresult")),
               hresult(entry.get("set_hresult")), flag(entry.get("requested")),
               flag(entry.get("readback"))))


def format_stroke_queue_by_speed(groups):
    """Stroke queue delay (stroke_paint to stroke_dab) by pointer speed."""
    if not groups:
        return "none"
    return " ; ".join(
        "%.2f px/ms n=%d p50=%.1f p95=%.1f max=%.1f" % (
            group["speed_px_ms"], group["n"], group["p50"], group["p95"], group["max"])
        for group in groups)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("runs", nargs="+", help="run directories")
    parser.add_argument("--json", help="write per-run results and the comparison as JSON")
    args = parser.parse_args(argv)

    results = collections.OrderedDict()
    for directory in args.runs:
        run = Run(directory)
        results[run.name] = run.analyze()

    for name, result in results.items():
        state = result["presentmon"]
        presentmon = state.get("status") or ("present" if state.get("present") else "none")
        print("== %s  presents=%d displayed=%d  PresentMon=%s" % (
            name, result["presents"]["n"], result["presents"]["displayed"], presentmon))
        if "input_path" in result:
            print("   input %s" % " ".join("%s=%d" % item for item in result["input_path"].items()))
        if "inject" in result:
            print("   injected samples=%d failed=%d" % (result["inject"]["n"], result["inject"]["failed"]))
        for key, stat in result.items():
            if key in ("presents", "inject", "presentmon", "input_path"):
                continue
            if key == "paint_cycle":
                print("   %-44s %s" % (key, format_paint_cycle(stat)))
                continue
            if key == "l1_events":
                print("   %-44s %s" % (key, format_l1_events(stat)))
                continue
            if key == "canvas_path":
                print("   %-44s %s" % (key, format_canvas_path(stat)))
                continue
            if key == "canvas_swap":
                print("   %-44s %s" % (key, format_canvas_swap(stat)))
                continue
            if key == "render_path":
                print("   %-44s %s" % (key, format_render_path(stat)))
                continue
            if key == "renderer_selection":
                print("   %-44s %s" % (key, format_renderer_selection(stat)))
                continue
            if key == "thread_cpu":
                print("   %-44s %s" % (key, format_thread_cpu(stat)))
                continue
            if key == "focus_gate":
                print("   %-44s %s" % (key, format_focus_gate(stat)))
                continue
            if key == "l1_last_frame_completed":
                print("   %-44s %s" % (key, format_l1_last_frame_completed(stat)))
                continue
            if key == "l1_h1":
                print("   %-44s %s" % (key, format_l1_h1(stat)))
                continue
            if key == "l1_h2":
                print("   %-44s %s" % (key, format_l1_h2(stat)))
                continue
            if key == "contact.stroke_paint_to_dab_speed":
                print("   %-44s %s" % (key, format_stroke_queue_by_speed(stat)))
                continue
            if key == "l2p.speed_groups":
                print("   %-44s %s" % (key, format_l2p_speed_groups(stat)))
                continue
            if key == "l2p.speed_change":
                print("   %-44s %s" % (key, format_l2p_speed_changes(stat)))
                continue
            if key == "stroke_queue":
                print("   %-44s paint=%d dab=%d paired=%d" % (
                    key, stat.get("paint", 0), stat.get("dab", 0), stat.get("paired", 0)))
                continue
            print("   %-44s %s" % (key, format_stat(stat)))

    states = [(result.get("presentmon") or {}).get("status")
              or ("present" if (result.get("presentmon") or {}).get("present") else "none")
              for result in results.values()]
    without_presentmon = states.count("none")
    partial = states.count("partial")
    if without_presentmon or partial:
        print("== PresentMon: %d partial, %d none left out of the breakdown comparison" % (
            partial, without_presentmon))
    comparison = compare(results)
    if comparison:
        print("== spread of run medians ((max - min) / median)")
        for metric, entry in comparison.items():
            print(
                "   %-44s median=%6.1f min=%6.1f max=%6.1f spread=%5.1f%% runs=%d"
                % (metric, entry["median_of_medians"], entry["min"], entry["max"], 100 * entry["spread"], entry["runs"])
            )

    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump({"runs": results, "comparison": comparison}, handle, indent=2)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
