# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/prediction_replay_synthetic.py.

The replay input of a synthetic scenario keeps only the moving samples of the
pressed strokes. A still dwell before the line and the release after it would
add samples that a capture never shows the predictor, and the timing columns
have to carry the injection time in the units of the replay loader.
"""

import importlib.util
import json
import os
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, "scripts", "perf", "pen-latency")
SCENARIOS = os.path.join(ROOT, "scripts", "perf", "scenarios")


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(TOOLS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


synthetic = load("prediction_replay_synthetic")
trajectory = load("trajectory")

WIDTH = 1024
HEIGHT = 768
RECT = (0.0, 0.0, WIDTH, HEIGHT)


def read_rows(path):
    """The rows of the CSV as (stroke, arrival_ns, event_ms, x, y, pressure)."""
    with open(path) as handle:
        header = handle.readline().strip()
        if header != "stroke,arrival_ns,event_ms,x,y,pressure":
            raise AssertionError("unexpected header: " + header)
        rows = []
        for line in handle:
            fields = line.strip().split(",")
            rows.append((
                int(fields[0]),
                int(fields[1]),
                float(fields[2]),
                float(fields[3]),
                float(fields[4]),
                float(fields[5]),
            ))
    return rows


def moving_samples(settings):
    """The pressed samples whose phase starts with "move"."""
    return [sample for sample in trajectory.samples(*RECT, settings)
            if sample[5] and sample[1].startswith("move")]


class SyntheticReplayInputTest(unittest.TestCase):
    def convert(self, settings):
        with tempfile.TemporaryDirectory() as directory:
            scenario = os.path.join(directory, "scenario.json")
            output = os.path.join(directory, "input.csv")
            with open(scenario, "w") as handle:
                json.dump({"trajectory": settings}, handle)
            strokes = synthetic.convert(scenario, output, WIDTH, HEIGHT)
            return strokes, read_rows(output)

    def test_a_stroke_keeps_only_its_moving_samples(self):
        settings = {"hover_passes": 1, "strokes": 2, "speed": 1.0,
                    "dwell": 0.05, "rate": 240.0}
        strokes, rows = self.convert(settings)

        source = list(trajectory.samples(*RECT, settings))
        expected = moving_samples(settings)

        # The still start of a stroke and the release are what the replay must
        # not see; both are part of the uncompressed trajectory.
        self.assertTrue([s for s in source if s[5] and s[1] == "dwell"])
        self.assertTrue([s for s in source if s[1] == "release"])

        self.assertEqual(strokes, 2)
        self.assertEqual(len(rows), len(expected))
        self.assertEqual([row[0] for row in rows],
                         [int(sample[2][len("stroke"):]) for sample in expected])
        for row, sample in zip(rows, expected):
            self.assertAlmostEqual(row[3], sample[3], places=5)
            self.assertAlmostEqual(row[4], sample[4], places=5)
            self.assertEqual(row[5], 1.0)

    def test_the_time_columns_use_milliseconds_and_nanoseconds(self):
        settings = {"hover_passes": 0, "strokes": 1, "speed": 1.0,
                    "dwell": 0.05, "rate": 240.0}
        _, rows = self.convert(settings)
        expected = moving_samples(settings)

        self.assertGreater(len(rows), 10)
        for row, sample in zip(rows, expected):
            seconds = sample[0]
            self.assertAlmostEqual(row[2], seconds * 1000.0, places=3)
            self.assertEqual(row[1], round(seconds * 1e9))

        # The samples come at the injection rate, so the event times advance
        # by the reciprocal of the rate in milliseconds.
        intervals = [b[2] - a[2] for a, b in zip(rows, rows[1:])]
        self.assertAlmostEqual(sum(intervals) / len(intervals), 1000.0 / 240.0, places=2)

    def test_a_decelerating_stroke_keeps_its_deceleration_samples(self):
        settings = {"hover_passes": 0, "strokes": 1, "speed": 3.0,
                    "stop_decel_ms": 90, "dwell": 0.05, "rate": 240.0}
        _, rows = self.convert(settings)
        expected = moving_samples(settings)
        deceleration = [sample for sample in expected if sample[1] == "move_decel"]

        self.assertGreater(len(deceleration), 3)
        self.assertEqual(len(rows), len(expected))

    def test_a_scenario_of_the_measurements_is_converted(self):
        with open(os.path.join(SCENARIOS, "pen-light-speed-3p0.json")) as handle:
            settings = json.load(handle)["trajectory"]

        strokes, rows = self.convert(settings)
        expected = moving_samples(settings)

        self.assertEqual(strokes, int(settings["strokes"]))
        self.assertEqual(len(rows), len(expected))
        self.assertEqual(sorted(set(row[0] for row in rows)), list(range(strokes)))


if __name__ == "__main__":
    unittest.main()
