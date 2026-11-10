# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/analyze.py.

The L0 pen-latency gate compares run medians of the input-to-display time.
These tests fix how one displayed frame is attributed to the pen input that
it reflects, so a change in the attribution shows up as a test failure rather
than as an apparent latency change.
"""

import contextlib
import importlib.util
import io
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
        for row in probe_rows:
            time, event, extra = row[:3]
            x = row[3] if len(row) > 3 else 0.0
            handle.write("%d,%s,%.2f,0.00,%d,1\n" % (time, event, x, extra))
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


class PresentMonTest(unittest.TestCase):
    def test_a_run_without_presentmon_is_reported_and_left_out_of_the_comparison(self):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [(ms(1000), "native_pointer", pointer(INRANGE))], [])
            os.remove(os.path.join(directory, "presentmon.csv"))
            result = analyze.Run(directory).analyze()
        self.assertFalse(result["presentmon"]["present"])
        results = {
            "with": {"hover.input_to_display": {"n": 10, "p50": 50.0}, "presentmon": {"present": True}},
            "with2": {"hover.input_to_display": {"n": 10, "p50": 51.0}, "presentmon": {"present": True}},
            "without": {"hover.input_to_display": {"n": 10, "p50": 52.0}, "presentmon": {"present": False}},
        }
        self.assertEqual(analyze.compare(results)["hover.input_to_display"]["runs"], 2)

    def test_a_short_presentmon_recording_is_partial(self):
        with tempfile.TemporaryDirectory() as directory:
            with open(os.path.join(directory, "probe.csv"), "w") as handle:
                handle.write("# frequency=10000000\n")
                handle.write("time,event,x,y,extra,thread\n")
                handle.write("%d,native_pointer,0.00,0.00,%d,1\n" % (ms(1000), pointer(INRANGE)))
            with open(os.path.join(directory, "inject.csv"), "w") as handle:
                handle.write("time_ns,phase,band,x,y,pressed\n")
                handle.write("1000000000,move,hover,1.0,1.0,0\n")
                handle.write("2000000000,move,hover,2.0,1.0,0\n")
            write_run(directory, [], [(ms(1000), "20.0"), (ms(1020), "20.0")])
            result = analyze.Run(directory).analyze()
        self.assertEqual(result["presentmon"]["status"], "partial")
        self.assertAlmostEqual(result["presentmon"]["coverage"], 0.02, places=2)


class PresentMonCoverageComparisonTest(unittest.TestCase):
    def test_partial_runs_are_left_out_of_the_breakdown_comparison(self):
        results = {
            "a": {"hover.input_to_display": {"n": 10, "p50": 50.0},
                  "presentmon": {"present": True, "status": "present"}},
            "b": {"hover.input_to_display": {"n": 10, "p50": 51.0},
                  "presentmon": {"present": True, "status": "present"}},
            "c": {"hover.input_to_display": {"n": 10, "p50": 52.0},
                  "presentmon": {"present": True, "status": "partial"}},
        }
        entry = analyze.compare(results)["hover.input_to_display"]
        self.assertEqual(entry["runs"], 2)
        self.assertAlmostEqual(entry["median_of_medians"], 50.5)


class L1SignalTest(unittest.TestCase):
    """H6 (frame update throttle) signal from the probe log.

    The values reproduce the 2026-11-10 l1-h6-throttle runs: with the
    throttle in place 829 of 1967 frames deferred the update; with it
    disabled 1868 of 1968 frames painted without deferring.
    """

    def write_signal(self, directory, frames, completed, deferred, skipped):
        rows = []
        for index in range(frames):
            rows.append((ms(1000) + index, "last_frame_completed_on_gpu", 1 if index < completed else 0))
        rows.extend((ms(1000) + index, "qt_update_deferred", 0) for index in range(deferred))
        rows.extend((ms(1000) + index, "qt_update_deferred_skipped", 0) for index in range(skipped))
        write_run(directory, rows, [])

    def test_counts_come_from_the_probe_log(self):
        with tempfile.TemporaryDirectory() as base_dir, tempfile.TemporaryDirectory() as off_dir:
            self.write_signal(base_dir, 1967, 1138, 829, 0)
            self.write_signal(off_dir, 1968, 100, 0, 1868)
            base = analyze.Run(base_dir).analyze()
            disabled = analyze.Run(off_dir).analyze()
        self.assertEqual(base["l1_events"]["last_frame_completed_on_gpu"], 1967)
        self.assertEqual(base["l1_events"]["qt_update_deferred"], 829)
        self.assertEqual(base["l1_last_frame_completed"], {"n": 1967, "completed": 1138})
        self.assertEqual(disabled["l1_events"]["qt_update_deferred_skipped"], 1868)
        self.assertEqual(disabled["l1_events"]["qt_update_deferred"], 0)
        self.assertEqual(disabled["l1_last_frame_completed"], {"n": 1968, "completed": 100})

    def test_summary_prints_counts_instead_of_an_empty_statistic(self):
        with tempfile.TemporaryDirectory() as base_dir, tempfile.TemporaryDirectory() as off_dir:
            self.write_signal(base_dir, 1967, 1138, 829, 0)
            self.write_signal(off_dir, 1968, 100, 0, 1868)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(analyze.main([base_dir, off_dir]), 0)
        text = output.getvalue()
        self.assertIn("qt_update_deferred=829", text)
        self.assertIn("completed=1138/1967 (57.9%)", text)
        self.assertIn("qt_update_deferred_skipped=1868", text)
        self.assertIn("completed=100/1968 (5.1%)", text)
        self.assertNotIn("l1_events                                    n=0", text)


class L1MaxFrameLatencyTest(unittest.TestCase):
    """H1 (frames queued ahead) outcome from the probe log.

    A working call records the EGL context, the D3D11 device, the
    IDXGIDevice1 query, the requested latency and the value read back. A
    failed device lookup stops after l1_h1_device, and a run without the
    switch records nothing.
    """

    def write_outcome(self, directory, rows):
        write_run(directory, rows, [])

    def test_records_every_step_and_the_read_back(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_outcome(directory, [
                (ms(1000), "l1_h1_egl_context", 1),
                (ms(1001), "l1_h1_client_extensions", 3),
                (ms(1002), "l1_h1_display_extensions", 2),
                (ms(1003), "l1_h1_device_query_ext", 2),
                (ms(1004), "l1_h1_entry_points", 3),
                (ms(1005), "l1_h1_display_attrib", 1, 1),
                (ms(1006), "l1_h1_device_attrib", 1, 1),
                (ms(1007), "l1_h1_device", 1),
                (ms(1008), "l1_h1_query_interface", 0),
                (ms(1009), "l1_h1_set_max_frame_latency", 0, 1),
                (ms(1010), "l1_h1_get_max_frame_latency", 1, 1),
            ])
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(analyze.main([directory]), 0)
        self.assertEqual(result["l1_h1"], {
            "egl_context": 1,
            "client_extensions": 3,
            "display_extensions": 2,
            "device_query_ext": 2,
            "entry_points": 3,
            "display_attrib_result": 1,
            "display_attrib_device": 1,
            "device_attrib_result": 1,
            "device_attrib_device": 1,
            "device": 1,
            "query_interface_hresult": 0,
            "set_hresult": 0,
            "requested": 1,
            "readback": 1,
        })
        self.assertIn(
            "egl=1 client_ext=3 display_ext=2 device_query=2 entry=3 "
            "display_attrib=1/1 device_attrib=1/1 device=1 query_interface=0x00000000 "
            "set=0x00000000 requested=1 readback=1",
            output.getvalue())

    def test_failed_device_lookup_stops_before_the_set_call(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_outcome(directory, [
                (ms(1000), "l1_h1_egl_context", 0),
            ])
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                analyze.main([directory])
        self.assertEqual(result["l1_h1"], {
            "egl_context": 0,
            "client_extensions": None,
            "display_extensions": None,
            "device_query_ext": None,
            "entry_points": None,
            "display_attrib_result": None,
            "display_attrib_device": None,
            "device_attrib_result": None,
            "device_attrib_device": None,
            "device": None,
            "query_interface_hresult": None,
            "set_hresult": None,
            "requested": None,
            "readback": None,
        })
        self.assertIn("egl=0 client_ext=none display_ext=none device_query=none entry=none",
                      output.getvalue())

    def test_a_missing_device_query_extension_is_reported(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_outcome(directory, [
                (ms(1000), "l1_h1_egl_context", 1),
                (ms(1001), "l1_h1_client_extensions", 1),
                (ms(1002), "l1_h1_display_extensions", 1),
                (ms(1003), "l1_h1_device_query_ext", 1),
                (ms(1004), "l1_h1_entry_points", 3),
                (ms(1005), "l1_h1_display_attrib", 0, 0),
                (ms(1006), "l1_h1_device", 0),
            ])
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                analyze.main([directory])
        self.assertEqual(result["l1_h1"], {
            "egl_context": 1,
            "client_extensions": 1,
            "display_extensions": 1,
            "device_query_ext": 1,
            "entry_points": 3,
            "display_attrib_result": 0,
            "display_attrib_device": 0,
            "device_attrib_result": None,
            "device_attrib_device": None,
            "device": 0,
            "query_interface_hresult": None,
            "set_hresult": None,
            "requested": None,
            "readback": None,
        })
        self.assertIn("egl=1 client_ext=1 display_ext=1 device_query=1 entry=3 "
                      "display_attrib=0/0 device_attrib=none/none device=0",
                      output.getvalue())

    def test_the_newest_call_describes_the_run(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_outcome(directory, [
                (ms(1000), "l1_h1_egl_context", 1),
                (ms(1001), "l1_h1_device", 1),
                (ms(1002), "l1_h1_query_interface", 0),
                (ms(1003), "l1_h1_set_max_frame_latency", 0, 1),
                (ms(1004), "l1_h1_get_max_frame_latency", 1, 1),
                (ms(2000), "l1_h1_egl_context", 1),
                (ms(2001), "l1_h1_device", 1),
                (ms(2002), "l1_h1_query_interface", 0),
                (ms(2003), "l1_h1_set_max_frame_latency", -2147024891, 1),
                (ms(2004), "l1_h1_get_max_frame_latency", 2, 1),
            ])
            result = analyze.Run(directory).analyze()
        self.assertEqual(result["l1_h1"]["set_hresult"], -2147024891)
        self.assertEqual(result["l1_h1"]["readback"], 2)

    def test_a_run_without_the_switch_reports_no_h1_entry(self):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [(ms(1000), "paintgl_begin", 0)], [])
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                analyze.main([directory])
        self.assertNotIn("l1_h1", result)
        self.assertNotIn("l1_h1", output.getvalue())


class L1WaitForGpuTest(unittest.TestCase):
    """H2 (waiting for the display handoff) decisions from the probe log.

    l1_h2_state carries the decision in extra: bit 1 = the switch is on,
    bit 2 = the previous frame's fence exists, bit 4 = the fence had already
    completed, bit 8 = the fence was waited for, bit 16 = update compression
    deferred the paint. A deferred paint must be able to report a wait, which
    is the case the probe could not see before the wait moved ahead of the
    update-compression early return.
    """

    def test_waiting_and_already_complete_paints_are_counted(self):
        rows = [(ms(1000) + index, "l1_h2_state", 7) for index in range(60)]
        rows += [(ms(2000) + index, "l1_h2_state", 27) for index in range(40)]
        rows += [(ms(2000) + index, "gpu_wait_begin", 0) for index in range(40)]
        rows += [(ms(2000) + index, "gpu_wait_end", 1) for index in range(39)]
        rows += [(ms(2000) + index, "gpu_wait_end", 0) for index in range(39, 40)]
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, rows, [])
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(analyze.main([directory]), 0)
        self.assertEqual(result["l1_h2"], {
            "n": 100, "fence": 100, "complete": 60, "waited": 40, "deferred": 40,
            "wait_completed": 39,
        })
        self.assertEqual(result["gpu_wait.duration"]["n"], 40)
        self.assertIn("n=100 fence=100 complete=60 waited=40 deferred=40 wait_done=39",
                      output.getvalue())

    def test_a_run_without_the_switch_reports_no_h2_entry(self):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [(ms(1000), "paintgl_begin", 0)], [])
            result = analyze.Run(directory).analyze()
        self.assertNotIn("l1_h2", result)


class StrokeQueueTest(unittest.TestCase):
    """Time from queueing a stroke job to painting its dab.

    stroke_paint is recorded when the point enters the stroke queue;
    stroke_dab is recorded after the stroke thread has painted it. The delay
    is grouped by the pointer speed at the queueing time, so a fast stroke
    that backs up the queue is visible.
    """

    def test_queue_delay_is_paired_and_grouped_by_speed(self):
        base = ms(1000)
        rows = [
            (base + ms(50), "stroke_paint", 0),
            (base + ms(55), "stroke_dab", 0),
            (base + ms(150), "stroke_paint", 0),
            (base + ms(158), "stroke_dab", 0),
        ]
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, rows, [])
            with open(os.path.join(directory, "inject.csv"), "w") as handle:
                handle.write("time_ns,phase,band,x,y,pressed\n")
                # 0-100 ms: 1 px/ms. 100-200 ms: 4 px/ms.
                for index in range(51):
                    time = 1_000_000_000 + index * 4_000_000
                    x = index * 4.0 if time < 1_100_000_000 else 96.0 + (time - 1_100_000_000) / 1e6 * 4.0
                    handle.write("%d,move,stroke0,%.2f,0.00,1\n" % (time, x))
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(analyze.main([directory]), 0)
        self.assertEqual(result["contact.stroke_paint_to_dab"]["n"], 2)
        self.assertAlmostEqual(result["contact.stroke_paint_to_dab"]["p50"], 6.5)
        groups = result["contact.stroke_paint_to_dab_speed"]
        self.assertEqual(len(groups), 2)
        self.assertAlmostEqual(groups[0]["speed_px_ms"], 1.0, delta=0.1)
        self.assertAlmostEqual(groups[0]["p50"], 5.0)
        self.assertAlmostEqual(groups[1]["speed_px_ms"], 4.0, delta=0.2)
        self.assertAlmostEqual(groups[1]["p50"], 8.0)
        self.assertIn("contact.stroke_paint_to_dab_speed", output.getvalue())


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
