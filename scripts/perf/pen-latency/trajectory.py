# SPDX-License-Identifier: GPL-3.0-or-later
"""Pointer trajectory shared by every capture-based latency measurement.

The trajectory runs inside a target rectangle given in the injector's
coordinate space:

- hover: back and forth along the hover row (top + 0.15 * height) with no
  button pressed; the brush outline follows the pointer.
- strokes: one row per stroke, from top + 0.35 * height to top + 0.85 *
  height, always left to right with the primary button held; the ink's
  leading edge follows the pointer.

A stroke can change speed inside the line through "speed_profiles": a list of
profiles, one per stroke in turn. Each profile is a list of segments with an
optional "until" (fraction of the stroke distance, default 1.0 for the last
segment), "speed" (fraction of the rectangle width per second) and "label".
The moving samples then carry the label as their phase, so capture_analyze.py
can group the delay before and after a speed change. Constant-speed strokes
keep the phase "move".

Every pass starts and ends with a still dwell. capture_analyze.py uses the
dwells to map injector coordinates to captured pixels and the moving parts
to measure latency. Rows are separated so that each one gets its own capture
band (see bands()).

"stop_decel_ms" makes the pressed strokes end at rest: the last part of the
line is drawn while the speed falls from the stroke's speed to zero. The
hover passes keep their constant speed.
"""

import math

DEFAULTS = {
    "hover_passes": 8,
    "strokes": 6,
    "speed": 0.5,  # fraction of the rectangle width per second
    "speed_profiles": [],  # per-stroke lists of {until, speed, label}
    "stop_decel_ms": 0,  # decelerate the pressed strokes to a stop over this time
    "dwell": 0.3,  # seconds of still pointer at both ends of every pass
    "rate": 240.0,  # injected samples per second
    "margin": 0.1,  # fraction of the width left unused at each end
}


def profile_segments(profile):
    """Normalise one speed profile into contiguous distance fractions."""
    segments = []
    previous = 0.0
    for index, entry in enumerate(profile):
        until = float(entry.get("until", 1.0))
        speed = float(entry["speed"])
        if until <= previous or until > 1.0:
            raise ValueError("speed profile 'until' must increase and end at 1.0")
        if speed <= 0.0:
            raise ValueError("speed profile 'speed' must be positive")
        label = entry.get("label") or "move%d" % (index + 1)
        if not label.startswith("move"):
            label = "move_" + label
        segments.append({
            "start": previous,
            "end": until,
            "speed": speed,
            "label": label,
        })
        previous = until
    if segments and previous < 1.0:
        raise ValueError("the last speed profile segment must end at 1.0")
    return segments


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
    profiles = [profile_segments(profile) for profile in (o.get("speed_profiles") or [])]
    t = 0.0

    def run(band, y, start, end, pressed, profile=None):
        nonlocal t
        stop_ms = float(o.get("stop_decel_ms") or 0.0)
        decel = pressed and stop_ms > 0.0
        direction = 1.0 if end > start else -1.0
        distance = abs(end - start)
        if decel:
            # The smooth fall of the speed from v0 to zero covers
            # v0 * T / 2000 pixels; the constant part of the line ends where it
            # begins. The speed follows the smoothstep shape, and the position
            # uses u - u ** 3 + u ** 4 / 2 over the normalised time u.
            v0 = (profile[-1]["speed"] if profile else o["speed"]) * width
            decel_px = v0 * stop_ms / 2000.0
            if decel_px >= distance:
                raise ValueError("stop_decel_ms must leave a constant-speed part")
            if profile:
                last = profile[-1]
                if decel_px >= distance * (last["end"] - last["start"]):
                    raise ValueError("stop_decel_ms must leave a constant-speed part "
                                     "of the last speed profile segment")
            x_decel = end - direction * decel_px
        for _ in range(dwell):
            yield t, "dwell", band, start, y, pressed
            t += step
        if profile:
            span = end - start
            segments_used = profile
            if decel:
                segments_used = [dict(segment) for segment in profile]
                segments_used[-1]["end"] = 1.0 - decel_px / distance
            for segment in segments_used:
                segment_seconds = abs(span) * (segment["end"] - segment["start"]) / (segment["speed"] * width)
                steps = max(1, int(round(segment_seconds / step)))
                for i in range(1, steps + 1):
                    fraction = segment["start"] + (segment["end"] - segment["start"]) * i / steps
                    yield t, segment["label"], band, start + span * fraction, y, pressed
                    t += step
        else:
            if decel:
                steps = int(abs(x_decel - start) / (o["speed"] * width) / step)
                moved = x_decel
            else:
                steps = int(duration / step)
                moved = end
            for i in range(steps + 1):
                yield t, "move", band, start + (moved - start) * i / steps, y, pressed
                t += step
        if decel:
            # The smooth fall reaches rest at the end: x(u) uses
            # u - u ** 3 + u ** 4 / 2, whose slope is 1 at u = 0 and 0 at
            # u = 1.
            seconds = stop_ms / 1000.0
            count = int(math.ceil(seconds / step))
            for i in range(1, count + 1):
                u = min(i * step / seconds, 1.0)
                position = x_decel + direction * v0 * seconds * (u - u ** 3 + u ** 4 / 2.0)
                if i == count:
                    position = end
                yield t, "move_decel", band, position, y, pressed
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
        profile = profiles[k % len(profiles)] if profiles else None
        yield from run(band, y, x0, x1, True, profile)
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
