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
import os
import statistics
import sys

POINTER_FLAG_INCONTACT = 0x4
WM_POINTERUP = 0x0247
MAX_LINK_MS = 200.0
MAX_PAINT_TO_PRESENT_MS = 100.0

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
                            "flags": int(row["flags"]), "ok": row["ok"] == "1"})
        else:
            if probe_frequency is None:
                raise SystemExit("%s: time_ns samples need the probe clock frequency" % path)
            samples.append({"time": int(row["time_ns"]) * probe_frequency // 1_000_000_000, "phase": row["phase"],
                            "flags": POINTER_FLAG_INRANGE_CONTACT if row["pressed"] == "1" else 0x2, "ok": True})
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


def describe(values):
    if not values:
        return {"n": 0}
    return {
        "n": len(values),
        "p05": percentile(values, 0.05),
        "p50": statistics.median(values),
        "p95": percentile(values, 0.95),
    }


def last_before(times, limit):
    index = bisect.bisect_right(times, limit) - 1
    return times[index] if index >= 0 else None


def float_or_none(text):
    try:
        return float(text)
    except (TypeError, ValueError):
        return None


class Run:
    def __init__(self, directory):
        self.directory = directory
        self.name = os.path.basename(os.path.normpath(directory))
        probe_path = os.path.join(directory, "probe.csv")
        inject_path = os.path.join(directory, "inject.csv")
        self.frequency = None
        self.events = None
        self.inject = None
        if os.path.exists(probe_path):
            self.frequency, self.events = load_probe(probe_path)
        if os.path.exists(inject_path):
            inject_frequency, self.inject = load_inject(inject_path, self.frequency)
            self.frequency = self.frequency or inject_frequency
        if self.frequency is None:
            raise SystemExit("%s: probe.csv or inject.csv with a '# frequency=' header is required" % directory)
        presentmon_path = os.path.join(directory, "presentmon.csv")
        self.presentmon_present = os.path.exists(presentmon_path)
        self.presents = load_presents(presentmon_path) if self.presentmon_present else []
        self.ticks_per_ms = self.frequency / 1000.0

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
            result["input_path"] = {
                event: len(self.events.get(event, []))
                for event in ("native_pointer", "native_wintab", "native_mouse", "qt_tablet_move")
            }
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
        if self.inject:
            result["inject"] = {
                "n": len(self.inject),
                "failed": sum(1 for sample in self.inject if not sample["ok"]),
            }
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


def format_l1_events(events):
    """Event counts from the probe log (no distribution, so format_stat does not apply)."""
    if not events:
        return "none"
    return " ".join("%s=%d" % (name, count) for name, count in events.items())


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
            if key == "l1_events":
                print("   %-44s %s" % (key, format_l1_events(stat)))
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
