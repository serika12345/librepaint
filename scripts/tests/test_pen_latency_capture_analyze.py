# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/capture_analyze.py.

The capture-based measurement compares LibrePaint across operating systems.
These tests replay the shared trajectory against a simulated display with a
known delay and check that the analysis recovers that delay for the hover
outline and for the ink, independent of the drawing's offset from the
pointer.
"""

import csv
import contextlib
import importlib.util
import io
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
presentation_check = load("presentation_check")

RECT = (100, 100, 1000, 600)
OPTIONS = {"hover_passes": 2, "strokes": 2}
FRAME_NS = 16_666_667
HOVER_OFFSET = 3
INK_OFFSET = 20


def simulate(directory, delay_ns, arrival_delay_ns=1_000_000, options=None):
    options = options or OPTIONS
    samples = list(trajectory.samples(*RECT, options))
    bands = trajectory.bands(RECT[1], RECT[3], trajectory.options(options))
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
            rows.append((frame, frame + arrival_delay_ns, name, xmin, xmax,
                         "" if dense == "" else dense - 200, dense, "", ""))
        frame += FRAME_NS
    with open(os.path.join(directory, "capture.csv"), "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["frame_ns", "arrival_ns", "band", "xmin", "xmax", "dense_xmin", "dense_xmax", "ymin", "ymax"])
        writer.writerows(rows)


def write_probe(directory, intervals_ns):
    """Write Qt tablet moves with the given gaps; frequency is 10 MHz."""
    with open(os.path.join(directory, "probe.csv"), "w") as handle:
        handle.write("# frequency=10000000\n")
        handle.write("time,event,x,y,extra,thread\n")
        time = 1_000_000_000
        for gap in intervals_ns:
            handle.write("%d,qt_tablet_move,0.00,0.00,0,1\n" % time)
            time += gap // 100  # the probe clock is 10 MHz


def write_frame_swaps(directory, interval_ns=FRAME_NS):
    """Append frame_swapped events over the injected span (probe clock 10 MHz)."""
    with open(os.path.join(directory, "inject.csv")) as handle:
        times = [int(row["time_ns"]) for row in csv.DictReader(handle)]
    begin, end = min(times), max(times)
    path = os.path.join(directory, "probe.csv")
    if not os.path.exists(path):
        with open(path, "w") as handle:
            handle.write("# frequency=10000000\n")
            handle.write("time,event,x,y,extra,thread\n")
    with open(path, "a") as handle:
        time = begin
        while time <= end:
            handle.write("%d,frame_swapped,0.00,0.00,0,1\n" % (time // 100))
            time += interval_ns


class HoverStateTest(unittest.TestCase):
    def test_indicators_of_the_two_states_are_recognised(self):
        self.assertEqual(capture_analyze.classify_hover_state(11.2, 3.1)[0], "low")
        self.assertEqual(capture_analyze.classify_hover_state(9.6, 0.8)[0], "high")

    def test_disagreeing_or_between_values_are_unknown(self):
        self.assertEqual(capture_analyze.classify_hover_state(11.2, 0.8)[0], "unknown")
        self.assertEqual(capture_analyze.classify_hover_state(10.5, None)[0], "unknown")

    def test_run_reports_state_and_presentmon(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000, arrival_delay_ns=11_200_000)
            write_probe(directory, [4_170_000] * 100)
            low = capture_analyze.analyze_run(directory)
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000, arrival_delay_ns=9_600_000)
            write_probe(directory, [200_000, 8_150_000] * 50)
            high = capture_analyze.analyze_run(directory)
        self.assertEqual(low["state"]["label"], "low")
        self.assertEqual(high["state"]["label"], "high")
        self.assertFalse(low["presentmon"]["present"])
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            open(os.path.join(directory, "presentmon.csv"), "w").close()
            self.assertTrue(capture_analyze.analyze_run(directory)["presentmon"]["present"])

    def test_a_presentmon_log_covering_only_the_start_is_partial(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            write_probe(directory, [4_170_000] * 100)
            with open(os.path.join(directory, "presentmon.csv"), "w") as handle:
                handle.write("Application,TimeInQPC\n")
                handle.write("LibrePaint.exe,10000000\n")
                handle.write("LibrePaint.exe,10010000\n")
            result = capture_analyze.analyze_run(directory)
        self.assertEqual(result["presentmon"]["status"], "partial")
        self.assertLess(result["presentmon"]["coverage"], 0.05)


class ForegroundStateTest(unittest.TestCase):
    def analyze(self, meta_lines, swap_interval_ns=FRAME_NS):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            if meta_lines is not None:
                with open(os.path.join(directory, "meta.txt"), "w") as handle:
                    handle.write("\n".join(meta_lines) + "\n")
            if swap_interval_ns:
                write_frame_swaps(directory, swap_interval_ns)
            return capture_analyze.analyze_run(directory)

    def test_a_run_whose_app_was_not_in_front_and_stopped_presenting_is_excluded(self):
        result = self.analyze(["app_foreground_capture=False"], swap_interval_ns=50_000_000)
        self.assertEqual(result["foreground"], {"known": True, "foreground": False})
        self.assertLess(result["presentation"]["ratio"], 0.5)
        self.assertIn("presented at 33% of the display rate", result["excluded"])

    def test_a_covered_window_that_still_presents_at_the_display_rate_stays(self):
        # 2026-11-10 14:42: a shell surface held the focus in every run while
        # the application stayed visible and presented at the display rate.
        result = self.analyze(["app_foreground_capture=False"])
        self.assertGreater(result["presentation"]["ratio"], 0.9)
        self.assertIsNone(result["excluded"])

    def test_a_foreground_run_stays_usable(self):
        result = self.analyze(["app_foreground_capture=True"], swap_interval_ns=None)
        self.assertEqual(result["foreground"], {"known": True, "foreground": True})
        self.assertIsNone(result["excluded"])

    def test_a_run_without_the_line_stays_usable(self):
        result = self.analyze(None, swap_interval_ns=None)
        self.assertEqual(result["foreground"], {"known": False, "foreground": None})
        self.assertIsNone(result["excluded"])

    def test_a_not_foreground_run_without_a_probe_is_excluded(self):
        result = self.analyze(["app_foreground_capture=False"], swap_interval_ns=None)
        self.assertEqual(result["excluded"], "app not foreground")

    def test_the_foreground_at_inject_start_is_read_from_the_meta(self):
        # PenInject records the window in front right before the first sample,
        # after one retry. It separates "in front when the capture began" from
        # "still in front when the input began".
        result = self.analyze([
            "app_foreground_capture=True",
            "foreground_inject_start=pid=1788 name=LibrePaint visible=True "
            "same_window=True app=False retried=True",
        ], swap_interval_ns=None)
        self.assertEqual(result["foreground_inject"], {
            "known": True, "foreground": False, "retried": True,
            "pid": "1788", "name": "LibrePaint",
        })

    def test_a_run_without_the_inject_line_reports_unknown(self):
        result = self.analyze(["app_foreground_capture=True"], swap_interval_ns=None)
        self.assertEqual(result["foreground_inject"],
                         {"known": False, "foreground": None, "retried": None})

    def test_the_summary_shows_the_inject_foreground(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            with open(os.path.join(directory, "meta.txt"), "w") as handle:
                handle.write("app_foreground_capture=True\n")
                handle.write("foreground_inject_start=pid=1 name=explorer "
                             "visible=True same_window=False app=False retried=True\n")
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                capture_analyze.main([directory])
        text = output.getvalue()
        self.assertIn("foreground=ok", text)
        self.assertIn("inject-foreground=not-foreground", text)


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

    def test_a_line_that_changes_speed_keeps_the_time_mapping(self):
        options = {
            "hover_passes": 0,
            "strokes": 1,
            "speed_profiles": [[
                {"until": 0.5, "speed": 0.3, "label": "move_slow"},
                {"until": 1.0, "speed": 3.0, "label": "move_fast"},
            ]],
        }
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000, options=options)
            result = capture_analyze.analyze_run(directory)
        groups = result["ink.speed_groups"]
        self.assertGreaterEqual(len(groups), 2)
        self.assertLess(groups[0]["speed_px_ms"], 0.5)
        self.assertGreater(groups[-1]["speed_px_ms"], 2.5)
        for group in groups:
            self.assertGreater(group["n"], 2)
            self.assertGreaterEqual(group["input_to_capture"]["p05"], 48.5)
            self.assertLessEqual(group["input_to_capture"]["p95"], 55.5)
            # The pointer travelled its speed times the delay while the
            # display showed the older position.
            self.assertAlmostEqual(group["pen_tip_to_edge_px"]["p50"],
                                   group["input_to_capture"]["p50"] * group["speed_px_ms"], delta=1.0)
        for group in result["l2p.speed_groups"]:
            # The direct pen tip to line end distance uses the frame time,
            # so it also equals the speed times the display delay.
            self.assertAlmostEqual(group["l2p_signed_gap_px"]["p50"],
                                   group["speed_px_ms"] * 50.0, delta=2.0)
            self.assertAlmostEqual(group["l2p_signed_gap_ms"]["p50"],
                                   50.0, delta=1.0)
        changes = result["ink.speed_change"]
        self.assertEqual(len(changes), 1)
        self.assertLess(changes[0]["before_speed_px_ms"], 0.5)
        self.assertGreater(changes[0]["after_speed_px_ms"], 2.5)
        self.assertGreaterEqual(changes[0]["max_latency_ms"], 48.5)
        self.assertGreaterEqual(changes[0]["steady_latency_ms"], 48.5)
        self.assertIsNotNone(changes[0]["settle_ms"])
        self.assertLessEqual(changes[0]["settle_ms"], 50.0)
        # The distance just after the change belongs to the fast interval.
        self.assertGreater(changes[0]["l2p_signed_gap_px"]["n"], 2)
        self.assertAlmostEqual(changes[0]["l2p_signed_gap_px"]["p50"], 150.0, delta=2.0)
        self.assertAlmostEqual(changes[0]["l2p_signed_gap_ms"]["p50"], 50.0, delta=1.0)
        self.assertAlmostEqual(changes[0]["l2p_speed_change_overshoot_px"], 0.0)


class DisplayGapTest(unittest.TestCase):
    """Pen tip to displayed line end distance from synthetic frames and input.

    A constant 0.5 px/ms stroke shown 50 ms late leaves the line end 25 px
    behind the pen tip (50 ms of travel). The capture analysis and analyze.py
    both report that known distance, in pixels and in milliseconds.
    """

    def test_a_known_distance_appears_in_both_analyzers(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            result = capture_analyze.analyze_run(directory)
            # analyze.py needs the probe clock to read the time_ns injection.
            write_probe(directory, [4_170_000] * 10)
            run_result = load("analyze").Run(directory).analyze()
        for entry in (result, run_result):
            self.assertAlmostEqual(entry["l2p.signed_gap_px"]["p50"], 25.0, delta=0.5)
            self.assertAlmostEqual(entry["l2p.signed_gap_px"]["p95"], 25.5, delta=0.5)
            self.assertAlmostEqual(entry["l2p.signed_gap_ms"]["p50"], 50.0, delta=0.5)
            self.assertEqual(entry["l2p.overshoot_px"]["p50"], 0.0)
        group = result["l2p.speed_groups"][0]
        self.assertAlmostEqual(group["speed_px_ms"], 0.5, delta=0.05)
        self.assertAlmostEqual(group["l2p_signed_gap_px"]["p50"], 25.0, delta=0.5)
        self.assertAlmostEqual(group["l2p_signed_gap_ms"]["p50"], 50.0, delta=0.5)


class PresentationCheckTest(unittest.TestCase):
    def test_check_reads_the_capture_frames_and_reports_the_presented_delay(self):
        # presentation_check consumes capture_analyze.frame_samples, whose
        # tuple gained the pointer speed. A simulated run with a reference
        # log keeps that contract observable.
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 30_000_000)
            injected = capture_analyze.read_inject(os.path.join(directory, "inject.csv"))
            with open(os.path.join(directory, "reference.log"), "w") as handle:
                for index, (time_ns, *_rest) in enumerate(injected, start=1):
                    shown = time_ns + 12_000_000
                    handle.write("input,%d,0,0\n" % time_ns)
                    handle.write("commit,%d,%d,%d\n" % (time_ns, index, time_ns))
                    handle.write("presented,%d,%d,16666667,7\n" % (index, shown))
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                presentation_check.check(directory)
        text = output.getvalue()
        self.assertIn("presented=", text)
        self.assertRegex(text, r"inject_to_presented\s+p05=.*p50=\s*12\.0")


if __name__ == "__main__":
    unittest.main()
