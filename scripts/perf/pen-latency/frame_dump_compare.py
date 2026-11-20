#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Compare the canvas and screen frame dumps of one Windows diagnosis run.

The dumps come from LIBREPAINT_DIAG_FRAME_DUMP=1: the application writes
fbo-<NNN>.png and fbo-dumps.csv for its canvas framebuffer, and
windows_capture.exe writes screen-<NNN>.bmp and screen-dumps.csv for the
measured screen region. This tool pairs the two series by time, measures the
dark pixels and dark columns inside the overlapping rectangle, and reports one
verdict that separates an application render without ink from a screen that
loses the ink the application rendered.

    frame_dump_compare.py RUN_DIR [--json]

A dark pixel is one whose red, green and blue are all below 128, the rule of
dark() in windows_capture.cpp. pen_extent_px is the width of the x range of the
pressed pen points since the last press before the canvas frame time. The
verdict uses the pairs whose pen_extent_px is at least 200.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import statistics
import struct
import sys
import zlib

DARK_THRESHOLD = 128

# One byte per possible channel value: 1 when the channel is dark.
_DARK = bytes(1 if value < DARK_THRESHOLD else 0 for value in range(256))


def read_png(path):
    """Reads an 8-bit RGBA PNG and returns (width, height, RGBA rows)."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("%s is not a PNG file" % path)
    position = 8
    compressed = []
    width = height = None
    color_type = None
    bit_depth = None
    while position + 8 <= len(data):
        length = struct.unpack(">I", data[position:position + 4])[0]
        kind = data[position + 4:position + 8]
        chunk = data[position + 8:position + 8 + length]
        position += 12 + length
        if kind == b"IHDR":
            width, height, bit_depth, color_type = struct.unpack(">IIBB", chunk[:10])
        elif kind == b"IDAT":
            compressed.append(chunk)
        elif kind == b"IEND":
            break
    if bit_depth != 8 or color_type != 6:
        raise ValueError("%s is not an 8-bit RGBA PNG" % path)
    raw = zlib.decompress(b"".join(compressed))
    stride = width * 4
    pixels = bytearray(width * height * 4)
    previous = bytearray(stride)
    position = 0
    for row in range(height):
        filter_type = raw[position]
        position += 1
        line = bytearray(raw[position:position + stride])
        position += stride
        if filter_type == 0:
            pass
        elif filter_type == 1:
            for i in range(4, stride):
                line[i] = (line[i] + line[i - 4]) & 0xFF
        elif filter_type == 2:
            if any(line):
                for i in range(stride):
                    line[i] = (line[i] + previous[i]) & 0xFF
            else:
                line = bytearray(previous)
        elif filter_type == 3:
            for i in range(stride):
                left = line[i - 4] if i >= 4 else 0
                line[i] = (line[i] + ((left + previous[i]) >> 1)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                left = line[i - 4] if i >= 4 else 0
                up = previous[i]
                upper_left = previous[i - 4] if i >= 4 else 0
                estimate = left + up - upper_left
                left_distance = abs(estimate - left)
                up_distance = abs(estimate - up)
                upper_left_distance = abs(estimate - upper_left)
                if left_distance <= up_distance and left_distance <= upper_left_distance:
                    predictor = left
                elif up_distance <= upper_left_distance:
                    predictor = up
                else:
                    predictor = upper_left
                line[i] = (line[i] + predictor) & 0xFF
        else:
            raise ValueError("%s uses an unsupported PNG filter %d" % (path, filter_type))
        pixels[row * stride:(row + 1) * stride] = line
        previous = line
    return width, height, bytes(pixels)


def read_bmp(path):
    """Reads the 32-bit top-down BMP written by saveBmp in windows_capture."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:2] != b"BM":
        raise ValueError("%s is not a BMP file" % path)
    offset = struct.unpack("<I", data[10:14])[0]
    width, height = struct.unpack("<ii", data[18:26])
    bits = struct.unpack("<H", data[28:30])[0]
    compression = struct.unpack("<I", data[30:34])[0]
    if bits != 32 or compression != 0:
        raise ValueError("%s is not an uncompressed 32-bit BMP" % path)
    top_down = height < 0
    height = abs(height)
    stride = width * 4
    pixels = bytearray(width * height * 4)
    for row in range(height):
        source_row = row if top_down else height - 1 - row
        start = offset + source_row * stride
        line = data[start:start + stride]
        rgba = bytearray(stride)
        rgba[0::4] = line[2::4]
        rgba[1::4] = line[1::4]
        rgba[2::4] = line[0::4]
        rgba[3::4] = line[3::4]
        pixels[row * stride:(row + 1) * stride] = rgba
    return width, height, bytes(pixels)


def read_csv(path):
    with open(path, newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def dark_counts(image, rect):
    """Counts dark pixels and dark columns of an image inside rect."""
    width, _, pixels = image
    x0, y0, x1, y1 = rect
    columns = x1 - x0
    if columns <= 0 or y1 <= y0:
        return 0, 0
    column_mask = 0
    dark_pixels = 0
    for y in range(y0, y1):
        start = (y * width + x0) * 4
        line = pixels[start:start + columns * 4]
        red = int.from_bytes(line[0::4].translate(_DARK), "big")
        green = int.from_bytes(line[1::4].translate(_DARK), "big")
        blue = int.from_bytes(line[2::4].translate(_DARK), "big")
        dark = red & green & blue
        dark_pixels += dark.to_bytes(columns, "big").count(1)
        column_mask |= dark
    return dark_pixels, column_mask.to_bytes(columns, "big").count(1)


def intersect(first, second):
    x0 = max(first[0], second[0])
    y0 = max(first[1], second[1])
    x1 = min(first[0] + first[2], second[0] + second[2])
    y1 = min(first[1] + first[3], second[1] + second[3])
    if x1 <= x0 or y1 <= y0:
        return None
    return (x0, y0, x1, y1)


def pen_extent(inject_rows, limit_ns):
    """Width of the pressed x range since the last press up to limit_ns."""
    last_press = None
    for index, row in enumerate(inject_rows):
        if int(row["time_ns"]) > limit_ns:
            break
        if int(row["pressed"]) and (index == 0 or not int(inject_rows[index - 1]["pressed"])):
            last_press = index
    if last_press is None:
        return 0.0
    xs = [float(row["x"]) for row in inject_rows[last_press:]
          if int(row["pressed"]) and int(row["time_ns"]) <= limit_ns]
    if not xs:
        return 0.0
    return max(xs) - min(xs)


def nearest_screen(screen_rows, time_ns):
    best = None
    best_difference = None
    for row in screen_rows:
        difference = abs(int(row["frame_ns"]) - time_ns)
        if best_difference is None or difference < best_difference:
            best = row
            best_difference = difference
    return best, best_difference


def compare(folder):
    fbo_rows = read_csv(os.path.join(folder, "fbo-dumps.csv"))
    screen_rows = read_csv(os.path.join(folder, "screen-dumps.csv"))
    inject_rows = read_csv(os.path.join(folder, "inject.csv"))
    with open(os.path.join(folder, "capture-meta.json"), encoding="utf-8") as handle:
        meta = json.load(handle)
    origin_x, origin_y = meta["origin"]

    pairs = []
    for fbo in fbo_rows:
        time_ns = int(fbo["time_ns"])
        screen, difference = nearest_screen(screen_rows, time_ns)
        if screen is None or difference > 50_000_000:
            continue
        dpr = float(fbo["dpr"])
        app_rect = (
            round(int(fbo["global_x"]) * dpr - origin_x),
            round(int(fbo["global_y"]) * dpr - origin_y),
            int(fbo["width_px"]),
            int(fbo["height_px"]),
        )
        screen_rect = (int(screen["left"]), int(screen["top"]),
                       int(screen["width"]), int(screen["height"]))
        overlap = intersect(app_rect, screen_rect)
        if overlap is None:
            continue
        app_image = read_png(os.path.join(folder, "fbo-%03d.png" % int(fbo["index"])))
        screen_image = read_bmp(os.path.join(folder, "screen-%03d.bmp" % int(screen["index"])))
        app_dark_px, app_dark_cols = dark_counts(
            app_image, (overlap[0] - app_rect[0], overlap[1] - app_rect[1],
                        overlap[2] - app_rect[0], overlap[3] - app_rect[1]))
        screen_dark_px, screen_dark_cols = dark_counts(
            screen_image, (overlap[0] - screen_rect[0], overlap[1] - screen_rect[1],
                           overlap[2] - screen_rect[0], overlap[3] - screen_rect[1]))
        pairs.append({
            "index": int(fbo["index"]),
            "time_ns": time_ns,
            "screen_index": int(screen["index"]),
            "fbo_dark_px": app_dark_px,
            "fbo_dark_cols": app_dark_cols,
            "screen_dark_px": screen_dark_px,
            "screen_dark_cols": screen_dark_cols,
            "pen_extent_px": pen_extent(inject_rows, time_ns),
        })

    after_dark_cols = None
    after_path = os.path.join(folder, "screen-after.bmp")
    canvas_path = os.path.join(folder, "canvas.json")
    if os.path.exists(after_path) and os.path.exists(canvas_path):
        with open(canvas_path, encoding="utf-8") as handle:
            canvas = json.load(handle)["canvas"]
        after_image = read_bmp(after_path)
        canvas_rect = (canvas[0] - origin_x, canvas[1] - origin_y, canvas[2], canvas[3])
        clipped = intersect(canvas_rect, (0, 0, after_image[0], after_image[1]))
        if clipped is not None:
            _, after_dark_cols = dark_counts(
                after_image, (clipped[0], clipped[1], clipped[2], clipped[3]))

    target = [pair for pair in pairs if pair["pen_extent_px"] >= 200]
    if len(target) < 3:
        verdict = "insufficient"
    else:
        app_columns = statistics.median([pair["fbo_dark_cols"] for pair in target])
        pen_width = statistics.median([pair["pen_extent_px"] for pair in target])
        if app_columns < 0.5 * pen_width:
            verdict = "app_missing"
        else:
            ratios = [pair["screen_dark_cols"] / pair["fbo_dark_cols"]
                      for pair in target if pair["fbo_dark_cols"] > 0]
            ratio = statistics.median(ratios) if ratios else 0.0
            verdict = "screen_missing" if ratio < 0.5 else "both_present"

    return {
        "pairs": pairs,
        "after_dark_cols": after_dark_cols,
        "verdict": verdict,
    }


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("run_dir", help="folder of one LIBREPAINT_DIAG_FRAME_DUMP run")
    parser.add_argument("--json", action="store_true", help="write the result as JSON")
    args = parser.parse_args(argv)

    result = compare(args.run_dir)
    if args.json:
        json.dump(result, sys.stdout, indent=2)
        sys.stdout.write("\n")
        return 0

    print("after_dark_cols=%s" % result["after_dark_cols"])
    for pair in result["pairs"]:
        print("pair fbo=%03d screen=%03d time_ns=%d fbo_dark_px=%d fbo_dark_cols=%d "
              "screen_dark_px=%d screen_dark_cols=%d pen_extent_px=%.1f"
              % (pair["index"], pair["screen_index"], pair["time_ns"],
                 pair["fbo_dark_px"], pair["fbo_dark_cols"],
                 pair["screen_dark_px"], pair["screen_dark_cols"], pair["pen_extent_px"]))
    print("verdict=%s" % result["verdict"])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
