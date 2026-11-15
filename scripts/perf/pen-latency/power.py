#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize battery power inside the injected input window of one run.

``power.ps1`` writes ``power.csv`` next to the latency log once a second.
Every sample holds the Win32_Battery values (charge percentage, battery
status, estimated runtime) and, when the device exposes them, the
root\\wmi:BatteryStatus values (remaining capacity in mWh, discharge rate in
mW, AC online). The timestamps are QueryPerformanceCounter ticks, the clock
that ``PenInject.cs`` and ``windows_capture.cpp`` use for ``inject.csv``.

This module reports the average discharge rate and the discharged energy
inside the input window. A run on AC power is marked so the runner can leave
it out of the power comparison. A run without ``power.csv`` returns None;
older runs keep working.
"""

import csv
import os


AC_BATTERY_STATUS = (2, 6, 7, 8, 9)


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


def number(value):
    """Return a float for a numeric cell, or None for a missing value."""
    if value is None:
        return None
    text = str(value).strip()
    if not text or text.lower() == "unavailable":
        return None
    try:
        return float(text)
    except (TypeError, ValueError):
        return None


def boolean(value):
    """Return True/False for a PowerShell boolean cell, or None."""
    text = str(value).strip().lower()
    if text in ("true", "1"):
        return True
    if text in ("false", "0"):
        return False
    return None


def read_samples(path):
    """Read power.csv as samples sorted by the QPC timestamp."""
    frequency, rows = read_marked_csv(path)
    samples = []
    for row in rows:
        try:
            sample = {
                "qpc": int(row["qpc"]),
                "charge_pct": number(row.get("charge_pct")),
                "battery_status": number(row.get("battery_status")),
                "estimated_run_time": number(row.get("estimated_run_time")),
                "remaining_mwh": number(row.get("remaining_mwh")),
                "discharge_mw": number(row.get("discharge_mw")),
                "power_online": boolean(row.get("power_online")),
                "charging": boolean(row.get("charging")),
                "discharging": boolean(row.get("discharging")),
            }
        except (KeyError, TypeError, ValueError):
            continue
        samples.append(sample)
    samples.sort(key=lambda sample: sample["qpc"])
    return frequency, samples


def inject_window(directory, frequency):
    """Return (start_qpc, end_qpc) of the injected input, or None.

    inject.csv holds either QPC ticks (PenInject.cs) or QPC-derived
    nanoseconds (windows_capture.cpp). The conversion uses the clock
    frequency of power.csv so both files stay on one clock.
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


def on_ac(samples):
    """Whether any sample saw AC power.

    root\\wmi's PowerOnline is the direct signal. Win32_Battery's
    BatteryStatus 2 means the system has access to AC, and 6-9 mean the
    battery is charging, so both also mean AC.
    """
    for sample in samples:
        if sample["power_online"] is True or sample["charging"] is True:
            return True
        status = sample["battery_status"]
        if status is not None and int(status) in AC_BATTERY_STATUS:
            return True
    return False


def summarize(directory):
    """Return average discharge rate and discharged energy, or None."""
    path = os.path.join(directory, "power.csv")
    if not os.path.exists(path):
        return None
    frequency, samples = read_samples(path)
    if not samples or not frequency:
        return None

    window = inject_window(directory, frequency)
    if window is None:
        window = (samples[0]["qpc"], samples[-1]["qpc"])
    start, end = window
    duration = (end - start) / float(frequency)
    if duration <= 0:
        return None

    inside = [sample for sample in samples if start <= sample["qpc"] <= end]
    result = {
        "window_s": duration,
        "samples": len(inside),
        "all_samples": len(samples),
        "ac": on_ac(samples),
    }

    rates = [sample["discharge_mw"] for sample in inside
             if sample["discharge_mw"] is not None]
    if rates:
        result["rate_mw"] = sum(rates) / len(rates)
        result["energy_mwh"] = result["rate_mw"] * duration / 3600.0

    remaining = [sample["remaining_mwh"] for sample in inside
                 if sample["remaining_mwh"] is not None]
    if len(remaining) >= 2:
        result["remaining_mwh_delta"] = remaining[0] - remaining[-1]

    charges = [sample["charge_pct"] for sample in inside
               if sample["charge_pct"] is not None]
    if len(charges) >= 2:
        result["charge_pct_delta"] = charges[0] - charges[-1]

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
            print("== %s  no power.csv" % name)
            continue
        print("== %s  window=%.1fs ac=%s samples=%d rate=%s mW energy=%s mWh" % (
            name, result["window_s"], "yes" if result["ac"] else "no", result["samples"],
            "-" if result.get("rate_mw") is None else "%.1f" % result["rate_mw"],
            "-" if result.get("energy_mwh") is None else "%.2f" % result["energy_mwh"]))
    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(results, handle, indent=2)
    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv[1:]))
