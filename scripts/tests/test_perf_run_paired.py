# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of the before and after comparison of scripts/perf/run.py.

The CHARTER judges a change by measuring the version before the change and
the changed version alternately and taking the 95 % interval of the paired
differences of the run medians. These tests fix the interval and its
verdict, the pairing of the targets in the hosts file, the runs that
measure_runs adds while the comparison is inconclusive, and how the summary
and the verdict rows carry the comparison.
"""

import importlib.util
import os
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location("perf_run_paired", os.path.join(ROOT, "scripts", "perf", "run.py"))
run = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(run)

METRIC = "hover.input_to_capture"
SCENARIO = {"name": "test", "runs": 3, "warmup_runs": 0, "max_runs": 6,
            "metrics": {METRIC: {"regression_ms": 2.0}}}
HOST = {"name": "l", "kind": "linux", "capture_offset_ms": 0,
        "targets": [{"name": "Old"}, {"name": "New", "before": "Old"}]}


def fake_run(hover):
    return {METRIC: {"n": 100, "p05": hover - 5, "p50": hover, "p95": hover + 10},
            "state": {"label": "low"}, "presentmon": {"present": False, "status": "none"},
            "excluded": None}


class PairedIntervalTest(unittest.TestCase):
    def test_a_clear_reduction_is_an_improvement(self):
        interval = run.paired_interval([40.0, 41.0, 40.5, 40.2], [35.0, 35.5, 35.2, 35.1])
        self.assertEqual(interval["verdict"], "improvement")
        self.assertEqual(interval["pairs"], 4)
        self.assertLess(interval["high"], 0)

    def test_a_clear_increase_is_a_regression(self):
        interval = run.paired_interval([40.0, 41.0, 40.5], [45.0, 46.2, 45.4])
        self.assertEqual(interval["verdict"], "regression")
        self.assertGreater(interval["low"], 0)

    def test_noise_around_zero_is_inconclusive(self):
        interval = run.paired_interval([40.0, 41.0, 40.5, 40.2], [41.0, 40.0, 40.9, 39.8])
        self.assertEqual(interval["verdict"], "inconclusive")
        self.assertLess(interval["low"], 0)
        self.assertGreater(interval["high"], 0)

    def test_the_interval_uses_the_t_quantile_of_the_pair_count(self):
        interval = run.paired_interval([0.0, 0.0, 0.0], [1.0, 2.0, 3.0])
        self.assertAlmostEqual(interval["mean"], 2.0)
        self.assertAlmostEqual(interval["high"] - interval["mean"], 4.303 * 1.0 / 3 ** 0.5, places=6)

    def test_values_without_a_partner_are_left_out(self):
        interval = run.paired_interval([40.0, 40.0, 40.0], [30.0, 31.0])
        self.assertEqual(interval["pairs"], 2)

    def test_fewer_than_two_pairs_give_no_interval(self):
        self.assertIsNone(run.paired_interval([40.0], [30.0, 31.0]))

    def test_identical_versions_are_inconclusive(self):
        self.assertEqual(run.paired_interval([40.0, 40.0], [40.0, 40.0])["verdict"], "inconclusive")


class CaptureMetricTest(unittest.TestCase):
    def test_only_input_to_capture_times_carry_the_offset(self):
        stat = {"n": 10, "p05": 20.0, "p50": 30.0, "p95": 40.0}
        self.assertEqual(run.capture_metric(stat, "ink.input_to_capture", 16)["p50"], 14.0)
        self.assertEqual(run.capture_metric(stat, "l2p.residual_px", 16)["p50"], 30.0)

    def test_a_distance_is_reported_in_pixels(self):
        self.assertEqual(run.metric_unit("l2p.residual_px"), "px")
        self.assertEqual(run.metric_unit("ink.input_to_capture"), "ms")


class ComparisonUnitsTest(unittest.TestCase):
    def test_windows_hover_pairs_runs_of_different_states(self):
        def runs(state, values):
            return [{"metrics": {METRIC: {"n": 100, "p50": value}}, "state": {"label": state}}
                    for value in values]
        units = run.comparison_units(runs("low", [54.0, 53.5, 54.2]), runs("high", [16.0, 16.4, 16.1]),
                                     "windows", SCENARIO)
        self.assertEqual(len(units), 1)
        metric, state, interval = units[0]
        self.assertEqual((metric, state, interval["verdict"]), (METRIC, None, "improvement"))
        self.assertEqual(interval["pairs"], 3)


class BeforePairsTest(unittest.TestCase):
    def test_a_target_names_the_target_before_its_change(self):
        self.assertEqual(run.before_pairs(HOST), {"New": "Old"})

    def test_an_unknown_before_target_is_refused(self):
        host = {"name": "l", "targets": [{"name": "New", "before": "Missing"}]}
        with self.assertRaises(SystemExit):
            run.before_pairs(host)

    def test_a_target_cannot_be_its_own_before_target(self):
        with self.assertRaises(SystemExit):
            run.before_pairs({"name": "l", "targets": [{"name": "New", "before": "New"}]})


class PairedMeasureTest(unittest.TestCase):
    """measure_runs adds runs to both targets of a pair while it is inconclusive."""

    def setUp(self):
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self._root = os.path.join(self._temp.name, "runs")
        self.events = os.path.join(self._temp.name, "events.log")

    def measure(self, old, new, max_runs=6, old_fails_from=None):
        calls = []
        values = {}
        counts = {"Old": 0, "New": 0}
        starts = {"Old": 0, "New": 0}
        series = {"Old": old, "New": new}

        def fake_runner(host, target, scenario, run_root):
            return ["true", target["name"]], {}, None

        def fake_command(argv, env=None, cwd=None):
            calls.append(argv[1])
            number = len(calls)
            name = argv[1]
            starts[name] += 1
            if name == "Old" and old_fails_from is not None and starts[name] >= old_fails_from:
                return 1
            path = os.path.join(self._root, "l", "%s-20261125-120000-%d" % (name, number))
            os.makedirs(path)
            stamp = time.time() + number
            os.utime(path, (stamp, stamp))
            open(os.path.join(path, "capture.csv"), "w").close()
            sequence = series[name]
            values[os.path.basename(path)] = sequence[min(counts[name], len(sequence) - 1)]
            counts[name] += 1
            return 0

        def analyze(path):
            return fake_run(values[os.path.basename(path)])

        scenario = dict(SCENARIO, max_runs=max_runs)
        with mock.patch.object(run, "runner_for", side_effect=fake_runner), \
                mock.patch.object(run, "run_command", side_effect=fake_command), \
                mock.patch.object(run.capture_analyze, "analyze_run", side_effect=analyze):
            failures = run.measure_runs([HOST], scenario, self._root, "pair", self.events)
        with open(self.events, encoding="utf-8") as handle:
            lines = [line.split("\t") for line in handle.read().splitlines()]
        settle = [fields for fields in lines if fields[1] == "settle"]
        return calls, failures, settle

    def test_a_clear_difference_needs_no_further_run(self):
        calls, failures, settle = self.measure([40.0, 40.3, 40.1], [30.0, 30.2, 30.1])
        self.assertEqual(calls, ["Old", "New"] * 3)
        self.assertEqual(failures, [])
        new = [fields for fields in settle if fields[3] == "New"][-1][5]
        self.assertIn("settled=yes", new)
        self.assertIn("vs-before:hover.input_to_capture=-10.0[", new)

    def test_both_targets_get_runs_until_the_difference_is_shown(self):
        old = [40.0, 41.0, 40.0, 40.0, 40.0, 40.0]
        new = [38.0, 41.5, 38.0, 38.0, 38.0, 38.0]
        calls, _, settle = self.measure(old, new)
        self.assertEqual(calls.count("Old"), calls.count("New"))
        self.assertGreater(len(calls), 6)
        self.assertLess(len(calls), 12)
        self.assertIn("settled=no", [f for f in settle if f[3] == "New"][0][5])
        self.assertIn("settled=yes", [f for f in settle if f[3] == "New"][-1][5])

    def test_an_inconclusive_pair_stops_at_max_runs(self):
        old = [40.0, 41.0, 40.0, 41.0]
        new = [41.0, 40.0, 41.0, 40.0]
        calls, _, settle = self.measure(old, new, max_runs=4)
        self.assertEqual(calls, ["Old", "New"] * 4)
        last = [fields for fields in settle if fields[3] == "New"][-1][5]
        self.assertIn("settled=no", last)
        self.assertIn("capped=runs", last)

    def test_the_start_limit_ends_a_pair_whose_before_target_keeps_failing(self):
        old = [40.0, 41.0, 40.0]
        new = [41.0, 40.0, 40.5]
        calls, failures, _ = self.measure(old, new, max_runs=4, old_fails_from=4)
        self.assertEqual(calls.count("Old"), 2 * 4)
        self.assertEqual(calls.count("New"), 4)
        self.assertEqual(len(failures), 5)


class ChainedPairMeasureTest(unittest.TestCase):
    """A target that is the after side of one pair and the before side of another."""

    HOST = {"name": "l", "kind": "linux", "capture_offset_ms": 0,
            "targets": [{"name": "Old"}, {"name": "Mid", "before": "Old"},
                        {"name": "New", "before": "Mid"}]}

    def test_the_middle_target_runs_while_its_later_pair_is_inconclusive(self):
        series = {"Old": [40.0, 40.3, 40.1], "Mid": [30.0, 30.2, 30.1],
                  "New": [31.0, 29.0, 30.5, 29.6, 30.4, 29.8]}
        calls, values, counts = [], {}, {"Old": 0, "Mid": 0, "New": 0}
        with tempfile.TemporaryDirectory() as temp:
            root = os.path.join(temp, "runs")
            events = os.path.join(temp, "events.log")

            def fake_runner(host, target, scenario, run_root):
                return ["true", target["name"]], {}, None

            def fake_command(argv, env=None, cwd=None):
                calls.append(argv[1])
                name = argv[1]
                path = os.path.join(root, "l", "%s-20261125-120000-%d" % (name, len(calls)))
                os.makedirs(path)
                stamp = time.time() + len(calls)
                os.utime(path, (stamp, stamp))
                open(os.path.join(path, "capture.csv"), "w").close()
                values[os.path.basename(path)] = series[name][min(counts[name], len(series[name]) - 1)]
                counts[name] += 1
                return 0

            with mock.patch.object(run, "runner_for", side_effect=fake_runner), \
                    mock.patch.object(run, "run_command", side_effect=fake_command), \
                    mock.patch.object(run.capture_analyze, "analyze_run",
                                      side_effect=lambda path: fake_run(values[os.path.basename(path)])):
                run.measure_runs([self.HOST], dict(SCENARIO, max_runs=5), root, "chain", events)
        self.assertEqual(calls.count("Mid"), calls.count("New"))
        self.assertGreater(calls.count("Mid"), 3)
        self.assertEqual(calls.count("Old"), 3)


class PairedSummaryTest(unittest.TestCase):
    """The summary carries the comparison and the verdict rows report it."""

    def summarize(self, old, new):
        with tempfile.TemporaryDirectory() as root:
            for name, values in (("Old", old), ("New", new)):
                for index, _ in enumerate(values, start=1):
                    path = os.path.join(root, "l", "%s-20261125-120000-%d" % (name, index))
                    os.makedirs(path)
                    open(os.path.join(path, "capture.csv"), "w").close()
            results = [fake_run(value) for value in list(new) + list(old)]
            with mock.patch.object(run.capture_analyze, "analyze_run", side_effect=results):
                return run.summarize(root, [HOST], SCENARIO)

    def row(self, summary):
        return [row for row in run.judge(summary, SCENARIO) if row[1] == "New"][0]

    def test_an_improvement_is_recorded_and_noted(self):
        summary = self.summarize([40.0, 40.3, 40.1], [30.0, 30.2, 30.1])
        data = summary["hosts"]["l"]["targets"]["New"]
        unit = data["comparison"]["units"][0]
        self.assertEqual(data["comparison"]["before"], "Old")
        self.assertEqual((unit["metric"], unit["state"], unit["verdict"]), (METRIC, None, "improvement"))
        self.assertEqual(data["settle"]["unsettled"], [])
        row = self.row(summary)
        self.assertEqual(row[7], "ok")
        self.assertIn("improvement vs Old -10.0 ms", row[8])

    def test_a_regression_needs_checking(self):
        row = self.row(self.summarize([30.0, 30.2, 30.1], [40.0, 40.3, 40.1]))
        self.assertEqual(row[7], "check")
        self.assertIn("regression vs Old +10.0 ms", row[8])

    def test_an_inconclusive_unit_is_unsettled_and_noted(self):
        summary = self.summarize([40.0, 41.0, 40.0], [41.0, 40.0, 40.5])
        data = summary["hosts"]["l"]["targets"]["New"]
        self.assertEqual(data["settle"]["unsettled"], [METRIC])
        row = self.row(summary)
        self.assertEqual(row[7], "ok")
        self.assertIn("not shown vs Old", row[8])

    def test_the_before_target_carries_no_comparison(self):
        summary = self.summarize([40.0, 40.3, 40.1], [30.0, 30.2, 30.1])
        self.assertNotIn("comparison", summary["hosts"]["l"]["targets"]["Old"])


if __name__ == "__main__":
    unittest.main()
