# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of the Windows device-recovery wait.

The wait runs before each measured run. On 2026-11-11 the runner sat for 36
minutes before the application started: the wait was called without the
mandatory -Out parameter, so the hidden, non-interactive console waited at the
parameter prompt and the 120 s cap never started. These tests fix the shape
that keeps that from happening again, on a machine without Windows PowerShell:
the wait receives every parameter it needs, runs as a separate process with a
hard wall-clock limit, and reads the value without Get-Counter or the Perflib
language lists.
"""

import os
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RUNNER = os.path.join(ROOT, "scripts", "perf", "pen-latency", "run-surface.ps1")
STATE = os.path.join(ROOT, "scripts", "perf", "pen-latency", "surface_state.ps1")


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def function_block(text, name):
    """Return one top-level PowerShell function (its closing brace is at column 0)."""
    start = text.index("function " + name)
    end = text.index("\n}\n", start) + 3
    return text[start:end]


def code_only(text):
    """Drop comment lines so a test checks what PowerShell runs, not its notes."""
    return "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("#"))


class RunnerWaitTest(unittest.TestCase):
    def setUp(self):
        self.text = read(RUNNER)
        self.block = function_block(self.text, "Invoke-SurfaceStateWait")

    def test_wait_runs_in_its_own_process(self):
        self.assertNotIn("& $surfaceScript -WaitForProcessorPerformance", self.text)
        self.assertIn("Start-Process -FilePath 'powershell.exe'", self.block)
        self.assertIn("'surface_state.ps1'", self.text)

    def test_wait_child_gets_every_required_parameter(self):
        # The 2026-11-11 hang: no -Out was passed, so the hidden console
        # waited at the mandatory-parameter prompt before the wait began.
        self.assertIn("'-Out'", self.block)
        self.assertIn("$waitCsv", self.block)
        self.assertIn("'-WaitForProcessorPerformance'", self.block)

    def test_parent_stops_a_blocked_wait_at_a_wall_clock_limit(self):
        self.assertIn("WaitForExit(", self.block)
        self.assertIn("$process.Kill()", self.block)
        self.assertRegex(self.block, r"\$timeoutSeconds \+ \$graceSeconds")
        self.assertIn("surface_state_wait_timed_out=process", self.block)

    def test_wait_result_goes_to_meta_and_a_file(self):
        self.assertIn("surface-state-wait.out", self.block)
        self.assertIn("surface-state-wait.err", self.block)
        self.assertRegex(self.text, r"Add-Content[^\n]*meta\.txt[^\n]*\$waitLines")


class StateWaitTest(unittest.TestCase):
    def setUp(self):
        self.text = read(STATE)
        self.block = function_block(self.text, "Wait-ForProcessorPerformance")

    def test_reading_is_language_independent_wmi(self):
        self.assertIn("Get-ProcessorPerformancePct", self.block)
        self.assertIn("surface_state_wait_source=wmi", self.block)
        self.assertIn("Win32_PerfFormattedData_Counters_ProcessorInformation", self.block)
        code = code_only(self.block)
        self.assertNotIn("Get-Counter", code)
        self.assertNotIn("Perflib", code)

    def test_timeout_and_result_keys_stay_stable(self):
        self.assertRegex(self.text, r"\[int\]\$WaitTimeoutSeconds = 120")
        for key in ("surface_state_wait=", "surface_state_wait_seconds=",
                    "surface_state_wait_min_pct=", "surface_state_wait_last_pct="):
            self.assertIn(key, self.block)

    def test_the_wait_mode_needs_no_output_file(self):
        self.assertNotIn("[Parameter(Mandatory = $true)][string]$Out", self.text)
        self.assertIn("[string]$Out = ''", self.text)
        self.assertIn("unless -WaitForProcessorPerformance is used", self.text)

    def test_wait_no_longer_uses_the_removed_counter_helpers(self):
        self.assertNotIn("function Read-ProcessorPerformancePct", self.text)
        self.assertNotIn("function Resolve-ProcessorPerformancePath", self.text)


class WaitScenarioTest(unittest.TestCase):
    def test_one_run_scenario_matches_the_check(self):
        path = os.path.join(ROOT, "scripts", "perf", "scenarios", "pen-light-wait-check.json")
        text = read(path)
        self.assertIn('"runs": 1', text)
        self.assertIn('"name": "pen-light-wait-check"', text)


if __name__ == "__main__":
    unittest.main()
