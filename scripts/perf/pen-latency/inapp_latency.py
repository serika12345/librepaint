#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Split the pen-to-display delay of runs into the part the application can
see and the part after the buffer swap.

A run directory needs probe.csv (with stroke_paint, stroke_dirty_issued,
paintgl_begin, frame_swapped and projection_update), inject.csv and
capture.csv. For every stroke sample k the script finds

  I  the injection time of k (inject.csv, the k-th pressed row after the
     first one, because the first press starts the stroke),
  A  the time the application painted k (stroke_paint),
  D  the first stroke_dirty_issued that covers k (the newest painted point
     of that update is k or a later sample),
  G  the first paintgl_begin after D, or after the first projection_update
     after D (the second variant),
  S  the first frame_swapped after G.

It prints the medians of S-I (injection to swap), A-I (input age at the
application) and S-A, and the median of t-S over the captured frames of the
ink band, where t is the capture frame time and S belongs to the sample that
frame shows. t-S is the part after the swap: compositor, scan-out and the
capture's own sampling. The median of the frames' input-to-capture delay is
printed for comparison.

The event time that the stroke passes on (extra of stroke_dirty_issued, in
milliseconds of the operating system's event clock) is compared with I: the
spread of its difference to I shows whether the event time can stand in for
the injection time.

Usage: inapp_latency.py RUN_DIR [RUN_DIR ...]
"""

import bisect
import csv
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capture_analyze  # noqa: E402


def read_probe(path):
    """Return (rows, ns_per_tick); rows are (ns, event, x, y, extra)."""
    freq = 1e9
    rows = []
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            if line.startswith("# frequency="):
                freq = float(line.split("=", 1)[1])
                continue
            if line.startswith("time,") or line.startswith("#"):
                continue
            parts = line.rstrip("\n").split(",")
            if len(parts) < 6:
                continue
            try:
                rows.append((int(parts[0]), parts[1], parts[2], parts[3], parts[4]))
            except ValueError:
                continue
    scale = 1e9 / freq
    rows = [(int(t * scale), e, x, y, ex) for t, e, x, y, ex in rows]
    rows.sort(key=lambda row: row[0])
    return rows


def first_at_or_after(times, t):
    i = bisect.bisect_left(times, t)
    return times[i] if i < len(times) else None


def median(values):
    return statistics.median(values) if values else None


def percentile(values, q):
    if not values:
        return None
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, int(len(ordered) * q))]


def stroke_inject_times(directory):
    """Injection times of the pressed rows, split into strokes in order."""
    strokes = {}
    with open(os.path.join(directory, "inject.csv"), encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if row["pressed"] == "1" and row["band"].startswith("stroke"):
                strokes.setdefault(row["band"], []).append(int(row["time_ns"]))
    return [strokes[key] for key in sorted(strokes)]


def analyze(directory):
    rows = read_probe(os.path.join(directory, "probe.csv"))
    paints = [(t, x, y) for t, e, x, y, _ in rows if e == "stroke_paint"]
    dirties = [(t, x, y, ex) for t, e, x, y, ex in rows if e == "stroke_dirty_issued"]
    begins = [t for t, e, *_ in rows if e == "paintgl_begin"]
    swaps = [t for t, e, *_ in rows if e == "frame_swapped"]
    projections = [t for t, e, *_ in rows if e == "projection_update"]
    if not dirties:
        raise SystemExit("%s: probe.csv has no stroke_dirty_issued (older build)" % directory)

    inject_strokes = stroke_inject_times(directory)
    inject = []
    for times in inject_strokes:
        inject.extend(times[1:])
    n = min(len(paints), len(inject))
    if abs(len(paints) - len(inject)) > 12:
        raise SystemExit("%s: %d painted samples against %d injected" % (directory, len(paints), len(inject)))

    # k-th paint event <-> k-th pressed inject row (without the first of each stroke).
    positions = {}
    for k, (_, x, y) in enumerate(paints[:n]):
        positions.setdefault((x, y), []).append(k)
    paint_times = [t for t, _, _ in paints[:n]]

    newest = []  # (dirty time, newest k covered)
    for t, x, y, ex in dirties:
        cands = [k for k in positions.get((x, y), []) if paint_times[k] <= t]
        if cands:
            newest.append((t, cands[-1], ex))
    running = -1
    firsts = {}
    for t, k, _ in newest:
        if k > running:
            for covered in range(running + 1, k + 1):
                firsts[covered] = t
            running = k

    def swap_for(dirty_time, via_projection):
        start = dirty_time
        if via_projection:
            start = first_at_or_after(projections, dirty_time)
            if start is None:
                return None
        paint = first_at_or_after(begins, start)
        if paint is None:
            return None
        return first_at_or_after(swaps, paint)

    result = {"samples": n}
    sample_swap = {}
    for variant, via in (("direct", False), ("projection", True)):
        s_minus_i, s_minus_a = [], []
        table = {}
        for k in range(n):
            if k not in firsts:
                continue
            swap = swap_for(firsts[k], via)
            if swap is None:
                continue
            table[k] = swap
            s_minus_i.append((swap - inject[k]) / 1e6)
            s_minus_a.append((swap - paint_times[k]) / 1e6)
        sample_swap[variant] = table
        result[variant] = {
            "S-I p50": median(s_minus_i), "S-I p90": percentile(s_minus_i, 0.9),
            "S-A p50": median(s_minus_a), "S-A p90": percentile(s_minus_a, 0.9),
            "samples": len(table),
        }
    result["A-I p50"] = median([(paint_times[k] - inject[k]) / 1e6 for k in range(n)])
    result["A-I p90"] = percentile([(paint_times[k] - inject[k]) / 1e6 for k in range(n)], 0.9)

    # Event time of the OS against the injection time: spread after removing the median.
    diffs = []
    for t, k, ex in newest:
        try:
            diffs.append(float(ex) * 1e6 - inject[k])
        except (ValueError, IndexError):
            continue
    if diffs:
        base = median(diffs)
        dev = [abs(d - base) / 1e6 for d in diffs]
        result["os_event_time"] = {"n": len(diffs), "dev p50 ms": median(dev), "dev p90 ms": percentile(dev, 0.9)}

    # After the swap: capture frame time minus the swap that showed the displayed sample.
    _, _, _, samples = capture_analyze.frame_samples(directory)
    inject_flat = inject
    after = {"direct": [], "projection": []}
    capture_ms = []
    for sample in samples:
        kind, t, crossed = sample[0], sample[1], sample[2]
        if kind != "ink":
            continue
        capture_ms.append((t - crossed) / 1e6)
        i = bisect.bisect_left(inject_flat, crossed)
        if i >= len(inject_flat):
            continue
        k = i if i < len(inject_flat) else len(inject_flat) - 1
        for variant in after:
            swap = sample_swap[variant].get(k)
            if swap is not None and 0 <= t - swap < 200e6:
                after[variant].append((t - swap) / 1e6)
    result["capture p50"] = median(capture_ms)
    for variant in after:
        result[variant]["t-S p50"] = median(after[variant])
        result[variant]["t-S frames"] = len(after[variant])
    swap_intervals = [(b - a) / 1e6 for a, b in zip(swaps, swaps[1:]) if b - a < 100e6]
    result["swap interval p50"] = median(swap_intervals)
    return result


def fmt(value):
    return "-" if value is None else "%.1f" % value


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    per_run = []
    for directory in argv[1:]:
        r = analyze(directory)
        per_run.append(r)
        print(os.path.basename(directory))
        print("  samples %d, capture p50 %s ms, swap interval p50 %s ms, A-I p50 %s p90 %s"
              % (r["samples"], fmt(r["capture p50"]), fmt(r["swap interval p50"]), fmt(r["A-I p50"]), fmt(r["A-I p90"])))
        for variant in ("direct", "projection"):
            v = r[variant]
            print("  %-10s S-I p50 %s p90 %s | S-A p50 %s p90 %s | t-S p50 %s (%s frames)"
                  % (variant, fmt(v["S-I p50"]), fmt(v["S-I p90"]), fmt(v["S-A p50"]), fmt(v["S-A p90"]),
                     fmt(v.get("t-S p50")), v.get("t-S frames")))
        if "os_event_time" in r:
            o = r["os_event_time"]
            print("  os event time vs injection: deviation p50 %s ms p90 %s ms (n=%d)"
                  % (fmt(o["dev p50 ms"]), fmt(o["dev p90 ms"]), o["n"]))
    if len(per_run) > 1:
        print("median over %d runs" % len(per_run))
        print("  capture p50 %s ms, swap interval %s ms, A-I p50 %s ms" % (
            fmt(median([r["capture p50"] for r in per_run if r["capture p50"] is not None])),
            fmt(median([r["swap interval p50"] for r in per_run if r["swap interval p50"] is not None])),
            fmt(median([r["A-I p50"] for r in per_run if r["A-I p50"] is not None]))))
        for variant in ("direct", "projection"):
            print("  %-10s S-I p50 %s | S-A p50 %s | t-S p50 %s" % (
                variant,
                fmt(median([r[variant]["S-I p50"] for r in per_run if r[variant]["S-I p50"] is not None])),
                fmt(median([r[variant]["S-A p50"] for r in per_run if r[variant]["S-A p50"] is not None])),
                fmt(median([r[variant]["t-S p50"] for r in per_run if r[variant].get("t-S p50") is not None]))))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
