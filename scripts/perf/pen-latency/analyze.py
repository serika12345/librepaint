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
        result["presentmon"] = {"present": self.presentmon_present, "n": len(self.presents)}
        if self.events:
            result["input_path"] = {
                event: len(self.events.get(event, []))
                for event in ("native_pointer", "native_wintab", "native_mouse", "qt_tablet_move")
            }
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

    The breakdown needs PresentMon; a run without presentmon.csv is left out
    so that a missing log does not look like a latency change.
    """
    summary = {}
    for metric in metrics:
        medians = [
            result[metric]["p50"] for result in results.values()
            if result.get(metric, {}).get("n")
            and not (require_presentmon and result.get("presentmon", {}).get("present") is False)
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
        presentmon = "present" if result["presentmon"]["present"] else "none"
        print("== %s  presents=%d displayed=%d  PresentMon=%s" % (
            name, result["presents"]["n"], result["presents"]["displayed"], presentmon))
        if "input_path" in result:
            print("   input %s" % " ".join("%s=%d" % item for item in result["input_path"].items()))
        if "inject" in result:
            print("   injected samples=%d failed=%d" % (result["inject"]["n"], result["inject"]["failed"]))
        for key, stat in result.items():
            if key in ("presents", "inject", "presentmon", "input_path"):
                continue
            print("   %-44s %s" % (key, format_stat(stat)))

    without_presentmon = sum(1 for result in results.values() if not result["presentmon"]["present"])
    if without_presentmon:
        print("== PresentMon none: %d run(s) left out of the breakdown comparison" % without_presentmon)
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
