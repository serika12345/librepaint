#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write the replay input of the pressed strokes of a scenario.

The scenario's "trajectory" settings feed trajectory.samples(), and the
samples with the primary button pressed whose phase starts with "move" become
the strokes of KisStrokePredictionReplay. The still dwell and the release
around a stroke are left out. The coordinates are the injected ones, the event
time is the injection time in milliseconds, the arrival time is the injection
time in nanoseconds, and the pressure is 1.0.

Usage: prediction_replay_synthetic.py SCENARIO OUTPUT.csv [--width 2048]
                                                        [--height 1536]
"""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import trajectory  # noqa: E402


def convert(scenario_path, output_path, width=2048, height=1536):
    with open(scenario_path) as handle:
        settings = json.load(handle)["trajectory"]

    stroke = -1
    band = None

    with open(output_path, "w", encoding="ascii", newline="") as handle:
        handle.write("stroke,arrival_ns,event_ms,x,y,pressure\n")
        for time_s, phase, current_band, x, y, pressed in trajectory.samples(
                0.0, 0.0, width, height, settings):
            if not pressed or not phase.startswith("move"):
                continue
            if current_band != band:
                band = current_band
                stroke += 1
            handle.write(
                "%d,%d,%.6f,%.6f,%.6f,1.0\n"
                % (stroke, round(time_s * 1e9), time_s * 1000.0, x, y)
            )

    return stroke + 1


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Write the replay input of the pressed strokes of a scenario."
    )
    parser.add_argument("scenario", help="scenario JSON with a trajectory")
    parser.add_argument("output", help="CSV for KisStrokePredictionReplay")
    parser.add_argument("--width", type=int, default=2048)
    parser.add_argument("--height", type=int, default=1536)
    args = parser.parse_args(argv)

    convert(args.scenario, args.output, args.width, args.height)
    return 0


if __name__ == "__main__":
    sys.exit(main())
