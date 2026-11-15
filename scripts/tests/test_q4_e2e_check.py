#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Contracts of the Q4 automatic check (scripts/perf/check.py).

The check runs on the Windows tablet, but its verdicts are made on the
development host from evidence files. These tests fix the item list, the
clipboard rule (PNG pixels against the document, DIB formats only in the
metrics), the tab-title and keyboard rules, the relative flicker threshold,
and the Windows-side entry points.
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


def paint_box(picture, x0: int, y0: int, width: int, height: int, color=(0, 0, 0)) -> None:
    for y in range(y0, y0 + height):
        for x in range(x0, x0 + width):
            offset = 3 * (y * picture.width + x)
            picture.rgb[offset:offset + 3] = bytes(color)


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
        with open(os.path.join(root, "doc-a.png"), "wb") as handle:
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

    def test_old_dib_formats_stay_in_the_metrics(self):
        with tempfile.TemporaryDirectory() as root:
            source = os.path.join(root, "source.png")
            write_png_rgba(source, 4, 4, rgba_picture(4, 4))
            self._write(root, ["8,CF_DIB,100,", "17,CF_DIBV5,100,", "2,CF_BITMAP,100,",
                               "49161,PNG,100,clipboard-png.png"],
                        file_bytes(source), file_bytes(source))
            result = check.judge_clipboard(root, {"clipboard": {}}, SCENARIO["checks"])
        self.assertEqual(result["verdict"], "pass", result)
        self.assertTrue(result["metrics"]["dib"])
        self.assertTrue(result["metrics"]["dibv5"])
        self.assertTrue(result["metrics"]["bitmap"])
        self.assertNotIn("DIB", " ".join(result["reasons"]))

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

    def test_documented_image_is_compared_even_if_another_source_is_named(self):
        with tempfile.TemporaryDirectory() as root:
            document = os.path.join(root, "document.png")
            other = os.path.join(root, "other.png")
            copied = os.path.join(root, "copy.png")
            write_png_rgba(document, 4, 4, rgba_picture(4, 4, (1, 2, 3, 255)))
            write_png_rgba(other, 4, 4, rgba_picture(4, 4, (9, 9, 9, 255)))
            write_png_rgba(copied, 4, 4, rgba_picture(4, 4, (1, 2, 3, 255)))
            self._write(root, ["49161,PNG,100,clipboard-png.png"], file_bytes(document),
                        file_bytes(copied))
            with open(os.path.join(root, "clipboard-source.png"), "wb") as handle:
                handle.write(file_bytes(other))
            result = check.judge_clipboard(root, {"clipboard": {"source": "clipboard-source.png"}},
                                           SCENARIO["checks"])
        self.assertEqual(result["verdict"], "pass", result)


class ActionLineTest(unittest.TestCase):
    def test_action_line_returns_the_qpc(self):
        with tempfile.TemporaryDirectory() as root:
            path = os.path.join(root, "actions.txt")
            with open(path, "w", encoding="utf-8") as handle:
                handle.write("action=single-start qpc=128772260208 detail=one-line\n")
            actions = check.read_action_lines(path)
        self.assertEqual(check.action_time(actions, "single-start"), 128772260208)


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


class PenCapturePathTest(unittest.TestCase):
    def test_a_capture_value_with_its_directory_is_used_as_written(self):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, "pen"))
            picture = display_check.Picture(200, 200, bytearray(b"\xff" * (200 * 200 * 3)))
            for y in range(30, 70):
                offset = 3 * (y * 200 + 100)
                picture.rgb[offset:offset + 3] = b"\x00\x00\x00"
            display_check.write_png(os.path.join(root, "pen", "single.bmp"), picture)
            facts = {"pen": {"captures": {"single": "pen/single.bmp"},
                             "canvas": [0, 0, 200, 200]}}
            items = check.judge_pen(root, facts, SCENARIO["checks"])
        single = next(entry for entry in items if entry["id"] == "pen.single_line")
        self.assertEqual(single["metrics"]["dark_runs"], 1)
        self.assertIn("pen/single.bmp", single["evidence"])


class PenDifferenceTest(unittest.TestCase):
    def test_a_line_is_counted_from_the_difference_to_before(self):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, "pen"))
            before = display_check.Picture(200, 200, bytearray(b"\xff" * (200 * 200 * 3)))
            after = display_check.Picture(200, 200, bytearray(b"\xff" * (200 * 200 * 3)))
            # Dark bands in both captures are the background the old rule
            # counted as line; only the new row in the difference is a line.
            for y in (40, 60):
                paint_box(before, 0, y, 200, 2)
                paint_box(after, 0, y, 200, 2)
            paint_box(after, 0, 50, 200, 2)
            display_check.write_png(os.path.join(root, "pen", "single.bmp"), after)
            display_check.write_png(os.path.join(root, "pen", "single-before.bmp"), before)
            facts = {"pen": {"captures": {"single": "pen/single.bmp",
                                          "single_before": "pen/single-before.bmp"},
                             "canvas": [0, 0, 200, 200]}}
            items = check.judge_pen(root, facts, SCENARIO["checks"])
        single = next(entry for entry in items if entry["id"] == "pen.single_line")
        self.assertEqual(single["metrics"]["dark_runs"], 1)

    def test_a_dot_is_counted_from_the_difference_to_before(self):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, "pen"))
            before = display_check.Picture(200, 200, bytearray(b"\xff" * (200 * 200 * 3)))
            after = display_check.Picture(200, 200, bytearray(b"\xff" * (200 * 200 * 3)))
            # Two dark blobs in both captures stay out of the difference.
            for corner in ((30, 90), (80, 140)):
                paint_box(before, corner[0], corner[1], 4, 4)
                paint_box(after, corner[0], corner[1], 4, 4)
            paint_box(after, 60, 112, 5, 5)
            display_check.write_png(os.path.join(root, "pen", "dot.bmp"), after)
            display_check.write_png(os.path.join(root, "pen", "dot-before.bmp"), before)
            facts = {"pen": {"captures": {"dot": "pen/dot.bmp",
                                          "dot_before": "pen/dot-before.bmp"},
                             "canvas": [0, 0, 200, 200]}}
            items = check.judge_pen(root, facts, SCENARIO["checks"])
        dot = next(entry for entry in items if entry["id"] == "pen.dot")
        self.assertEqual(dot["metrics"]["components"], 1)


class PenDifferencePositionTest(unittest.TestCase):
    """The line and the dot are counted where run-surface.ps1 draws them.

    The pictures are 200 x 100 and the canvas.json rectangle is wrong on
    purpose: the difference gives the positions, not the rectangle.
    """

    WIDTH, HEIGHT = 200, 100
    CANVASES = ([0, 0, 200, 100], [1097, 231, 944, 918])

    def _judged(self, name: str, draw, canvas: list) -> dict:
        with tempfile.TemporaryDirectory() as root:
            pen = os.path.join(root, "pen")
            os.makedirs(pen)
            before = display_check.Picture(self.WIDTH, self.HEIGHT,
                                           bytearray(b"\xff" * (self.WIDTH * self.HEIGHT * 3)))
            after = display_check.Picture(self.WIDTH, self.HEIGHT,
                                          bytearray(b"\xff" * (self.WIDTH * self.HEIGHT * 3)))
            draw(after)
            display_check.write_png(os.path.join(pen, name + ".bmp"), after)
            display_check.write_png(os.path.join(pen, name + "-before.bmp"), before)
            with open(os.path.join(pen, "canvas.json"), "w", encoding="utf-8") as handle:
                json.dump({"canvas": canvas}, handle)
            facts = {"pen": {"captures": {
                name: "pen/%s.bmp" % name,
                name + "_before": "pen/%s-before.bmp" % name}}}
            items = check.judge_pen(root, facts, SCENARIO["checks"])
        return {entry["id"]: entry for entry in items}

    @staticmethod
    def _stroke(after) -> None:
        # The stroke runs at 0.25 of the height; the middle 0.45..0.55 of the
        # width is unchanged, as dark ink drawn over dark ink would be.
        paint_box(after, 40, 24, 50, 4)
        paint_box(after, 110, 24, 50, 4)

    @staticmethod
    def _double_stroke(after) -> None:
        paint_box(after, 40, 18, 120, 4)
        paint_box(after, 40, 28, 120, 4)

    @staticmethod
    def _dot(after) -> None:
        paint_box(after, 58, 58, 5, 5)

    def test_a_stroke_with_a_gap_between_them_is_one_line(self):
        for canvas in self.CANVASES:
            with self.subTest(canvas=canvas):
                single = self._judged("single", self._stroke, canvas)["pen.single_line"]
                self.assertEqual(single["metrics"]["dark_runs"], 1)
                self.assertEqual(single["metrics"]["dark_runs_columns"],
                                 [1, 1, 1, 1, 0, 1, 1, 1, 1])
                self.assertNotIn("二重の線", " ".join(single["reasons"]))

    def test_two_strokes_six_pixels_apart_fail(self):
        for canvas in self.CANVASES:
            with self.subTest(canvas=canvas):
                single = self._judged("single", self._double_stroke, canvas)["pen.single_line"]
                self.assertEqual(single["metrics"]["dark_runs"], 2)
                self.assertIn("二重の線", " ".join(single["reasons"]))

    def test_a_dot_at_the_pen_position_is_one_dot(self):
        for canvas in self.CANVASES:
            with self.subTest(canvas=canvas):
                dot = self._judged("dot", self._dot, canvas)["pen.dot"]
                self.assertEqual(dot["metrics"]["components"], 1)
                self.assertNotIn("個に分かれて", " ".join(dot["reasons"]))


class PopupListTest(unittest.TestCase):
    def test_an_empty_popup_list_means_no_menu_and_a_missing_list_is_reported(self):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, "pen"))
            popups = os.path.join(root, "pen", "popups.txt")
            open(popups, "w", encoding="utf-8").close()
            empty = {entry["id"]: entry for entry in check.judge_pen(root, {}, SCENARIO["checks"])}
            os.remove(popups)
            missing = {entry["id"]: entry for entry in check.judge_pen(root, {}, SCENARIO["checks"])}
        self.assertNotIn("長押しの間の窓の一覧がない", empty["pen.long_press"]["reasons"])
        self.assertIn("長押しの間の窓の一覧がない", missing["pen.long_press"]["reasons"])


class AnalyzeOnlyTest(unittest.TestCase):
    def test_the_last_nested_run_is_judged_under_the_target_name(self):
        with tempfile.TemporaryDirectory() as root:
            for stamp, dialog in (("20260101-000000", False), ("20260102-000000", True)):
                run_dir = os.path.join(root, "windows", "q4-e2e-fork", stamp)
                os.makedirs(run_dir)
                with open(os.path.join(run_dir, "facts.json"), "w", encoding="utf-8") as handle:
                    json.dump({"flicker": {"dialog_opened": dialog, "monitor_exit": 0}}, handle)
                with open(os.path.join(run_dir, "flicker.csv"), "w", encoding="utf-8") as handle:
                    handle.write("frame_ns,arrival_ns,rect_x,rect_y,rect_w,rect_h,rect_changed,"
                                 "canvas_white_ratio,canvas_dark_pixels,changed_ratio,"
                                 "old_area_unchanged_ratio,frames\n")
                    handle.write("1000,1000,0,0,100,100,1,0.1,0,0.0,0.0,0\n")
            hosts = os.path.join(root, "hosts.json")
            with open(hosts, "w", encoding="utf-8") as handle:
                json.dump({"hosts": [{"name": "windows"}]}, handle)
            with redirect_stdout(io.StringIO()):
                status = check.main([
                    "--scenario", str(REPO_ROOT / "scripts/perf/scenarios/q4-e2e.json"),
                    "--hosts", hosts, "--analyze-only", root])
            summary = json.loads((Path(root) / "checks.json").read_text(encoding="utf-8"))
        self.assertEqual(status, 1)
        self.assertEqual([entry["target"] for entry in summary["targets"]], ["q4-e2e-fork"])
        flicker = next(entry for entry in summary["targets"][0]["items"]
                       if entry["id"] == "flicker")
        self.assertEqual(flicker["verdict"], "pass", flicker)
        self.assertEqual(flicker["reasons"], ["基準（フォーク版）"])


class StepEventTest(unittest.TestCase):
    """check.py copies the streamed step lines into the event log."""

    def test_step_lines_become_events(self):
        with tempfile.TemporaryDirectory() as root:
            path = os.path.join(root, "events.log")
            self.assertEqual(check.write_step_event(
                path, "q4-e2e", "q4-e2e-stage3", "status: step clipboard start"), "step")
            self.assertEqual(check.write_step_event(
                path, "q4-e2e", "q4-e2e-stage3", "status: step clipboard end done"), "step")
            self.assertEqual(check.write_step_event(
                path, "q4-e2e", "q4-e2e-stage3", "status: step flicker end timeout"), "step-timeout")
            self.assertIsNone(check.write_step_event(
                path, "q4-e2e", "q4-e2e-stage3", "status: presentmon=present"))
            with open(path, encoding="utf-8") as handle:
                fields = [line.split("\t") for line in handle.read().splitlines()]
        self.assertEqual([entry[1] for entry in fields], ["step", "step", "step-timeout"])
        self.assertEqual([entry[3] for entry in fields], ["q4-e2e-stage3"] * 3)
        self.assertEqual([entry[4] for entry in fields], ["1"] * 3)
        self.assertEqual([entry[5] for entry in fields],
                         ["clipboard start", "clipboard end done", "flicker"])

    def test_no_event_log_ignores_the_lines(self):
        self.assertIsNone(check.write_step_event(None, "q4-e2e", "T", "status: step tabs start"))


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
