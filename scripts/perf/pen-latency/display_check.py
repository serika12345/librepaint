#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare a display-check capture with the original test image.

The display check opens build/perf/l2-display-check.png in LibrePaint and
captures the whole screen at 100 % and 400 % zoom (run-surface.ps1 writes
display-100.bmp and display-400.bmp). This tool reads those BMP files and the
original PNG, finds the solid colour bands, the grey steps and the 1 px line
grid in the capture, and reports:

- the colour difference of every solid band and grey step in 8-bit levels;
- whether the 1 px lines survive (line width, gap and contrast);
- the width of the transition between two adjacent colours (the edge).

It needs no image library: PNG (8-bit RGB/RGBA) and uncompressed BMP are read
and written here, so the contract tests run on the development host.

Usage:
  display_check.py RUN_DIR --image build/perf/l2-display-check.png [--json OUT]
"""

import argparse
import json
import os
import struct
import sys
import zlib

# A solid band may differ from the original by a few levels through colour
# management. The pass limits are documented in DEVELOPMENT.md.
COLOR_LIMIT = 8
LINE_CONTRAST_LIMIT = 128
MIN_TRANSITIONS = 8


class Picture:
    """An 8-bit RGB image with three bytes per pixel, top row first."""

    def __init__(self, width, height, rgb):
        self.width = width
        self.height = height
        self.rgb = rgb

    def pixel(self, x, y):
        offset = 3 * (y * self.width + x)
        return self.rgb[offset], self.rgb[offset + 1], self.rgb[offset + 2]


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_png(path):
    """Read an 8-bit RGB or RGBA, non-interlaced PNG."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("%s is not a PNG" % path)
    position = 8
    width = height = channels = None
    compressed = []
    while position < len(data):
        length = struct.unpack(">I", data[position:position + 4])[0]
        kind = data[position + 4:position + 8]
        chunk = data[position + 8:position + 8 + length]
        position += 12 + length
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
            if depth != 8 or color not in (2, 6) or interlace != 0:
                raise ValueError("unsupported PNG: depth %d colour %d interlace %d" % (depth, color, interlace))
            channels = 3 if color == 2 else 4
        elif kind == b"IDAT":
            compressed.append(chunk)
        elif kind == b"IEND":
            break
    if width is None:
        raise ValueError("%s has no IHDR" % path)
    raw = zlib.decompress(b"".join(compressed))
    stride = width * channels
    out = bytearray(width * height * 3)
    previous = bytearray(stride)
    position = 0
    for y in range(height):
        filter_type = raw[position]
        position += 1
        row = bytearray(raw[position:position + stride])
        position += stride
        if filter_type == 1:
            for x in range(channels, stride):
                row[x] = (row[x] + row[x - channels]) & 0xFF
        elif filter_type == 2:
            for x in range(stride):
                row[x] = (row[x] + previous[x]) & 0xFF
        elif filter_type == 3:
            for x in range(stride):
                left = row[x - channels] if x >= channels else 0
                row[x] = (row[x] + ((left + previous[x]) >> 1)) & 0xFF
        elif filter_type == 4:
            for x in range(stride):
                left = row[x - channels] if x >= channels else 0
                upper = previous[x]
                upper_left = previous[x - channels] if x >= channels else 0
                row[x] = (row[x] + _paeth(left, upper, upper_left)) & 0xFF
        elif filter_type != 0:
            raise ValueError("unsupported PNG filter %d" % filter_type)
        target = 3 * y * width
        for x in range(width):
            source = x * channels
            out[target + 3 * x] = row[source]
            out[target + 3 * x + 1] = row[source + 1]
            out[target + 3 * x + 2] = row[source + 2]
        previous = row
    return Picture(width, height, out)


def read_bmp(path):
    """Read an uncompressed 24- or 32-bit BMP (windows_capture writes 32-bit)."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:2] != b"BM":
        raise ValueError("%s is not a BMP" % path)
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    header_size = struct.unpack_from("<I", data, 14)[0]
    if header_size < 40:
        raise ValueError("unsupported BMP header")
    width, height, planes, bits, compression = struct.unpack_from("<iiHHI", data, 18)
    if planes != 1 or compression != 0 or bits not in (24, 32):
        raise ValueError("unsupported BMP: %d bits compression %d" % (bits, compression))
    top_down = height < 0
    height = abs(height)
    bytes_per_pixel = bits // 8
    row_size = ((width * bytes_per_pixel + 3) // 4) * 4
    out = bytearray(width * height * 3)
    for y in range(height):
        source_y = y if top_down else height - 1 - y
        row = pixel_offset + source_y * row_size
        target = 3 * y * width
        for x in range(width):
            source = row + x * bytes_per_pixel
            out[target + 3 * x] = data[source + 2]
            out[target + 3 * x + 1] = data[source + 1]
            out[target + 3 * x + 2] = data[source]
    return Picture(width, height, out)


def write_png(path, picture):
    """Write a filter-0 PNG; used by the contract tests."""
    raw = bytearray()
    for y in range(picture.height):
        raw.append(0)
        start = 3 * y * picture.width
        raw.extend(picture.rgb[start:start + 3 * picture.width])

    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", struct.pack(">IIBBBBB", picture.width, picture.height, 8, 2, 0, 0, 0)))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        handle.write(chunk(b"IEND", b""))


def write_bmp(path, picture):
    """Write a top-down 32-bit BMP; used by the contract tests."""
    row_size = picture.width * 4
    size = 14 + 40 + row_size * picture.height
    header = struct.pack("<2sIHHI", b"BM", size, 0, 0, 54)
    info = struct.pack("<IiiHHIIiiII", 40, picture.width, -picture.height, 1, 32, 0,
                       row_size * picture.height, 2835, 2835, 0, 0)
    with open(path, "wb") as handle:
        handle.write(header + info)
        for y in range(picture.height):
            start = 3 * y * picture.width
            row = bytearray(row_size)
            for x in range(picture.width):
                row[4 * x] = picture.rgb[start + 3 * x + 2]
                row[4 * x + 1] = picture.rgb[start + 3 * x + 1]
                row[4 * x + 2] = picture.rgb[start + 3 * x]
                row[4 * x + 3] = 255
            handle.write(row)


def load(path):
    with open(path, "rb") as handle:
        magic = handle.read(2)
    if magic == b"\x89P":
        return read_png(path)
    if magic == b"BM":
        return read_bmp(path)
    raise ValueError("%s is neither PNG nor BMP" % path)


def is_gray(color):
    return color[0] == color[1] == color[2]


def dominant_colors(picture):
    """Return colours that cover a meaningful part of the image, common first."""
    step = max(1, min(picture.width, picture.height) // 96)
    counts = {}
    for y in range(0, picture.height, step):
        for x in range(0, picture.width, step):
            color = picture.pixel(x, y)
            counts[color] = counts.get(color, 0) + 1
    total = sum(counts.values())
    threshold = max(8, int(total * 0.003))
    return [color for color, count in sorted(counts.items(), key=lambda item: -item[1])
            if count >= threshold]


def classify(color, palette, tolerance):
    best = None
    best_distance = 256
    for candidate in palette:
        distance = max(abs(color[i] - candidate[i]) for i in range(3))
        if distance < best_distance:
            best_distance = distance
            best = candidate
    return best if best_distance <= tolerance else None


def row_runs(picture, y, palette, tolerance=40, minimum=6):
    """Group one row into runs of one palette colour; return mean colours."""
    runs = []
    start = 0
    current = classify(picture.pixel(0, y), palette, tolerance)
    for x in range(1, picture.width):
        found = classify(picture.pixel(x, y), palette, tolerance)
        if found != current:
            if current is not None and x - start >= minimum:
                runs.append((start, x - start, current, _mean(picture, y, start, x)))
            start = x
            current = found
    if current is not None and picture.width - start >= minimum:
        runs.append((start, picture.width - start, current,
                     _mean(picture, y, start, picture.width)))
    return runs


def _mean(picture, y, start, end):
    # Average the middle half so anti-aliased run edges do not move the value.
    margin = max(0, (end - start) // 4)
    first, last = start + margin, end - margin
    if last <= first:
        first, last = start, end
    sums = [0, 0, 0]
    for x in range(first, last):
        color = picture.pixel(x, y)
        sums[0] += color[0]
        sums[1] += color[1]
        sums[2] += color[2]
    count = last - first
    return tuple(value // count for value in sums)


def best_row(picture, palette, wants_gray, step=4):
    """Pick the row that shows the most distinct palette colours."""
    best = None
    best_score = (0, 0)
    for y in range(0, picture.height, step):
        runs = row_runs(picture, y, palette, minimum=4)
        if wants_gray:
            runs = [run for run in runs if is_gray(run[2])]
        else:
            runs = [run for run in runs if not is_gray(run[2])]
        distinct = len({run[2] for run in runs})
        length = sum(run[1] for run in runs)
        score = (distinct, length)
        if score > best_score:
            best_score = score
            best = (y, runs)
    return best


def stripe_row(picture, step=2):
    """Pick the row with the most black/white transitions (the 1 px grid)."""
    best = None
    best_count = 0
    for y in range(0, picture.height, step):
        runs = []
        start = 0
        current = _dark(picture.pixel(0, y))
        for x in range(1, picture.width):
            found = _dark(picture.pixel(x, y))
            if found != current:
                runs.append((start, x - start, current))
                start = x
                current = found
        runs.append((start, picture.width - start, current))
        if len(runs) > best_count:
            best_count = len(runs)
            best = (y, runs)
    return best


def _dark(color):
    return (color[0] + color[1] + color[2]) / 3.0 < 128.0


def _luma(color):
    return 0.2126 * color[0] + 0.7152 * color[1] + 0.0722 * color[2]


def stripe_metrics(picture, y, runs):
    black = [length for _, length, dark in runs if dark]
    white = [length for _, length, dark in runs if not dark]
    if len(black) < 2 or len(white) < 2:
        return None
    black_mean = sum(_luma(picture.pixel(start + length // 2, y))
                     for start, length, dark in runs if dark) / len(black)
    white_mean = sum(_luma(picture.pixel(start + length // 2, y))
                     for start, length, dark in runs if not dark) / len(white)
    return {
        "transitions": len(runs) - 1,
        "line_px": sorted(black)[len(black) // 2],
        "gap_px": sorted(white)[len(white) // 2],
        "period_px": sorted(black)[len(black) // 2] + sorted(white)[len(white) // 2],
        "contrast": round(white_mean - black_mean, 1),
    }


def transition_widths(picture, y, runs):
    """Screen-pixel width of the 10 %..90 % transition at each colour boundary."""
    widths = []
    for index in range(len(runs) - 1):
        start_a, length_a, _, mean_a = runs[index]
        start_b, length_b, _, mean_b = runs[index + 1]
        step = max(abs(mean_a[i] - mean_b[i]) for i in range(3))
        if step < 12:
            continue
        first = start_a + length_a // 2
        last = start_b + length_b // 2
        left = None
        right = None
        for x in range(first, last + 1):
            color = picture.pixel(x, y)
            from_a = max(abs(color[i] - mean_a[i]) for i in range(3))
            from_b = max(abs(color[i] - mean_b[i]) for i in range(3))
            if from_a > step * 0.1 and left is None:
                left = x
            if from_b <= step * 0.1:
                right = x
                break
        if left is not None and right is not None:
            widths.append(right - left + 1)
    return widths


def source_metrics(picture, colors):
    grays = sorted({color for color in colors if is_gray(color)})
    saturated = [color for color in colors if not is_gray(color)]
    color_row = best_row(picture, saturated, wants_gray=False)
    gray_row = best_row(picture, grays, wants_gray=True)
    color_width = None
    gray_width = None
    if color_row and color_row[1]:
        color_width = sorted(run[1] for run in color_row[1])[len(color_row[1]) // 2]
    if gray_row and gray_row[1]:
        gray_width = sorted(run[1] for run in gray_row[1])[len(gray_row[1]) // 2]
    stripe = stripe_row(picture)
    line = stripe_metrics(picture, stripe[0], stripe[1]) if stripe else None
    return {
        "color_width": color_width or 256,
        "gray_width": gray_width or 128,
        "line_px": line["line_px"] if line else 1,
        "period_px": line["period_px"] if line else 0,
    }


def _full_runs(runs):
    """Runs that are not clipped by the screen edge (>= 3/4 of the longest)."""
    if not runs:
        return []
    longest = max(run[1] for run in runs)
    return [run for run in runs if run[1] >= longest * 0.75]


def analyze(capture_path, original_path):
    original = load(original_path)
    capture = load(capture_path)
    colors = dominant_colors(original)
    grays = sorted({color for color in colors if is_gray(color)})
    saturated = [color for color in colors if not is_gray(color)]
    source = source_metrics(original, colors)

    color_row = best_row(capture, saturated, wants_gray=False)
    gray_row = best_row(capture, grays, wants_gray=True)
    stripe = stripe_row(capture)

    color_stats = None
    if color_row and color_row[1]:
        runs = _full_runs(color_row[1])
        diffs = [max(abs(run[3][i] - run[2][i]) for i in range(3)) for run in runs]
        color_stats = {
            "row": color_row[0],
            "bands": len(color_row[1]),
            "full_bands": len(runs),
            "max_diff_levels": max(diffs) if diffs else None,
            "mean_diff_levels": round(sum(diffs) / len(diffs), 2) if diffs else None,
        }

    gray_stats = None
    if gray_row and gray_row[1]:
        runs = gray_row[1]
        levels = [run[2][0] for run in runs]
        diffs = [abs(run[3][0] - run[2][0]) for run in runs]
        gray_stats = {
            "row": gray_row[0],
            "levels": len(set(levels)),
            "max_diff_levels": max(diffs) if diffs else None,
            "mean_diff_levels": round(sum(diffs) / len(diffs), 2) if diffs else None,
            "monotonic": all(levels[i] <= levels[i + 1] for i in range(len(levels) - 1)),
        }

    line_stats = None
    if stripe:
        line_stats = stripe_metrics(capture, stripe[0], stripe[1])

    scale_candidates = []
    if color_row and color_row[1]:
        for run in _full_runs(color_row[1]):
            scale_candidates.append(run[1] / source["color_width"])
    if gray_row and gray_row[1]:
        for run in _full_runs(gray_row[1]):
            scale_candidates.append(run[1] / source["gray_width"])
    scale = round(sorted(scale_candidates)[len(scale_candidates) // 2], 3) if scale_candidates else None

    if line_stats and scale:
        line_stats["source_line_px"] = round(line_stats["line_px"] / scale, 2)

    edge_stats = None
    edge_source = color_row if color_row and color_row[1] else gray_row
    if edge_source and edge_source[1]:
        widths = transition_widths(capture, edge_source[0], edge_source[1])
        if widths:
            edge_stats = {
                "max_px": max(widths),
                "median_px": sorted(widths)[len(widths) // 2],
                "count": len(widths),
            }

    checks = {}
    if color_stats and color_stats["max_diff_levels"] is not None:
        checks["colors"] = color_stats["max_diff_levels"] <= COLOR_LIMIT
    else:
        checks["colors"] = None
    if gray_stats:
        checks["gray"] = (gray_stats["levels"] >= 1
                          and gray_stats["max_diff_levels"] <= COLOR_LIMIT
                          and gray_stats["monotonic"])
    else:
        checks["gray"] = None
    if line_stats:
        source_line = line_stats.get("source_line_px")
        checks["lines"] = (line_stats["transitions"] >= MIN_TRANSITIONS
                          and line_stats["line_px"] >= 1
                          and line_stats["contrast"] >= LINE_CONTRAST_LIMIT
                          and (source_line is None or 0.5 <= source_line <= 2.5))
    else:
        checks["lines"] = False
    if edge_stats:
        limit = max(2.0, (scale or 1.0) * 1.5)
        checks["edges"] = edge_stats["max_px"] <= limit
    else:
        checks["edges"] = None

    visible = any(value is not None for value in checks.values())
    definite = [value for value in checks.values() if value is not None]
    ok = visible and bool(definite) and all(definite)

    return {
        "capture": os.path.abspath(capture_path),
        "original": os.path.abspath(original_path),
        "scale": scale,
        "colors": color_stats,
        "gray": gray_stats,
        "lines": line_stats,
        "edges": edge_stats,
        "checks": checks,
        "ok": ok,
    }


def report(result):
    lines = ["display check: %s" % result["capture"],
             "  scale: %s" % ("unknown" if result["scale"] is None else "%.3f px/source px" % result["scale"])]
    colors = result["colors"]
    if colors:
        lines.append("  colours: %d bands (%d full), max diff %s levels, mean %s -> %s" % (
            colors["bands"], colors["full_bands"], colors["max_diff_levels"],
            colors["mean_diff_levels"], _verdict(result["checks"]["colors"])))
    else:
        lines.append("  colours: not visible")
    gray = result["gray"]
    if gray:
        lines.append("  grey: %d levels, max diff %s levels, monotonic %s -> %s" % (
            gray["levels"], gray["max_diff_levels"], gray["monotonic"],
            _verdict(result["checks"]["gray"])))
    else:
        lines.append("  grey: not visible")
    line = result["lines"]
    if line:
        lines.append("  lines: %d transitions, line %.1f px, gap %.1f px, contrast %.1f, source line %s px -> %s" % (
            line["transitions"], line["line_px"], line["gap_px"], line["contrast"],
            line.get("source_line_px", "-"), _verdict(result["checks"]["lines"])))
    else:
        lines.append("  lines: not visible -> check")
    edge = result["edges"]
    if edge:
        lines.append("  edges: max %d px, median %d px over %d boundaries -> %s" % (
            edge["max_px"], edge["median_px"], edge["count"], _verdict(result["checks"]["edges"])))
    else:
        lines.append("  edges: not measured")
    lines.append("  result: %s" % ("ok" if result["ok"] else "check"))
    return "\n".join(lines)


def _verdict(value):
    if value is None:
        return "-"
    return "ok" if value else "check"


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("run_dir", help="directory with display-100.bmp and display-400.bmp")
    parser.add_argument("--image", required=True, help="original test image (PNG)")
    parser.add_argument("--json", help="write the result here")
    args = parser.parse_args(argv)

    results = {}
    for zoom in ("100", "400"):
        path = os.path.join(args.run_dir, "display-%s.bmp" % zoom)
        if not os.path.exists(path):
            results[zoom] = {"capture": path, "ok": False, "error": "missing"}
            continue
        results[zoom] = analyze(path, args.image)
        print(report(results[zoom]))
        print()

    destination = args.json or os.path.join(args.run_dir, "display-check.json")
    with open(destination, "w") as handle:
        json.dump(results, handle, indent=2, sort_keys=True)
    print("summary: " + destination)
    return 0 if all(result.get("ok") for result in results.values()) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
