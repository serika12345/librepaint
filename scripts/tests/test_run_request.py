# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/run_request.py.

run.py passes the target's "env" from hosts.json to the Windows runner. The
request file is the only path to the measured application, so names and
values must be rejected before the runner writes request.json, and the
PresentMon fields must keep their existing shape.
"""

import importlib.util
import os
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location(
    "run_request", os.path.join(ROOT, "scripts", "perf", "pen-latency", "run_request.py"))
request_module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(request_module)


class RequestTest(unittest.TestCase):
    def test_environment_reaches_the_request(self):
        request = request_module.build_request(
            "LibrePaint", 1, "C:/LibrePaint.exe", name="T",
            env={"LIBREPAINT_L1_SWAP_INTERVAL": "0", "LIBREPAINT_L1_WAIT_FOR_GPU": "1"})
        self.assertEqual(request["env"],
                         {"LIBREPAINT_L1_SWAP_INTERVAL": "0", "LIBREPAINT_L1_WAIT_FOR_GPU": "1"})

    def test_without_environment_the_field_is_absent(self):
        request = request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", name="T")
        self.assertNotIn("env", request)

    def test_invalid_names_and_values_are_rejected(self):
        for env in ({"1BAD": "x"}, {"BAD-NAME": "x"}, {"OK": "line\nbreak"}, {"OK": "nul\x00"}):
            with self.assertRaises(ValueError):
                request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", env=env)

    def test_presentmon_fields_keep_their_shape(self):
        request = request_module.build_request(
            "LibrePaint", 2, "C:/LibrePaint.exe", capture=True, name="T", presentmon="off",
            presentmon_fields={"presentmon_session": "LibrePaintPenLatency"})
        self.assertEqual(request["runs"], 2)
        self.assertTrue(request["capture"])
        self.assertEqual(request["presentmon"], "off")
        self.assertEqual(request["presentmon_session"], "LibrePaintPenLatency")

    def test_kritarc_keys_reach_the_request(self):
        request = request_module.build_request(
            "LibrePaint", 1, "C:/LibrePaint.exe", name="T",
            kritarc={"useMaximumFrameLatencyOne": "false"})
        self.assertEqual(request["kritarc"], {"useMaximumFrameLatencyOne": "false"})

    def test_invalid_kritarc_keys_and_values_are_rejected(self):
        for kritarc in ({"[Group]": "x"}, {"KEY": "line\nbreak"}, {"KEY": "a=b"}, {"KEY": "a\\b"}):
            with self.assertRaises(ValueError):
                request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", kritarc=kritarc)
        self.assertNotIn("kritarc", request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe"))

    def test_display_check_needs_an_image(self):
        request = request_module.build_request(
            "LibrePaint", 1, "C:/LibrePaint.exe", display_check={"image": "C:/image.png"})
        self.assertEqual(request["displayCheck"], {"image": "C:/image.png"})
        with self.assertRaises(ValueError):
            request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", display_check={"image": ""})

    def test_device_state_recording_is_off_by_default(self):
        request = request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", name="T")
        self.assertEqual(request["surface_state"], "off")

    def test_device_state_recording_modes_reach_the_request(self):
        for mode in ("on", "light"):
            request = request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", surface_state=mode)
            self.assertEqual(request["surface_state"], mode)

    def test_unknown_device_state_recording_mode_is_rejected(self):
        for mode in ("heavy", "ON", 1):
            with self.assertRaises(ValueError):
                request_module.build_request("LibrePaint", 1, "C:/LibrePaint.exe", surface_state=mode)


if __name__ == "__main__":
    unittest.main()
