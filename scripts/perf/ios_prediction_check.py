#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Judge the system predicted touches recorded during an iPad drawing session.

Stage E of docs/perf/l2p-predictor-design.md requires two results from a
drawing on the device:
- Predicted points are captured, drawn, and cleared.
- The keys of the real samples and the OS entries agree at 99% or more.

The latency record of the app supplies the evidence. This tool counts the
prediction events and reports the counts, then checks the conditions below.
Each violated condition is written to stderr as one `ANOMALY` line and the
exit status is 1:
- No `prediction_source` row has `extra == 3`, so no iOS stroke was recorded.
- Fewer than 200 distinct `prediction_update` keys, so too little was drawn.
- The match rate is below 0.99, that is 1 - miss_missing / update_keys.
- No `prediction_os_entry` row carries a point, meaning `extra >= 1`.
- No `prediction_point` row was recorded.
- No `prediction_clear` row was recorded.

The record skips the `#` metadata lines and reads the columns
`time,event,x,y,extra,thread`.

Usage:
  ios_prediction_check.py LATENCY.csv
"""

from __future__ import annotations

import argparse
import collections
import sys
from pathlib import Path

HEADER = "time,event,x,y,extra,thread"


def read_events(path: Path) -> list[tuple[str, int]]:
    """Return (event, extra) for each data row of the latency record."""
    events: list[tuple[str, int]] = []
    with path.open(newline="", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith("#"):
                continue
            if line.startswith(HEADER):
                continue
            fields = line.rstrip("\r\n").split(",")
            if len(fields) < 5:
                continue
            try:
                extra = int(fields[4])
            except ValueError:
                continue
            events.append((fields[1], extra))
    return events


def count_events(events: list[tuple[str, int]]) -> dict[str, int]:
    counters = collections.Counter()
    update_keys: set[int] = set()
    clear_reasons: collections.Counter[int] = collections.Counter()

    for event, extra in events:
        if event == "prediction_source":
            counters["strokes"] += 1
            if extra == 3:
                counters["ios_strokes"] += 1
        elif event == "prediction_os_entry":
            counters["os_entries"] += 1
            if extra >= 1:
                counters["os_entries_with_points"] += 1
        elif event == "prediction_update":
            update_keys.add(extra)
        elif event == "prediction_miss":
            if extra == 1:
                counters["miss_missing"] += 1
            elif extra == 2:
                counters["miss_incomplete"] += 1
        elif event == "prediction_point":
            counters["points"] += 1
        elif event == "prediction_clear":
            counters["clear_rows"] += 1
            clear_reasons[extra] += 1

    counters["update_keys"] = len(update_keys)
    for reason in sorted(clear_reasons):
        counters[f"clear_{reason}"] = clear_reasons[reason]
    return dict(counters)


def match_rate(miss_missing: int, update_keys: int) -> float:
    if update_keys == 0:
        return 0.0
    return 1.0 - miss_missing / update_keys


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check the system prediction events of an iPad latency record."
    )
    parser.add_argument("record", help="latency CSV collected from the device")
    arguments = parser.parse_args()

    counters = count_events(read_events(Path(arguments.record)))
    rate = match_rate(counters.get("miss_missing", 0), counters.get("update_keys", 0))

    order = [
        "strokes",
        "ios_strokes",
        "os_entries",
        "os_entries_with_points",
        "update_keys",
        "miss_missing",
        "miss_incomplete",
        "match_rate",
        "points",
    ]
    for name in order:
        value = rate if name == "match_rate" else counters.get(name, 0)
        print(f"{name}={value:.4f}" if name == "match_rate" else f"{name}={value}")
    for name in sorted(n for n in counters if n.startswith("clear_")):
        print(f"{name}={counters[name]}")

    anomalies: list[str] = []
    if counters.get("ios_strokes", 0) == 0:
        anomalies.append(
            "ios_strokes: no prediction_source row carries extra == 3"
        )
    if counters.get("update_keys", 0) < 200:
        anomalies.append(
            f"update_keys: {counters.get('update_keys', 0)} keys, fewer than 200"
        )
    if rate < 0.99:
        anomalies.append(f"match_rate: {rate:.4f}, below 0.9900")
    if counters.get("os_entries_with_points", 0) == 0:
        anomalies.append(
            "os_entries_with_points: no prediction_os_entry row carries a point"
        )
    if counters.get("points", 0) == 0:
        anomalies.append("points: no prediction_point row")
    if counters.get("clear_rows", 0) == 0:
        anomalies.append("prediction_clear: no clear row")

    for anomaly in anomalies:
        print(f"ANOMALY {anomaly}", file=sys.stderr)
    return 1 if anomalies else 0


if __name__ == "__main__":
    raise SystemExit(main())
