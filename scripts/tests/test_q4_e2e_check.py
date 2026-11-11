#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Contracts of the Q4 automatic check (scripts/perf/check.py).

The check runs on the Windows tablet, but its verdicts are made on the
development host from evidence files. These tests fix the item list, the
clipboard rule (PNG and no old DIB formats), the tab-title and keyboard
rules, the relative flicker threshold, and the Windows-side entry points.
"""

from __future__ import annotations

import importlib.util
import io
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from contextlib import redirect_stdout
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "scripts" / "perf" / "pen-latency"))
import display_check  # noqa: E402


def load_module(name: str, relative: str):
    spec = importlib.util.spec_from_file_location(name, REPO_ROOT / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


check = load_module("q4_e2e_check", "scripts/perf/check.py")
run_request = load_module("q4_run_request", "scripts/perf/pen-latency/run_request.py")
SCENARIO = json.loads((REPO_ROOT / "scripts/perf/scenarios/q4-e2e.json").read_text(encoding="utf-8"))


def write_png_rgba(path: str, width: int, height: int, pixels: bytes) -> None:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[4 * y * width:4 * (y + 1) * width])

    def chunk(kind: bytes, payload: bytes) -> bytes:
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        handle.write(chunk(b"IEND", b""))


def rgba_picture(width: int, height: int, color=(20, 40, 60, 255)) -> bytes:
    return bytes(color) * (width * height)


def file_bytes(path: str) -> bytes:
    with open(path, "rb") as handle:
        return handle.read()


class ScenarioTest(unittest.TestCase):
    def test_scenario_names_every_automated_item(self):
        self.assertEqual(SCENARIO["baseline_target"], "q4-e2e-fork")
        self.assertEqual(SCENARIO["checks"]["keyboard"]["layout"], "00000411")
        self.assertEqual(SCENARIO["checks"]["clipboard"]["pixel_tolerance"], 1)

    def test_unreachable_checks_are_listed(self):
        identifiers = {entry["id"] for entry in SCENARIO["not_reproducible"]}
        self.assertIn("pen.proximity", identifiers)
        self.assertIn("touch.finger", identifiers)

    def test_item_list_covers_the_ticket(self):
        for identifier in ("clipboard", "tabs", "keyboard", "flicker") + check.PEN_ITEMS:
            self.assertIn(identifier, check.ITEMS)


class ClipboardTest(unittest.TestCase):
    def _write(self, root: str, rows: list[str], source: bytes, copied: bytes) -> str:
        with open(os.path.join(root, "clipboard-formats.txt"), "w", encoding="utf-8") as handle:
            handle.write("id,name,bytes,path\n")
            handle.write("\n".join(rows) + "\n")
        with open(os.path.join(root, "clipboard-source.png"), "wb") as handle:
            handle.write(source)
        if copied:
            with open(os.path.join(root, "clipboard-png.png"), "wb") as handle:
                handle.write(copied)
        return root

    def test_png_without_dib_passes_and_pixels_are_compared(self):
        with tempfile.TemporaryDirectory() as root:
            source = os.path.join(root, "source.png")
            copied = os.path.join(root, "copy.png")
            pixels = rgba_picture(8, 6, (10, 20, 30, 255))
            write_png_rgba(source, 8, 6, pixels)
            write_png_rgba(copied, 8, 6, pixels)
            self._write(root, ["49161,PNG,100,clipboard-png.png"], file_bytes(source),
                        file_bytes(copied))
            result = check.judge_clipboard(root, {"clipboard": {"png": "clipboard-png.png",
                                                                "source": "clipboard-source.png"}},
                                           SCENARIO["checks"])
        self.assertEqual(result["verdict"], "pass", result)
        self.assertTrue(result["metrics"]["dib"] is False)

    def test_old_dib_formats_fail(self):
        with tempfile.TemporaryDirectory() as root:
            source = os.path.join(root, "source.png")
            write_png_rgba(source, 4, 4, rgba_picture(4, 4))
            self._write(root, ["8,CF_DIB,100,", "49161,PNG,100,clipboard-png.png"],
                        file_bytes(source), file_bytes(source))
            result = check.judge_clipboard(root, {"clipboard": {}}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "fail")
        self.assertIn("DIB", " ".join(result["reasons"]))

    def test_pixel_difference_fails(self):
        with tempfile.TemporaryDirectory() as root:
            source = os.path.join(root, "source.png")
            copied = os.path.join(root, "copy.png")
            write_png_rgba(source, 4, 4, rgba_picture(4, 4, (0, 0, 0, 255)))
            write_png_rgba(copied, 4, 4, rgba_picture(4, 4, (0, 0, 0, 0)))
            self._write(root, ["49161,PNG,100,clipboard-png.png"],
                        file_bytes(source), file_bytes(copied))
            result = check.judge_clipboard(root, {"clipboard": {}}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "fail")
        self.assertTrue(result["metrics"]["mismatched"] > 0)


class TitleAndKeyboardTest(unittest.TestCase):
    def _tabs(self, root: str, initial: str, after: str, back: str) -> None:
        with open(os.path.join(root, "tabs.txt"), "w", encoding="utf-8") as handle:
            handle.write("\n".join([
                "initial=" + initial, "after=" + after, "back=" + back,
                "expected_a=doc-a", "expected_b=doc-b"]) + "\n")

    def test_tab_title_follows_the_active_document(self):
        with tempfile.TemporaryDirectory() as root:
            self._tabs(root, "doc-a - LibrePaint", "doc-b - LibrePaint", "doc-a - LibrePaint")
            result = check.judge_tabs(root, {}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "pass", result)

    def test_tab_title_that_never_changes_fails(self):
        with tempfile.TemporaryDirectory() as root:
            self._tabs(root, "doc-a - LibrePaint", "doc-a - LibrePaint", "doc-a - LibrePaint")
            result = check.judge_tabs(root, {}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "fail")

    def test_keyboard_needs_the_layout_and_no_modifier_left(self):
        with tempfile.TemporaryDirectory() as root:
            self._tabs(root, "doc-a", "doc-b", "doc-a")
            with open(os.path.join(root, "keyboard.txt"), "w", encoding="utf-8") as handle:
                handle.write("\n".join([
                    "layout=loaded=123 active=123 posted=True current=123",
                    "send=sent=4",
                    "title_before=doc-a",
                    "title_after=doc-b",
                    "key_states=control=0;async=0 menu=0;async=0 shift=0;async=0 lwin=0;async=0 rwin=0;async=0",
                    "modifier_down=0"]) + "\n")
            result = check.judge_keyboard(root, {}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "pass", result)

    def test_keyboard_with_a_left_control_fails(self):
        with tempfile.TemporaryDirectory() as root:
            self._tabs(root, "doc-a", "doc-b", "doc-a")
            with open(os.path.join(root, "keyboard.txt"), "w", encoding="utf-8") as handle:
                handle.write("\n".join([
                    "layout=loaded=123 active=123 posted=True current=123",
                    "send=sent=4", "title_before=doc-a", "title_after=doc-b",
                    "key_states=control=-128;async=-128 menu=0;async=0 shift=0;async=0",
                    "modifier_down=1"]) + "\n")
            result = check.judge_keyboard(root, {}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "fail")
        self.assertIn("修飾キー", " ".join(result["reasons"]))


class FlickerTest(unittest.TestCase):
    def _flicker(self, root: str, blank: int, stale: int, dialog: bool = True) -> dict:
        with open(os.path.join(root, "flicker.csv"), "w", encoding="utf-8") as handle:
            handle.write("frame_ns,arrival_ns,rect_x,rect_y,rect_w,rect_h,rect_changed,"
                         "canvas_white_ratio,canvas_dark_pixels,changed_ratio,"
                         "old_area_unchanged_ratio,frames\n")
            for index in range(3):
                white = "0.999" if index < blank else "0.1"
                changed = "0.0" if index < stale else "0.5"
                handle.write("%d,%d,0,0,100,100,1,%s,0,%s,0.0,%d\n" % (
                    1000 + index, 1000 + index, white, changed, index))
        with open(os.path.join(root, "flicker-log.txt"), "w", encoding="utf-8") as handle:
            handle.write("action=resize start=1 end=2 detail=ok\n")
        return {"flicker": {"dialog_opened": dialog, "monitor_exit": 0}}

    def test_counts_blank_and_unchanged_frames(self):
        with tempfile.TemporaryDirectory() as root:
            facts = self._flicker(root, blank=1, stale=1)
            result = check.judge_flicker(root, facts, SCENARIO["checks"])
        self.assertEqual(result["metrics"]["blank_frames"], 1)
        self.assertEqual(result["metrics"]["stale_frames"], 1)
        self.assertEqual(result["verdict"], "check")

    def test_dialog_that_does_not_open_fails(self):
        with tempfile.TemporaryDirectory() as root:
            facts = self._flicker(root, blank=0, stale=0, dialog=False)
            result = check.judge_flicker(root, facts, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "fail")

    def test_fork_baseline_sets_the_verdict(self):
        stages = {
            "target": "q4-e2e-stage10",
            "items": [{"id": "flicker", "verdict": "check", "reasons": [],
                       "metrics": {"blank_frames": 3, "stale_frames": 1}}],
        }
        fork = {
            "target": "q4-e2e-fork",
            "items": [{"id": "flicker", "verdict": "check", "reasons": [],
                       "metrics": {"blank_frames": 1, "stale_frames": 1}}],
        }
        check.apply_fork_baseline([stages, fork], SCENARIO)
        self.assertEqual(stages["items"][0]["verdict"], "pass")
        self.assertEqual(fork["items"][0]["verdict"], "pass")
        stages["items"][0]["metrics"]["blank_frames"] = 10
        stages["items"][0]["verdict"] = "check"
        check.apply_fork_baseline([stages, fork], SCENARIO)
        self.assertEqual(stages["items"][0]["verdict"], "fail")


class PenEvidenceTest(unittest.TestCase):
    def test_empty_evidence_fails_every_pen_item(self):
        with tempfile.TemporaryDirectory() as root:
            items = check.judge_pen(root, {}, SCENARIO["checks"])
        self.assertEqual([entry["id"] for entry in items], list(check.PEN_ITEMS))
        self.assertTrue(all(entry["verdict"] == "fail" for entry in items))

    def test_outline_and_stroke_markers_are_read(self):
        events = {
            "qt_event": [(10, 7, 0, 1), (20, 8, 0, 0)],
            "outline_state": [(12, 0, 0, 4)],
            "stroke_begin": [(30, 0, 0, 0)],
            "stroke_paint": [(32, 0, 0, 0)],
        }
        self.assertTrue(check._has_outline(events, 1, 15))
        self.assertFalse(check._has_outline(events, 21, 40))
        self.assertEqual(check._probe_strokes(events, 25, 35), (1, 1))


class TargetTimeoutTest(unittest.TestCase):
    def test_target_past_the_limit_is_recorded_as_timeout(self):
        class SlowRunner:
            pid = None

            def __init__(self):
                self.killed = False

            def wait(self, timeout=None):
                if timeout is not None:
                    raise subprocess.TimeoutExpired("run-remote.sh", timeout)
                return 1

            def kill(self):
                self.killed = True

        process = SlowRunner()
        original = check.subprocess.Popen
        check.subprocess.Popen = lambda *args, **kwargs: process
        try:
            with tempfile.TemporaryDirectory() as root:
                with redirect_stdout(io.StringIO()):
                    result = check.run_checked(
                        {"name": "tablet", "ssh": "tablet",
                         "capture_tool": "windows_capture.exe"},
                        {"name": "q4-e2e-stage3", "runner_target": "LibrePaint",
                         "app": "C:/LibrePaint.exe"},
                        SCENARIO, root)
        finally:
            check.subprocess.Popen = original
        self.assertEqual(result["target"], "q4-e2e-stage3")
        self.assertEqual(result["run_status"], "timeout")
        self.assertEqual({entry["verdict"] for entry in result["items"]}, {"timeout"})
        self.assertTrue(process.killed)

    def test_timeout_is_not_overwritten_by_the_fork_baseline(self):
        timed_out = {
            "target": "q4-e2e-stage3",
            "items": [{"id": "flicker", "verdict": "timeout", "reasons": [],
                       "metrics": {"blank_frames": 0, "stale_frames": 0}}],
        }
        fork = {
            "target": "q4-e2e-fork",
            "items": [{"id": "flicker", "verdict": "check", "reasons": [],
                       "metrics": {"blank_frames": 3, "stale_frames": 3}}],
        }
        check.apply_fork_baseline([timed_out, fork], SCENARIO)
        self.assertEqual(timed_out["items"][0]["verdict"], "timeout")


class WindowsEntryTest(unittest.TestCase):
    def test_request_carries_the_check_object(self):
        request = run_request.build_request(
            "LibrePaint", 1, "C:/LibrePaint.exe", name="q4-e2e",
            check={"enabled": True, "flicker": {"blank_white_ratio": 0.98}})
        self.assertEqual(request["check"]["flicker"]["blank_white_ratio"], 0.98)
        self.assertNotIn("check", run_request.build_request("LibrePaint", 1, "C:/LibrePaint.exe"))

    def test_runner_passes_check_json_and_copies_the_helper(self):
        runner = (REPO_ROOT / "scripts/perf/pen-latency/run-remote.sh").read_text(encoding="utf-8")
        self.assertIn("CHECK_JSON", runner)
        self.assertIn("Q4Check.cs", runner)
        self.assertIn("check_json", runner)

    def test_surface_runner_has_the_check_entry(self):
        surface = (REPO_ROOT / "scripts/perf/pen-latency/run-surface.ps1").read_text(encoding="utf-8")
        self.assertIn("function Invoke-Q4Check", surface)
        self.assertIn("if ($request.check)", surface)
        for name in ("clipboard", "tabs", "keyboard", "flicker", "first_line"):
            self.assertIn(name, surface)

    def test_settings_dialog_is_opened_with_the_keyboard_only(self):
        surface = (REPO_ROOT / "scripts/perf/pen-latency/run-surface.ps1").read_text(encoding="utf-8")
        body = surface.split("function Invoke-SettingsDialog", 1)[1].split("\nfunction ", 1)[0]
        self.assertNotIn("InvokePattern", body)
        self.assertIn("SendKeys('%s')", body)
        self.assertIn("SendKeys('c')", body)

    def test_check_run_starts_a_hidden_watchdog(self):
        surface = (REPO_ROOT / "scripts/perf/pen-latency/run-surface.ps1").read_text(encoding="utf-8")
        body = surface.split("function Invoke-Q4Check", 1)[1]
        self.assertIn("Start-CheckWatchdog", body)
        self.assertIn("Stop-CheckWatchdog", body)
        self.assertIn("600", body)
        watchdog = surface.split("function Start-CheckWatchdog", 1)[1]
        self.assertIn("powershell.exe", watchdog)
        self.assertIn("-WindowStyle Hidden", watchdog)
        self.assertIn("'timeout'", watchdog)

    def test_capture_tool_records_flicker_frames(self):
        source = (REPO_ROOT / "scripts/perf/pen-latency/windows_capture.cpp").read_text(encoding="utf-8")
        self.assertIn("--monitor", source)
        self.assertIn("flicker.csv", source)
        self.assertIn("canvas_white_ratio", source)
        self.assertIn("old_area_unchanged_ratio", source)

    def test_win32_helper_uses_the_required_apis(self):
        source = (REPO_ROOT / "scripts/perf/pen-latency/Q4Check.cs").read_text(encoding="utf-8")
        for name in ("EnumClipboardFormats", "GetWindowTextW", "LoadKeyboardLayoutW",
                     "SendInput", "GetKeyState", "SetWindowPos", "GetCursorInfo"):
            self.assertIn(name, source)


if __name__ == "__main__":
    unittest.main()
