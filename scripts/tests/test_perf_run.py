# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/run.py.

Performance changes are accepted or rejected by the verdict of run.py. These
tests fix how run directories are assigned to targets, how the capture clock
correction is applied, and when a result is flagged: too few runs, spread of
run medians at or above the scenario limit, or a median above the baseline by
more than the scenario threshold.
"""

import importlib.util
import json
import os
import sys
import tempfile
import unittest
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


def fake_run(state, hover, ink, presentmon_status="present"):
    return {
        "hover.input_to_capture": {"n": 100, "p05": hover - 5, "p50": hover, "p95": hover + 10},
        "ink.input_to_capture": {"n": 100, "p05": ink - 5, "p50": ink, "p95": ink + 10},
        "state": {"label": state},
        "presentmon": {"present": presentmon_status != "none", "status": presentmon_status},
    }


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


if __name__ == "__main__":
    unittest.main()
