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


def write_run(directory, probe_rows, presents, modes=None):
    with open(os.path.join(directory, "probe.csv"), "w") as handle:
        handle.write("# frequency=%d\n" % (TICKS_PER_MS * 1000))
        handle.write("time,event,x,y,extra,thread\n")
        for row in probe_rows:
            time, event, extra = row[:3]
            x = row[3] if len(row) > 3 else 0.0
            handle.write("%d,%s,%.2f,0.00,%d,1\n" % (time, event, x, extra))
    with open(os.path.join(directory, "presentmon.csv"), "w") as handle:
        if modes is None:
            handle.write(PRESENTMON_HEADER)
        else:
            handle.write(PRESENTMON_HEADER.rstrip("\n") + ",PresentMode\n")
        for index, (present, until_displayed) in enumerate(presents):
            line = "LibrePaint.exe,1,%d,%s,%d,1.0,0.5" % (
                present, until_displayed, present - ms(10))
            if modes is not None:
                line += ",%s" % modes[index]
            handle.write(line + "\n")


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


class PaintCycleTest(unittest.TestCase):
    def test_paint_cadence_display_period_phase_and_present_mode(self):
        base = ms(1000)
        with tempfile.TemporaryDirectory() as directory:
            write_run(
                directory,
                [
                    (base + ms(15), "paintgl_begin", 0),
                    (base + ms(31.68), "paintgl_begin", 0),
                    (base + ms(48.36), "paintgl_begin", 0),
                ],
                [(base, "10.0"), (base + ms(16.68), "10.0"), (base + ms(33.36), "10.0")],
                modes=["Composed: Copy with GPU GDI",
                       "Composed: Copy with GPU GDI",
                       "Hardware Composed: Independent Flip"],
            )
            result = analyze.Run(directory).analyze()

        cycle = result["paint_cycle"]
        self.assertAlmostEqual(cycle["display_period_ms"], 16.68, places=2)
        self.assertAlmostEqual(cycle["paint_interval"]["p50"], 16.68, places=2)
        # Only the paints between the first and last display change form a
        # paint interval in the ratio window: 15 -> 31.68 ms.
        self.assertEqual(cycle["paint_interval_bins"][">=16"], 1)
        # Every paint starts 5 ms after a display change: phase 5/16.68.
        self.assertAlmostEqual(cycle["phase"]["p50"], 5.0 / 16.68, places=2)
        self.assertAlmostEqual(cycle["phase"]["concentration"], 1.0, places=2)
        self.assertEqual(cycle["present_modes"], {
            "Composed: Copy with GPU GDI": 2,
            "Hardware Composed: Independent Flip": 1,
        })


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


class CanvasPathTest(unittest.TestCase):
    """Where the canvas update path stopped, from the probe markers.

    The values reproduce the 2026-11-10 22:55 official Qt run: 7845 outline
    requests, but only 31 canvas updates (only the first few reached
    slotDoCanvasUpdate) and 8 paints, with the canvas widget size missing.
    """

    def write_path(self, directory, outline_requests, dispatch, paint_events, widget_size):
        width, height = widget_size
        rows = []
        for index in range(outline_requests):
            rows.append((ms(1000) + index, "outline_request", 2))
            rows.append((ms(1000) + index, "outline_state", 7))
        for index in range(dispatch):
            rows.append((ms(1000) + index, "canvas_update_dispatch", 0))
            rows.append((ms(1000) + index, "canvas_outline_wdg", 1, width))
            rows.append((ms(1000) + index, "canvas_outline_doc", 0, 1800.0))
        for index in range(paint_events):
            rows.append((ms(1000) + index, "canvas_paint_event", 1))
        write_run(directory, rows, [])
        return height

    def test_counts_and_state_bits_come_from_the_probe_log(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_path(directory, 7845, 31, 8, (0, 0))
            result = analyze.Run(directory).analyze()
        path = result["canvas_path"]
        self.assertEqual(path["outline_request"], 7845)
        self.assertEqual(path["outline_state"], 7845)
        self.assertEqual(path["outline_state_extra"], {7: 7845})
        self.assertEqual(path["canvas_update_dispatch"], 31)
        self.assertEqual(path["canvas_outline_wdg_extra"], {1: 31})
        self.assertEqual(path["canvas_paint_event"], 8)
        self.assertEqual(path["canvas_paint_event_extra"], {1: 8})

    def test_summary_prints_the_path_instead_of_an_empty_statistic(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_path(directory, 7845, 31, 8, (0, 0))
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                analyze.main([directory])
        text = output.getvalue()
        self.assertIn("canvas_path", text)
        self.assertIn("outline_request=7845", text)
        self.assertIn("outline_state=7845{7:7845}", text)
        self.assertIn("canvas_update_dispatch=31", text)
        self.assertIn("canvas_paint_event=8{1:8}", text)
        self.assertNotIn("canvas_path                                  n=0", text)


class FocusGateTest(unittest.TestCase):
    """Focus, outline gate and tool-action markers around a pen press.

    The values reproduce the shape of the 2026-11-11 stalled official Qt run:
    the outline stays enabled but is not visible, and the tool action is
    deactivated after the focus changes while the presses still reach the
    window.
    """

    def write_gate(self, directory, rows):
        with open(os.path.join(directory, "probe.csv"), "w") as handle:
            handle.write("# frequency=%d\n" % (TICKS_PER_MS * 1000))
            handle.write("time,event,x,y,extra,thread\n")
            for time, event, x, y, extra in rows:
                handle.write("%d,%s,%.2f,%.2f,%d,1\n" % (time, event, x, y, extra))

    def rows(self):
        base = ms(1000)
        rows = []
        for index in range(12):
            rows.append((base + index, "qt_event", 3, 4, 3))   # tablet press, gesture
            rows.append((base + index, "qt_event", 4, 4, 3))   # tablet release
        rows.append((base, "qt_event", 1, 0, 1))               # focus in, hover
        rows.append((base + 1, "qt_event", 2, 0, 1))           # focus out, hover
        for index in range(7845):
            rows.append((base + index, "outline_gate", 1.0, 2.0, 3))
        rows.append((base, "tool_action", 0, 1, 1))            # primary activated
        rows.append((base + 1, "tool_action", 0, 1, 0))        # primary deactivated
        return rows

    def test_counts_codes_modes_and_gate_bits(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_gate(directory, self.rows())
            result = analyze.Run(directory).analyze()
        gate = result["focus_gate"]
        self.assertEqual(gate["qt_event"], 26)
        self.assertEqual(gate["qt_event_codes"], {1: 1, 2: 1, 3: 12, 4: 12})
        self.assertEqual(gate["qt_event_modes"], {0: 2, 4: 24})
        self.assertEqual(gate["qt_event_extra"], {1: 2, 3: 24})
        self.assertEqual(gate["outline_gate"], 7845)
        self.assertEqual(gate["outline_gate_extra"], {3: 7845})
        self.assertEqual(gate["outline_gate_view_zoom"], {"min": 1.0, "max": 1.0})
        self.assertEqual(gate["outline_gate_physical_zoom"], {"min": 2.0, "max": 2.0})
        self.assertEqual(gate["tool_action_actions"], {0: 2})
        self.assertEqual(gate["tool_action_extra"], {0: 1, 1: 1})

    def test_counts_a_delivered_press_and_enter_leave_separately(self):
        rows = self.rows()
        base = ms(1000)
        for index in range(12):
            rows.append((base + index, "qt_event", 3, 4, 3 | 32))
        rows.append((base, "qt_event", 7, 0, 5))     # enter, inside + matcher entered
        rows.append((base + 1, "qt_event", 8, 0, 0))  # leave
        with tempfile.TemporaryDirectory() as directory:
            self.write_gate(directory, rows)
            result = analyze.Run(directory).analyze()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                analyze.main([directory])
        gate = result["focus_gate"]
        self.assertEqual(gate["qt_event"], 28)
        self.assertEqual(gate["qt_event_delivered"], 12)
        self.assertEqual(gate["qt_event_delivered_extra"], {35: 12})
        self.assertEqual(gate["qt_event_codes"],
                         {1: 1, 2: 1, 3: 12, 4: 12, 7: 1, 8: 1})
        text = output.getvalue()
        self.assertIn("qt_event_delivered=12{extra 35:12}", text)
        self.assertIn("enter:1", text)
        self.assertIn("leave:1", text)

    def test_summary_prints_the_gate_instead_of_an_empty_statistic(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_gate(directory, self.rows())
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                analyze.main([directory])
        text = output.getvalue()
        self.assertIn("focus_gate", text)
        self.assertIn("tablet_press:12", text)
        self.assertIn("focus_out:1", text)
        self.assertIn("outline_gate=7845{extra 3:7845}", text)
        self.assertIn("zoom=1.00/2.00", text)
        self.assertIn("tool_action=2{primary:2}", text)
        self.assertNotIn("focus_gate                                   n=0", text)


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
