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

The position offset between pointer and drawing comes from the still dwells
at the ends of every pass: outline center minus pointer position for hover,
ink edge minus pointer position (about the brush radius) for strokes.

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

SETTLE_NS = 150_000_000  # use only the dwell part after the drawing has caught up
EDGE_PX = 12  # ignore positions this close to the dwell positions
MAX_LATENCY_NS = 300_000_000


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
    """Displayed position minus pointer position during the settled part of both dwells."""
    offsets = []
    for dwell in (pass_.before, pass_.after):
        if not dwell or (pass_.kind == "ink" and not dwell[0][5]):
            continue
        start, end, x = dwell[0][0] + SETTLE_NS, dwell[-1][0], dwell[0][3]
        for t in frame_times[bisect.bisect_left(frame_times, start): bisect.bisect_right(frame_times, end)]:
            detection = frames[t].get(pass_.band)
            if detection is None:
                continue
            value = position(pass_.kind, detection)
            if value is not None:
                offsets.append(value - x)
    return offsets


def analyze_run(directory):
    samples = read_inject(os.path.join(directory, "inject.csv"))
    frames = read_capture(os.path.join(directory, "capture.csv"))
    frame_times = list(frames)
    all_passes = passes(samples)

    offsets = {"hover": [], "ink": []}
    for pass_ in all_passes:
        offsets[pass_.kind].extend(dwell_offsets(pass_, frames, frame_times))
    offset = {kind: statistics.median(values) if values else None for kind, values in offsets.items()}

    stats = collections.defaultdict(list)
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
            stats[pass_.kind + ".input_to_capture"].append((t - crossed) / 1e6)
            stats[pass_.kind + ".frame_to_arrival"].append((detection["arrival"] - t) / 1e6)

    intervals = [(b - a) / 1e6 for a, b in zip(frame_times, frame_times[1:]) if b - a < 100_000_000]
    result = {key: analyze.describe(values) for key, values in sorted(stats.items())}
    result["frames"] = {"n": len(frame_times), "interval": analyze.describe(intervals)}
    result["calibration"] = {
        kind: {"offset_px": offset[kind], "spread_px": (max(v) - min(v)) if v else None, "n": len(v)}
        for kind, v in offsets.items()
    }
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
        print("== %s  frames=%d interval %s" % (name, frames["n"], analyze.format_stat(frames["interval"])))
        for kind, entry in result["calibration"].items():
            print("   calibration %-5s offset=%s px spread=%s px n=%d" % (kind, entry["offset_px"], entry["spread_px"], entry["n"]))
        for key, stat in result.items():
            if key in ("frames", "calibration"):
                continue
            print("   %-28s %s" % (key, analyze.format_stat(stat)))
    comparison = analyze.compare(results, PRIMARY)
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
