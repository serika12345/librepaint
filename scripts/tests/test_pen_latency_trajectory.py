# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of the speed profiles in scripts/perf/pen-latency/trajectory.py.

A measurement of fast strokes needs lines that change speed inside one
stroke. These tests fix that the samples stay on the line, cover the whole
stroke, and carry the profile's label and the requested speed.
"""

import importlib.util
import json
import os
import statistics
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, "scripts", "perf", "pen-latency")
SCENARIOS = os.path.join(ROOT, "scripts", "perf", "scenarios")
SPEC = importlib.util.spec_from_file_location("pen_latency_trajectory", os.path.join(TOOLS, "trajectory.py"))
trajectory = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(trajectory)

RECT = (100, 100, 1000, 600)


def measured_speed(samples):
    """Median pointer speed in px/ms for consecutive samples of one phase."""
    speeds = []
    for first, second in zip(samples, samples[1:]):
        milliseconds = (second[0] - first[0]) * 1000.0
        if milliseconds > 0:
            speeds.append(abs(second[3] - first[3]) / milliseconds)
    return statistics.median(speeds) if speeds else None


class SpeedProfileTest(unittest.TestCase):
    def options(self, **overrides):
        options = {"hover_passes": 0, "strokes": 2, "dwell": 0.05, "rate": 240.0}
        options.update(overrides)
        return options

    def test_a_profile_changes_speed_inside_one_stroke(self):
        profile = [
            {"until": 0.5, "speed": 0.3, "label": "move_slow"},
            {"until": 1.0, "speed": 3.0, "label": "move_fast"},
        ]
        samples = list(trajectory.samples(*RECT, self.options(speed_profiles=[profile])))
        moving = [sample for sample in samples if sample[2] == "stroke0" and sample[1] != "dwell"]
        slow = [sample for sample in moving if sample[1] == "move_slow"]
        fast = [sample for sample in moving if sample[1] == "move_fast"]
        self.assertGreater(len(slow), 10)
        self.assertGreater(len(fast), 10)
        self.assertAlmostEqual(measured_speed(slow), 0.3, delta=0.03)
        self.assertAlmostEqual(measured_speed(fast), 3.0, delta=0.3)
        xs = [sample[3] for sample in moving]
        self.assertEqual(xs, sorted(xs))
        self.assertGreater(xs[-1], xs[0] + 0.7 * RECT[2])

    def test_profiles_are_used_in_turn(self):
        profiles = [
            [{"until": 1.0, "speed": 0.5, "label": "move_a"}],
            [{"until": 1.0, "speed": 2.0, "label": "move_b"}],
        ]
        samples = list(trajectory.samples(*RECT, self.options(speed_profiles=profiles)))
        labels = {}
        for band in ("stroke0", "stroke1"):
            labels[band] = set(sample[1] for sample in samples
                               if sample[2] == band and sample[1].startswith("move"))
        self.assertEqual(labels["stroke0"], {"move_a"})
        self.assertEqual(labels["stroke1"], {"move_b"})

    def test_a_profile_must_end_at_one(self):
        with self.assertRaises(ValueError):
            trajectory.profile_segments([{"until": 0.5, "speed": 1.0}])
        with self.assertRaises(ValueError):
            trajectory.profile_segments([{"until": 0.5, "speed": 0.0}, {"until": 1.0, "speed": 1.0}])


def scenario_trajectory(name):
    with open(os.path.join(SCENARIOS, name + ".json")) as handle:
        return json.load(handle)["trajectory"]


class StopDecelerationTest(unittest.TestCase):
    """A stroke that decelerates to rest over stop_decel_ms.

    The last part of a pressed stroke falls from the stroke's speed to zero,
    so the capture analysis can measure what the prediction line does after a
    slow stop as well as after a sudden one.
    """

    def options(self, **overrides):
        options = {"hover_passes": 0, "strokes": 1, "dwell": 0.05, "rate": 240.0}
        options.update(overrides)
        return options

    def deceleration(self, samples, band="stroke0"):
        return [sample for sample in samples if sample[2] == band and sample[1] == "move_decel"]

    def pair_speeds(self, samples):
        speeds = []
        for first, second in zip(samples, samples[1:]):
            milliseconds = (second[0] - first[0]) * 1000.0
            speeds.append(abs(second[3] - first[3]) / milliseconds)
        return speeds

    def test_the_default_and_zero_keep_the_output(self):
        for name in ("pen-light-speed-3p0", "pen-light-speed-varied"):
            settings = scenario_trajectory(name)
            plain = list(trajectory.samples(*RECT, settings))
            zero = list(trajectory.samples(*RECT, dict(settings, stop_decel_ms=0)))
            self.assertEqual(plain, zero, name)

    def check_deceleration(self, v0, **overrides):
        samples = list(trajectory.samples(*RECT, self.options(**overrides)))
        decel = self.deceleration(samples)
        self.assertGreater(len(decel), 3)
        self.assertEqual(decel[-1][3], RECT[0] + 0.9 * RECT[2])
        speeds = self.pair_speeds(decel)
        self.assertEqual(speeds, sorted(speeds, reverse=True))
        self.assertLessEqual(abs(speeds[0] - v0), 0.05 * v0)
        self.assertLess(speeds[-1], 0.1 * v0)
        self.assertFalse([sample for sample in samples
                          if sample[2] == "hover" and sample[1] == "move_decel"])

    def test_a_constant_speed_stroke_decelerates_to_rest(self):
        self.check_deceleration(3.0, speed=3.0, stop_decel_ms=90)

    def test_a_speed_profile_stroke_decelerates_from_its_last_speed(self):
        profile = [
            {"until": 0.5, "speed": 0.3, "label": "move_slow"},
            {"until": 1.0, "speed": 3.0, "label": "move_fast"},
        ]
        self.check_deceleration(3.0, speed=0.5, speed_profiles=[profile], stop_decel_ms=90)

    def test_a_deceleration_covering_the_whole_stroke_is_rejected(self):
        with self.assertRaises(ValueError):
            list(trajectory.samples(*RECT, self.options(speed=3.0, stop_decel_ms=600)))

    def test_a_deceleration_covering_the_last_profile_segment_is_rejected(self):
        profile = [
            {"until": 0.9, "speed": 0.3, "label": "move_slow"},
            {"until": 1.0, "speed": 3.0, "label": "move_fast"},
        ]
        with self.assertRaises(ValueError):
            list(trajectory.samples(*RECT, self.options(
                speed=0.5, speed_profiles=[profile], stop_decel_ms=90)))

    def test_twelve_strokes_keep_separate_bands(self):
        bands = trajectory.bands(RECT[1], RECT[3], trajectory.options({"strokes": 12}))
        self.assertEqual(len(bands), 13)
        for (name_a, y_a, half_a), (name_b, y_b, half_b) in zip(bands, bands[1:]):
            self.assertGreater(y_b - half_b, y_a + half_a, "%s and %s" % (name_a, name_b))


if __name__ == "__main__":
    unittest.main()
