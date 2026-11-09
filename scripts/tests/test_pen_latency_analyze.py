# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/analyze.py.

The L0 pen-latency gate compares run medians of the input-to-display time.
These tests fix how one displayed frame is attributed to the pen input that
it reflects, so a change in the attribution shows up as a test failure rather
than as an apparent latency change.
"""

import importlib.util
import os
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location(
    "pen_latency_analyze", os.path.join(ROOT, "scripts", "perf", "pen-latency", "analyze.py")
)
analyze = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(analyze)

TICKS_PER_MS = 10_000  # QueryPerformanceCounter at 10 MHz
WM_POINTERUPDATE = 0x0245
INRANGE = 0x2
INCONTACT = 0x4

PRESENTMON_HEADER = "Application,ProcessID,TimeInQPC,MsUntilDisplayed,CPUStartQPC,MsGPULatency,MsGPUBusy\n"


def ms(value):
    return int(value * TICKS_PER_MS)


def write_run(directory, probe_rows, presents):
    with open(os.path.join(directory, "probe.csv"), "w") as handle:
        handle.write("# frequency=%d\n" % (TICKS_PER_MS * 1000))
        handle.write("time,event,x,y,extra,thread\n")
        for time, event, extra in probe_rows:
            handle.write("%d,%s,0.00,0.00,%d,1\n" % (time, event, extra))
    with open(os.path.join(directory, "presentmon.csv"), "w") as handle:
        handle.write(PRESENTMON_HEADER)
        for present, until_displayed in presents:
            handle.write("LibrePaint.exe,1,%d,%s,%d,1.0,0.5\n" % (present, until_displayed, present - ms(10)))


def pointer(flags):
    return (WM_POINTERUPDATE << 32) | flags


class FrameAttributionTest(unittest.TestCase):
    def analyze(self, probe_rows, presents):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, probe_rows, presents)
            return analyze.Run(directory).analyze()

    def test_hover_frame_starts_at_the_pointer_message_before_the_outline_request(self):
        base = ms(1000)
        result = self.analyze(
            [
                (base, "native_pointer", pointer(INRANGE)),
                (base + ms(1), "native_pointer", pointer(INRANGE)),
                (base + ms(2), "outline_request", 0),
                (base + ms(3), "native_pointer", pointer(INRANGE)),  # arrives after the request
                (base + ms(4), "paintgl_begin", 0),
                (base + ms(5), "paintgl_end", 0),
            ],
            [(base + ms(6), "20.0")],
        )
        # Display at 6 + 20 ms, newest input before the request at 1 ms.
        self.assertAlmostEqual(result["hover.input_to_display"]["p50"], 25.0)
        self.assertAlmostEqual(result["hover.outline_request_to_paint_begin"]["p50"], 2.0)
        self.assertAlmostEqual(result["hover.paint_end_to_present"]["p50"], 1.0)
        self.assertNotIn("contact.input_to_display", result)

    def test_ink_frame_starts_at_the_pointer_message_before_the_newest_stroke_job(self):
        base = ms(1000)
        touching = pointer(INRANGE | INCONTACT)
        result = self.analyze(
            [
                (base, "native_pointer", touching),
                (base + ms(1), "stroke_paint", 0),
                (base + ms(2), "native_pointer", touching),
                (base + ms(3), "outline_request", 0),
                (base + ms(4), "projection_update", 0),
                (base + ms(5), "paintgl_begin", 0),
                (base + ms(6), "paintgl_end", 0),
            ],
            [(base + ms(8), "30.0")],
        )
        # Display at 38 ms; the stroke job at 1 ms came from the input at 0 ms.
        self.assertAlmostEqual(result["contact.ink_input_to_display"]["p50"], 38.0)
        self.assertAlmostEqual(result["contact.input_to_display"]["p50"], 36.0)

    def test_frames_that_were_never_displayed_are_excluded(self):
        base = ms(1000)
        result = self.analyze(
            [
                (base, "native_pointer", pointer(INRANGE)),
                (base + ms(1), "outline_request", 0),
                (base + ms(2), "paintgl_begin", 0),
            ],
            [(base + ms(3), "NA")],
        )
        self.assertNotIn("hover.input_to_display", result)
        self.assertEqual(result["presents"], {"n": 1, "displayed": 0})


class RunComparisonTest(unittest.TestCase):
    def test_spread_is_range_of_run_medians_over_their_median(self):
        results = {
            "a": {"hover.input_to_display": {"n": 10, "p50": 50.0}},
            "b": {"hover.input_to_display": {"n": 10, "p50": 52.0}},
            "c": {"hover.input_to_display": {"n": 10, "p50": 51.0}},
        }
        entry = analyze.compare(results)["hover.input_to_display"]
        self.assertAlmostEqual(entry["median_of_medians"], 51.0)
        self.assertAlmostEqual(entry["spread"], 2.0 / 51.0)


if __name__ == "__main__":
    unittest.main()
