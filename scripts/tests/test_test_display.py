#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest


SCRIPTS = Path(__file__).resolve().parents[1]
ENTRY = SCRIPTS / "run-with-test-display"


@unittest.skipUnless(sys.platform == "linux", "Xvfb is used on Linux")
class TestDisplayTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="test display ")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.environment = os.environ | {
            "PATH": str(self.bin) + os.pathsep + os.environ["PATH"],
            "TEST_DISPLAY_STATE": str(self.root),
            "QT_QPA_PLATFORM": "offscreen",
        }
        self.environment.pop("DISPLAY", None)
        self.write_executable(self.bin / "Xvfb", """
import os, pathlib, signal, sys
root = pathlib.Path(os.environ['TEST_DISPLAY_STATE'])
(root / 'xvfb.pid').write_text(str(os.getpid()))
mode = os.environ.get('TEST_XVFB_MODE', 'ready')
if mode == 'fail':
    print('test Xvfb startup failure', file=sys.stderr, flush=True)
    sys.exit(23)
if mode != 'pending':
    descriptor = int(sys.argv[sys.argv.index('-displayfd') + 1])
    os.write(descriptor, b'invalid\\n' if mode == 'invalid' else b'123\\n')
    os.close(descriptor)
signal.pause()
""")
        self.consumer = self.root / "consumer"
        self.write_executable(self.consumer, """
import json, os, pathlib, signal, sys
value = {'display': os.environ.get('DISPLAY'),
         'platform': os.environ.get('QT_QPA_PLATFORM'),
         'arguments': sys.argv[1:], 'pid': os.getpid()}
payload = json.dumps(value)
(pathlib.Path(os.environ['TEST_DISPLAY_STATE']) / 'consumer.json').write_text(payload)
print(payload, flush=True)
if os.environ.get('TEST_CONSUMER_MODE') == 'pending': signal.pause()
if os.environ.get('TEST_CONSUMER_MODE') == 'signal': os.kill(os.getpid(), signal.SIGTERM)
sys.exit(int(os.environ.get('TEST_CONSUMER_EXIT', '0')))
""")

    def write_executable(self, path, body):
        path.write_text(f"#!{sys.executable}\n" + body)
        path.chmod(0o755)

    def invoke(self, command=None, controls=None):
        return subprocess.run(
            [str(ENTRY), *(command or [str(self.consumer)])],
            env=self.environment | (controls or {}),
            capture_output=True, text=True, timeout=15,
        )

    def assert_stopped(self, pid):
        with self.assertRaises(ProcessLookupError, msg=f"process {pid} was left running"):
            os.kill(pid, 0)

    def xvfb_pid(self):
        return int((self.root / "xvfb.pid").read_text())

    def wait_for_file(self, name):
        path = self.root / name
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if path.exists() and path.stat().st_size:
                return path
            time.sleep(0.01)
        self.fail(f"timed out waiting for {name}")

    def test_ready_display_preserves_arguments_platform_and_exit_status(self):
        arguments = ["argument with spaces", "literal$(value);*"]
        for code in (0, 23):
            with self.subTest(exit_code=code):
                result = self.invoke([str(self.consumer), *arguments],
                                     {"TEST_CONSUMER_EXIT": str(code)})
                self.assertEqual(result.returncode, code, result.stderr)
                value = json.loads(result.stdout)
                self.assertEqual(value["display"], ":123")
                self.assertEqual(value["platform"], "offscreen")
                self.assertEqual(value["arguments"], arguments)
                self.assert_stopped(value["pid"])
                self.assert_stopped(self.xvfb_pid())

    def test_existing_display_runs_directly(self):
        result = self.invoke(controls={"DISPLAY": ":88", "TEST_CONSUMER_EXIT": "7"})
        self.assertEqual(result.returncode, 7, result.stderr)
        self.assertEqual(json.loads(result.stdout)["display"], ":88")
        self.assertFalse((self.root / "xvfb.pid").exists())

    def test_failed_or_invalid_startup_prevents_the_command_and_reports_failure(self):
        for mode in ("fail", "invalid"):
            with self.subTest(mode=mode):
                result = self.invoke(controls={"TEST_XVFB_MODE": mode})
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn("invalid display number", result.stderr)
                if mode == "fail":
                    self.assertIn("test Xvfb startup failure", result.stderr)
                self.assertFalse((self.root / "consumer.json").exists())
                self.assert_stopped(self.xvfb_pid())

    def test_signal_during_startup_or_test_reaps_the_owned_processes(self):
        for phase in ("startup", "test"):
            with self.subTest(phase=phase):
                environment = self.environment | {
                    "TEST_XVFB_MODE": "pending" if phase == "startup" else "ready",
                    "TEST_CONSUMER_MODE": "pending",
                }
                process = subprocess.Popen([str(ENTRY), str(self.consumer)], env=environment,
                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                try:
                    self.wait_for_file("xvfb.pid" if phase == "startup" else "consumer.json")
                    process.send_signal(signal.SIGTERM)
                    _, error = process.communicate(timeout=10)
                    self.assertEqual(process.returncode, 143, error)
                    self.assert_stopped(self.xvfb_pid())
                    if phase == "test":
                        value = json.loads((self.root / "consumer.json").read_text())
                        self.assert_stopped(value["pid"])
                finally:
                    if process.poll() is None:
                        process.send_signal(signal.SIGTERM)
                        process.communicate(timeout=10)
                    (self.root / "xvfb.pid").unlink(missing_ok=True)

    def test_test_signal_is_returned_and_display_is_reaped(self):
        result = self.invoke(controls={"TEST_CONSUMER_MODE": "signal"})
        self.assertEqual(result.returncode, 143, result.stderr)
        self.assert_stopped(self.xvfb_pid())

    def test_missing_command_reaps_the_display(self):
        result = self.invoke([str(self.root / "absent-command")])
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("absent-command", result.stderr)
        self.assert_stopped(self.xvfb_pid())

    def prepare_entrypoints(self):
        scripts = self.root / "scripts"
        scripts.mkdir()
        for name in ("run-test", "verify", "run-with-test-display"):
            shutil.copy2(SCRIPTS / name, scripts / name)
        self.write_executable(scripts / "verify-quick", "")
        self.write_executable(scripts / "build-incremental", "")
        shutil.copy2(self.consumer, self.bin / "ctest")
        return scripts

    def test_test_entrypoints_supply_display_to_ctest_and_propagate_failure(self):
        scripts = self.prepare_entrypoints()
        for entry, arguments in (("run-test", ["sample-target", "sample-regex$"]), ("verify", [])):
            with self.subTest(entry=entry):
                result = subprocess.run(
                    [str(scripts / entry), *arguments], capture_output=True, text=True,
                    env=self.environment | {"LIBREPAINT_TEST_SHELL": "1", "TEST_CONSUMER_EXIT": "23",
                                            "LIBREPAINT_BUILD_INCREMENTAL_PATH": str(scripts / "build-incremental")},
                    timeout=15,
                )
                self.assertEqual(result.returncode, 23, result.stderr)
                value = json.loads(result.stdout)
                self.assertEqual(value["display"], ":123")
                expected = ["--preset", "tdd-linux"]
                if entry == "run-test":
                    expected += ["--tests-regex", "sample-regex$"]
                self.assertEqual(value["arguments"], expected)
                self.assert_stopped(self.xvfb_pid())

    def test_verify_interruption_reaps_ctest_and_display(self):
        scripts = self.prepare_entrypoints()
        process = subprocess.Popen(
            [str(scripts / "verify")], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            env=self.environment | {"LIBREPAINT_TEST_SHELL": "1", "TEST_CONSUMER_MODE": "pending"},
        )
        try:
            self.wait_for_file("consumer.json")
            process.send_signal(signal.SIGTERM)
            _, error = process.communicate(timeout=10)
            self.assertEqual(process.returncode, 143, error)
            self.assert_stopped(self.xvfb_pid())
            self.assert_stopped(json.loads((self.root / "consumer.json").read_text())["pid"])
        finally:
            if process.poll() is None:
                process.send_signal(signal.SIGTERM)
                process.communicate(timeout=10)


if __name__ == "__main__":
    unittest.main()
