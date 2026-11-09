# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/capture_analyze.py.

The capture-based measurement compares LibrePaint across operating systems.
These tests replay the shared trajectory against a simulated display with a
known delay and check that the analysis recovers that delay for the hover
outline and for the ink, independent of the drawing's offset from the
pointer.
"""

import csv
import importlib.util
import os
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, "scripts", "perf", "pen-latency")
sys.path.insert(0, TOOLS)


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(TOOLS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


trajectory = load("trajectory")
capture_analyze = load("capture_analyze")

RECT = (100, 100, 1000, 600)
OPTIONS = {"hover_passes": 2, "strokes": 2}
FRAME_NS = 16_666_667
HOVER_OFFSET = 3
INK_OFFSET = 20


def simulate(directory, delay_ns):
    samples = list(trajectory.samples(*RECT, OPTIONS))
    bands = trajectory.bands(RECT[1], RECT[3], trajectory.options(OPTIONS))
    start = 1_000_000_000
    injected = [(start + int(t * 1e9), phase, band, x, y, pressed) for t, phase, band, x, y, pressed in samples]
    with open(os.path.join(directory, "inject.csv"), "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["time_ns", "phase", "band", "x", "y", "pressed"])
        writer.writerows((t, p, b, "%.2f" % x, "%.2f" % y, int(d)) for t, p, b, x, y, d in injected)

    ink = {name: None for name, _, _ in bands}  # rightmost inked x per band
    rows = []
    index = 0
    frame = start
    end = injected[-1][0] + 500_000_000
    shown = None
    while frame < end:
        # The frame shows the newest input that is at least delay_ns old.
        while index < len(injected) and injected[index][0] <= frame - delay_ns:
            shown = injected[index]
            if shown[5]:
                band = shown[2]
                ink[band] = max(ink[band] or 0, shown[3])
            index += 1
        for name, _, _ in bands:
            xmin = xmax = dense = ""
            if shown is not None and not shown[5] and shown[2] == name:
                center = shown[3] + HOVER_OFFSET
                xmin, xmax = int(round(center - 15)), int(round(center + 15))
            if ink[name] is not None:
                dense = int(round(ink[name] + INK_OFFSET))
                xmin = dense - 200 if xmin == "" else xmin
                xmax = dense if xmax == "" else max(xmax, dense)
            rows.append((frame, frame + 1_000_000, name, xmin, xmax, "" if dense == "" else dense - 200, dense, "", ""))
        frame += FRAME_NS
    with open(os.path.join(directory, "capture.csv"), "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["frame_ns", "arrival_ns", "band", "xmin", "xmax", "dense_xmin", "dense_xmax", "ymin", "ymax"])
        writer.writerows(rows)


class SimulatedDisplayTest(unittest.TestCase):
    def test_recovers_known_delay_for_outline_and_ink(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            result = capture_analyze.analyze_run(directory)
        # A frame shows an input between 50 ms and 50 ms + one injection step
        # (4.2 ms) old; rounding positions to whole pixels adds up to 1.5 ms at
        # the trajectory speed of 500 px/s, on both the position and the calibration.
        for metric in ("hover.input_to_capture", "ink.input_to_capture"):
            self.assertGreater(result[metric]["n"], 50, metric)
            self.assertGreaterEqual(result[metric]["p05"], 48.5, metric)
            self.assertLessEqual(result[metric]["p95"], 55.5, metric)
            self.assertTrue(49.0 <= result[metric]["p50"] <= 54.5, metric)
        self.assertAlmostEqual(result["calibration"]["hover"]["offset_px"], HOVER_OFFSET, delta=1)
        self.assertAlmostEqual(result["calibration"]["ink"]["offset_px"], INK_OFFSET, delta=1)


if __name__ == "__main__":
    unittest.main()
