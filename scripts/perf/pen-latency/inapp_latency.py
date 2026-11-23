#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Split the pen-to-display delay of runs into the part the application can
see and the part after the buffer swap.

A run directory needs probe.csv (with stroke_begin, an arrival event
stroke_paint, qt_tablet_move or qt_mouse_move, stroke_dirty_issued,
paintgl_begin, frame_swapped and projection_update), inject.csv and
capture.csv. For every stroke sample k the script finds

  I  the injection time of k (inject.csv; the pressed rows after the press
     whose position changed, because a pen held still sends no event),
  A  the time the application received k (the k-th arrival event of the
     stroke),
  D  the first stroke_dirty_issued that covers k (the newest painted point
     of that update is k or a later sample; its event time, in milliseconds
     since the press, selects the sample),
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
milliseconds since the press) is compared with I minus the press time: the
spread of the difference shows how well the operating system's event time
follows the real input time.

Usage: inapp_latency.py [--macos-clocks] RUN_DIR [RUN_DIR ...]

On macOS the injection and the capture use CLOCK_UPTIME_RAW while the probe
uses CLOCK_MONOTONIC_RAW, which also counts the time asleep. --macos-clocks
subtracts the current difference of the two clocks from the probe times; it
is valid for runs made since the machine last woke up.
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


def read_strokes(directory):
    """Pressed rows of each stroke in order: [(time_ns, x, y), ...]."""
    strokes = {}
    with open(os.path.join(directory, "inject.csv"), encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if row["pressed"] == "1" and row["band"].startswith("stroke"):
                strokes.setdefault(row["band"], []).append(
                    (int(row["time_ns"]), float(row["x"]), float(row["y"])))
    return [strokes[key] for key in sorted(strokes)]


def moving_rows(rows):
    """The press time and the rows after the press whose position changed,
    as [(time_ns, x)].

    Rows that repeat the previous position (a pen held still) produce no
    input event, so they have no arrival to match."""
    press = rows[0][0]
    moving = []
    last = rows[0][1:]
    for t, x, y in rows[1:]:
        if (x, y) != last:
            moving.append((t, x))
        last = (x, y)
    return press, moving


def pair_events(moving, events):
    """Pair arrival events [(time_ns, x)] with moving rows [(time_ns, x)].

    With equal counts the pairing follows the order. When the system merged
    or dropped events, each event goes to the row whose position matches
    through a line fitted from the first and last positions, refined by least
    squares. Returns [(row index, event time)] or None if the fit is poor."""
    if len(events) == len(moving):
        return [(i, e[0]) for i, e in enumerate(events)]
    rx = [x for _, x in moving]
    ex = [x for _, x in events]
    if len(ex) < 3 or rx[-1] == rx[0]:
        return None
    a = (ex[-1] - ex[0]) / (rx[-1] - rx[0])
    b = ex[0] - a * rx[0]
    step = abs(a) * abs(rx[-1] - rx[0]) / max(1, len(rx) - 1)
    pairs = []
    for _ in range(3):
        pairs = []
        for j, x in enumerate(ex):
            i = min(range(len(rx)), key=lambda i: abs(a * rx[i] + b - x))
            pairs.append((i, j))
        n = len(pairs)
        mx = sum(rx[i] for i, _ in pairs) / n
        my = sum(ex[j] for _, j in pairs) / n
        var = sum((rx[i] - mx) ** 2 for i, _ in pairs)
        if var == 0:
            return None
        a = sum((rx[i] - mx) * (ex[j] - my) for i, j in pairs) / var
        b = my - a * mx
    residual = [abs(a * rx[i] + b - ex[j]) for i, j in pairs]
    if statistics.median(residual) > 0.25 * step:
        return None
    seen, out = set(), []
    for (i, j), r in zip(pairs, residual):
        if r <= 0.6 * step and i not in seen:
            seen.add(i)
            out.append((i, events[j][0]))
    return out


ARRIVAL_EVENTS = ("stroke_paint", "qt_tablet_move", "qt_mouse_move")
MATCH_TOLERANCE_MS = 2.5


def analyze(directory, probe_offset_ns=0):
    """probe_offset_ns is the probe clock minus the injection clock; it is
    subtracted from every probe time (0 when both clocks are the same)."""
    rows = [(t - probe_offset_ns,) + row[1:] for row in read_probe(os.path.join(directory, "probe.csv"))
            for t in (row[0],)]
    begins_stroke = [t for t, e, *_ in rows if e == "stroke_begin"]
    dirties = [(t, ex) for t, e, _, _, ex in rows if e == "stroke_dirty_issued"]
    begins = [t for t, e, *_ in rows if e == "paintgl_begin"]
    swaps = [t for t, e, *_ in rows if e == "frame_swapped"]
    projections = [t for t, e, *_ in rows if e == "projection_update"]
    if not dirties:
        raise SystemExit("%s: probe.csv has no stroke_dirty_issued (older build)" % directory)

    strokes = [moving_rows(rows_) for rows_ in read_strokes(directory)]
    if len(begins_stroke) != len(strokes):
        raise SystemExit("%s: %d stroke_begin rows against %d injected strokes"
                         % (directory, len(begins_stroke), len(strokes)))
    arrival_name = None
    for name in ARRIVAL_EVENTS:
        if any(e == name for _, e, *_ in rows):
            arrival_name = name
            break
    if arrival_name is None:
        raise SystemExit("%s: probe.csv has none of %s" % (directory, ", ".join(ARRIVAL_EVENTS)))

    # Per stroke: the injection time of every moving row, its arrival at the
    # application, and the stroke's press time (the origin of the event time).
    inject, arrive, press_of, stroke_of, dropped = [], [], [], [], []
    window_end = begins_stroke[1:] + [None]
    for s, ((press, moving), begin, end) in enumerate(zip(strokes, begins_stroke, window_end)):
        limit = moving[-1][0] + 300e6 if moving else begin
        events = [(t, float(x)) for t, e, x, _, _ in rows
                  if e == arrival_name and begin <= t < (end if end is not None else limit)
                  and t <= limit]
        paired = pair_events(moving, events)
        if paired is None:
            raise SystemExit("%s: stroke %d has %d %s events against %d moving rows and the positions do not fit"
                             % (directory, s, len(events), arrival_name, len(moving)))
        dropped.append(len(moving) - len(paired))
        for i, t_arrive in paired:
            inject.append(moving[i][0])
            arrive.append(t_arrive)
            press_of.append(press)
            stroke_of.append(s)
    n = len(inject)

    def newest_sample(dirty_time, extra, s):
        """The moving row of stroke s whose event time (ms since the press)
        is nearest to extra, if it had arrived before the dirty mark."""
        try:
            wanted = float(extra)
        except ValueError:
            return None
        best, best_gap = None, MATCH_TOLERANCE_MS
        for k in range(n):
            if stroke_of[k] != s:
                continue
            gap = abs((inject[k] - press_of[k]) / 1e6 - wanted)
            if gap <= best_gap and arrive[k] <= dirty_time:
                best, best_gap = k, gap
        return best

    newest = []  # (dirty time, newest k covered, extra)
    for t, ex in dirties:
        s = bisect.bisect_right(begins_stroke, t) - 1
        if s < 0:
            continue
        k = newest_sample(t, ex, s)
        if k is not None:
            newest.append((t, k, ex))
    running = -1
    firsts = {}
    for t, k, _ in newest:
        if k > running:
            for covered in range(running + 1, k + 1):
                if stroke_of[covered] == stroke_of[k]:
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

    # The first stroke_begin follows the first press by the delivery delay of
    # the press; a value outside 0 to 100 ms means the clocks are not aligned.
    press_to_begin = [(b - st[0]) / 1e6 for b, st in zip(begins_stroke, strokes)]
    result = {"press to begin p50": median(press_to_begin)}
    result.update({"samples": n, "rows without event": sum(dropped), "arrival event": arrival_name, "dirty matched": len(newest), "dirty rows": len(dirties)})
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
            s_minus_a.append((swap - arrive[k]) / 1e6)
        sample_swap[variant] = table
        result[variant] = {
            "S-I p50": median(s_minus_i), "S-I p90": percentile(s_minus_i, 0.9),
            "S-A p50": median(s_minus_a), "S-A p90": percentile(s_minus_a, 0.9),
            "samples": len(table),
        }
    a_minus_i = [(arrive[k] - inject[k]) / 1e6 for k in range(n)]
    result["A-I p50"] = median(a_minus_i)
    result["A-I p90"] = percentile(a_minus_i, 0.9)

    # Event time of the OS (since the press) against the injection time.
    diffs = []
    for t, k, ex in newest:
        diffs.append(float(ex) - (inject[k] - press_of[k]) / 1e6)
    if diffs:
        base = median(diffs)
        dev = [abs(d - base) for d in diffs]
        result["os_event_time"] = {"n": len(diffs), "dev p50 ms": median(dev), "dev p90 ms": percentile(dev, 0.9)}

    # After the swap: capture frame time minus the swap that showed the displayed sample.
    _, _, _, samples = capture_analyze.frame_samples(directory)
    after = {"direct": [], "projection": []}
    raw = {"t-D": [], "t-S": [], "t-A": []}
    capture_ms = []
    order = sorted(range(n), key=lambda k: inject[k])
    sorted_inject = [inject[k] for k in order]
    for sample in samples:
        kind, t, crossed = sample[0], sample[1], sample[2]
        if kind != "ink":
            continue
        capture_ms.append((t - crossed) / 1e6)
        i = bisect.bisect_left(sorted_inject, crossed)
        near = [j for j in (i - 1, i) if 0 <= j < len(order)]
        if not near:
            continue
        j = min(near, key=lambda j: abs(sorted_inject[j] - crossed))
        if abs(sorted_inject[j] - crossed) > MATCH_TOLERANCE_MS * 1e6:
            continue
        k = order[j]
        raw["t-A"].append((t - arrive[k]) / 1e6)
        if k in firsts:
            raw["t-D"].append((t - firsts[k]) / 1e6)
        if k in sample_swap["direct"]:
            raw["t-S"].append((t - sample_swap["direct"][k]) / 1e6)
        for variant in after:
            swap = sample_swap[variant].get(k)
            if swap is not None and 0 <= t - swap < 200e6:
                after[variant].append((t - swap) / 1e6)
    result["raw"] = {name: (len(v), percentile(v, 0.1), median(v), percentile(v, 0.9),
                            sum(1 for x in v if x < 0) / len(v) if v else None)
                     for name, v in raw.items()}
    result["capture p50"] = median(capture_ms)
    for variant in after:
        result[variant]["t-S p50"] = median(after[variant])
        result[variant]["t-S p90"] = percentile(after[variant], 0.9)
        result[variant]["t-S frames"] = len(after[variant])
    swap_intervals = [(b - a) / 1e6 for a, b in zip(swaps, swaps[1:]) if b - a < 100e6]
    result["swap interval p50"] = median(swap_intervals)
    return result


def fmt(value):
    return "-" if value is None else "%.1f" % value


def main(argv):
    args = argv[1:]
    offset_ns = 0
    if args and args[0] == "--macos-clocks":
        import time
        offset_ns = time.clock_gettime_ns(time.CLOCK_MONOTONIC_RAW) - time.clock_gettime_ns(time.CLOCK_UPTIME_RAW)
        args = args[1:]
    if not args:
        print(__doc__)
        return 2
    per_run = []
    for directory in args:
        r = analyze(directory, offset_ns)
        per_run.append(r)
        print(os.path.basename(directory))
        print("  %s: %d samples (%d rows without an event), %d of %d dirty rows matched, press to stroke_begin p50 %s ms"
              % (r["arrival event"], r["samples"], r["rows without event"], r["dirty matched"], r["dirty rows"], fmt(r["press to begin p50"])))
        print("  capture p50 %s ms, swap interval p50 %s ms, A-I p50 %s p90 %s"
              % (fmt(r["capture p50"]), fmt(r["swap interval p50"]), fmt(r["A-I p50"]), fmt(r["A-I p90"])))
        for variant in ("direct", "projection"):
            v = r[variant]
            print("  %-10s S-I p50 %s p90 %s | S-A p50 %s p90 %s | t-S p50 %s p90 %s (%s frames)"
                  % (variant, fmt(v["S-I p50"]), fmt(v["S-I p90"]), fmt(v["S-A p50"]), fmt(v["S-A p90"]),
                     fmt(v.get("t-S p50")), fmt(v.get("t-S p90")), v.get("t-S frames")))
        for name, (count, p10, p50, p90, negative) in r["raw"].items():
            print("  frame-based %s: p10 %s p50 %s p90 %s ms, %s of %d frames negative"
                  % (name, fmt(p10), fmt(p50), fmt(p90), "-" if negative is None else "%.0f%%" % (negative * 100), count))
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
