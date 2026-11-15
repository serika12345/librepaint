# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of the PresentMon command line for the Windows measurement.

The Windows runner starts PresentMon once per run. These tests fix the
argument construction so it can be checked on a machine without Windows
PowerShell: one short fixed session name, a process ID filter for the app that
is already running, an absolute CSV path, and no session left to chance.
"""

import importlib.util
import json
import os
import re
import subprocess
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PATH = os.path.join(ROOT, "scripts", "perf", "pen-latency", "presentmon_command.py")
SPEC = importlib.util.spec_from_file_location("presentmon_command", PATH)
presentmon = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(presentmon)


class SessionNameTest(unittest.TestCase):
    def test_session_name_is_short_and_plain_ascii(self):
        name = presentmon.SESSION_NAME
        self.assertRegex(name, r"^[A-Za-z0-9_-]+$")
        self.assertLessEqual(len(name), 64)
        self.assertGreater(len(name), 0)

    def test_request_fields_carry_the_same_session_name(self):
        fields = presentmon.request_fields()
        self.assertEqual(fields["presentmon_session"], presentmon.SESSION_NAME)
        self.assertIn(fields["presentmon_session"], fields["presentmon_arguments"])


class ArgumentTest(unittest.TestCase):
    def test_arguments_select_the_running_process_by_id(self):
        args = presentmon.argument_template()
        self.assertEqual(args[args.index("--process_id") + 1], "{pid}")
        self.assertNotIn("--process_name", args)

    def test_arguments_write_one_csv_to_the_runner_path(self):
        args = presentmon.argument_template()
        self.assertEqual(args[args.index("--output_file") + 1], "{output}")
        self.assertIn("--qpc_time", args)
        self.assertIn("--no_console_stats", args)

    def test_runner_owns_session_lifecycle(self):
        args = presentmon.argument_template()
        self.assertNotIn("--stop_existing_session", args)
        self.assertNotIn("--terminate_existing_session", args)
        self.assertIn("--terminate_on_proc_exit", args)
        self.assertEqual(args[args.index("--session_name") + 1], presentmon.SESSION_NAME)

    def test_render_fills_every_placeholder(self):
        rendered = presentmon.render(1234, r"C:\runs\one\presentmon.csv")
        self.assertEqual(rendered[rendered.index("--process_id") + 1], "1234")
        self.assertEqual(rendered[rendered.index("--output_file") + 1], r"C:\runs\one\presentmon.csv")
        self.assertFalse(any(re.search(r"\{[a-z]+\}", value) for value in rendered))

    def test_command_line_prints_the_request_fields(self):
        result = subprocess.run([sys.executable, "-I", PATH], check=True, capture_output=True, text=True)
        self.assertEqual(json.loads(result.stdout), presentmon.request_fields())


if __name__ == "__main__":
    unittest.main()
