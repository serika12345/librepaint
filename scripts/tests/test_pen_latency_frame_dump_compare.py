# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/pen-latency/frame_dump_compare.py.

The Windows ink diagnosis pairs the canvas framebuffer dump of the application
with the desktop screen dump of the same moment. These tests build small
synthetic dumps and check that the tool reports the four verdicts:
app_missing, screen_missing, both_present and insufficient.
"""

import contextlib
import importlib.util
import io
import json
import os
import struct
import sys
import tempfile
import unittest
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, "scripts", "perf", "pen-latency")


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(TOOLS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


frame_dump_compare = load("frame_dump_compare")

WIDTH = 400
HEIGHT = 40
GLOBAL = (100, 200)
DPR = 1.0
ORIGIN = (0, 0)
BLACK = b"\x00\x00\x00\xff"
WHITE = b"\xff\xff\xff\xff"


def image_rows(dark_columns):
    rows = []
    for _ in range(HEIGHT):
        row = bytearray()
        for x in range(WIDTH):
            row += BLACK if x < dark_columns else WHITE
        rows.append(bytes(row))
    return rows


def write_png(path, rows):
    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    raw = b"".join(b"\x00" + row for row in rows)
    header = struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 6, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", header))
        handle.write(chunk(b"IDAT", zlib.compress(raw)))
        handle.write(chunk(b"IEND", b""))


def write_after(path):
    # The full screen frame covers the canvas rectangle and its 160 dark
    # columns; everything outside the canvas is white.
    width, height = 500, 240
    rows = []
    for _ in range(height):
        row = bytearray()
        for x in range(width):
            row += BLACK if GLOBAL[0] <= x < GLOBAL[0] + 160 else WHITE
        rows.append(bytes(row))
    write_bmp(path, rows, width, height)


def write_bmp(path, rows, width=WIDTH, height=HEIGHT):
    stride = width * 4
    size = stride * height
    file_header = struct.pack("<2sIHHI", b"BM", 14 + 40 + size, 0, 0, 54)
    info_header = struct.pack("<IiiHHIIiiII", 40, width, -height, 1, 32, 0, size, 0, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(file_header)
        handle.write(info_header)
        for row in rows:
            handle.write(row)


def write_csv(path, header, rows):
    with open(path, "w", encoding="utf-8", newline="") as handle:
        handle.write(header + "\n")
        for row in rows:
            handle.write(",".join(str(value) for value in row) + "\n")


def write_run(folder, pairs, after_dark_columns=None):
    """pairs: list of (app_dark_columns, screen_dark_columns)."""
    fbo_rows = []
    screen_rows = []
    for index, (app_columns, screen_columns) in enumerate(pairs):
        time_ns = 1_000_000_000 + index * 100_000_000
        fbo_rows.append((index, time_ns, GLOBAL[0], GLOBAL[1], WIDTH, HEIGHT, DPR, 1, 0, 0, WIDTH, HEIGHT))
        screen_rows.append((index, time_ns, time_ns, GLOBAL[0], GLOBAL[1], WIDTH, HEIGHT))
        write_png(os.path.join(folder, "fbo-%03d.png" % index), image_rows(app_columns))
        write_bmp(os.path.join(folder, "screen-%03d.bmp" % index), image_rows(screen_columns))
    write_csv(os.path.join(folder, "fbo-dumps.csv"),
              "index,time_ns,global_x,global_y,width_px,height_px,dpr,native_window,"
              "update_x,update_y,update_w,update_h", fbo_rows)
    write_csv(os.path.join(folder, "screen-dumps.csv"),
              "index,frame_ns,arrival_ns,left,top,width,height", screen_rows)
    write_csv(os.path.join(folder, "inject.csv"), "time_ns,phase,band,x,y,pressed", [
        (0, "ink", "band", 0.0, 0.0, 1),
        (50_000_000, "ink", "band", 150.0, 0.0, 1),
        (100_000_000, "ink", "band", 300.0, 0.0, 1),
    ])
    with open(os.path.join(folder, "capture-meta.json"), "w", encoding="utf-8") as handle:
        json.dump({"origin": list(ORIGIN)}, handle)
    with open(os.path.join(folder, "canvas.json"), "w", encoding="utf-8") as handle:
        json.dump({"origin": list(ORIGIN), "canvas": [GLOBAL[0], GLOBAL[1], WIDTH, HEIGHT]}, handle)
    if after_dark_columns is not None:
        write_after(os.path.join(folder, "screen-after.bmp"))


def run(folder):
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        status = frame_dump_compare.main([folder, "--json"])
    assert status == 0
    return json.loads(output.getvalue())


class FrameDumpCompareTest(unittest.TestCase):
    def test_app_missing(self):
        with tempfile.TemporaryDirectory() as folder:
            write_run(folder, [(10, 300)] * 3)
            result = run(folder)
        self.assertEqual(result["verdict"], "app_missing")
        self.assertEqual([pair["fbo_dark_cols"] for pair in result["pairs"]], [10, 10, 10])
        self.assertEqual([pair["pen_extent_px"] for pair in result["pairs"]], [300.0, 300.0, 300.0])

    def test_screen_missing(self):
        with tempfile.TemporaryDirectory() as folder:
            write_run(folder, [(160, 40)] * 3)
            result = run(folder)
        self.assertEqual(result["verdict"], "screen_missing")
        self.assertEqual([pair["screen_dark_cols"] for pair in result["pairs"]], [40, 40, 40])

    def test_both_present(self):
        with tempfile.TemporaryDirectory() as folder:
            write_run(folder, [(160, 150)] * 3, after_dark_columns=160)
            result = run(folder)
        self.assertEqual(result["verdict"], "both_present")
        self.assertEqual(result["after_dark_cols"], 160)

    def test_insufficient(self):
        with tempfile.TemporaryDirectory() as folder:
            write_run(folder, [(160, 150)] * 2)
            result = run(folder)
        self.assertEqual(result["verdict"], "insufficient")


if __name__ == "__main__":
    unittest.main()
