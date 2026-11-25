# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/run.py.

Performance changes are accepted or rejected by the verdict of run.py. These
tests fix how run directories are assigned to targets, how the capture clock
correction is applied, and when a result is flagged: too few runs, or a
median above the baseline by more than the scenario threshold. The paired
comparison of a changed target with the target before the change is fixed by
test_perf_run_paired.py.
"""

import importlib.util
import io
import json
import os
import shutil
import sys
import tempfile
import time
import unittest
import csv
from contextlib import redirect_stdout
from unittest import mock

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "scripts", "tests"))
SPEC = importlib.util.spec_from_file_location("perf_run", os.path.join(ROOT, "scripts", "perf", "run.py"))
run = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(run)
import test_pen_latency_capture_analyze as simulated  # noqa: E402

SCENARIO = {
    "name": "test",
    "runs": 2,
    "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}, "ink.input_to_capture": {"regression_ms": 2.0}},
}


def summary_with(median, spread=0.0, runs=2):
    return {"hosts": {"h": {"targets": {"T": {"metrics": {
        "hover.input_to_capture": {"median": median, "p95": median + 5, "spread": spread, "runs": runs}}}}}}}


class TargetAssignmentTest(unittest.TestCase):
    def test_longest_target_name_wins(self):
        targets = [{"name": "LibrePaint"}, {"name": "LibrePaint-1.0.3"}]
        self.assertEqual(run.target_of("LibrePaint-1.0.3-20261108-120000-1", targets), "LibrePaint-1.0.3")
        self.assertEqual(run.target_of("LibrePaint-20261108-120000-1", targets), "LibrePaint")
        self.assertIsNone(run.target_of("Krita-20261108-120000-1", targets))


class SpeedTableTest(unittest.TestCase):
    def test_speed_groups_are_collected_across_runs(self):
        runs = [
            {"speed_groups": [
                {"speed_px_ms": 0.9, "n": 10, "input_to_capture": {"n": 10, "p50": 50.0, "p95": 55.0},
                 "pen_tip_to_edge_px": {"n": 10, "p50": 45.0}},
                {"speed_px_ms": 5.0, "n": 10, "input_to_capture": {"n": 10, "p50": 52.0, "p95": 58.0},
                 "pen_tip_to_edge_px": {"n": 10, "p50": 260.0}},
            ]},
            {"speed_groups": [
                {"speed_px_ms": 0.9, "n": 10, "input_to_capture": {"n": 10, "p50": 51.0, "p95": 56.0},
                 "pen_tip_to_edge_px": {"n": 10, "p50": 46.0}},
                {"speed_px_ms": 5.0, "n": 10, "input_to_capture": {"n": 10, "p50": 53.0, "p95": 59.0},
                 "pen_tip_to_edge_px": {"n": 10, "p50": 265.0}},
            ]},
        ]
        rows = run.speed_table(runs)
        self.assertEqual(len(rows), 2)
        self.assertAlmostEqual(rows[0][0], 0.9)
        self.assertAlmostEqual(rows[0][2], 50.5)
        self.assertAlmostEqual(rows[1][0], 5.0)
        self.assertAlmostEqual(rows[1][2], 52.5)

    def test_speed_change_reports_the_worst_change(self):
        runs = [{"speed_change": [
            {"max_latency_ms": 60.0, "steady_latency_ms": 50.0, "settle_ms": 20.0, "after_speed_px_ms": 5.0},
            {"max_latency_ms": 70.0, "steady_latency_ms": 50.0, "settle_ms": None, "after_speed_px_ms": 0.4},
        ]}]
        line = run.speed_change_lines(runs)[0]
        self.assertIn("70.0 ms", line)
        self.assertIn("settle none", line)


class SummaryTest(unittest.TestCase):
    def test_capture_offset_is_subtracted_from_every_run(self):
        with tempfile.TemporaryDirectory() as root:
            for n in (1, 2):
                path = os.path.join(root, "h", "T-20261108-120000-%d" % n)
                os.makedirs(path)
                simulated.simulate(path, 50_000_000)
            host = {"name": "h", "kind": "linux", "capture_offset_ms": 16, "targets": [{"name": "T"}]}
            summary = run.summarize(root, [host], SCENARIO)
        metric = summary["hosts"]["h"]["targets"]["T"]["metrics"]["hover.input_to_capture"]
        self.assertEqual(metric["runs"], 2)
        # The simulated display shows input 50 to 54 ms old; minus 16 ms.
        self.assertTrue(33.0 <= metric["median"] <= 38.5, metric)
        self.assertAlmostEqual(metric["spread"], 0.0, places=6)


def fake_run(state, hover, ink, presentmon_status="present", excluded=None):
    return {
        "hover.input_to_capture": {"n": 100, "p05": hover - 5, "p50": hover, "p95": hover + 10},
        "ink.input_to_capture": {"n": 100, "p05": ink - 5, "p50": ink, "p95": ink + 10},
        "state": {"label": state},
        "presentmon": {"present": presentmon_status != "none", "status": presentmon_status},
        "excluded": excluded,
    }


class DeviceShiftTest(unittest.TestCase):
    """Runs that cannot be compared because the device changed are marked."""

    def stored(self, name, hover, ink, surface=None):
        return {
            "name": name,
            "metrics": {
                "hover.input_to_capture": {"n": 100, "p05": hover - 5, "p50": hover, "p95": hover + 10},
                "ink.input_to_capture": {"n": 100, "p05": ink - 5, "p50": ink, "p95": ink + 10},
            },
            "surface_state": surface or {},
        }

    def test_a_large_change_from_the_previous_run_is_marked(self):
        notes = run.shift_notes([
            self.stored("T-20261111-023347-1", 12.5, 18.1),
            self.stored("T-20261111-023605-1", 14.1, 29.4),
        ])
        self.assertTrue(any("T-20261111-023605-1" in note and "ink.input_to_capture" in note for note in notes), notes)
        self.assertTrue(any("+62%" in note for note in notes), notes)

    def test_a_change_below_the_limit_is_not_marked(self):
        notes = run.shift_notes([
            self.stored("T-1", 12.5, 18.1),
            self.stored("T-2", 12.9, 18.7),
        ])
        self.assertEqual(notes, [])

    def test_a_hotter_or_throttled_device_is_marked(self):
        notes = run.shift_notes([
            self.stored("T-1", 12.5, 18.1, {"during_max_temp_c": "44.0", "during_min_processor_performance_pct": "77"}),
            self.stored("T-2", 12.5, 18.1, {"during_max_temp_c": "58.0", "during_min_processor_performance_pct": "60"}),
        ])
        self.assertTrue(any("during_max_temp_c 44C -> 58C" in note for note in notes), notes)
        self.assertTrue(any("during_min_processor_performance_pct 77% -> 60%" in note for note in notes), notes)

    def test_surface_state_is_read_from_meta_txt(self):
        with tempfile.TemporaryDirectory() as root:
            with open(os.path.join(root, "meta.txt"), "w") as handle:
                handle.write("foreground_capture=pid=1 name=LibrePaint visible=True\n")
                handle.write("surface_state_mode=light\n")
                handle.write("surface_state_wait_mode=on\n")
                handle.write("surface_state_wait=ready\n")
                handle.write("surface_state_wait_seconds=13.4\n")
                handle.write("surface_state_wait_first_c=44.2\n")
                handle.write("surface_state_wait_last_c=41.9\n")
                handle.write("surface_state_during_max_temp_c=58.0\n")
                handle.write("surface_state_during_min_processor_performance_pct=60\n")
                handle.write("surface_state_top_process=MsMpEng 40%\n")
            state = run.surface_state(root)
        self.assertEqual(state["mode"], "light")
        self.assertEqual(state["wait_mode"], "on")
        self.assertEqual(state["wait"], "ready")
        self.assertEqual(state["wait_seconds"], "13.4")
        self.assertEqual(state["wait_first_c"], "44.2")
        self.assertEqual(state["wait_last_c"], "41.9")
        self.assertEqual(state["during_max_temp_c"], "58.0")
        self.assertEqual(state["during_min_processor_performance_pct"], "60")
        self.assertEqual(state["top_process"], "MsMpEng 40%")
        self.assertNotIn("foreground_capture", state)

    def test_a_throttled_device_is_told_from_a_device_without_a_record(self):
        self.assertFalse(run.device_performance_low({"surface_state": {}}))
        self.assertFalse(run.device_performance_low(
            {"surface_state": {"during_min_processor_performance_pct": "70"}}))
        self.assertTrue(run.device_performance_low(
            {"surface_state": {"during_min_processor_performance_pct": "69.9"}}))
        self.assertTrue(run.device_performance_low(
            {"surface_state": {"during_min_processor_performance_pct": "42"}}))
        self.assertFalse(run.device_performance_low(
            {"surface_state": {"during_min_processor_performance_pct": "unavailable"}}))
        self.assertFalse(run.device_performance_low(
            {"surface_state": {"during_min_processor_performance_pct": "0"}}))

    def test_three_low_runs_in_a_row_are_counted(self):
        low = {"surface_state": {"during_min_processor_performance_pct": "42"}}
        streak = 0
        for _ in range(3):
            streak = run.device_low_streak(streak, low)
        self.assertEqual(streak, 3)

    def test_a_run_that_is_not_low_resets_the_streak(self):
        low = {"surface_state": {"during_min_processor_performance_pct": "42"}}
        high = {"surface_state": {"during_min_processor_performance_pct": "80"}}
        self.assertEqual(run.device_low_streak(run.device_low_streak(0, low), high), 0)

    def test_a_failed_zero_reading_does_not_count_as_low(self):
        zero = {"surface_state": {"during_min_processor_performance_pct": "0"}}
        self.assertEqual(run.device_low_streak(2, zero), 0)

    def write_device_state(self, path, values):
        with open(os.path.join(path, "surface-state.csv"), "w", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(["time", "phase", "max_temp_c", "processor_performance_pct",
                             "total_cpu_pct", "top_processes"])
            writer.writerow(["t0", "before", "45.0", "103", "8", ""])
            for index, value in enumerate(values, start=1):
                writer.writerow(["t%d" % index, "during", "unavailable", value, "14", ""])
            writer.writerow(["t9", "after", "45.0", "85", "0", ""])

    def test_a_zero_processor_reading_is_a_failed_read_not_a_throttled_run(self):
        with tempfile.TemporaryDirectory() as root:
            with open(os.path.join(root, "meta.txt"), "w") as handle:
                handle.write("surface_state_mode=light\n")
                handle.write("surface_state_during_min_processor_performance_pct=0\n")
            self.write_device_state(root, ["181", "0", "180"])
            state = run.surface_state(root)
        self.assertEqual(state["during_readings"], "3")
        self.assertEqual(state["during_readings_missing"], "1")
        self.assertEqual(state["during_min_processor_performance_pct"], "180")

    def test_a_run_with_a_failed_reading_stays_in_the_aggregation(self):
        with tempfile.TemporaryDirectory() as root:
            readings = (["70"], ["181", "0", "180"], ["75"])
            for n, values in enumerate(readings, start=1):
                path = os.path.join(root, "h", "T-20261111-120000-%d" % n)
                os.makedirs(path)
                open(os.path.join(path, "capture.csv"), "w").close()
                with open(os.path.join(path, "meta.txt"), "w") as handle:
                    handle.write("surface_state_wait=ready\n")
                    handle.write("surface_state_during_min_processor_performance_pct=0\n")
                self.write_device_state(path, values)
            host = {"name": "h", "kind": "windows", "capture_offset_ms": 0,
                    "targets": [{"name": "T"}]}
            scenario = dict(SCENARIO, runs=len(readings))
            with mock.patch.object(run.capture_analyze, "analyze_run",
                                   side_effect=[fake_run("low", 55.0, 61.0) for _ in readings]):
                summary = run.summarize(root, [host], scenario)
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["device_low"], 0)
        self.assertEqual(data["device_readings_missing"], 1)
        self.assertEqual(data["metrics"]["hover.input_to_capture"]["runs"], 3)
        names = [state.get("during_min_processor_performance_pct")
                 for state in (entry["surface_state"] for entry in data["runs"])]
        self.assertEqual(sorted(names), ["180", "70", "75"])
        rows = run.judge(summary, scenario)
        notes = "; ".join(row[8] for row in rows)
        self.assertIn("1 run(s) with a failed processor performance reading (kept)", notes)

    def test_the_table_notes_carry_the_mark(self):
        summary = {"hosts": {"h": {"kind": "windows", "targets": {"T": {
            "runs": [],
            "metrics": {"hover.input_to_capture": {"median": 40.0, "p95": 45.0, "spread": 0.0, "runs": 2}},
            "states": {},
            "excluded": 0,
            "shifts": ["T-2 ink.input_to_capture +62% (18.1 -> 29.4 ms) from the run before"],
        }}}}}
        rows = run.judge(summary, SCENARIO)
        notes = "; ".join(row[8] for row in rows)
        self.assertIn("marked: T-2 ink.input_to_capture +62%", notes)
        self.assertTrue(all(row[7] == "ok" for row in rows))


class RunnerCommandTest(unittest.TestCase):
    def test_windows_target_passes_the_presentmon_setting(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe"}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["PRESENTMON"], "on")
            target["presentmon"] = "off"
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["PRESENTMON"], "off")

    def test_capture_dense_reaches_the_macos_and_linux_runners(self):
        scenario = {"trajectory": {}, "name": "test"}
        macos = {"name": "m", "kind": "macos", "capture_tool": "/tmp/capture", "work": "/tmp/work"}
        linux = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/repo", "work": "/work", "remote_out": "/out"}
        target = {"name": "T", "command": ["app"]}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(macos, target, scenario, root)
            self.assertNotIn("CAPTURE_DENSE", env)
            argv, _, _ = run.runner_for(linux, target, scenario, root)
            self.assertNotIn("CAPTURE_DENSE", argv[2])
            target["capture_dense"] = 18
            _, env, _ = run.runner_for(macos, target, scenario, root)
            self.assertEqual(env["CAPTURE_DENSE"], "18")
            argv, _, _ = run.runner_for(linux, target, scenario, root)
            self.assertIn("CAPTURE_DENSE=18 TRAJECTORY_JSON=", argv[2])

    def test_device_state_recording_is_light_by_default(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe"}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["SURFACE_STATE"], "light")
            target["surface_state"] = "off"
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["SURFACE_STATE"], "off")
            for mode in ("on", "light"):
                target["surface_state"] = mode
                _, env, _ = run.runner_for(host, target, scenario, root)
                self.assertEqual(env["SURFACE_STATE"], mode)

    def test_invalid_surface_state_is_rejected(self):
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                  "surface_state": "heavy"}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_the_recovery_wait_is_on_unless_the_target_turns_it_off(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe"}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["SURFACE_STATE_WAIT"], "on")
            target["surface_state_wait"] = "off"
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["SURFACE_STATE_WAIT"], "off")

    def test_invalid_recovery_wait_is_rejected(self):
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        for value in ("auto", "ON", 1):
            target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                      "surface_state_wait": value}
            with tempfile.TemporaryDirectory() as root:
                with self.assertRaises(SystemExit):
                    run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_recovery_wait_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "surface_state_wait": "off"}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_surface_state_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "surface_state": "on"}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_thread_cpu_is_off_unless_the_target_asks_for_it(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe"}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["THREAD_CPU"], "off")
            target["thread_cpu"] = "on"
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["THREAD_CPU"], "on")

    def test_invalid_thread_cpu_is_rejected(self):
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        for value in ("auto", "ON", 1):
            target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                      "thread_cpu": value}
            with tempfile.TemporaryDirectory() as root:
                with self.assertRaises(SystemExit):
                    run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_thread_cpu_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "thread_cpu": "on"}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_power_is_off_unless_the_target_asks_for_it(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe"}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["POWER"], "off")
            target["power"] = "on"
            _, env, _ = run.runner_for(host, target, scenario, root)
            self.assertEqual(env["POWER"], "on")

    def test_invalid_power_is_rejected(self):
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        for value in ("auto", "ON", 1):
            target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                      "power": value}
            with tempfile.TemporaryDirectory() as root:
                with self.assertRaises(SystemExit):
                    run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_power_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "power": "on"}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_windows_target_passes_environment_to_the_runner(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                  "env": {"LIBREPAINT_L1_SWAP_INTERVAL": "0"}}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
        self.assertEqual(json.loads(env["TARGET_ENV"]), {"LIBREPAINT_L1_SWAP_INTERVAL": "0"})

    def test_invalid_environment_name_is_rejected(self):
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                  "env": {"1BAD": "x"}}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_environment_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "env": {"LIBREPAINT_L1_SWAP_INTERVAL": "0"}}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_windows_target_passes_kritarc_to_the_runner(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                  "kritarc": {"useMaximumFrameLatencyOne": "false"}}
        with tempfile.TemporaryDirectory() as root:
            _, env, _ = run.runner_for(host, target, scenario, root)
        self.assertEqual(json.loads(env["TARGET_KRITARC"]), {"useMaximumFrameLatencyOne": "false"})

    def test_invalid_kritarc_key_is_rejected(self):
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe",
                  "kritarc": {"BAD KEY": "x"}}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)

    def test_kritarc_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "kritarc": {"useMaximumFrameLatencyOne": "false"}}
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaises(SystemExit):
                run.runner_for(host, target, {"trajectory": {}, "name": "test"}, root)


    def test_linux_scenario_kritarc_lands_in_the_empty_group(self):
        scenario = {"kritarc": {"General": {"newOutlineStyle": "2", "fpsLimit": "100"}}}
        self.assertEqual(run.kritarc_text(scenario), "newOutlineStyle=2\nfpsLimit=100\n")
        scenario["kritarc"]["Other"] = {"key": "value"}
        self.assertEqual(run.kritarc_text(scenario),
                         "newOutlineStyle=2\nfpsLimit=100\n[Other]\nkey=value\n")


class PowerAggregateTest(unittest.TestCase):
    def entry(self, rate, energy, ac=False):
        return {"power": {"window_s": 30.0, "samples": 30, "ac": ac,
                          "rate_mw": rate, "energy_mwh": energy},
                "power_state": {"mode": "on", "ac": "1" if ac else "0"}}

    def test_ac_runs_are_left_out_of_the_power_median(self):
        runs = [self.entry(4000.0, 33.0), self.entry(6000.0, 50.0),
                self.entry(9000.0, 75.0, ac=True)]
        result = run.aggregate_power(runs)
        self.assertEqual(result["runs"], 2)
        self.assertEqual(result["ac_runs"], 1)
        self.assertAlmostEqual(result["rate_mw"], 5000.0)
        self.assertAlmostEqual(result["energy_mwh"], 41.5)

    def test_a_run_without_a_power_record_stays_out(self):
        result = run.aggregate_power([{"power": None, "power_state": {}},
                                      self.entry(4000.0, 33.0)])
        self.assertEqual(result["runs"], 1)
        self.assertEqual(result["ac_runs"], 0)

    def test_power_state_is_read_from_meta_txt(self):
        with tempfile.TemporaryDirectory() as root:
            with open(os.path.join(root, "meta.txt"), "w") as handle:
                handle.write("power_mode=on\n")
                handle.write("power_ac=1\n")
            state = run.power_state(root)
        self.assertEqual(state, {"mode": "on", "ac": "1"})


class PresentMonAggregateTest(unittest.TestCase):
    def run_result(self, status, hover, display):
        return {
            "metrics": {
                "hover.input_to_capture": {"n": 10, "p05": hover - 5, "p50": hover, "p95": hover + 10},
                "hover.input_to_display": {"n": 10, "p05": display - 5, "p50": display, "p95": display + 10},
            },
            "state": {"label": "low"},
            "presentmon": {"present": status != "none", "status": status},
        }

    def test_partial_runs_stay_in_capture_metrics_and_leave_display_metrics(self):
        runs = [
            self.run_result("present", 50.0, 60.0),
            self.run_result("present", 51.0, 62.0),
            self.run_result("partial", 52.0, 90.0),
        ]
        aggregated = run.aggregate(runs, {"hover.input_to_capture": {}, "hover.input_to_display": {}})
        self.assertEqual(aggregated["hover.input_to_capture"]["runs"], 3)
        self.assertEqual(aggregated["hover.input_to_display"]["runs"], 2)
        self.assertAlmostEqual(aggregated["hover.input_to_display"]["median"], 61.0)

    def test_old_summaries_without_status_count_as_present(self):
        self.assertEqual(run.presentmon_status({"presentmon": {"present": True}}), "present")
        self.assertEqual(run.presentmon_status({"presentmon": {"present": False}}), "none")


class StateSummaryTest(unittest.TestCase):
    SCENARIO = {
        "name": "test",
        "runs": 4,
        "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}, "ink.input_to_capture": {"regression_ms": 2.0}},
    }

    def summarize(self, results):
        with tempfile.TemporaryDirectory() as root:
            for n in range(1, len(results) + 1):
                path = os.path.join(root, "h", "T-20261108-120000-%d" % n)
                os.makedirs(path)
                open(os.path.join(path, "capture.csv"), "w").close()
            host = {"name": "h", "kind": "windows", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
            with mock.patch.object(run.capture_analyze, "analyze_run", side_effect=results):
                return run.summarize(root, [host], self.SCENARIO)

    def test_hover_is_grouped_by_state_and_ink_is_not(self):
        summary = self.summarize([
            fake_run("low", 55.0, 61.0),
            fake_run("high", 62.0, 61.0),
            fake_run("low", 56.0, 61.0),
            fake_run("high", 63.0, 61.0),
        ])
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["states"]["low"]["runs"], 2)
        self.assertEqual(data["states"]["high"]["runs"], 2)
        self.assertAlmostEqual(data["states"]["low"]["metrics"]["hover.input_to_capture"]["median"], 55.5)
        rows = run.judge(summary, self.SCENARIO)
        hover_states = {row[9] for row in rows if row[2] == "hover.input_to_capture"}
        self.assertEqual(hover_states, {"low", "high"})
        self.assertTrue(all(row[9] is None for row in rows if row[2] == "ink.input_to_capture"))

    def test_presentmon_status_counts_are_reported(self):
        summary = self.summarize([
            fake_run("low", 55.0, 61.0),
            fake_run("low", 56.0, 61.0, "partial"),
            fake_run("high", 62.0, 61.0, "none"),
            fake_run("high", 63.0, 61.0),
        ])
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["presentmon"], {"present": 2, "partial": 1, "none": 1})

    def test_device_state_recording_modes_are_counted(self):
        with tempfile.TemporaryDirectory() as root:
            modes = ("off", "on", "light")
            for n, mode in enumerate(modes, start=1):
                path = os.path.join(root, "h", "T-20261108-120000-%d" % n)
                os.makedirs(path)
                open(os.path.join(path, "capture.csv"), "w").close()
                with open(os.path.join(path, "meta.txt"), "w") as handle:
                    handle.write("surface_state_mode=%s\n" % mode)
            host = {"name": "h", "kind": "windows", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
            with mock.patch.object(run.capture_analyze, "analyze_run",
                                   side_effect=[fake_run("low", 55.0, 61.0) for _ in modes]):
                summary = run.summarize(root, [host], self.SCENARIO)
        self.assertEqual(summary["hosts"]["h"]["targets"]["T"]["surface_state_modes"],
                         {"off": 1, "on": 1, "light": 1})

    def test_a_run_below_70_percent_is_left_out_and_marked(self):
        with tempfile.TemporaryDirectory() as root:
            values = ("77", "42", "75")
            for n, value in enumerate(values, start=1):
                path = os.path.join(root, "h", "T-20261108-120000-%d" % n)
                os.makedirs(path)
                open(os.path.join(path, "capture.csv"), "w").close()
                with open(os.path.join(path, "meta.txt"), "w") as handle:
                    handle.write("surface_state_wait=ready\n")
                    handle.write("surface_state_during_min_processor_performance_pct=%s\n" % value)
            host = {"name": "h", "kind": "windows", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
            with mock.patch.object(run.capture_analyze, "analyze_run",
                                   side_effect=[fake_run("low", 55.0, 61.0),
                                                fake_run("low", 70.0, 61.0),
                                                fake_run("low", 56.0, 61.0)]):
                summary = run.summarize(root, [host], self.SCENARIO)
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["device_low"], 1)
        self.assertEqual(data["excluded"], 0)
        self.assertEqual(data["metrics"]["hover.input_to_capture"]["runs"], 2)
        self.assertAlmostEqual(data["metrics"]["hover.input_to_capture"]["median"], 55.5)
        self.assertEqual(data["surface_state_waits"], {"ready": 3})
        rows = run.judge(summary, self.SCENARIO)
        notes = "; ".join(row[8] for row in rows)
        self.assertIn("1 run(s) left out: processor performance below 70%", notes)

    def test_a_run_whose_app_was_not_in_front_is_left_out(self):
        summary = self.summarize([
            fake_run("high", 60.0, 61.0),
            fake_run("high", 62.0, 61.0),
            fake_run("high", 120.0, 61.0, "partial", excluded="app not foreground and presented at 39% of the display rate"),
            fake_run("high", 61.0, 61.0),
        ])
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["excluded"], 1)
        self.assertEqual(data["metrics"]["hover.input_to_capture"]["runs"], 3)
        self.assertAlmostEqual(data["metrics"]["hover.input_to_capture"]["median"], 61.0)
        self.assertEqual(data["presentmon"], {"present": 3, "partial": 0, "none": 0})
        rows = run.judge(summary, self.SCENARIO)
        notes = "; ".join(row[8] for row in rows)
        self.assertIn("1 run(s) left out: app not foreground or not presenting", notes)
        self.assertIn("high state: 3 of 3 runs", notes)


class ShortTargetsTest(unittest.TestCase):
    """A target whose window counted too few runs makes the measurement fail."""

    SCENARIO = {"name": "test", "runs": 3, "metrics": {}}
    HOSTS = [{"name": "h", "kind": "linux", "targets": [{"name": "A"}, {"name": "B"}]}]

    def test_targets_with_enough_runs_are_not_returned(self):
        summary = {"hosts": {"h": {"targets": {
            "A": {"settle": {"window_runs": 3}},
            "B": {"settle": {"window_runs": 4}},
        }}}}
        self.assertEqual(run.short_targets(summary, self.HOSTS, self.SCENARIO), [])

    def test_a_target_without_counted_runs_and_a_missing_target_are_returned(self):
        summary = {"hosts": {"h": {"targets": {"A": {"settle": {"window_runs": 0},
                                                   "excluded": 1, "device_low": 2}}}}}
        self.assertEqual(run.short_targets(summary, self.HOSTS, self.SCENARIO),
                         [("h", "A", 0, 1, 2), ("h", "B", 0, 0, 0)])


class EarlyStopTest(unittest.TestCase):
    """A target that fails twice in a row for the same reason stops early."""

    SCENARIO = {
        "name": "test",
        "runs": 5,
        "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}},
    }
    HOST = {"name": "w", "kind": "windows", "capture_offset_ms": 0,
            "targets": [{"name": "LibrePaint", "runner_target": "LibrePaint",
                         "app": "C:/LibrePaint.exe"}]}

    def measure(self, statuses):
        """Run measure_runs with a fake runner that returns the given lines."""
        calls = []

        def fake_runner(host, target, scenario, run_root):
            return ["true"], {}, None

        def fake_command(argv, env=None, cwd=None):
            calls.append(list(argv))
            line = statuses[len(calls) - 1] if len(calls) <= len(statuses) else None
            host_dir = os.path.join(self._root, "w")
            os.makedirs(host_dir, exist_ok=True)
            with open(os.path.join(host_dir, "status-LibrePaint.txt"), "w") as handle:
                handle.write("starting\n")
                if line:
                    handle.write(line + "\n")
            return 0 if line is None else 1

        with mock.patch.object(run, "runner_for", side_effect=fake_runner), \
                mock.patch.object(run, "run_command", side_effect=fake_command):
            failures = run.measure_runs([self.HOST], self.SCENARIO, self._root, "l1", self.events)
        with open(self.events, encoding="utf-8") as handle:
            lines = [line.split("\t") for line in handle.read().splitlines()]
        return calls, failures, lines

    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self._root = os.path.join(self._temp.name, "runs")
        self.events = os.path.join(self._temp.name, "events.log")

    def kinds(self, lines):
        return [fields[1] for fields in lines]

    def test_the_same_failure_twice_skips_the_remaining_rounds(self):
        calls, failures, lines = self.measure(
            ["failed: app exited before showing a window"] * 5)
        self.assertEqual(len(calls), 2, calls)
        self.assertEqual(len(failures), 2)
        self.assertEqual(self.kinds(lines).count("target-failed"), 2)
        self.assertEqual(self.kinds(lines).count("target-abandoned"), 1)
        abandoned = next(fields for fields in lines if fields[1] == "target-abandoned")
        self.assertEqual(abandoned[3], "LibrePaint")
        self.assertEqual(abandoned[4], "2")
        self.assertIn("app exited before showing a window", abandoned[5])
        self.assertEqual(self.kinds(lines)[0], "run-start")

    def test_a_different_reason_does_not_abandon(self):
        calls, failures, lines = self.measure(
            ["failed: app exited before showing a window", "failed: capture failed",
             "failed: no main window", "failed: other", "failed: other again"])
        # The five failed runs are not usable, so the target gets five more
        # runs up to the limit of twice the scenario's count; the successes
        # are usable and the target finishes at the limit.
        self.assertEqual(len(calls), 10, calls)
        self.assertEqual(len(failures), 5)
        self.assertNotIn("target-abandoned", self.kinds(lines))

    def test_a_failed_line_is_recorded_as_written(self):
        _, _, lines = self.measure(["failed: capture failed", "failed: capture failed"])
        failed = [fields for fields in lines if fields[1] == "target-failed"]
        self.assertEqual([fields[5] for fields in failed], ["failed: capture failed"] * 2)
        self.assertEqual([fields[4] for fields in failed], ["1", "2"])

    def measure_device_runs(self, values, rest_seconds=None, runs=2):
        """Run measure_runs where each run records a processor performance."""
        calls = []

        def fake_runner(host, target, scenario, run_root):
            return ["true"], {}, None

        def fake_command(argv, env=None, cwd=None):
            calls.append(list(argv))
            number = len(calls)
            host_dir = os.path.join(self._root, "w")
            path = os.path.join(host_dir, "LibrePaint-20261112-120000-%d" % number)
            os.makedirs(path)
            # Make the newest directory unambiguous when runs finish together.
            stamp = time.time() + number
            os.utime(path, (stamp, stamp))
            value = values[number - 1] if number <= len(values) else "80"
            with open(os.path.join(path, "meta.txt"), "w") as handle:
                handle.write("surface_state_mode=light\n")
                if value is not None:
                    handle.write("surface_state_during_min_processor_performance_pct=%s\n" % value)
            return 0

        scenario = dict(self.SCENARIO)
        scenario["runs"] = runs
        if rest_seconds is not None:
            scenario["rest_seconds"] = rest_seconds
        with mock.patch.object(run, "runner_for", side_effect=fake_runner), \
                mock.patch.object(run, "run_command", side_effect=fake_command), \
                mock.patch.object(run.time, "sleep") as sleep:
            failures = run.measure_runs([self.HOST], scenario, self._root, "l1", self.events)
        with open(self.events, encoding="utf-8") as handle:
            lines = [line.split("\t") for line in handle.read().splitlines()]
        return calls, failures, lines, sleep

    def test_a_low_run_is_replaced_by_another_run(self):
        calls, failures, _, _ = self.measure_device_runs(["42", "80"])
        self.assertEqual(len(calls), 3, calls)
        self.assertEqual(failures, [])

    def test_added_runs_stop_at_twice_the_scenario_count(self):
        with mock.patch.object(run, "DEVICE_LOW_STOP_RUNS", 100):
            calls, failures, _, _ = self.measure_device_runs(["42", "42", "42", "42"])
        self.assertEqual(len(calls), 4, calls)
        self.assertEqual(failures, [])

    def test_rest_seconds_waits_between_windows_runs_only(self):
        calls, _, lines, sleep = self.measure_device_runs(["80", "80"], rest_seconds=90)
        self.assertEqual(len(calls), 2, calls)
        sleep.assert_called_once_with(90.0)
        self.assertEqual(self.kinds(lines).count("rest"), 1)


class VerdictTest(unittest.TestCase):
    def verdict(self, summary, baseline=None):
        return run.judge(summary, SCENARIO, baseline)[0]

    def test_stable_result_without_baseline_is_ok(self):
        self.assertEqual(self.verdict(summary_with(40.0))[7], "ok")

    def test_spread_alone_does_not_need_checking(self):
        row = self.verdict(summary_with(40.0, spread=0.20))
        self.assertEqual(row[7], "ok")
        self.assertNotIn("spread", row[8])

    def test_missing_runs_need_checking(self):
        self.assertEqual(self.verdict(summary_with(40.0, runs=1))[7], "check")

    def test_regression_beyond_threshold_needs_checking(self):
        row = self.verdict(summary_with(42.5), summary_with(40.0))
        self.assertEqual(row[7], "check")
        self.assertAlmostEqual(row[6], 2.5)

    def test_change_within_threshold_and_improvement_are_ok(self):
        self.assertEqual(self.verdict(summary_with(41.5), summary_with(40.0))[7], "ok")
        row = self.verdict(summary_with(30.0), summary_with(40.0))
        self.assertEqual(row[7], "ok")
        self.assertIn("improvement", row[8])


class MainExitTest(unittest.TestCase):
    """The process status follows the verdict rows, also when speed rows are printed."""

    SPEEDS = [{"speed_px_ms": 0.9, "n": 10,
               "input_to_capture": {"n": 10, "p50": 50.0, "p95": 55.0},
               "pen_tip_to_edge_px": {"n": 10, "p50": 45.0}},
              {"speed_px_ms": 5.0, "n": 10,
               "input_to_capture": {"n": 10, "p50": 52.0, "p95": 58.0},
               "pen_tip_to_edge_px": {"n": 10, "p50": 260.0}}]

    def run_main(self, hovers, before=()):
        scenario = {
            "name": "test",
            "runs": len(hovers),
            "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}},
        }
        with tempfile.TemporaryDirectory() as root:
            run_root = os.path.join(root, "runs")
            for name, values in (("Old", before), ("T", hovers)):
                for index, _ in enumerate(values, start=1):
                    path = os.path.join(run_root, "h", "%s-20261108-120000-%d" % (name, index))
                    os.makedirs(path)
                    open(os.path.join(path, "capture.csv"), "w").close()
            scenario_path = os.path.join(root, "scenario.json")
            hosts_path = os.path.join(root, "hosts.json")
            with open(scenario_path, "w") as handle:
                json.dump(scenario, handle)
            with open(hosts_path, "w") as handle:
                targets = [{"name": "T"}]
                if before:
                    targets = [{"name": "Old"}, {"name": "T", "before": "Old"}]
                json.dump({"hosts": [{"name": "h", "kind": "linux", "capture_offset_ms": 0,
                                      "targets": targets}]}, handle)
            results = []
            for hover in list(before) + list(hovers):
                result = fake_run("low", hover, 40.0)
                result["ink.speed_groups"] = self.SPEEDS
                result["speed_change"] = []
                results.append(result)
            output = io.StringIO()
            with mock.patch.object(run.capture_analyze, "analyze_run", side_effect=results), \
                    mock.patch.object(run.subprocess, "run",
                                      return_value=mock.Mock(stdout="0123456789\n")), \
                    redirect_stdout(output):
                status = run.main(["--scenario", scenario_path, "--hosts", hosts_path,
                                   "--analyze-only", run_root])
        return status, output.getvalue()

    def test_speed_rows_do_not_change_a_passing_status(self):
        status, output = self.run_main([40.0, 40.5])
        self.assertIn("== ink speed groups", output)
        self.assertEqual(status, 0)

    def test_a_flagged_metric_still_exits_nonzero_with_speed_rows(self):
        status, output = self.run_main([50.0, 50.3, 50.1], before=[40.0, 40.2, 40.1])
        self.assertIn("== ink speed groups", output)
        self.assertIn("| check |", output)
        self.assertEqual(status, 1)


FIXTURES = os.path.join(ROOT, "scripts", "tests", "fixtures", "perf-canvas")


def capture_dirs(root, count, host="h", target="T"):
    """Create count run directories with a capture.csv and return their paths."""
    paths = []
    for number in range(1, count + 1):
        path = os.path.join(root, host, "%s-20261108-120000-%d" % (target, number))
        os.makedirs(path)
        open(os.path.join(path, "capture.csv"), "w").close()
        paths.append(path)
    return paths


class ConditionsTest(unittest.TestCase):
    """The capture conditions that every writer records for one run."""

    def conditions(self, platform):
        with tempfile.TemporaryDirectory() as root:
            path = os.path.join(root, "T-20261108-120000-1")
            os.makedirs(path)
            source = os.path.join(FIXTURES, platform)
            for name in os.listdir(source):
                shutil.copy(os.path.join(source, name), os.path.join(path, name))
            return run.canvas_conditions(path)

    def test_macos_canvas_is_recorded_in_captured_pixels(self):
        conditions = self.conditions("macos")
        self.assertEqual(conditions["canvas"], [1100, 900])
        self.assertEqual(conditions["frame"], [3360, 2100])
        self.assertEqual(conditions["scale"], 2.0)
        self.assertEqual(conditions["refresh"], 60.0)

    def test_windows_canvas_is_recorded_in_captured_pixels(self):
        conditions = self.conditions("windows")
        self.assertEqual(conditions["canvas"], [1000, 800])
        self.assertEqual(conditions["frame"], [1920, 1080])
        self.assertIsNone(conditions["scale"])
        self.assertEqual(conditions["refresh"], 60.0)

    def test_linux_capture_meta_is_normalized(self):
        conditions = self.conditions("linux")
        self.assertEqual(conditions["canvas"], [1004, 806])
        self.assertEqual(conditions["frame"], [1920, 1080])
        self.assertIsNone(conditions["scale"])
        self.assertIsNone(conditions["refresh"])

    def test_a_run_without_a_canvas_file_has_no_conditions(self):
        with tempfile.TemporaryDirectory() as root:
            self.assertIsNone(run.canvas_conditions(root))

    def test_an_unreadable_canvas_file_has_no_conditions(self):
        for content in ("{", '{"canvas": [1, 2]}', '{"canvas": [0, 0, 0, 0]}'):
            with tempfile.TemporaryDirectory() as root:
                with open(os.path.join(root, "canvas.json"), "w") as handle:
                    handle.write(content)
                self.assertIsNone(run.canvas_conditions(root), content)


class TargetConditionsTest(unittest.TestCase):
    def conditions(self, canvas=(1000, 800), scale=2, refresh=60):
        return {"canvas": list(canvas), "frame": [1920, 1080], "scale": scale, "refresh": refresh}

    def test_the_window_is_stable_when_every_run_recorded_the_same_conditions(self):
        window = [{"conditions": self.conditions()}, {"conditions": self.conditions()}]
        self.assertTrue(run.target_conditions(window)["stable"])

    def test_another_canvas_scale_or_refresh_makes_the_window_unstable(self):
        first = {"conditions": self.conditions()}
        for other in (self.conditions(canvas=(1100, 800)), self.conditions(scale=1),
                      self.conditions(refresh=50)):
            self.assertFalse(run.target_conditions([first, {"conditions": other}])["stable"], other)

    def test_a_window_without_a_recorded_condition_has_none(self):
        self.assertIsNone(run.target_conditions([{"conditions": None}]))
        self.assertFalse(run.target_conditions([{"conditions": None},
                                                {"conditions": self.conditions()}])["stable"])


class WindowTest(unittest.TestCase):
    """The warm-ups and the window of the counted runs of one target."""

    SCENARIO = {"name": "test", "runs": 2, "warmup_runs": 1, "max_runs": 4,
                "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}}

    def summarize(self, results, scenario=None):
        scenario = scenario or self.SCENARIO
        with tempfile.TemporaryDirectory() as root:
            capture_dirs(root, len(results))
            host = {"name": "h", "kind": "windows", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
            with mock.patch.object(run.capture_analyze, "analyze_run", side_effect=results):
                return run.summarize(root, [host], scenario)

    def test_the_warm_up_run_is_marked_and_left_out(self):
        summary = self.summarize([fake_run("low", 100.0, 40.0),
                                  fake_run("low", 40.0, 40.0),
                                  fake_run("low", 41.0, 40.0)])
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual([entry["warmup"] for entry in data["runs"]], [True, False, False])
        self.assertEqual(data["metrics"]["hover.input_to_capture"]["runs"], 2)
        self.assertAlmostEqual(data["metrics"]["hover.input_to_capture"]["median"], 40.5)
        self.assertEqual(data["settle"], {"warmup_runs": 1, "counted_runs": 2,
                                          "window_runs": 2, "unsettled": []})

    def test_a_warm_up_run_stays_out_of_the_run_counts(self):
        warm = fake_run("low", 40.0, 40.0, excluded="app not foreground and presented at 39%")
        summary = self.summarize([warm, fake_run("low", 40.0, 40.0), fake_run("low", 40.0, 40.0)])
        self.assertEqual(summary["hosts"]["h"]["targets"]["T"]["excluded"], 0)

    def test_the_window_is_the_last_runs_of_the_counted_runs(self):
        scenario = dict(self.SCENARIO, warmup_runs=0)
        summary = self.summarize([fake_run("low", 100.0, 40.0), fake_run("low", 90.0, 40.0),
                                  fake_run("low", 40.0, 40.0), fake_run("low", 41.0, 40.0)], scenario)
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual([entry["in_window"] for entry in data["runs"]], [False, False, True, True])
        self.assertAlmostEqual(data["metrics"]["hover.input_to_capture"]["median"], 40.5)
        self.assertEqual(data["settle"]["counted_runs"], 4)
        self.assertEqual(data["settle"]["window_runs"], 2)

    def test_judge_counts_the_window_and_not_the_runs_before_it(self):
        scenario = dict(self.SCENARIO, warmup_runs=0)
        summary = self.summarize([fake_run("low", 100.0, 40.0), fake_run("low", 40.0, 40.0),
                                  fake_run("low", 40.5, 40.0)], scenario)
        hover = [row for row in run.judge(summary, scenario)
                 if row[2] == "hover.input_to_capture"][0]
        self.assertEqual(hover[7], "ok")
        self.assertNotIn("3 of 2 runs", hover[8])

    def test_a_target_outside_a_pair_has_no_unsettled_unit(self):
        scenario = dict(self.SCENARIO, runs=4, warmup_runs=0)
        summary = self.summarize([fake_run("low", 40.0, 40.0), fake_run("low", 50.0, 40.0),
                                  fake_run("high", 60.0, 40.0), fake_run("high", 60.5, 40.0)], scenario)
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["settle"]["unsettled"], [])
        self.assertNotIn("comparison", data)


class SettleMeasureTest(unittest.TestCase):
    """measure_runs adds runs until the window settles or a cap ends the target."""

    HOST = {"name": "w", "kind": "windows", "capture_offset_ms": 0,
            "targets": [{"name": "LibrePaint", "runner_target": "LibrePaint",
                         "app": "C:/LibrePaint.exe"}]}

    def measure(self, values, runs=2, warmup_runs=0, max_runs=2, failed=()):
        calls = []
        hovers = {}

        def fake_runner(host, target, scenario, run_root):
            return ["true"], {}, None

        def fake_command(argv, env=None, cwd=None):
            calls.append(list(argv))
            number = len(calls)
            if number in failed:
                return 1
            path = os.path.join(self._root, "w", "LibrePaint-20261112-120000-%d" % number)
            os.makedirs(path)
            stamp = time.time() + number
            os.utime(path, (stamp, stamp))
            open(os.path.join(path, "capture.csv"), "w").close()
            hovers[number] = values[min(len(hovers), len(values) - 1)]
            return 0

        def analyze(path):
            return fake_run("low", hovers[int(path.rsplit("-", 1)[1])], 40.0)

        scenario = {"name": "test", "runs": runs, "warmup_runs": warmup_runs, "max_runs": max_runs,
                    "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}}
        with mock.patch.object(run, "runner_for", side_effect=fake_runner), \
                mock.patch.object(run, "run_command", side_effect=fake_command), \
                mock.patch.object(run.capture_analyze, "analyze_run", side_effect=analyze):
            failures = run.measure_runs([self.HOST], scenario, self._root, "l1", self.events)
        with open(self.events, encoding="utf-8") as handle:
            lines = [line.split("\t") for line in handle.read().splitlines()]
        return calls, failures, lines

    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self._root = os.path.join(self._temp.name, "runs")
        self.events = os.path.join(self._temp.name, "events.log")

    def settle_lines(self, lines):
        return [fields for fields in lines if fields[1] == "settle"]

    def test_a_target_outside_a_pair_stops_at_runs_whatever_the_spread(self):
        calls, failures, lines = self.measure([40.0, 61.0, 50.0, 50.4], max_runs=4)
        self.assertEqual(len(calls), 2, calls)
        self.assertEqual(failures, [])
        settle = self.settle_lines(lines)
        self.assertEqual(len(settle), 1, settle)
        self.assertIn("counted=2", settle[0][5])
        self.assertIn("window=1..2", settle[0][5])
        self.assertIn("settled=yes", settle[0][5])
        self.assertNotIn("capped", settle[0][5])

    def test_a_stable_window_at_runs_adds_no_run(self):
        calls, _, lines = self.measure([40.0, 40.4])
        self.assertEqual(len(calls), 2, calls)
        self.assertIn("settled=yes", self.settle_lines(lines)[-1][5])

    def test_a_warm_up_run_is_not_counted_and_has_no_settle_event(self):
        calls, _, lines = self.measure([40.0, 40.0, 40.4], warmup_runs=1)
        self.assertEqual(len(calls), 3, calls)
        settle = self.settle_lines(lines)
        self.assertEqual(len(settle), 1, settle)
        self.assertIn("window=2..3", settle[0][5])
        self.assertIn("counted=2", settle[0][5])

    def test_directories_of_an_interrupted_earlier_invocation_are_set_aside(self):
        stale = os.path.join(self._root, "w", "LibrePaint-20261111-090000-1")
        os.makedirs(stale)
        os.utime(stale, (1000.0, 1000.0))
        other = os.path.join(self._root, "w", "notes")
        os.makedirs(other)
        self.measure([40.0, 40.4])
        left = sorted(os.listdir(os.path.join(self._root, "w")))
        self.assertEqual(left, ["LibrePaint-20261112-120000-1", "LibrePaint-20261112-120000-2", "notes"])
        self.assertEqual(os.listdir(os.path.join(self._root, "_unmeasured", "w")),
                         ["LibrePaint-20261111-090000-1"])

    def test_the_summary_does_not_count_a_set_aside_directory(self):
        stale = os.path.join(self._root, "w", "LibrePaint-20261111-090000-1")
        os.makedirs(stale)
        os.utime(stale, (1000.0, 1000.0))
        self.measure([40.0, 40.4])
        with mock.patch.object(run.capture_analyze, "analyze_run", return_value=fake_run("low", 40.0, 40.0)):
            summary = run.summarize(self._root, [self.HOST], {
                "name": "test", "runs": 2, "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}})
        names = [r["name"] for r in summary["hosts"]["w"]["targets"]["LibrePaint"]["runs"]]
        self.assertEqual(names, ["LibrePaint-20261112-120000-1", "LibrePaint-20261112-120000-2"])

    def test_the_start_limit_ends_a_target_that_keeps_failing(self):
        calls, failures, lines = self.measure([40.0, 60.0], max_runs=4,
                                              failed=(1, 2, 3, 4, 5, 6, 7))
        self.assertEqual(len(calls), 8, calls)
        self.assertEqual(len(failures), 7)
        self.assertEqual(self.settle_lines(lines), [])


class JudgeSettleTest(unittest.TestCase):
    """The problem text, the baseline notes and the conditions of the judge."""

    SCENARIO = {"name": "test", "runs": 5,
                "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}}
    METRICS = {"hover.input_to_capture": {"median": 40.0, "p95": 45.0, "spread": 0.075, "runs": 5}}
    CONDITIONS = {"canvas": [402, 370], "frame": [402, 370], "scale": 1, "refresh": 60, "stable": True}

    def summary(self, counted=10, conditions=None, metrics=None):
        return {"hosts": {"h": {"kind": "linux", "targets": {"T": {
            "runs": [],
            "metrics": metrics or self.METRICS,
            "states": {},
            "conditions": conditions,
            "settle": {"warmup_runs": 1, "counted_runs": counted, "window_runs": 5,
                       "unsettled": ["hover.input_to_capture"]},
        }}}}}

    def baseline(self, conditions=None, metrics=None, unsettled=None):
        data = {"metrics": metrics or {"hover.input_to_capture": {
            "median": 10.0, "p95": 15.0, "spread": 0.0, "runs": 5}}}
        if conditions is not None:
            data["conditions"] = conditions
        if unsettled is not None:
            data["settle"] = {"warmup_runs": 0, "counted_runs": 5, "window_runs": 5,
                              "unsettled": unsettled}
        return {"hosts": {"h": {"targets": {"T": data}}}}

    def test_a_wide_spread_is_not_a_problem(self):
        row = run.judge(self.summary(counted=10), self.SCENARIO)[0]
        self.assertEqual(row[7], "ok")
        self.assertNotIn("spread", row[8])

    def test_an_unsettled_baseline_is_noted(self):
        row = run.judge(self.summary(), self.SCENARIO,
                        self.baseline(unsettled=["hover.input_to_capture"]))[0]
        self.assertIn("baseline unsettled", row[8])

    def test_conditions_that_differ_reject_the_delta(self):
        baseline = self.baseline(conditions=dict(self.CONDITIONS, canvas=[791, 589]))
        row = run.judge(self.summary(conditions=self.CONDITIONS), self.SCENARIO, baseline)[0]
        self.assertEqual(row[7], "check")
        self.assertIn("conditions differ from baseline: canvas 402x370 vs baseline 791x589", row[8])
        self.assertIsNone(row[6])

    def test_conditions_that_match_keep_the_delta(self):
        row = run.judge(self.summary(conditions=self.CONDITIONS), self.SCENARIO,
                        self.baseline(conditions=self.CONDITIONS))[0]
        self.assertAlmostEqual(row[6], 30.0)

    def test_a_missing_condition_is_only_noted(self):
        row = run.judge(self.summary(), self.SCENARIO, self.baseline())[0]
        self.assertIn("conditions unknown", row[8])
        self.assertAlmostEqual(row[6], 30.0)

    def test_a_window_with_changing_conditions_is_noted(self):
        conditions = dict(self.CONDITIONS, stable=False)
        row = run.judge(self.summary(conditions=conditions), self.SCENARIO)[0]
        self.assertIn("conditions vary inside the window", row[8])

    def test_no_conditions_are_noted_without_a_baseline(self):
        row = run.judge(self.summary(), self.SCENARIO)[0]
        self.assertNotIn("conditions", row[8])


class ReferenceColumnTest(unittest.TestCase):
    """The target-minus-reference column of the verdict table."""

    SCENARIO = {"name": "test", "runs": 5,
                "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}}
    HOSTS = [{"name": "h", "kind": "linux", "targets": [
        {"name": "LibrePaint"}, {"name": "Reference", "reference": True}]}]

    def summary(self, target_median=40.0, reference_median=36.9):
        return {"hosts": {"h": {"kind": "linux", "targets": {
            "LibrePaint": {"runs": [], "states": {}, "metrics": {"hover.input_to_capture": {
                "median": target_median, "p95": 45.0, "spread": 0.0, "runs": 5}}},
            "Reference": {"runs": [], "states": {}, "metrics": {"hover.input_to_capture": {
                "median": reference_median, "p95": 40.0, "spread": 0.0, "runs": 5}}},
        }}}}

    def test_the_cell_is_the_target_median_minus_the_reference_median(self):
        summary = self.summary()
        cells = run.reference_columns(summary, None, self.HOSTS)
        self.assertEqual(cells[("h", "LibrePaint", "hover.input_to_capture", None)], "+3.1")
        self.assertNotIn(("h", "Reference", "hover.input_to_capture", None), cells)
        table = run.table(run.judge(summary, self.SCENARIO), cells)
        self.assertIn("minus reference", table)
        self.assertIn("| +3.1 |", table)

    def test_the_baseline_difference_follows_in_parentheses(self):
        baseline = self.summary(target_median=37.2, reference_median=34.4)
        cells = run.reference_columns(self.summary(), baseline, self.HOSTS)
        self.assertEqual(cells[("h", "LibrePaint", "hover.input_to_capture", None)],
                         "+3.1 (base +2.8)")

    def test_a_host_without_a_reference_target_has_no_cell(self):
        hosts = [{"name": "h", "kind": "linux", "targets": [{"name": "LibrePaint"}]}]
        self.assertEqual(run.reference_columns(self.summary(), None, hosts), {})
        table = run.table(run.judge(self.summary(), self.SCENARIO), {})
        self.assertIn("| - |", table)

    def test_the_table_without_extra_has_no_reference_column(self):
        table = run.table(run.judge(self.summary(), self.SCENARIO))
        self.assertNotIn("minus reference", table)

    METRIC = "hover.input_to_capture"

    def two_reference_summary(self):
        def target(median):
            return {"runs": [], "states": {}, "metrics": {self.METRIC: {
                "median": median, "p95": 45.0, "spread": 0.0, "runs": 5}}}
        return {"hosts": {"h": {"kind": "linux", "targets": {
            "LP-x11": target(30.1), "Ref-x11": target(29.4),
            "LP-wayland": target(13.0), "Ref-wayland": target(13.9)}}}}

    def test_each_target_is_paired_with_the_reference_it_names(self):
        hosts = [{"name": "h", "kind": "linux", "targets": [
            {"name": "LP-x11", "against": "Ref-x11"}, {"name": "Ref-x11", "reference": True},
            {"name": "LP-wayland", "against": "Ref-wayland"},
            {"name": "Ref-wayland", "reference": True}]}]
        cells = run.reference_columns(self.two_reference_summary(), None, hosts)
        self.assertEqual(cells[("h", "LP-x11", self.METRIC, None)], "+0.7")
        self.assertEqual(cells[("h", "LP-wayland", self.METRIC, None)], "-0.9")
        self.assertEqual(len(cells), 2)

    def test_a_target_that_does_not_name_one_of_several_references_has_no_cell(self):
        hosts = [{"name": "h", "kind": "linux", "targets": [
            {"name": "LP-x11"}, {"name": "Ref-x11", "reference": True},
            {"name": "LP-wayland", "against": "LP-x11"},
            {"name": "Ref-wayland", "reference": True}]}]
        self.assertEqual(run.reference_columns(self.two_reference_summary(), None, hosts), {})

    def test_a_single_reference_serves_every_other_target(self):
        hosts = [{"name": "h", "kind": "linux", "targets": [
            {"name": "LP-x11"}, {"name": "LP-wayland"}, {"name": "Ref-x11", "reference": True}]}]
        cells = run.reference_columns(self.two_reference_summary(), None, hosts)
        self.assertEqual(set(cells), {("h", "LP-x11", self.METRIC, None),
                                      ("h", "LP-wayland", self.METRIC, None)})


class RunDetailStateTest(unittest.TestCase):
    """The event detail carries the state label of Windows hosts only."""

    def detail(self, kind):
        host = {"name": "h", "kind": kind, "capture_offset_ms": 0}
        result = {"state": {"label": "high"}, "hover.input_to_capture": {"p50": 30.0},
                  "ink.input_to_capture": {"p50": 31.0}}
        with mock.patch.object(run, "latest_run_dir", return_value="/run"), \
                mock.patch.object(run.os.path, "exists", return_value=True), \
                mock.patch.object(run.capture_analyze, "analyze_run", return_value=result):
            return run.run_detail("/root", host, {"name": "T"}, 0.0)

    def test_a_windows_run_reports_its_state_label(self):
        self.assertEqual(self.detail("windows"), "state=high hover=30.0 ink=31.0")

    def test_a_linux_or_macos_run_reports_no_state(self):
        self.assertEqual(self.detail("linux"), "state=- hover=30.0 ink=31.0")
        self.assertEqual(self.detail("macos"), "state=- hover=30.0 ink=31.0")


class NonWindowsStateTest(unittest.TestCase):
    """Only a Windows target is split by state."""

    SCENARIO = {"name": "test", "runs": 2,
                "metrics": {"hover.input_to_capture": {"regression_ms": 2.0},
                            "ink.input_to_capture": {"regression_ms": 2.0}}}

    def test_a_linux_target_has_no_state_groups_and_one_hover_row(self):
        with tempfile.TemporaryDirectory() as root:
            capture_dirs(root, 2)
            host = {"name": "h", "kind": "linux", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
            with mock.patch.object(run.capture_analyze, "analyze_run",
                                   side_effect=[fake_run("low", 55.0, 61.0),
                                                fake_run("high", 62.0, 61.0)]):
                summary = run.summarize(root, [host], self.SCENARIO)
        data = summary["hosts"]["h"]["targets"]["T"]
        self.assertEqual(data["states"], {})
        rows = run.judge(summary, self.SCENARIO)
        self.assertEqual({row[9] for row in rows if row[2] == "hover.input_to_capture"}, {None})
        self.assertEqual(run.speed_table(data["runs"]), [])

    def test_a_windows_target_keeps_the_state_groups(self):
        with tempfile.TemporaryDirectory() as root:
            capture_dirs(root, 2)
            host = {"name": "h", "kind": "windows", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
            with mock.patch.object(run.capture_analyze, "analyze_run",
                                   side_effect=[fake_run("low", 55.0, 61.0),
                                                fake_run("high", 62.0, 61.0)]):
                summary = run.summarize(root, [host], self.SCENARIO)
        self.assertEqual(sorted(summary["hosts"]["h"]["targets"]["T"]["states"]), ["high", "low"])


class BaselineDocumentTest(unittest.TestCase):
    """The allowlisted baseline document and its use as a baseline."""

    SCENARIO = {"name": "test", "runs": 2, "warmup_runs": 1, "max_runs": 4,
                "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}}
    HOSTS = [{"name": "windows", "kind": "windows", "ssh": "/tmp/secret-host",
              "system": "tablet", "capture_offset_ms": 0,
              "targets": [{"name": "T", "runner_target": "LibrePaint", "reference": True},
                          {"name": "U", "runner_target": "Krita"}]}]

    def summary(self):
        stat = {"median": 40.0, "p95": 45.0, "spread": 0.01, "runs": 2}
        return {
            "label": "l1", "date": "2026-11-15", "commit": "abc1234567", "scenario": "test",
            "hosts": {"windows": {
                "kind": "windows", "system": "tablet", "capture_offset_ms": 0,
                "targets": {
                    "T": {"runs": [
                        {"name": "/tmp/secret-run/T-1", "in_window": False,
                         "metrics": {"hover.input_to_capture": {"n": 1, "p50": 99.0}}},
                        {"name": "/tmp/secret-run/T-2", "in_window": True,
                         "metrics": {"hover.input_to_capture": {"n": 1, "p50": 40.0}}},
                    ], "metrics": {"hover.input_to_capture": dict(stat)},
                        "states": {"low": {"runs": 2, "metrics": {"hover.input_to_capture": dict(stat)}}},
                        "conditions": {"canvas": [402, 370], "frame": [402, 370],
                                       "scale": 1, "refresh": 60, "stable": True},
                        "settle": {"warmup_runs": 1, "counted_runs": 2, "window_runs": 1,
                                   "unsettled": []}},
                    "U": {"runs": [], "metrics": {"hover.input_to_capture": dict(stat, median=30.0)},
                          "states": {}, "conditions": None,
                          "settle": {"warmup_runs": 0, "counted_runs": 2, "window_runs": 1,
                                     "unsettled": []}},
                },
            }},
        }

    def test_only_the_allowlisted_keys_are_written(self):
        document = run.baseline_document(self.summary(), self.HOSTS, self.SCENARIO)
        self.assertEqual(set(document), {"format", "label", "date", "commit", "scenario",
                                         "settings", "hosts"})
        self.assertEqual(document["format"], 1)
        self.assertEqual(document["settings"], {"runs": 2, "warmup_runs": 1, "max_runs": 4})
        target = document["hosts"]["windows"]["targets"]["T"]
        self.assertEqual(set(target), {"reference", "metrics", "states", "run_medians",
                                       "conditions", "settle"})
        self.assertEqual(set(target["metrics"]["hover.input_to_capture"]),
                         {"median", "p95", "spread", "runs"})
        self.assertTrue(target["reference"])
        self.assertFalse(document["hosts"]["windows"]["targets"]["U"]["reference"])
        self.assertEqual(target["run_medians"]["hover.input_to_capture"], [40.0])

    def test_no_run_name_path_or_ssh_value_appears(self):
        text = json.dumps(run.baseline_document(self.summary(), self.HOSTS, self.SCENARIO))
        for secret in ("/tmp/secret", "secret-host", "runner_target", "LibrePaint"):
            self.assertNotIn(secret, text)

    def test_the_document_is_readable_as_a_baseline(self):
        document = run.baseline_document(self.summary(), self.HOSTS, self.SCENARIO)
        self.assertEqual(run.baseline_metric(document, "windows", "T",
                                             "hover.input_to_capture", None)["median"], 40.0)
        self.assertEqual(run.baseline_metric(document, "windows", "T",
                                             "hover.input_to_capture", "low")["median"], 40.0)
        rows = run.judge(self.summary(), self.SCENARIO, document)
        self.assertEqual({row[9] for row in rows
                          if row[1] == "T" and row[2] == "hover.input_to_capture"}, {"low"})


class BaselineOutTest(unittest.TestCase):
    """--baseline-out writes the document after the summary."""

    def test_main_writes_the_allowlisted_baseline(self):
        scenario = {"name": "test", "runs": 2, "warmup_runs": 0, "max_runs": 2,
                    "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}}}
        with tempfile.TemporaryDirectory() as root:
            run_root = os.path.join(root, "runs")
            capture_dirs(run_root, 2)
            scenario_path = os.path.join(root, "scenario.json")
            hosts_path = os.path.join(root, "hosts.json")
            baseline_path = os.path.join(root, "baseline.json")
            with open(scenario_path, "w") as handle:
                json.dump(scenario, handle)
            with open(hosts_path, "w") as handle:
                json.dump({"hosts": [{"name": "h", "kind": "linux", "capture_offset_ms": 0,
                                      "targets": [{"name": "T"}]}]}, handle)
            with mock.patch.object(run.capture_analyze, "analyze_run",
                                   side_effect=[fake_run("low", 40.0, 40.0), fake_run("low", 40.5, 40.0)]), \
                    mock.patch.object(run.subprocess, "run",
                                      return_value=mock.Mock(stdout="0123456789\n")), \
                    redirect_stdout(io.StringIO()):
                status = run.main(["--scenario", scenario_path, "--hosts", hosts_path,
                                   "--analyze-only", run_root, "--baseline-out", baseline_path])
            with open(baseline_path) as handle:
                document = json.load(handle)
            self.assertEqual(status, 0)
        self.assertEqual(document["commit"], "0123456789")
        self.assertEqual(document["settings"]["runs"], 2)
        self.assertNotIn(root, json.dumps(document))


if __name__ == "__main__":
    unittest.main()
