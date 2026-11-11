# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/power.py and its runner wiring.

The Windows runner writes power.csv with Win32_Battery and root\\wmi
BatteryStatus samples on the QueryPerformanceCounter clock. The analyzer
turns those into the average discharge rate and the discharged energy inside
the injected input window, and marks the runs that saw AC power so the runner
can leave them out.
"""

import importlib.util
import os
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PEN_LATENCY = os.path.join(ROOT, "scripts", "perf", "pen-latency")
sys.path.insert(0, PEN_LATENCY)
SPEC = importlib.util.spec_from_file_location(
    "power", os.path.join(PEN_LATENCY, "power.py"))
power = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(power)

FREQUENCY = 1_000_000_000


def write_run(directory, samples, inject_rows=((500_000_000,), (2_500_000_000,))):
    with open(os.path.join(directory, "power.csv"), "w", encoding="utf-8") as handle:
        handle.write("# frequency=%d\n" % FREQUENCY)
        handle.write("qpc,charge_pct,battery_status,estimated_run_time,remaining_mwh,"
                     "discharge_mw,power_online,charging,discharging\n")
        for row in samples:
            handle.write(",".join(str(value) for value in row) + "\n")
    with open(os.path.join(directory, "inject.csv"), "w", encoding="utf-8") as handle:
        handle.write("# frequency=%d\n" % FREQUENCY)
        handle.write("time_ns,phase,band,x,y,pressed\n")
        for row in inject_rows:
            handle.write(",".join(str(value) for value in row) + ",move,b,0,0,0\n")


def sample(seconds, charge=100, status=1, remaining=50_000, discharge=5_000,
           online="False", charging="False"):
    return (seconds * FREQUENCY, charge, status, 120, remaining, discharge,
            online, charging, "True")


class PowerSummaryTest(unittest.TestCase):
    def test_discharge_rate_and_energy_are_measured_inside_the_input_window(self):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [
                sample(0, charge=100, remaining=50_010, discharge=4_000),
                sample(1, charge=99, remaining=50_000, discharge=4_000),
                sample(2, charge=98, remaining=49_990, discharge=6_000),
                sample(3, charge=97, remaining=49_980, discharge=6_000),
            ])
            result = power.summarize(directory)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result["window_s"], 2.0)
        self.assertEqual(result["samples"], 2)
        self.assertEqual(result["all_samples"], 4)
        self.assertFalse(result["ac"])
        self.assertAlmostEqual(result["rate_mw"], 5_000.0)
        self.assertAlmostEqual(result["energy_mwh"], 5_000.0 * 2.0 / 3600.0)
        self.assertAlmostEqual(result["remaining_mwh_delta"], 10.0)
        self.assertAlmostEqual(result["charge_pct_delta"], 1.0)

    def test_ac_power_is_marked_from_root_wmi_or_the_battery_status(self):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [sample(1, online="True")])
            self.assertTrue(power.summarize(directory)["ac"])
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [sample(1, status=2, online="unavailable")])
            self.assertTrue(power.summarize(directory)["ac"])
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [sample(1, status=6, charging="True")])
            self.assertTrue(power.summarize(directory)["ac"])

    def test_a_missing_root_wmi_sample_keeps_the_charge_record(self):
        with tempfile.TemporaryDirectory() as directory:
            write_run(directory, [
                (0, 100, 1, 120, "unavailable", "unavailable", "unavailable", "unavailable", "unavailable"),
                (FREQUENCY, 99, 1, 120, "unavailable", "unavailable", "unavailable", "unavailable", "unavailable"),
                (2 * FREQUENCY, 98, 1, 120, "unavailable", "unavailable", "unavailable", "unavailable", "unavailable"),
            ])
            result = power.summarize(directory)
        self.assertIsNotNone(result)
        self.assertNotIn("rate_mw", result)
        self.assertNotIn("energy_mwh", result)
        self.assertAlmostEqual(result["charge_pct_delta"], 1.0)

    def test_without_power_csv_the_summary_is_absent(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertIsNone(power.summarize(directory))

    def test_analyzer_prints_the_rate_and_the_discharged_energy(self):
        entry = {"window_s": 32.7, "samples": 33, "ac": False,
                 "rate_mw": 4321.5, "energy_mwh": 39.25}
        text = __import__("analyze").format_power(entry)
        self.assertIn("rate=4321.5mW", text)
        self.assertIn("discharged=39.25mWh", text)
        self.assertIn("ac=no", text)


class PowerSamplerTest(unittest.TestCase):
    def test_sampler_writes_the_battery_columns(self):
        path = os.path.join(PEN_LATENCY, "power.ps1")
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        self.assertIn("# frequency=$frequency", text)
        self.assertIn("qpc,charge_pct,battery_status,estimated_run_time,remaining_mwh,"
                      "discharge_mw,power_online,charging,discharging", text)
        self.assertIn("Win32_Battery", text)
        self.assertIn("root\\wmi", text)
        self.assertIn("$StopFile", text)


if __name__ == "__main__":
    unittest.main()
