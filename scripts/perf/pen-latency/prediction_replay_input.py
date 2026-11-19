#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Stroke input for the prediction replay, cut from one probe.csv.

The probe records a prediction_policy row with extra 0 when the prediction
starts a stroke, prediction_update rows for every pointer event the predictor
saw, and a prediction_clear row when the stroke ends. This script turns the
updates between those marks into the input of KisStrokePredictionReplay:

    stroke,arrival_ns,event_ms,x,y,pressure

arrival_ns is the probe time in nanoseconds and event_ms is the event time the
tool recorded on the same marker. Markers written before the event time was
recorded carry extra 0; for those the event time is taken from the arrival
time and one note is written to standard error.

Usage: prediction_replay_input.py PROBE.csv OUTPUT.csv
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import analyze  # noqa: E402


def event_time_note_written(state):
    if state["noted"]:
        return
    state["noted"] = True
    print(
        "note: prediction_update rows carry no event time; using the arrival time",
        file=sys.stderr,
    )


def stroke_updates(events):
    """Updates grouped by stroke, in probe order.

    A stroke starts at a prediction_policy row whose extra is 0 and ends at the
    next prediction_clear row. Updates after that clear belong to the next
    stroke and are cut the same way.
    """
    starts = [row for row in events.get("prediction_policy", []) if row[3] == 0]
    clears = sorted(row[0] for row in events.get("prediction_clear", []))
    updates = events.get("prediction_update", [])

    strokes = []
    for start_time, _x, _y, _extra in starts:
        end_time = next((time for time in clears if time >= start_time), None)
        if end_time is None:
            continue
        rows = [row for row in updates if start_time < row[0] <= end_time]
        if rows:
            strokes.append(rows)
    return strokes


def write_rows(handle, strokes, frequency):
    state = {"noted": False}
    for index, rows in enumerate(strokes):
        for time, x, y, extra in rows:
            arrival_ns = round(time * 1e9 / frequency)
            if extra:
                event_ms = float(extra)
            else:
                event_time_note_written(state)
                event_ms = arrival_ns / 1e6
            handle.write(
                "%d,%d,%.6f,%.6f,%.6f,1.0\n"
                % (index, arrival_ns, event_ms, x, y)
            )


def convert(probe_path, output_path):
    frequency, events = analyze.load_probe(probe_path)
    if not frequency:
        raise SystemExit("probe file has no frequency header: %s" % probe_path)

    strokes = stroke_updates(events)

    with open(output_path, "w", encoding="ascii", newline="") as handle:
        handle.write("stroke,arrival_ns,event_ms,x,y,pressure\n")
        write_rows(handle, strokes, frequency)
    return len(strokes)


def main(argv):
    if len(argv) != 3:
        print("usage: prediction_replay_input.py PROBE.csv OUTPUT.csv", file=sys.stderr)
        return 2
    convert(argv[1], argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
