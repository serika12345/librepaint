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


CAPTURE_FIELDS = ["frame_ns", "arrival_ns", "band", "xmin", "xmax",
                  "dense_xmin", "dense_xmax", "ymin", "ymax", "dense_gap"]


def write_csv(path, fields, rows):
    with open(path, "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(fields)
        writer.writerows(rows)


def sample(frame_ns=0, dense_gap=None, pass_id=0, direction=1, line_end=0.0,
           signed_gap=0.0, kind="ink"):
    """One frame sample in the 12-element shape of frame_samples()."""
    return (kind, frame_ns, frame_ns - 1_000_000, frame_ns, 0.5, 0.0, 0.5,
            signed_gap, dense_gap, pass_id, direction, line_end)


def write_settle_run(directory, settle_densities):
    """One ink pass with controlled line ends after its last injection.

    The calibration offset is 19.5 px (the dwell positions are 0.5 and 200.5
    and the displayed line ends are 20 and 220), so a dense_xmax of 230 is
    10.5 px ahead of the last injected x of 200.
    """
    t_end = 1_116_000_000
    inject = [
        (1_000_000_000, "dwell", "stroke0", 0.5, 300.0, 1),
        (1_010_000_000, "dwell", "stroke0", 0.5, 300.0, 1),
    ]
    inject += [(1_100_000_000 + i * 4_000_000, "move", "stroke0", x, 300.0, 1)
               for i, x in enumerate((0.0, 50.0, 100.0, 150.0, 200.0))]
    inject.append((1_300_000_000, "dwell", "stroke0", 200.5, 300.0, 1))
    write_csv(os.path.join(directory, "inject.csv"),
              ["time_ns", "phase", "band", "x", "y", "pressed"], inject)

    rows = [(1_000_000_000, 1_000_000_000, "stroke0", 0, 220, 0, 20, 310, 320, "")]
    rows += [(1_100_000_000 + i * 4_000_000, 1_100_000_000, "stroke0", 0, dense,
              max(0, dense - 40), dense, 310, 320, "")
             for i, dense in enumerate((20, 70, 120, 170))]
    rows += [(t_end + (i + 1) * 4_000_000, t_end, "stroke0", 0, dense,
              max(0, dense - 40), dense, 310, 320, "")
             for i, dense in enumerate(settle_densities)]
    rows.append((1_300_000_000, 1_300_000_000, "stroke0", 0, 220, 180, 220, 310, 320, ""))
    write_csv(os.path.join(directory, "capture.csv"), CAPTURE_FIELDS, rows)


def write_ahead_run(directory):
    """One ink pass whose displayed line end is ahead of the pen tip.

    The dwell positions are 0.5 and 200.5 with displayed line ends 20 and 220
    (calibration offset 19.5 px). In the move frames the line end is 50 px
    ahead of the injected position, as a prediction line would draw it; no
    Pass.locate crossing exists for these frames at the start and the end.
    """
    inject = [
        (1_000_000_000, "dwell", "stroke0", 0.5, 300.0, 1),
        (1_010_000_000, "dwell", "stroke0", 0.5, 300.0, 1),
    ]
    inject += [(1_100_000_000 + i * 4_000_000, "move", "stroke0", x, 300.0, 1)
               for i, x in enumerate((0.0, 50.0, 100.0, 150.0, 200.0))]
    inject.append((1_300_000_000, "dwell", "stroke0", 200.5, 300.0, 1))
    write_csv(os.path.join(directory, "inject.csv"),
              ["time_ns", "phase", "band", "x", "y", "pressed"], inject)
    rows = [(1_000_000_000, 1_000_000_000, "stroke0", 0, 220, 0, 20, 310, 320, "")]
    rows += [(1_100_000_000 + i * 4_000_000, 1_100_000_000, "stroke0", 0, dense,
              max(0, dense - 40), dense, 310, 320, "")
             for i, dense in enumerate((70, 120, 170, 220, 270))]
    rows.append((1_300_000_000, 1_300_000_000, "stroke0", 0, 220, 180, 220, 310, 320, ""))
    write_csv(os.path.join(directory, "capture.csv"), CAPTURE_FIELDS, rows)


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


class CollapsedIntervalTest(unittest.TestCase):
    """A short injected interval does not become a speed of its own.

    The injector can send two samples 0.2 ms apart when it catches up after a
    scheduling delay while the positions still advance one step. The 2026-11-11
    08:47 run showed one ink frame at 18.14 px/ms in a 0.9 px/ms trajectory
    for that reason. The speed then comes from the nearest normal pair, so the
    frame stays in the trajectory's speed group and the milliseconds distance
    keeps the same divisor as the rest of the line.
    """

    def test_a_collapsed_interval_stays_in_the_trajectory_speed_group(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            path = os.path.join(directory, "inject.csv")
            with open(path, newline="") as handle:
                rows = list(csv.DictReader(handle))
                fields = list(rows[0].keys())
            target = None
            for index in range(20, len(rows) - 20):
                row, previous = rows[index], rows[index - 1]
                if (row["phase"].startswith("move") and row["pressed"] == "1"
                        and previous["phase"].startswith("move") and previous["pressed"] == "1"
                        and row["band"] == previous["band"]):
                    target = index
                    break
            self.assertIsNotNone(target)
            rows[target]["time_ns"] = str(int(rows[target - 1]["time_ns"]) + 200_000)
            with open(path, "w", newline="") as handle:
                writer = csv.DictWriter(handle, fieldnames=fields)
                writer.writeheader()
                writer.writerows(rows)
            result = capture_analyze.analyze_run(directory)
        speeds = [group["speed_px_ms"] for group in result["ink.speed_groups"]]
        self.assertEqual(len(speeds), 1, speeds)
        self.assertAlmostEqual(speeds[0], 0.5, delta=0.1)
        for group in result["l2p.speed_groups"]:
            self.assertLess(group["speed_px_ms"], 1.0)
            self.assertAlmostEqual(group["l2p_signed_gap_ms"]["p50"], 50.0, delta=1.0)


class DenseGapColumnTest(unittest.TestCase):
    def test_read_capture_reads_dense_gap_and_accepts_rows_without_it(self):
        rows = [
            (100, 1, "ink", 10, 20, 10, 20, 5, 6, "0"),
            (200, 2, "ink", 10, 20, 10, 20, 5, 6, "7"),
            (300, 3, "ink", 10, 20, 10, 20, 5, 6, ""),
        ]
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "capture.csv")
            write_csv(path, CAPTURE_FIELDS, rows)
            frames = capture_analyze.read_capture(path)
        self.assertEqual(frames[100]["ink"]["dense_gap"], 0)
        self.assertEqual(frames[200]["ink"]["dense_gap"], 7)
        self.assertIsNone(frames[300]["ink"]["dense_gap"])

        old = [(400, 4, "ink", 10, 20, 10, 20, 5, 6),
               (500, 5, "ink", 10, 20, 10, 20, 5, 6)]
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "capture.csv")
            write_csv(path, CAPTURE_FIELDS[:9], old)
            frames = capture_analyze.read_capture(path)
        self.assertIsNone(frames[400]["ink"]["dense_gap"])
        self.assertIsNone(frames[500]["ink"]["dense_gap"])

    def test_frame_samples_have_twelve_fields_and_keep_the_existing_values(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            _, _, _, samples = capture_analyze.frame_samples(directory)
            result = capture_analyze.analyze_run(directory)
        self.assertTrue(samples)
        for entry in samples:
            self.assertEqual(len(entry), 12)
        ink = [entry for entry in samples if entry[0] == "ink"]
        self.assertTrue(ink)
        for entry in ink:
            self.assertIsNone(entry[8])  # simulate() writes the old 9 columns
            self.assertIsInstance(entry[9], int)
            self.assertIn(entry[10], (1, -1))
            self.assertIsNotNone(entry[11])
        self.assertAlmostEqual(result["l2p.signed_gap_px"]["p50"], 25.0, delta=0.5)
        self.assertAlmostEqual(result["l2p.signed_gap_ms"]["p50"], 50.0, delta=0.5)


class PredictionMetricTest(unittest.TestCase):
    def test_hole_px_is_the_dense_gap_distribution(self):
        samples = [sample(frame_ns=(i + 1) * 10, dense_gap=gap, line_end=100.0 + 10 * i)
                   for i, gap in enumerate((0, 0, 30))]
        result = capture_analyze.l2p_summary(samples)
        self.assertEqual(result["l2p.hole_px"]["n"], 3)
        self.assertEqual(result["l2p.hole_px"]["p50"], 0)
        self.assertEqual(result["l2p.hole_px"]["p95"], 30)

    def test_retract_px_is_the_backward_movement_of_the_line_end(self):
        forward = [sample(frame_ns=(i + 1) * 100, line_end=x)
                   for i, x in enumerate((100.0, 110.0, 105.0, 120.0))]
        result = capture_analyze.l2p_summary(forward)
        self.assertEqual(result["l2p.retract_px"]["n"], 3)
        self.assertEqual(result["l2p.retract_px"]["p50"], 0)
        self.assertEqual(result["l2p.retract_px"]["p95"], 5)

        backward = [sample(frame_ns=(i + 1) * 100, direction=-1, line_end=x)
                    for i, x in enumerate((200.0, 190.0, 195.0, 180.0))]
        result = capture_analyze.l2p_summary(backward)
        self.assertEqual(result["l2p.retract_px"]["n"], 3)
        self.assertEqual(result["l2p.retract_px"]["p50"], 0)
        self.assertEqual(result["l2p.retract_px"]["p95"], 5)

    def test_overshoot_is_measured_for_frames_ahead_of_the_pen_tip(self):
        with tempfile.TemporaryDirectory() as directory:
            write_ahead_run(directory)
            result = capture_analyze.analyze_run(directory)
        self.assertEqual(result["l2p.overshoot_px"]["n"], 5)
        self.assertAlmostEqual(result["l2p.overshoot_px"]["p50"], 50.5, delta=0.6)
        self.assertEqual(result["l2p.residual_px"]["p50"], 0.0)
        self.assertLess(result["l2p.signed_gap_px"]["p50"], -50.0)

    def test_settle_ms_waits_for_the_line_end_to_be_within_one_pixel(self):
        with tempfile.TemporaryDirectory() as directory:
            write_settle_run(directory, (230, 223, 220))
            result = capture_analyze.analyze_run(directory)
        self.assertEqual(result["l2p.settle_ms"]["n"], 1)
        self.assertEqual(result["l2p.settle_ms"]["p50"], 12.0)

    def test_settle_ms_is_the_first_qualifying_frame(self):
        with tempfile.TemporaryDirectory() as directory:
            write_settle_run(directory, (220, 223, 230))
            result = capture_analyze.analyze_run(directory)
        self.assertEqual(result["l2p.settle_ms"]["p50"], 4.0)

    def test_a_pass_that_does_not_settle_within_100_ms_is_not_counted(self):
        with tempfile.TemporaryDirectory() as directory:
            write_settle_run(directory, (260, 260, 260))
            result = capture_analyze.analyze_run(directory)
        self.assertNotIn("l2p.settle_ms", result)

    def test_an_old_capture_without_dense_gap_has_no_hole_px(self):
        with tempfile.TemporaryDirectory() as directory:
            simulate(directory, 50_000_000)
            result = capture_analyze.analyze_run(directory)
        self.assertNotIn("l2p.hole_px", result)
        self.assertIn("l2p.signed_gap_px", result)


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
