# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/run.py.

Performance changes are accepted or rejected by the verdict of run.py. These
tests fix how run directories are assigned to targets, how the capture clock
correction is applied, and when a result is flagged: too few runs, spread of
run medians at or above the scenario limit, or a median above the baseline by
more than the scenario threshold.
"""

import importlib.util
import os
import sys
import tempfile
import unittest

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
