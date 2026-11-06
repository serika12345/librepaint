# SPDX-License-Identifier: GPL-3.0-or-later
"""Pointer trajectory shared by every capture-based latency measurement.

The trajectory runs inside a target rectangle given in the injector's
coordinate space:

- hover: back and forth along the hover row (top + 0.15 * height) with no
  button pressed; the brush outline follows the pointer.
- strokes: one row per stroke, from top + 0.35 * height to top + 0.85 *
  height, always left to right with the primary button held; the ink's
  leading edge follows the pointer.

Every pass starts and ends with a still dwell. capture_analyze.py uses the
dwells to map injector coordinates to captured pixels and the moving parts
to measure latency. Rows are separated so that each one gets its own capture
band (see bands()).
"""

DEFAULTS = {
    "hover_passes": 8,
    "strokes": 6,
    "speed": 0.5,  # fraction of the rectangle width per second
    "dwell": 0.3,  # seconds of still pointer at both ends of every pass
    "rate": 240.0,  # injected samples per second
    "margin": 0.1,  # fraction of the width left unused at each end
}


def options(overrides=None):
    o = dict(DEFAULTS)
    o.update(overrides or {})
    return o


def rows(top, height, o):
    hover = top + 0.15 * height
    n = int(o["strokes"])
    strokes = [top + (0.35 + 0.5 * (k / (n - 1) if n > 1 else 0.5)) * height for k in range(n)]
    return hover, strokes


def bands(top, height, o):
    """Return [(name, y_center, half_height)] in the injector's coordinate space."""
    hover, strokes = rows(top, height, o)
    n = int(o["strokes"])
    spacing = 0.5 * height / (n - 1) if n > 1 else 0.3 * height
    half = 0.45 * min(spacing, 0.2 * height)
    return [("hover", hover, half)] + [("stroke%d" % k, y, half) for k, y in enumerate(strokes)]


def samples(left, top, width, height, overrides=None):
    """Yield (time_s, phase, band, x, y, pressed) relative to the start.

    phase is "dwell", "move" or "release"; band names the row (see bands()).
    """
    o = options(overrides)
    x0 = left + o["margin"] * width
    x1 = left + (1 - o["margin"]) * width
    duration = (x1 - x0) / (o["speed"] * width)
    step = 1.0 / o["rate"]
    dwell = int(o["dwell"] / step)
    hover_y, stroke_ys = rows(top, height, o)
    t = 0.0

    def run(band, y, start, end, pressed):
        nonlocal t
        for _ in range(dwell):
            yield t, "dwell", band, start, y, pressed
            t += step
        steps = int(duration / step)
        for i in range(steps + 1):
            yield t, "move", band, start + (end - start) * i / steps, y, pressed
            t += step
        for _ in range(dwell):
            yield t, "dwell", band, end, y, pressed
            t += step

    for index in range(int(o["hover_passes"])):
        start, end = (x0, x1) if index % 2 == 0 else (x1, x0)
        yield from run("hover", hover_y, start, end, False)
    for k, y in enumerate(stroke_ys):
        band = "stroke%d" % k
        # Approach the row start without pressing, then draw left to right.
        for _ in range(dwell):
            yield t, "dwell", band, x0, y, False
            t += step
        yield from run(band, y, x0, x1, True)
        yield t, "release", band, x1, y, False
        t += step


def write_plan(path, overrides=None):
    """Write plan.csv for native injectors, relative to the canvas rectangle.

    All positions are fractions of the canvas width (x) and height (y), so
    that the injector applies the plan to the canvas it detects at run time.
    Line 1: "# bands=" followed by name:x0:x1:y0:y1 entries separated by ";".
    Then the columns time_s,phase,band,x,y,pressed.
    """
    o = options(overrides)
    band_text = ";".join(
        "%s:%.6f:%.6f:%.6f:%.6f" % (name, 0.0, 1.0, y - half, y + half) for name, y, half in bands(0.0, 1.0, o)
    )
    with open(path, "w") as handle:
        handle.write("# bands=%s\n" % band_text)
        handle.write("time_s,phase,band,x,y,pressed\n")
        for t, phase, band, x, y, pressed in samples(0.0, 0.0, 1.0, 1.0, o):
            handle.write("%.6f,%s,%s,%.6f,%.6f,%d\n" % (t, phase, band, x, y, int(pressed)))


if __name__ == "__main__":
    import argparse
    import json

    parser = argparse.ArgumentParser(description="Write the injection plan relative to the canvas rectangle.")
    parser.add_argument("--options", default="{}", help="trajectory option overrides as JSON")
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    write_plan(args.out, json.loads(args.options))
