# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/thread_cpu.py.

The Windows runner writes threads.csv with cumulative process and thread CPU
counters on the QueryPerformanceCounter clock. The analyzer turns those into
the CPU time inside the injected input window and names the GUI and paint
threads. These tests use a synthetic file with known values so the
interpolation and the window boundaries stay fixed.
"""

import importlib.util
import os
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location(
    "thread_cpu", os.path.join(ROOT, "scripts", "perf", "pen-latency", "thread_cpu.py"))
thread_cpu = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(thread_cpu)

FREQUENCY = 1_000_000_000


def write_run(directory, threads, inject_rows):
    with open(os.path.join(directory, "threads.csv"), "w", encoding="utf-8") as handle:
        handle.write("# frequency=%d\n" % FREQUENCY)
        handle.write("qpc,pid,thread_id,cpu_seconds,thread_state,wait_reason,priority,"
                     "process_cpu_seconds\n")
        for row in threads:
            handle.write(",".join(str(value) for value in row) + "\n")
    with open(os.path.join(directory, "inject.csv"), "w", encoding="utf-8") as handle:
        handle.write("# frequency=%d\n" % FREQUENCY)
        handle.write("time_ns,phase,band,x,y,pressed\n")
        for row in inject_rows:
            handle.write(",".join(str(value) for value in row) + "\n")
    with open(os.path.join(directory, "probe.csv"), "w", encoding="utf-8") as handle:
        handle.write("# frequency=%d\n" % FREQUENCY)
        handle.write("time,event,x,y,extra\n")
        handle.write("0,main_thread,100,0,0\n")
        handle.write("0,paintgl_thread,200,0,0\n")


class ThreadCpuTest(unittest.TestCase):
    def test_cpu_is_interpolated_inside_the_input_window(self):
        with tempfile.TemporaryDirectory() as directory:
            rows = [
                (0, 7, 100, 1.0, "Running", "Executive", "Normal", 5.0),
                (0, 7, 200, 0.0, "Running", "Executive", "Normal", 5.0),
                (1_000_000_000, 7, 100, 2.0, "Running", "Executive", "Normal", 6.0),
                (1_000_000_000, 7, 200, 0.2, "Waiting", "WrUserRequest", "Normal", 6.0),
                (2_000_000_000, 7, 100, 3.0, "Running", "Executive", "Normal", 7.0),
                (2_000_000_000, 7, 200, 0.4, "Running", "Executive", "Normal", 7.0),
            ]
            inject = [
                (500_000_000, "move", "b", 0, 0, 0),
                (1_500_000_000, "move", "b", 10, 0, 0),
            ]
            write_run(directory, rows, inject)
            result = thread_cpu.summarize(directory)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result["window_s"], 1.0)
        # Thread 100 uses 1.0 s and thread 200 uses 0.2 s across the 1 s
        # window; the process counter advanced 1.0 s because the remaining
        # threads are not part of this synthetic sample.
        self.assertAlmostEqual(result["cpu_seconds"], 1.2)
        self.assertAlmostEqual(result["percent"], 120.0)
        self.assertAlmostEqual(result["process_seconds"], 1.0)
        self.assertAlmostEqual(result["unaccounted_seconds"], 0.0)
        self.assertEqual(result["threads"], 2)
        self.assertEqual(result["roles"]["main_thread"], 100)
        self.assertEqual(result["roles"]["paintgl_thread"], 200)
        self.assertAlmostEqual(result["roles"]["main_seconds"], 1.0)
        self.assertAlmostEqual(result["roles"]["paint_seconds"], 0.2)
        self.assertEqual(result["top"][0]["id"], 100)
        self.assertEqual(result["top"][0]["waits"].get("Executive"), 1)

    def test_qpc_inject_rows_use_the_same_clock(self):
        with tempfile.TemporaryDirectory() as directory:
            rows = [
                (0, 7, 100, 0.0, "Running", "Executive", "Normal", 0.0),
                (FREQUENCY, 7, 100, 1.0, "Running", "Executive", "Normal", 1.0),
            ]
            with open(os.path.join(directory, "threads.csv"), "w", encoding="utf-8") as handle:
                handle.write("# frequency=%d\n" % FREQUENCY)
                handle.write("qpc,pid,thread_id,cpu_seconds,thread_state,wait_reason,priority,"
                             "process_cpu_seconds\n")
                for row in rows:
                    handle.write(",".join(str(value) for value in row) + "\n")
            with open(os.path.join(directory, "inject.csv"), "w", encoding="utf-8") as handle:
                handle.write("# frequency=%d\n" % FREQUENCY)
                handle.write("qpc,phase,flags,ok,x,y\n")
                handle.write("0,move,0,1,0,0\n")
                handle.write("%d,move,0,1,10,0\n" % FREQUENCY)
            result = thread_cpu.summarize(directory)
        self.assertAlmostEqual(result["window_s"], 1.0)
        self.assertAlmostEqual(result["cpu_seconds"], 1.0)

    def test_without_threads_csv_the_summary_is_absent(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertIsNone(thread_cpu.summarize(directory))

    def test_sampler_writes_the_clock_and_the_thread_columns(self):
        path = os.path.join(ROOT, "scripts", "perf", "pen-latency", "thread_cpu.ps1")
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        self.assertIn("# frequency=$frequency", text)
        self.assertIn("qpc,pid,thread_id,cpu_seconds,thread_state,wait_reason,priority,"
                      "process_cpu_seconds", text)
        self.assertIn("TotalProcessorTime", text)
        self.assertIn("$StopFile", text)
        self.assertNotIn("Get-Counter -Counter", text)

    def test_runner_sends_and_starts_the_sampler(self):
        remote = os.path.join(ROOT, "scripts", "perf", "pen-latency", "run-remote.sh")
        with open(remote, encoding="utf-8") as handle:
            remote_text = handle.read()
        self.assertIn("thread_cpu.ps1", remote_text)
        self.assertIn("THREAD_CPU", remote_text)
        runner = os.path.join(ROOT, "scripts", "perf", "pen-latency", "run-surface.ps1")
        with open(runner, encoding="utf-8") as handle:
            runner_text = handle.read()
        self.assertIn("thread_cpu_mode", runner_text)
        self.assertIn("threads.csv", runner_text)
        self.assertIn("thread_cpu.ps1", runner_text)


if __name__ == "__main__":
    unittest.main()
