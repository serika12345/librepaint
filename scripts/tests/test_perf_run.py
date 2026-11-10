# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/run.py.

Performance changes are accepted or rejected by the verdict of run.py. These
tests fix how run directories are assigned to targets, how the capture clock
correction is applied, and when a result is flagged: too few runs, spread of
run medians at or above the scenario limit, or a median above the baseline by
more than the scenario threshold.
"""

import importlib.util
import io
import json
import os
import sys
import tempfile
import unittest
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
    "spread_limit": 0.05,
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
                handle.write("surface_state_during_max_temp_c=58.0\n")
                handle.write("surface_state_during_min_processor_performance_pct=60\n")
                handle.write("surface_state_top_process=MsMpEng 40%\n")
            state = run.surface_state(root)
        self.assertEqual(state["mode"], "light")
        self.assertEqual(state["during_max_temp_c"], "58.0")
        self.assertEqual(state["during_min_processor_performance_pct"], "60")
        self.assertEqual(state["top_process"], "MsMpEng 40%")
        self.assertNotIn("foreground_capture", state)

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

    def test_device_state_recording_is_off_unless_the_target_asks_for_it(self):
        scenario = {"trajectory": {}, "name": "test"}
        host = {"name": "w", "kind": "windows", "ssh": "host", "capture_tool": "/tmp/capture"}
        target = {"name": "T", "runner_target": "LibrePaint", "app": "C:/LibrePaint.exe"}
        with tempfile.TemporaryDirectory() as root:
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

    def test_surface_state_on_a_non_windows_target_is_rejected(self):
        host = {"name": "l", "kind": "linux", "ssh": "host", "repo": "/r", "work": "/w",
                "remote_out": "/o", "capture_offset_ms": 16}
        target = {"name": "T", "command": ["app"], "surface_state": "on"}
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
        "spread_limit": 0.05,
        "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}, "ink.input_to_capture": {"regression_ms": 2.0}},
    }

    def summarize(self, results):
        with tempfile.TemporaryDirectory() as root:
            for n in range(1, len(results) + 1):
                path = os.path.join(root, "h", "T-20261108-120000-%d" % n)
                os.makedirs(path)
                open(os.path.join(path, "capture.csv"), "w").close()
            host = {"name": "h", "kind": "linux", "capture_offset_ms": 0, "targets": [{"name": "T"}]}
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


class VerdictTest(unittest.TestCase):
    def verdict(self, summary, baseline=None):
        return run.judge(summary, SCENARIO, baseline)[0]

    def test_stable_result_without_baseline_is_ok(self):
        self.assertEqual(self.verdict(summary_with(40.0))[7], "ok")

    def test_spread_at_limit_needs_checking(self):
        row = self.verdict(summary_with(40.0, spread=0.05))
        self.assertEqual(row[7], "check")
        self.assertIn("spread", row[8])

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

    def run_main(self, hovers):
        scenario = {
            "name": "test",
            "runs": len(hovers),
            "spread_limit": 0.05,
            "metrics": {"hover.input_to_capture": {"regression_ms": 2.0}},
        }
        with tempfile.TemporaryDirectory() as root:
            run_root = os.path.join(root, "runs")
            for index, _ in enumerate(hovers, start=1):
                path = os.path.join(run_root, "h", "T-20261108-120000-%d" % index)
                os.makedirs(path)
                open(os.path.join(path, "capture.csv"), "w").close()
            scenario_path = os.path.join(root, "scenario.json")
            hosts_path = os.path.join(root, "hosts.json")
            with open(scenario_path, "w") as handle:
                json.dump(scenario, handle)
            with open(hosts_path, "w") as handle:
                json.dump({"hosts": [{"name": "h", "kind": "linux", "capture_offset_ms": 0,
                                      "targets": [{"name": "T"}]}]}, handle)
            results = []
            for hover in hovers:
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
        status, output = self.run_main([40.0, 50.0])
        self.assertIn("== ink speed groups", output)
        self.assertIn("| check |", output)
        self.assertEqual(status, 1)


if __name__ == "__main__":
    unittest.main()
