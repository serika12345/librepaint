#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pointer input-to-capture latency from screen-capture measurement runs.

A run directory holds inject.csv and capture.csv as written by
mutter_capture.py, macos_capture.mm, or windows_capture.cpp (times in
nanoseconds of one monotonic clock, positions in captured pixels).

For every captured frame during a pass, the script reads where the brush
outline (hover band: center of the dark pixels) or the ink (stroke band:
right edge of the dense dark columns) is shown, finds the time at which the
injected pointer was at the corresponding position, and reports the frame
time minus that input time.

The position offset between pointer and drawing comes from the screen at the
end of the still dwells around every pass: outline center minus pointer position for hover,
ink edge minus pointer position (about the brush radius) for strokes.

Each run also gets a hover state ("low", "high" or "unknown") from the frame
delivery delay and the Qt tablet move interval, and a note whether
presentmon.csv exists. The states alternate from one launch to the next, so
hover comparisons are made within a state (L1 of Issue #108).

Usage: capture_analyze.py [--json OUT] RUN_DIR [RUN_DIR ...]
"""

import argparse
import bisect
import collections
import csv
import json
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import analyze  # noqa: E402  (describe, compare)

EDGE_PX = 12  # ignore positions this close to the dwell positions
MAX_LATENCY_NS = 300_000_000

# Hover latency alternates between two states from one launch to the next
# (L1 of Issue #108). The states are recognised by when the capture tool
# receives a frame after its present time and by whether Qt receives the pen
# moves evenly or in bursts. The limits come from the 2026-11-09 runs: the low
# state had 11.2 ms and 3.0-3.3 ms, the high state 9.6-9.8 ms and 0.5-2.0 ms.
# Values between the low and high limits are left unknown rather than guessed.
FRAME_TO_ARRIVAL_LOW_MS = 10.8
FRAME_TO_ARRIVAL_HIGH_MS = 10.2
TABLET_INTERVAL_LOW_MS = 2.8
TABLET_INTERVAL_HIGH_MS = 2.4
STATE_UNKNOWN = "unknown"


def read_inject(path):
    with open(path) as handle:
        rows = list(csv.DictReader(handle))
    return [
        (int(r["time_ns"]), r["phase"], r["band"], float(r["x"]), float(r["y"]), r["pressed"] == "1")
        for r in rows
    ]


def read_capture(path):
    frames = collections.defaultdict(dict)
    with open(path) as handle:
        for r in csv.DictReader(handle):
            if r["xmin"] == "":
                continue
            frames[int(r["frame_ns"])][r["band"]] = {
                "xmin": int(r["xmin"]),
                "xmax": int(r["xmax"]),
                "dense_xmax": int(r["dense_xmax"]) if r["dense_xmax"] else None,
                "arrival": int(r["arrival_ns"]),
            }
    return dict(sorted(frames.items()))


def segments(samples):
    """Split the injected samples into consecutive runs with the same band, phase and pressed state."""
    result = []
    for sample in samples:
        key = (sample[2], sample[1], sample[5])
        if result and result[-1]["key"] == key:
            result[-1]["samples"].append(sample)
        else:
            result.append({"key": key, "samples": [sample]})
    return result


class Pass:
    """One moving part of the trajectory with the dwells around it."""

    def __init__(self, band, kind, before, move, after):
        self.band, self.kind = band, kind
        self.before, self.move, self.after = before, move, after
        self.times = [s[0] for s in move]
        self.xs = [s[3] for s in move]
        self.start_x, self.end_x = self.xs[0], self.xs[-1]
        self.direction = 1 if self.end_x > self.start_x else -1

    def time_at(self, x):
        """Time at which the pointer crossed x on this pass, or None outside the moving range."""
        lo, hi = sorted((self.start_x, self.end_x))
        if not lo + EDGE_PX < x < hi - EDGE_PX:
            return None
        keys = self.xs if self.direction > 0 else [-v for v in self.xs]
        target = x if self.direction > 0 else -x
        i = bisect.bisect_left(keys, target)
        if i <= 0 or i >= len(keys):
            return None
        x0, x1 = keys[i - 1], keys[i]
        t0, t1 = self.times[i - 1], self.times[i]
        return t0 + (t1 - t0) * (target - x0) / (x1 - x0) if x1 != x0 else t0


def passes(samples):
    parts = segments(samples)
    found = []
    for i, part in enumerate(parts):
        band, phase, pressed = part["key"]
        if phase != "move":
            continue
        before = parts[i - 1]["samples"] if i > 0 and parts[i - 1]["key"][0] == band else []
        after = parts[i + 1]["samples"] if i + 1 < len(parts) and parts[i + 1]["key"][0] == band else []
        found.append(Pass(band, "ink" if pressed else "hover", before, part["samples"], after))
    return found


def position(kind, detection):
    if kind == "hover":
        return (detection["xmin"] + detection["xmax"]) / 2.0
    return detection["dense_xmax"]


def dwell_offsets(pass_, frames, frame_times):
    """Displayed position minus pointer position at the end of both dwells.

    Capture APIs deliver a frame only when the screen changes, so the screen
    at the end of a dwell is the last frame delivered before that time. The
    dwell is long enough for the drawing to catch up with the still pointer.
    """
    offsets = []
    for dwell in (pass_.before, pass_.after):
        if not dwell or (pass_.kind == "ink" and not dwell[0][5]):
            continue
        end, x = dwell[-1][0], dwell[0][3]
        for t in reversed(frame_times[: bisect.bisect_right(frame_times, end)]):
            detection = frames[t].get(pass_.band)
            if detection is None:
                continue
            value = position(pass_.kind, detection)
            if value is not None:
                offsets.append(value - x)
            break
    return offsets


def frame_samples(directory):
    """Return (frames, offsets, samples) for a run.

    samples lists (kind, frame_ns, crossed_ns, arrival_ns): a captured frame
    and the time at which the injected pointer was at the position the frame
    shows.
    """
    injected = read_inject(os.path.join(directory, "inject.csv"))
    frames = read_capture(os.path.join(directory, "capture.csv"))
    frame_times = list(frames)
    all_passes = passes(injected)

    offsets = {"hover": [], "ink": []}
    for pass_ in all_passes:
        offsets[pass_.kind].extend(dwell_offsets(pass_, frames, frame_times))
    offset = {kind: statistics.median(values) if values else None for kind, values in offsets.items()}

    samples = []
    for pass_ in all_passes:
        if offset[pass_.kind] is None:
            continue
        begin, end = pass_.times[0], pass_.times[-1] + MAX_LATENCY_NS
        for t in frame_times[bisect.bisect_left(frame_times, begin): bisect.bisect_right(frame_times, end)]:
            detection = frames[t].get(pass_.band)
            if detection is None:
                continue
            value = position(pass_.kind, detection)
            if value is None:
                continue
            crossed = pass_.time_at(value - offset[pass_.kind])
            if crossed is None or not 0 <= t - crossed < MAX_LATENCY_NS:
                continue
            samples.append((pass_.kind, t, crossed, detection["arrival"]))
    return frame_times, offsets, offset, samples


META_FOREGROUND = "app_foreground_capture"
PRESENTATION_MIN_RATIO = 0.8


def read_meta(directory):
    """Return the key=value lines of a run's meta.txt (empty when absent)."""
    path = os.path.join(directory, "meta.txt")
    values = {}
    if not os.path.exists(path):
        return values
    with open(path, encoding="utf-8", errors="replace") as handle:
        for line in handle:
            name, separator, value = line.partition("=")
            if separator:
                values[name.strip()] = value.strip()
    return values


def foreground_state(directory):
    """Whether the application was the foreground window when capture began.

    run-surface.ps1 records app_foreground_capture=True or False. A run whose
    application was not in front is a candidate for leaving out of the
    aggregation. Runs before that line existed stay usable (known False).
    """
    value = read_meta(directory).get(META_FOREGROUND)
    if value is None:
        return {"known": False, "foreground": None}
    return {"known": True, "foreground": value.lower() == "true"}


def presentation_state(directory, frame_interval_ms):
    """How often the application presented frames while the pen was moving.

    The probe records frame_swapped. Windows throttles a window that another
    window covers, so it presents far fewer frames than the display refresh
    and its frames no longer follow the input. A rate near the display rate
    means the application stayed visible even when it did not hold the
    keyboard focus (for example while a shell surface did).
    """
    probe = os.path.join(directory, "probe.csv")
    inject = os.path.join(directory, "inject.csv")
    if not os.path.exists(probe) or not os.path.exists(inject) or not frame_interval_ms:
        return None
    frequency, events = analyze.load_probe(probe)
    with open(inject) as handle:
        times = [int(row["time_ns"]) for row in csv.DictReader(handle)]
    if len(times) < 2 or not frequency:
        return None
    begin, end = min(times), max(times)
    to_ns = 1_000_000_000 / frequency
    swaps = [t for t, *_ in events.get("frame_swapped", []) if begin <= t * to_ns <= end]
    span = (end - begin) / 1e9
    if span <= 0:
        return None
    expected = 1000.0 / frame_interval_ms
    rate = len(swaps) / span
    return {"n": len(swaps), "span_s": span, "per_s": rate,
            "expected_per_s": expected, "ratio": rate / expected}


def exclusion_reason(foreground, presentation):
    """Why a run measured another window, or None when it stays usable."""
    if foreground.get("foreground") is not False:
        return None
    ratio = (presentation or {}).get("ratio")
    if ratio is None:
        return "app not foreground"
    if ratio < PRESENTATION_MIN_RATIO:
        return "app not foreground and presented at %.0f%% of the display rate" % (100 * ratio)
    return None


def presentmon_state(directory):
    """PresentMon coverage of the injected trajectory for one run.

    The capture clock and the probe clock are both nanoseconds; PresentMon
    writes TimeInQPC ticks, so the probe frequency converts the injected span.
    """
    path = os.path.join(directory, "presentmon.csv")
    inject_path = os.path.join(directory, "inject.csv")
    frequency = analyze.probe_frequency(directory)
    window = None
    if frequency and os.path.exists(inject_path):
        with open(inject_path) as handle:
            times = [int(row["time_ns"]) for row in csv.DictReader(handle)]
        if len(times) >= 2:
            window = (min(times) * frequency // 1_000_000_000,
                      max(times) * frequency // 1_000_000_000)
    presents = analyze.load_presents(path) if os.path.exists(path) else []
    return analyze.presentmon_state(path, presents, window)


def tablet_move_interval_p05(directory):
    """p05 of the interval between Qt tablet moves, or None without a probe log."""
    path = os.path.join(directory, "probe.csv")
    if not os.path.exists(path):
        return None
    frequency, events = analyze.load_probe(path)
    if not frequency:
        return None
    times = [entry[0] for entry in events.get("qt_tablet_move", [])]
    gaps = [(b - a) * 1000.0 / frequency for a, b in zip(times, times[1:])
            if (b - a) * 1000.0 / frequency < 200.0]
    return analyze.percentile(gaps, 0.05) if gaps else None


def classify_hover_state(frame_to_arrival_p50_ms, tablet_interval_p05_ms):
    """Return (label, reason) for the hover state of one run.

    Both indicators vote; disagreement or a value between the limits gives
    "unknown". A run with one indicator absent uses the other.
    """
    votes = []
    if frame_to_arrival_p50_ms is not None:
        if frame_to_arrival_p50_ms >= FRAME_TO_ARRIVAL_LOW_MS:
            votes.append(("low", "hover.frame_to_arrival p50 %.1f ms >= %.1f" % (
                frame_to_arrival_p50_ms, FRAME_TO_ARRIVAL_LOW_MS)))
        elif frame_to_arrival_p50_ms <= FRAME_TO_ARRIVAL_HIGH_MS:
            votes.append(("high", "hover.frame_to_arrival p50 %.1f ms <= %.1f" % (
                frame_to_arrival_p50_ms, FRAME_TO_ARRIVAL_HIGH_MS)))
    if tablet_interval_p05_ms is not None:
        if tablet_interval_p05_ms >= TABLET_INTERVAL_LOW_MS:
            votes.append(("low", "rate.qt_tablet_move_interval p05 %.1f ms >= %.1f" % (
                tablet_interval_p05_ms, TABLET_INTERVAL_LOW_MS)))
        elif tablet_interval_p05_ms <= TABLET_INTERVAL_HIGH_MS:
            votes.append(("high", "rate.qt_tablet_move_interval p05 %.1f ms <= %.1f" % (
                tablet_interval_p05_ms, TABLET_INTERVAL_HIGH_MS)))
    labels = {label for label, _ in votes}
    if len(labels) != 1:
        reason = "no indicator in the low or high range" if not votes else "indicators disagree"
        return STATE_UNKNOWN, reason
    return labels.pop(), "; ".join(reason for _, reason in votes)


def analyze_run(directory):
    frame_times, offsets, offset, samples = frame_samples(directory)
    stats = collections.defaultdict(list)
    for kind, t, crossed, arrival in samples:
        stats[kind + ".input_to_capture"].append((t - crossed) / 1e6)
        stats[kind + ".frame_to_arrival"].append((arrival - t) / 1e6)

    intervals = [(b - a) / 1e6 for a, b in zip(frame_times, frame_times[1:]) if b - a < 100_000_000]
    result = {key: analyze.describe(values) for key, values in sorted(stats.items())}
    result["frames"] = {"n": len(frame_times), "interval": analyze.describe(intervals)}
    result["calibration"] = {
        kind: {"offset_px": offset[kind], "spread_px": (max(v) - min(v)) if v else None, "n": len(v)}
        for kind, v in offsets.items()
    }
    frame_to_arrival = result.get("hover.frame_to_arrival", {}).get("p50")
    tablet_interval = tablet_move_interval_p05(directory)
    label, reason = classify_hover_state(frame_to_arrival, tablet_interval)
    result["state"] = {
        "label": label,
        "reason": reason,
        "hover_p50_ms": result.get("hover.input_to_capture", {}).get("p50"),
        "frame_to_arrival_p50_ms": frame_to_arrival,
        "tablet_move_interval_p05_ms": tablet_interval,
    }
    result["presentmon"] = presentmon_state(directory)
    foreground = foreground_state(directory)
    presentation = presentation_state(directory, result["frames"]["interval"].get("p50"))
    result["foreground"] = foreground
    result["presentation"] = presentation
    result["excluded"] = exclusion_reason(foreground, presentation)
    return result


PRIMARY = ("hover.input_to_capture", "ink.input_to_capture")


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("runs", nargs="+")
    parser.add_argument("--json")
    args = parser.parse_args(argv)
    results = collections.OrderedDict()
    for directory in args.runs:
        results[os.path.basename(os.path.normpath(directory))] = analyze_run(directory)
    for name, result in results.items():
        frames = result["frames"]
        presentmon = result["presentmon"].get("status") or (
            "present" if result["presentmon"].get("present") else "none")
        coverage = result["presentmon"].get("coverage")
        if coverage is not None and presentmon == "partial":
            presentmon += " (coverage %.1f%%)" % (100 * coverage)
        foreground = result["foreground"]
        foreground_text = "-" if not foreground["known"] else (
            "ok" if foreground["foreground"] else "not-foreground")
        presentation = result.get("presentation") or {}
        ratio = presentation.get("ratio")
        print("== %s  frames=%d interval %s  state=%s  PresentMon=%s  foreground=%s%s" % (
            name, frames["n"], analyze.format_stat(frames["interval"]), result["state"]["label"],
            presentmon, foreground_text,
            "" if ratio is None else " presented=%.0f%%" % (100 * ratio)))
        if result.get("excluded"):
            print("   left out: %s" % result["excluded"])
        print("   state: %s" % result["state"]["reason"])
        for kind, entry in result["calibration"].items():
            print("   calibration %-5s offset=%s px spread=%s px n=%d" % (kind, entry["offset_px"], entry["spread_px"], entry["n"]))
        for key, stat in result.items():
            if key in ("frames", "calibration", "state", "presentmon", "foreground", "presentation", "excluded"):
                continue
            print("   %-28s %s" % (key, analyze.format_stat(stat)))
    comparison = analyze.compare(results, PRIMARY, require_presentmon=False)
    if comparison:
        print("== spread of run medians ((max - min) / median)")
        for metric, entry in comparison.items():
            print("   %-28s median=%6.1f min=%6.1f max=%6.1f spread=%5.1f%% runs=%d" % (
                metric, entry["median_of_medians"], entry["min"], entry["max"], 100 * entry["spread"], entry["runs"]))
    if args.json:
        with open(args.json, "w") as handle:
            json.dump({"runs": results, "comparison": comparison}, handle, indent=2)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
