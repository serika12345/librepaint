# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/display_check.py.

The L2 display check compares a screen capture with the original test image.
These tests build a small image with the same features (solid colours, grey
steps, a 1 px grid and a 1 px checkerboard), scale it the way 100 % and 400 %
zoom show it, and check the verdicts and the reported values.
"""

import importlib.util
import os
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location(
    "display_check", os.path.join(ROOT, "scripts", "perf", "pen-latency", "display_check.py"))
display = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(display)

COLORS = [(255, 0, 0), (0, 255, 0), (0, 0, 255),
          (0, 255, 255), (255, 0, 255), (255, 255, 0)]
GRAYS = [17 * level for level in range(8)]


def synthetic(width=96, height=96):
    rgb = bytearray(width * height * 3)

    def put(x, y, color):
        offset = 3 * (y * width + x)
        rgb[offset:offset + 3] = bytes(color)

    band = width // len(COLORS)
    for y in range(0, height // 4):
        for x in range(width):
            put(x, y, COLORS[min(x // band, len(COLORS) - 1)])
    step = width // len(GRAYS)
    for y in range(height // 4, height // 2):
        for x in range(width):
            level = GRAYS[min(x // step, len(GRAYS) - 1)]
            put(x, y, (level, level, level))
    for y in range(height // 2, 3 * height // 4):
        for x in range(width):
            put(x, y, (0, 0, 0) if (x % 4 == 0 or y % 4 == 0) else (255, 255, 255))
    for y in range(3 * height // 4, height):
        for x in range(width):
            put(x, y, (0, 0, 0) if (x + y) % 2 == 0 else (255, 255, 255))
    return display.Picture(width, height, rgb)


def scaled(picture, factor):
    width, height = picture.width * factor, picture.height * factor
    rgb = bytearray(width * height * 3)
    for y in range(height):
        source = 3 * (y // factor) * picture.width
        row = bytearray()
        for x in range(picture.width):
            row.extend(picture.rgb[source + 3 * x:source + 3 * x + 3] * factor)
        target = 3 * y * width
        rgb[target:target + len(row)] = row
    return display.Picture(width, height, rgb)


def cropped(picture, x0, y0, width, height):
    rgb = bytearray(width * height * 3)
    for y in range(height):
        source = 3 * ((y0 + y) * picture.width + x0)
        target = 3 * y * width
        rgb[target:target + 3 * width] = picture.rgb[source:source + 3 * width]
    return display.Picture(width, height, rgb)


def on_screen(picture, screen_width, screen_height, x0, y0):
    """Paste a canvas into a desktop-like picture with a UI and wallpaper."""
    background = (12, 34, 56)
    rgb = bytearray(bytes(background) * (screen_width * screen_height))
    for y in range(picture.height):
        source = 3 * y * picture.width
        target = 3 * ((y0 + y) * screen_width + x0)
        rgb[target:target + 3 * picture.width] = picture.rgb[source:source + 3 * picture.width]
    # A window border and a menu strip, neither of which uses a source colour.
    for y in range(y0 - 8, y0):
        target = 3 * (y * screen_width)
        rgb[target:target + 3 * screen_width] = bytes((70, 70, 70)) * screen_width
    return display.Picture(screen_width, screen_height, rgb)


class DecodeTest(unittest.TestCase):
    def test_png_and_bmp_round_trip(self):
        picture = synthetic(32, 32)
        with tempfile.TemporaryDirectory() as root:
            png = os.path.join(root, "image.png")
            bmp = os.path.join(root, "image.bmp")
            display.write_png(png, picture)
            display.write_bmp(bmp, picture)
            for path in (png, bmp):
                loaded = display.load(path)
                self.assertEqual((loaded.width, loaded.height), (32, 32))
                self.assertEqual(loaded.rgb, picture.rgb)


class AnalyzeTest(unittest.TestCase):
    def analyze(self, capture, original):
        with tempfile.TemporaryDirectory() as root:
            original_path = os.path.join(root, "original.png")
            capture_path = os.path.join(root, "capture.bmp")
            display.write_png(original_path, original)
            display.write_bmp(capture_path, capture)
            return display.analyze(capture_path, original_path)

    def test_100_percent_passes_with_colours_grey_and_lines(self):
        original = synthetic()
        result = self.analyze(original, original)
        self.assertTrue(result["ok"], display.report(result))
        self.assertAlmostEqual(result["scale"], 1.0)
        self.assertEqual(result["checks"], {"colors": True, "gray": True, "lines": True, "edges": True})
        self.assertLessEqual(result["colors"]["max_diff_levels"], 8)
        self.assertEqual(result["lines"]["source_line_px"], 1.0)

    def test_400_percent_keeps_one_pixel_lines(self):
        original = synthetic()
        result = self.analyze(scaled(original, 4), original)
        self.assertTrue(result["ok"], display.report(result))
        self.assertAlmostEqual(result["scale"], 4.0)
        self.assertTrue(result["checks"]["lines"])
        self.assertAlmostEqual(result["lines"]["source_line_px"], 1.0)

    def test_colour_shift_is_reported_in_levels(self):
        original = synthetic()
        capture = synthetic()
        for y in range(0, capture.height // 4):
            for x in range(0, capture.width // len(COLORS)):
                offset = 3 * (y * capture.width + x)
                capture.rgb[offset + 1] = min(255, capture.rgb[offset + 1] + 20)
        result = self.analyze(capture, original)
        self.assertFalse(result["ok"])
        self.assertFalse(result["checks"]["colors"])
        self.assertEqual(result["colors"]["max_diff_levels"], 20)

    def test_missing_lines_are_reported(self):
        original = synthetic()
        capture = synthetic()
        for y in range(capture.height // 2, capture.height):
            for x in range(capture.width):
                offset = 3 * (y * capture.width + x)
                capture.rgb[offset:offset + 3] = b"\xff\xff\xff"
        result = self.analyze(capture, original)
        self.assertFalse(result["ok"])
        self.assertFalse(result["checks"]["lines"])

    def test_an_empty_screen_is_not_ok(self):
        original = synthetic()
        blank = display.Picture(96, 96, bytearray(b"\xff" * (96 * 96 * 3)))
        result = self.analyze(blank, original)
        self.assertFalse(result["ok"])
        self.assertFalse(result["checks"]["lines"])


class CanvasTest(unittest.TestCase):
    def analyze(self, capture, original):
        with tempfile.TemporaryDirectory() as root:
            original_path = os.path.join(root, "original.png")
            capture_path = os.path.join(root, "capture.bmp")
            display.write_png(original_path, original)
            display.write_bmp(capture_path, capture)
            return display.analyze(capture_path, original_path)

    def test_the_canvas_is_cropped_from_the_desktop(self):
        original = synthetic()
        screen = on_screen(original, 480, 360, 120, 80)
        result = self.analyze(screen, original)
        self.assertTrue(result["ok"], display.report(result))
        self.assertEqual(result["canvas"]["box"], [120, 80, 215, 175])
        self.assertEqual(result["canvas"]["width"], 96)
        self.assertEqual(result["canvas"]["height"], 96)
        self.assertAlmostEqual(result["scale"], 1.0)

    def test_400_percent_canvas_is_cropped_and_scaled_from_the_line_width(self):
        original = synthetic()
        zoomed = scaled(original, 4)
        visible = cropped(zoomed, 100, 100, 140, 140)
        screen = on_screen(visible, 480, 360, 120, 80)
        result = self.analyze(screen, original)
        self.assertTrue(result["ok"], display.report(result))
        self.assertEqual(result["canvas"]["box"], [120, 80, 259, 219])
        # The grey runs are clipped by the canvas edge, so the 4 px line width
        # is what fixes the scale.
        self.assertAlmostEqual(result["scale"], 4.0)
        self.assertAlmostEqual(result["lines"]["source_line_px"], 1.0)

if __name__ == "__main__":
    unittest.main()
