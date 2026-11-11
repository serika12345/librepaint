#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize per-thread CPU inside the injected input window of one run.

``thread_cpu.ps1`` writes ``threads.csv`` next to the latency log. Every
sample holds the process and thread CPU counters (the values behind the
Windows Process and Thread performance counters) on the QueryPerformanceCounter
clock, which is the clock that ``PenInject.cs`` and ``windows_capture.cpp``
use for ``inject.csv``.

This module turns the cumulative counters into the CPU time each thread used
inside the input window. It also names the GUI and paint threads when the
probe log has the ``main_thread`` and ``paintgl_thread`` markers. A run
without ``threads.csv`` returns None; older runs keep working.
"""

import bisect
import collections
import csv
import os


def read_marked_csv(path):
    """Return (frequency, rows) for a CSV with # frequency= headers."""
    with open(path, encoding="utf-8-sig", errors="replace") as handle:
        lines = handle.read().splitlines()
    frequency = None
    for line in lines:
        if line.startswith("# frequency="):
            frequency = int(line.split("=", 1)[1])
            break
    rows = list(csv.DictReader(line for line in lines if not line.startswith("#")))
    return frequency, rows


def read_samples(path):
    """Read threads.csv as samples sorted by the QPC timestamp."""
    frequency, rows = read_marked_csv(path)
    samples = []
    for row in rows:
        try:
            sample = {
                "qpc": int(row["qpc"]),
                "pid": int(row["pid"]),
                "thread_id": int(row["thread_id"]),
                "cpu_seconds": float(row["cpu_seconds"]),
                "thread_state": row.get("thread_state") or "",
                "wait_reason": row.get("wait_reason") or "",
                "priority": row.get("priority") or "",
            }
            process_cpu = row.get("process_cpu_seconds")
            if process_cpu not in (None, "", "unavailable"):
                sample["process_cpu_seconds"] = float(process_cpu)
        except (KeyError, TypeError, ValueError):
            continue
        samples.append(sample)
    samples.sort(key=lambda sample: (sample["thread_id"], sample["qpc"]))
    return frequency, samples


def inject_window(directory, frequency):
    """Return (start_qpc, end_qpc) of the injected input, or None.

    inject.csv holds either QPC ticks (PenInject.cs) or QPC-derived
    nanoseconds (windows_capture.cpp). The conversion uses the clock
    frequency of threads.csv so both files stay on one clock.
    """
    path = os.path.join(directory, "inject.csv")
    if not os.path.exists(path) or not frequency:
        return None
    _, rows = read_marked_csv(path)
    times = []
    for row in rows:
        if row.get("qpc"):
            times.append(int(row["qpc"]))
        elif row.get("time_ns"):
            times.append(int(row["time_ns"]) * frequency // 1_000_000_000)
    if len(times) < 2:
        return None
    return min(times), max(times)


def probe_roles(directory):
    """Return the thread IDs that the probe named for the GUI and paint work."""
    path = os.path.join(directory, "probe.csv")
    roles = {}
    if not os.path.exists(path):
        return roles
    _, rows = read_marked_csv(path)
    for row in rows:
        event = row.get("event")
        if event == "main_thread" and "main_thread" not in roles:
            roles["main_thread"] = int(float(row["x"]))
        elif event == "paintgl_thread" and "paintgl_thread" not in roles:
            roles["paintgl_thread"] = int(float(row["x"]))
    return roles


def value_at(series, moment):
    """Linearly interpolate a cumulative counter at a QPC time."""
    if not series:
        return None
    times = [entry[0] for entry in series]
    if moment <= times[0]:
        return series[0][1]
    if moment >= times[-1]:
        return series[-1][1]
    index = bisect.bisect_right(times, moment)
    left, right = series[index - 1], series[index]
    if right[0] == left[0]:
        return right[1]
    fraction = (moment - left[0]) / (right[0] - left[0])
    return left[1] + (right[1] - left[1]) * fraction


def summarize(directory):
    """Return per-thread CPU time inside the input window, or None."""
    path = os.path.join(directory, "threads.csv")
    if not os.path.exists(path):
        return None
    frequency, samples = read_samples(path)
    if not samples or not frequency:
        return None

    window = inject_window(directory, frequency)
    if window is None:
        window = (samples[0]["qpc"], max(sample["qpc"] for sample in samples))
    start, end = window
    duration = (end - start) / float(frequency)
    if duration <= 0:
        return None

    # Samples just outside the window are kept: value_at() needs the pair
    # that straddles each boundary for interpolation.
    filtered = collections.defaultdict(list)
    process_series = []
    seen_process = set()
    for sample in samples:
        filtered[sample["thread_id"]].append((sample["qpc"], sample["cpu_seconds"]))
        if "process_cpu_seconds" in sample and sample["qpc"] not in seen_process:
            seen_process.add(sample["qpc"])
            process_series.append((sample["qpc"], sample["process_cpu_seconds"]))

    entries = []
    total = 0.0
    for thread_id, series in filtered.items():
        series.sort()
        before = value_at(series, start)
        after = value_at(series, end)
        seconds = max(0.0, (after or 0.0) - (before or 0.0))
        total += seconds
        if seconds <= 0:
            continue
        states = collections.Counter(
            sample["thread_state"] for sample in samples
            if sample["thread_id"] == thread_id and start <= sample["qpc"] <= end)
        waits = collections.Counter(
            sample["wait_reason"] for sample in samples
            if sample["thread_id"] == thread_id and start <= sample["qpc"] <= end)
        entries.append({
            "id": thread_id,
            "seconds": seconds,
            "percent": 100.0 * seconds / duration,
            "states": {key: count for key, count in states.items() if key},
            "waits": {key: count for key, count in waits.items() if key},
        })
    entries.sort(key=lambda entry: entry["seconds"], reverse=True)

    process_seconds = None
    if process_series:
        process_series.sort()
        before = value_at(process_series, start)
        after = value_at(process_series, end)
        if before is not None and after is not None:
            process_seconds = max(0.0, after - before)

    result = {
        "window_s": duration,
        "samples": len(samples),
        "threads": len(filtered),
        "cpu_seconds": total,
        "percent": 100.0 * total / duration,
        "top": entries[:5],
        "wait_reasons": {
            key: sum(entry["waits"].get(key, 0) for entry in entries)
            for key in sorted({key for entry in entries for key in entry["waits"]})
        },
    }
    if process_seconds is not None:
        result["process_seconds"] = process_seconds
        result["unaccounted_seconds"] = max(0.0, process_seconds - total)

    roles = probe_roles(directory)
    role_result = {}
    by_id = {entry["id"]: entry for entry in entries}
    role_names = {"main_thread": "main_seconds", "paintgl_thread": "paint_seconds"}
    for name, seconds_name in role_names.items():
        thread_id = roles.get(name)
        role_result[name] = thread_id
        if thread_id is not None:
            role_result[seconds_name] = by_id.get(thread_id, {}).get("seconds", 0.0)
    if role_result:
        result["roles"] = role_result
    return result


def main(argv):
    import argparse
    import json
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("runs", nargs="+", help="run directories")
    parser.add_argument("--json", help="write the per-run summaries as JSON")
    args = parser.parse_args(argv)
    results = {}
    for directory in args.runs:
        name = os.path.basename(os.path.normpath(directory))
        results[name] = summarize(directory)
    for name, result in results.items():
        if not result:
            print("== %s  no threads.csv" % name)
            continue
        print("== %s  window=%.1fs cpu=%.2fs (%.1f%%) threads=%d" % (
            name, result["window_s"], result["cpu_seconds"],
            result["percent"], result["threads"]))
        for entry in result["top"]:
            print("   thread %d  %.2fs (%.1f%%)  states=%s waits=%s" % (
                entry["id"], entry["seconds"], entry["percent"],
                entry["states"], entry["waits"]))
        roles = result.get("roles") or {}
        if roles:
            print("   roles %s" % roles)
    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(results, handle, indent=2)
    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv[1:]))
