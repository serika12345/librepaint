# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/ui-check/check_ui.py."""

import importlib.util
import os
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location("check_ui", os.path.join(ROOT, "scripts", "perf", "ui-check", "check_ui.py"))
check_ui = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_ui)


def states(popups=None, platform="wayland", pressure=None):
    return [
        {"state": "start", "platform": platform, "main": [0, 0, 1530, 711], "visible": True},
        {"state": "tablet", "platform": platform},
        {"state": "tablet_result", "platform": platform, "events": 600, "pressed": 300,
         "pressure": [0.1, 1.0] if pressure is None else pressure, "x_tilt": [-30, 30], "y_tilt": [15, 15]},
        {"state": "popup", "platform": platform, "button": [391, 21, 32, 32],
         "popups": [{"rect": [391, 53, 452, 552]}] if popups is None else popups},
        {"state": "docker", "platform": platform, "found": True, "floating": True, "visible": True,
         "rect": [1268, 212, 262, 216]},
        {"state": "window", "platform": platform, "count": 2, "visible": [True, True]},
        {"state": "done", "platform": platform},
    ]


def failed(results):
    return [name for name, ok, _ in results if not ok]


class CheckUiTest(unittest.TestCase):
    def test_a_complete_wayland_run_passes(self):
        self.assertEqual(failed(check_ui.judge(states(), "wayland")), [])

    def test_a_pen_without_pressure_fails(self):
        self.assertEqual(failed(check_ui.judge(states(pressure=[1.0, 1.0]), "wayland")), ["pen pressure"])

    def test_a_popup_that_did_not_open_fails(self):
        self.assertEqual(failed(check_ui.judge(states(popups=[]), "wayland")), ["popup"])

    def test_a_popup_away_from_its_button_fails(self):
        self.assertEqual(failed(check_ui.judge(states(popups=[{"rect": [0, 0, 452, 552]}]), "wayland")), ["popup"])

    def test_a_run_on_another_platform_fails(self):
        self.assertIn("platform", failed(check_ui.judge(states(platform="xcb"), "wayland")))

    def test_an_unfinished_run_fails(self):
        self.assertIn("finished", failed(check_ui.judge(states()[:-1], "wayland")))


if __name__ == "__main__":
    unittest.main()
