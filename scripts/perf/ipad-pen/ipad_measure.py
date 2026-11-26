#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Mac-side steps of the iPad pen latency measurement (run-ipad.sh).

The iPad and the Mac keep separate clocks. The pen samples are timed on the
iPad (UITouch timestamps, which LibrePaint records as ios_pencil_sample rows of
its latency probe), the screen is captured on the Mac (ipad_capture.mm). The
flash calibration maps one clock to the other: the probe application turns its
window black or white at display link frames whose target display time it
logs on the iPad clock, and the capture sees each change in a frame whose
presentation time is on the Mac clock. The median of the differences is the
offset; it also absorbs the constant delay of the capture path, so a captured
frame's time minus the offset is the iPad display time of that frame.

Subcommands:
  offset FRAMES_CSV FLASH_CSV [--from-ns T] [--to-ns T]
      Print the offset in nanoseconds (Mac time minus iPad time) and check it,
      using the frames of the flash's time window on the Mac clock.
  calibration XCODEBUILD_LOG
      Print the affine map from native panel points to window points that the
      CAL lines of testCalibrate give, and the window size, as JSON.
  canvas FIRST_FRAME_PGM
      Print the white canvas rectangle in captured pixels as JSON.
  plan --calibration JSON --canvas JSON --capture-width W --bundle ID
       --options TRAJECTORY_JSON --plan-out plan.json --bands-out bands.txt
      Turn trajectory.py strokes inside the canvas into the stroke plan of
      testPlayPlan (native points) and the capture bands (captured pixels).
  inject --probe latency.csv --plan plan.json --calibration BEFORE.json
         --calibration AFTER.json --out inject.csv
      Write inject.csv for capture_analyze.py from the ios_pencil_sample rows:
      times on the Mac clock, positions in captured pixels. The offset of each
      sample is interpolated between the two calibrations by its iPad time.

Anomalies (too few flash pairs, a spread over one frame, no canvas, fewer
probe samples than planned) print ANOMALY lines on standard error and exit 1.
"""

import argparse
import csv
import json
import math
import os
import re
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TRAJECTORY = os.path.join(HERE, "..", "pen-latency", "trajectory.py")
MIN_FLASH_PAIRS = 40
MAX_FLASH_SPREAD_NS = 3_000_000  # within the on-time group
MODE_WIDTH_NS = 1_500_000
MIN_CLUSTER_PAIRS = 15
STROKE_GAP_S = 0.25  # pen samples further apart start a new stroke


def load_json(path):
    with open(path) as handle:
        return json.load(handle)


def anomaly(message):
    print("ANOMALY " + message, file=sys.stderr)
    sys.exit(1)


def flash_offset(frames_path, flash_path, from_ns=None, to_ns=None):
    with open(frames_path) as handle:
        frames = [r for r in csv.DictReader(handle)
                  if (from_ns is None or int(r["frame_ns"]) >= from_ns) and (to_ns is None or int(r["frame_ns"]) <= to_ns)]
    with open(flash_path) as handle:
        flashes = [(r["color"], float(r["target_s"])) for r in csv.DictReader(handle)]
    # The first frame of each new colour shows the change.
    changes = []
    previous = None
    for row in frames:
        luma = float(row["luma"])
        colour = "white" if luma > 200 else "black" if luma < 55 else None
        if colour and colour != previous:
            if previous is not None:
                changes.append((colour, int(row["frame_ns"])))
            previous = colour
    def pair(skip):
        result = []
        i = 0
        for colour, frame_ns in changes[skip:]:
            while i < len(flashes) and flashes[i][0] != colour:
                i += 1
            if i >= len(flashes):
                break
            result.append((flashes[i][1], frame_ns - round(flashes[i][1] * 1e9)))
            i += 1
        return result

    def clusters(pairs):
        """Groups of pairs whose offsets lie within MODE_WIDTH_NS of each other."""
        remaining = sorted(pairs, key=lambda p: p[1])
        groups = []
        while remaining:
            centre = max(remaining, key=lambda c: sum(1 for p in remaining if abs(p[1] - c[1]) <= MODE_WIDTH_NS))[1]
            group = [p for p in remaining if abs(p[1] - centre) <= MODE_WIDTH_NS]
            groups.append(group)
            remaining = [p for p in remaining if p not in group]
        return groups

    def on_time(pairs):
        """The earliest group with at least MIN_CLUSTER_PAIRS pairs."""
        big = [g for g in clusters(pairs) if len(g) >= MIN_CLUSTER_PAIRS]
        return min(big, key=lambda g: statistics.median(p[1] for p in g)) if big else []

    # Other screen changes (the switch between applications) can precede the
    # flashes, so the pairing tries every number of leading changes to skip
    # and keeps the one with the most pairs in the two main groups. The
    # flashes change every 13 frames of the 120 Hz panel and the capture
    # delivers 60 frames per second, so every other change falls between two
    # captured frames and shows one 120 Hz frame (8.3 ms) late. The offsets
    # form two groups 8.3 ms apart, and the earlier one is on time. Stray
    # pairs right after an application switch are dropped.
    def score(pairs):
        groups = sorted(clusters(pairs), key=len, reverse=True)
        return sum(len(g) for g in groups[:2])

    best = max((pair(skip) for skip in range(min(len(changes), 10))), key=score, default=[])
    lower = on_time(best)
    if len(lower) < MIN_CLUSTER_PAIRS:
        anomaly("flash calibration has no group of %d agreeing changes" % MIN_CLUSTER_PAIRS)
    values = sorted(o for _, o in lower)
    spread = values[-1] - values[0]
    if spread > MAX_FLASH_SPREAD_NS:
        anomaly("flash calibration spread %.2f ms over %.2f ms" % (spread / 1e6, MAX_FLASH_SPREAD_NS / 1e6))
    return int(statistics.median(values)), statistics.median(t for t, _ in lower), len(lower), spread



def calibration(log_path):
    with open(log_path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    size = re.search(r"PencilProbe app frame ([\d.]+)x([\d.]+)", text)
    pairs = re.findall(r"PencilProbe CAL point ([-\d.]+),([-\d.]+) .*?-> done \d+ events ended pencil x=([-\d.]+) y=([-\d.]+)", text)
    if not size or len(pairs) != 3:
        anomaly("calibration log has %d CAL points and %s window size" % (len(pairs), "a" if size else "no"))
    src = [(float(a), float(b)) for a, b, _, _ in pairs]
    dst = [(float(c), float(d)) for _, _, c, d in pairs]
    # Solve window = M * native + t from the three points.
    (x0, y0), (x1, y1), (x2, y2) = src
    det = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
    if abs(det) < 1e-6:
        anomaly("calibration points are collinear")

    def solve(values):
        v0, v1, v2 = values
        a = ((v1 - v0) * (y2 - y0) - (v2 - v0) * (y1 - y0)) / det
        b = ((x1 - x0) * (v2 - v0) - (x2 - x0) * (v1 - v0)) / det
        return a, b, v0 - a * x0 - b * y0

    ax, bx, cx = solve([d[0] for d in dst])
    ay, by, cy = solve([d[1] for d in dst])
    return {"window": [float(size.group(1)), float(size.group(2))], "to_window": [[ax, bx, cx], [ay, by, cy]]}


def to_native(cal, x, y):
    (ax, bx, cx), (ay, by, cy) = cal["to_window"]
    det = ax * by - bx * ay
    dx, dy = x - cx, y - cy
    return (by * dx - bx * dy) / det, (ax * dy - ay * dx) / det


def read_pgm(path):
    with open(path, "rb") as handle:
        data = handle.read()
    header = data.split(b"\n", 3)
    width, height = (int(v) for v in header[1].split())
    return width, height, header[3]


def canvas(path):
    """The largest block of near-white pixels: the white canvas."""
    width, height, pixels = read_pgm(path)
    white = [[pixels[y * width + x] > 240 for x in range(width)] for y in range(height)]
    # Rows and columns where most pixels are white, around the frame centre.
    def runs(flags):
        best, start = (0, 0, -1), None
        for i, flag in enumerate(flags + [False]):
            if flag and start is None:
                start = i
            elif not flag and start is not None:
                if i - start > best[0]:
                    best = (i - start, start, i - 1)
                start = None
        return best
    cy = height // 2
    _, left, right = runs(white[cy])
    cx = (left + right) // 2
    _, top, bottom = runs([white[y][cx] for y in range(height)])
    if right - left < width // 4 or bottom - top < height // 4:
        anomaly("no white canvas found in %s" % path)
    return {"x0": left, "x1": right, "y0": top, "y1": bottom, "width": width, "height": height}


def plan(args):
    cal = load_json(args.calibration)
    rect = load_json(args.canvas)
    scale = args.capture_width / cal["window"][0]  # captured pixels per window point
    options = json.loads(args.options)
    options["hover_passes"] = 0
    plan_csv = args.plan_out + ".csv"
    subprocess.run([sys.executable, "-I", TRAJECTORY, "--options", json.dumps(options), "--out", plan_csv], check=True)
    with open(plan_csv) as handle:
        bands_line = handle.readline()
        rows = list(csv.DictReader(handle))
    rate = float(options.get("rate", 240.0))
    left, top = rect["x0"] / scale, rect["y0"] / scale
    width, height = (rect["x1"] - rect["x0"]) / scale, (rect["y1"] - rect["y0"]) / scale
    strokes = []
    by_band = {}
    for r in rows:
        if r["pressed"] == "1":
            by_band.setdefault(r["band"], []).append(r)
    for band, samples in by_band.items():
        moving = [s for s in samples if s["phase"].startswith("move")]
        t_move = float(moving[-1]["time_s"]) - float(moving[0]["time_s"])
        dwell = (len(samples) - len(moving)) / 2.0 / rate
        y = top + float(samples[0]["y"]) * height
        x0 = left + float(samples[0]["x"]) * width
        x1 = left + float(samples[-1]["x"]) * width
        strokes.append({"band": band, "window_from": [x0, y], "window_to": [x1, y],
                        "from": list(to_native(cal, x0, y)), "to": list(to_native(cal, x1, y)),
                        "seconds": t_move, "dwell": dwell})
    document = {"bundle": args.bundle, "rate": rate, "gap": 1.0, "pressure": args.pressure,
                "scale": scale, "strokes": strokes}
    with open(args.plan_out, "w") as handle:
        json.dump(document, handle, indent=1)
    # One band per stroke row: the stroke height of trajectory.py's band.
    with open(args.bands_out, "w") as handle:
        for entry in bands_line.split("=", 1)[1].strip().split(";"):
            name, _, _, b0, b1 = entry.split(":")
            if not name.startswith("stroke"):
                continue
            handle.write("%s %d %d %d %d\n" % (name, rect["x0"], rect["x1"],
                                              round(rect["y0"] + float(b0) * (rect["y1"] - rect["y0"])),
                                              round(rect["y0"] + float(b1) * (rect["y1"] - rect["y0"]))))


# The two clocks differ in rate by tens of parts per million, and the capture
# corrects its times in steps of about 0.8 ms; a larger rate means a bad pairing.
MAX_DRIFT_PPM = 200


def inject(args):
    points = sorted((c["ipad_time_s"], c["offset_ns"]) for c in (load_json(path) for path in args.calibration))
    (t0, o0), (t1, o1) = points[0], points[-1]
    slope = (o1 - o0) / (t1 - t0) if t1 > t0 else 0.0  # nanoseconds per second
    if abs(slope) / 1e3 > MAX_DRIFT_PPM:
        anomaly("clock offset moved %.2f ms in %.1f s (%.0f ppm)" % ((o1 - o0) / 1e6, t1 - t0, slope / 1e3))

    def offset_at(t_s):
        return round(o0 + slope * (t_s - t0))
    plan_doc = load_json(args.plan)
    samples = []
    with open(args.probe) as handle:
        lines = [line for line in handle if not line.startswith("#")]
    for r in csv.DictReader(lines):
        if r["event"] == "ios_pencil_sample":
            samples.append((int(r["extra"]), float(r["x"]), float(r["y"])))
    samples.sort()
    strokes = []
    for sample in samples:
        if not strokes or sample[0] - strokes[-1][-1][0] > STROKE_GAP_S * 1e6:
            strokes.append([])
        strokes[-1].append(sample)
    if len(strokes) != len(plan_doc["strokes"]):
        anomaly("probe has %d strokes, the plan %d" % (len(strokes), len(plan_doc["strokes"])))
    scale = plan_doc["scale"]
    with open(args.out, "w") as handle:
        handle.write("time_ns,phase,band,x,y,pressed\n")
        for planned, stroke in zip(plan_doc["strokes"], strokes):
            expected = round((planned["seconds"] + 2 * planned["dwell"]) * plan_doc["rate"])
            if len(stroke) < 0.9 * expected:
                anomaly("%s has %d probe samples, %d planned" % (planned["band"], len(stroke), expected))
            start = (stroke[0][1], stroke[0][2])
            end = (stroke[-1][1], stroke[-1][2])
            for t_us, x, y in stroke:
                still = (x, y) == start or (x, y) == end
                phase = "dwell" if still else "move"
                handle.write("%d,%s,%s,%.2f,%.2f,1\n" % (t_us * 1000 + offset_at(t_us / 1e6), phase, planned["band"], x * scale, y * scale))
            t_us, x, y = stroke[-1]
            handle.write("%d,release,%s,%.2f,%.2f,0\n" % (t_us * 1000 + offset_at(t_us / 1e6) + 4_000_000, planned["band"], x * scale, y * scale))


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("offset")
    p.add_argument("frames")
    p.add_argument("flash")
    p.add_argument("--from-ns", type=int, help="ignore frames before this Mac time")
    p.add_argument("--to-ns", type=int, help="ignore frames after this Mac time")
    p = sub.add_parser("calibration")
    p.add_argument("log")
    p = sub.add_parser("canvas")
    p.add_argument("pgm")
    p = sub.add_parser("plan")
    p.add_argument("--calibration", required=True)
    p.add_argument("--canvas", required=True)
    p.add_argument("--capture-width", type=float, required=True)
    p.add_argument("--bundle", required=True)
    p.add_argument("--options", default="{}")
    p.add_argument("--pressure", type=float, default=350.0)
    p.add_argument("--plan-out", required=True)
    p.add_argument("--bands-out", required=True)
    p = sub.add_parser("inject")
    p.add_argument("--probe", required=True)
    p.add_argument("--plan", required=True)
    p.add_argument("--calibration", action="append", required=True, help="output of the offset subcommand; give the one before and the one after the strokes")
    p.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    if args.command == "offset":
        offset, time_s, pairs, spread = flash_offset(args.frames, args.flash, args.from_ns, args.to_ns)
        print(json.dumps({"offset_ns": offset, "ipad_time_s": time_s, "pairs": pairs, "spread_ms": spread / 1e6}))
    elif args.command == "calibration":
        print(json.dumps(calibration(args.log)))
    elif args.command == "canvas":
        print(json.dumps(canvas(args.pgm)))
    elif args.command == "plan":
        plan(args)
    elif args.command == "inject":
        inject(args)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
