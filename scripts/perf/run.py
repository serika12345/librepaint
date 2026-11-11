#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run one latency scenario on several hosts and judge it against a baseline.

Inputs:
- a scenario file (scripts/perf/scenarios/*.json): trajectory, application
  settings, number of runs, spread limit, and the per-metric regression
  threshold in milliseconds;
- a local hosts file (not tracked; see scripts/perf/hosts.example.json): how
  to reach every host and how to start every target there.

For each round the targets of each host run once, in the listed order, so
that targets alternate. The per-system runners under scripts/perf/pen-latency
do the work (run-remote.sh for Windows, run-linux.sh, run-macos.sh) and stop
the applications they start. Afterwards the script analyzes every run with
capture_analyze.py, applies the host's capture clock correction, writes a
summary JSON, compares it with an optional baseline summary, and prints a
Markdown table with the verdict.

Hover latency alternates between two states from one launch to the next
(L1 of Issue #108). The summary carries each run's state and every run's
PresentMon presence. Hover metrics are judged within a state; ink is judged
over all runs. A Windows target can set "presentmon": "off" to leave
PresentMon out for a comparison, "env" to pass environment variables to the
measured application (run-surface.ps1 records them in meta.txt), and
"kritarc" to set root kritarc keys for one run (the file is restored after
the run). A Windows target records the device state (surface_state.ps1) only
when it sets "surface_state": "on" (one WMI sample per second) or "light"
(one Get-Counter call per five seconds, no per-second process list); the
default is off, so the recording tool does not perturb the measurement.
Before every run the runner waits, unless the target sets
"surface_state_wait": "off", for the device's % Processor Performance to reach
70 % twice in a row (one Get-Counter read per five seconds, at most 120 s);
a run whose reading stayed below 60 % during the measurement is marked and
left out of the aggregation. A Windows target that sets "thread_cpu": "on"
has thread_cpu.ps1 sample the process and thread CPU counters; the table then
shows the CPU time inside the injected input window. A target that sets
"power": "on" has power.ps1 sample the battery once a second; the table then
shows the average discharge rate and the discharged energy inside the input
window, and runs that saw AC power are left out of that comparison. A run
whose PresentMon CSV covers too little of the injected trajectory is
reported as partial and left out of PresentMon-based metrics. A run whose
application was not the foreground window when the capture began and that
presented far fewer frames than the display refresh (Windows throttles a
covered window) measured another window; it is left out of every metric and
counted in the table.

The group writes one line per event to the event log (build/perf/events.log,
or --events): the start of the group and of every run, each result or failure,
the early stop of a target that failed twice in a row for the same reason, and
the exit code at the end (format: scripts/perf/events_log.py).

Usage:
  run.py --scenario scripts/perf/scenarios/pen-light.json --hosts build/perf/hosts.json \
         --label q1 --out build/perf/runs [--only windows,linux] [--baseline OLD.json] \
         [--summary docs/perf/results/<file>.json]
  run.py --analyze-only RUN_ROOT ...   (re-analyze collected runs)
"""

import argparse
import csv
import datetime
import json
import os
import re
import shlex
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "pen-latency"))
import capture_analyze  # noqa: E402
import events_log  # noqa: E402
import power  # noqa: E402
import thread_cpu  # noqa: E402

DEFAULT_EVENTS = os.path.join(ROOT, "build", "perf", "events.log")


def kritarc_text(scenario):
    """Render the scenario's kritarc for the Linux and macOS runners.

    LibrePaint reads KisConfig and KisImageConfig from the empty group, which
    KConfig keeps before the first "[Group]" line; Krita does not read
    "General". The scenario names its runtime settings "General", so those
    keys are written as root keys and any other section follows as a group.
    """
    sections = scenario.get("kritarc", {})
    general = sections.get("General", {})
    lines = ["%s=%s" % (key, value) for key, value in general.items()]
    for section, values in sections.items():
        if section == "General":
            continue
        lines.append("[%s]" % section)
        lines.extend("%s=%s" % (key, value) for key, value in values.items())
    return "\n".join(lines) + "\n"


def run_command(argv, env=None, cwd=None):
    print("$ " + " ".join(shlex.quote(a) for a in argv), flush=True)
    merged = dict(os.environ)
    merged.update(env or {})
    return subprocess.run(argv, env=merged, cwd=cwd).returncode


FAILED_LINE = re.compile(r"^\s*failed:\s*(.*)$")


def status_files(run_root, host, target, since=None):
    """Return the possible status.txt files of one target, newest run first.

    run-surface.ps1 writes a status.txt into every run directory and
    run-remote.sh copies the shared status.txt as status-<target>.txt
    (run-linux.sh and run-macos.sh write neither). The newest run directory is
    listed first because its status.txt belongs to the run that just failed.
    A file written before the run started belongs to an earlier run and is
    left out, so a runner that died before writing anything is not blamed for
    the previous failure.
    """
    out = os.path.join(run_root, host["name"])

    def fresh(path):
        return since is None or os.path.getmtime(path) >= since - 5

    newest, newest_time = None, None
    if os.path.isdir(out):
        for name in os.listdir(out):
            path = os.path.join(out, name)
            status = os.path.join(path, "status.txt")
            if not os.path.isdir(path) or not os.path.exists(status) or not fresh(status):
                continue
            stamp = os.path.getmtime(status)
            if newest_time is None or stamp > newest_time:
                newest, newest_time = status, stamp
    paths = [newest] if newest else []
    for name in (target.get("runner_target"), target["name"]):
        if name:
            path = os.path.join(out, "status-%s.txt" % name)
            if os.path.exists(path) and fresh(path):
                paths.append(path)
    return paths


def status_last_line(run_root, host, target, since=None):
    """Return the last line of the target's status.txt, or None."""
    for path in status_files(run_root, host, target, since):
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8", errors="replace") as handle:
            lines = [line.strip() for line in handle if line.strip()]
        if lines:
            return lines[-1]
    return None


def failed_reason(line):
    """Return the text after "failed: " of a status line, or None.

    Two failed runs are abandoned only when this text is equal: the text names
    the failure, so a run that failed for another reason does not count.
    """
    if not line:
        return None
    match = FAILED_LINE.match(line)
    return match.group(1) if match else None


def run_detail(run_root, host, target, started):
    """Return the state and medians of the run that just finished.

    The runner names each run directory <target>-<date>-<time>-<n>; only the
    directory made after this run started belongs to it. A run without a
    capture (the application did not start) has no medians.
    """
    out = os.path.join(run_root, host["name"])
    newest, newest_time = None, None
    if os.path.isdir(out):
        for name in os.listdir(out):
            path = os.path.join(out, name)
            if not os.path.isdir(path) or not name.startswith(target["name"] + "-"):
                continue
            stamp = os.path.getmtime(path)
            if stamp < started - 5:
                continue
            if newest_time is None or stamp > newest_time:
                newest, newest_time = path, stamp
    if newest is None or not os.path.exists(os.path.join(newest, "capture.csv")):
        return "state=unknown hover=- ink=-"
    result = capture_analyze.analyze_run(newest)
    offset = float(host.get("capture_offset_ms", 0.0))

    def median(metric):
        value = (result.get(metric) or {}).get("p50")
        return "-" if value is None else "%.1f" % (value - offset)

    state = (result.get("state") or {}).get("label", "unknown")
    return "state=%s hover=%s ink=%s" % (
        state, median("hover.input_to_capture"), median("ink.input_to_capture"))


def measure_runs(hosts, scenario, run_root, label, events=None):
    """Run every target once per round and stop a target that keeps failing.

    A target whose run fails twice in a row for the same reason (the text
    after "failed: " of status.txt) skips its remaining rounds; the other
    targets continue. Every start, result and early stop is written to the
    event log when one is given.

    Returns the names of the failed runs.
    """
    failures = []
    abandoned = set()
    previous_reason = {}
    if events:
        events_log.write(events, "run-start", label, "-", "-",
                         "scenario %s, %d rounds, %d host(s)" % (
                             scenario["name"], scenario["runs"], len(hosts)))
    for round_index in range(scenario["runs"]):
        if all((host["name"], target["name"]) in abandoned
               for host in hosts for target in host["targets"]):
            break
        for host in hosts:
            for target in host["targets"]:
                key = (host["name"], target["name"])
                if key in abandoned:
                    continue
                number = round_index + 1
                if events:
                    events_log.write(events, "target-start", label, target["name"], number,
                                     "host %s" % host["name"])
                started = time.time()
                argv, env, collect = runner_for(host, target, scenario, run_root)
                status = run_command(argv, env)
                if collect and status == 0:
                    status = run_command(collect)
                if status == 0:
                    previous_reason.pop(key, None)
                    if events:
                        events_log.write(events, "target-done", label, target["name"], number,
                                         run_detail(run_root, host, target, started))
                    continue
                failures.append("%s/%s round %d" % (host["name"], target["name"], number))
                line = status_last_line(run_root, host, target, started)
                reason = failed_reason(line)
                if events:
                    events_log.write(events, "target-failed", label, target["name"], number,
                                     line or "no status.txt")
                if reason is not None and previous_reason.get(key) == reason:
                    abandoned.add(key)
                    if events:
                        events_log.write(events, "target-abandoned", label, target["name"], number,
                                         "same failure twice: %s" % reason)
                else:
                    if reason is None:
                        previous_reason.pop(key, None)
                    else:
                        previous_reason[key] = reason
    return failures


ENV_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
KRITARC_KEY = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]*$")
SURFACE_STATE_MODES = ("off", "on", "light")
SURFACE_STATE_WAIT_MODES = ("on", "off")
THREAD_CPU_MODES = ("off", "on")
POWER_MODES = ("off", "on")


def target_environment(target):
    """Return the target's "env" as strings after checking names and values."""
    env = target.get("env", {})
    if not isinstance(env, dict):
        raise SystemExit("target %s: env must be an object" % target["name"])
    checked = {}
    for name, value in env.items():
        if not ENV_NAME.match(name):
            raise SystemExit("target %s: invalid environment variable name: %s" % (target["name"], name))
        text = str(value)
        if "\x00" in text or "\n" in text or "\r" in text:
            raise SystemExit("target %s: invalid value for %s" % (target["name"], name))
        checked[name] = text
    return checked


def target_kritarc(target):
    """Return the target's root kritarc keys after checking names and values.

    The keys are written to the empty group of kritarc, where LibrePaint reads
    the hidden useMaximumFrameLatencyOne switch. run-surface.ps1 backs the file
    up before the run and restores it afterwards.
    """
    kritarc = target.get("kritarc", {})
    if not isinstance(kritarc, dict):
        raise SystemExit("target %s: kritarc must be an object" % target["name"])
    checked = {}
    for name, value in kritarc.items():
        if not KRITARC_KEY.match(name):
            raise SystemExit("target %s: invalid kritarc key: %s" % (target["name"], name))
        text = str(value)
        if any(ch in text for ch in "\x00\r\n\\[]="):
            raise SystemExit("target %s: invalid value for kritarc key %s" % (target["name"], name))
        checked[name] = text
    return checked


def target_surface_state(target):
    """Return how the Windows runner records the device state for this target.

    "off" (the default) records nothing: the per-second WMI samples were
    suspected of changing the device's power state during a measurement
    (2026-11-11 05:40). "on" keeps the per-second samples of temperature,
    processor performance, total CPU and the busiest processes. "light" reads
    only the CPU counters, with one Get-Counter call per five seconds, and
    samples the temperature and the busiest processes before and after the
    run.
    """
    mode = target.get("surface_state", "off")
    if not isinstance(mode, str) or mode not in SURFACE_STATE_MODES:
        raise SystemExit("target %s: surface_state must be one of %s" % (
            target["name"], ", ".join(SURFACE_STATE_MODES)))
    return mode


def target_surface_state_wait(target):
    """Return whether the Windows runner waits for the device to recover.

    The wait is on by default: each run starts only after % Processor
    Performance reached 70 % twice in a row (one Get-Counter read every five
    seconds, at most 120 s). The 2026-11-11 measurements showed the device at
    42-50 % for minutes, and runs taken there were not comparable
    (progress.md, q4-cpu). A target sets "surface_state_wait": "off" to
    measure the same runs without the wait.
    """
    mode = target.get("surface_state_wait", "on")
    if not isinstance(mode, str) or mode not in SURFACE_STATE_WAIT_MODES:
        raise SystemExit("target %s: surface_state_wait must be one of %s" % (
            target["name"], ", ".join(SURFACE_STATE_WAIT_MODES)))
    return mode


def target_thread_cpu(target):
    """Return whether the Windows runner samples per-thread CPU.

    The sampler is off by default. A CPU comparison turns it on for every
    target of the comparison so the sampling cost is the same on both sides.
    """
    mode = target.get("thread_cpu", "off")
    if not isinstance(mode, str) or mode not in THREAD_CPU_MODES:
        raise SystemExit("target %s: thread_cpu must be one of %s" % (
            target["name"], ", ".join(THREAD_CPU_MODES)))
    return mode


def target_power(target):
    """Return whether the Windows runner samples battery power.

    The sampler is off by default. A battery comparison turns it on for every
    target of the comparison so the sampling cost is the same on both sides;
    runs that see AC power are left out of the power aggregation.
    """
    mode = target.get("power", "off")
    if not isinstance(mode, str) or mode not in POWER_MODES:
        raise SystemExit("target %s: power must be one of %s" % (
            target["name"], ", ".join(POWER_MODES)))
    return mode


def runner_for(host, target, scenario, run_root):
    """Return (argv, env, collect) to run one measurement of target on host."""
    trajectory = json.dumps(scenario["trajectory"])
    out = os.path.join(run_root, host["name"])
    os.makedirs(out, exist_ok=True)
    kind = host["kind"]
    if kind == "windows":
        argv = [os.path.join(HERE, "pen-latency", "run-remote.sh"), host["ssh"], target["runner_target"], "1",
                target.get("app", "none"), out]
        env = {"CAPTURE_TOOL": host["capture_tool"], "TRAJECTORY_JSON": trajectory, "RUN_NAME": target["name"],
               "PRESENTMON": target.get("presentmon", "on"),
               "SURFACE_STATE": target_surface_state(target),
               "SURFACE_STATE_WAIT": target_surface_state_wait(target),
               "THREAD_CPU": target_thread_cpu(target),
               "POWER": target_power(target),
               "TARGET_ENV": json.dumps(target_environment(target), sort_keys=True),
               "TARGET_KRITARC": json.dumps(target_kritarc(target), sort_keys=True)}
        return argv, env, None
    if kind in ("linux", "macos") and target.get("env"):
        raise SystemExit("target %s: env is supported only for Windows targets" % target["name"])
    if kind in ("linux", "macos") and target.get("kritarc"):
        raise SystemExit("target %s: kritarc is supported only for Windows targets" % target["name"])
    if kind in ("linux", "macos") and target.get("surface_state"):
        raise SystemExit("target %s: surface_state is supported only for Windows targets" % target["name"])
    if kind in ("linux", "macos") and target.get("surface_state_wait") is not None:
        raise SystemExit("target %s: surface_state_wait is supported only for Windows targets" % target["name"])
    if kind in ("linux", "macos") and target.get("thread_cpu") is not None:
        raise SystemExit("target %s: thread_cpu is supported only for Windows targets" % target["name"])
    if kind in ("linux", "macos") and target.get("power") is not None:
        raise SystemExit("target %s: power is supported only for Windows targets" % target["name"])
    if kind == "linux":
        command = " ".join(shlex.quote(a) for a in target["command"])
        remote_out = host["remote_out"] + "/" + scenario["name"]
        script = (
            "cd %s && TRAJECTORY_JSON=%s KRITARC=%s scripts/perf/pen-latency/run-linux.sh %s 1 %s %s -- %s"
            % (shlex.quote(host["repo"]), shlex.quote(trajectory), shlex.quote(kritarc_text(scenario)),
               shlex.quote(target["name"]), shlex.quote(host["work"] + "/" + target["name"]),
               shlex.quote(remote_out), command)
        )
        collect = ["rsync", "-a", "--remove-source-files", "%s:%s/" % (host["ssh"], remote_out), out + "/"]
        return ["ssh", host["ssh"], script], {}, collect
    if kind == "macos":
        argv = [os.path.join(HERE, "pen-latency", "run-macos.sh"), target["name"], "1",
                os.path.join(host["work"], target["name"]), out, "--"] + target["command"]
        env = {"MACOS_CAPTURE": host["capture_tool"], "TRAJECTORY_JSON": trajectory,
               "KRITARC": kritarc_text(scenario)}
        return argv, env, None
    raise SystemExit("unknown host kind: %s" % kind)


def target_of(run_name, targets):
    """Runners name each run directory <target name>-<date>-<time>-<n>."""
    for target in sorted(targets, key=lambda t: -len(t["name"])):
        if run_name.startswith(target["name"] + "-"):
            return target["name"]
    return None


def shifted(stat, offset):
    if not stat.get("n"):
        return stat
    return {key: (value - offset if key in ("p05", "p50", "p95") else value) for key, value in stat.items()}


def presentmon_status(run):
    """Return "present", "partial" or "none" for one analyzed run."""
    state = run.get("presentmon") or {}
    status = state.get("status")
    if status is None:
        return "present" if state.get("present") else "none"
    return status


def run_excluded(run):
    """Whether a run measured another window and is left out of the aggregation."""
    return bool(run.get("excluded"))


# A device that falls to this % Processor Performance during a run is far from
# its nominal clock; the 2026-11-11 sessions showed runs at 42-50 % whose
# latency did not match the runs around them (progress.md, q4-cpu).
DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT = 60.0


def device_performance_low(run):
    """Whether the device's recorded % Processor Performance stayed below 60.

    A reading of 0 is a failed read rather than a measurement (see
    during_performance_readings), so it does not make the run low. Runs
    without a recorded device state (Linux, macOS, surface_state off)
    return False and stay in the aggregation.
    """
    state = run.get("surface_state") or {}
    value = number_or_none(state.get("during_min_processor_performance_pct"))
    return value is not None and 0 < value < DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT


def usable_runs(data):
    """Runs of one target that took part in the measurement and can be aggregated."""
    return [run for run in data.get("runs", [])
            if not run_excluded(run) and not device_performance_low(run)]


def metric_needs_presentmon(metric):
    """Whether a metric reads presentmon.csv (so a partial log cannot be used)."""
    return "input_to_display" in metric or "present_to_display" in metric or metric.startswith("present.")


# A change this large between consecutive runs of the same target means the
# device moved, not the version under test. The runner records the device
# state in meta.txt (surface_state_*) so the shift can be shown next to the
# run that made the comparison invalid.
SHIFT_RELATIVE = 0.10
SHIFT_TEMP_C = 5.0
SHIFT_PROCESSOR_PERFORMANCE_PCT = 10.0
SHIFT_TOTAL_CPU_PCT = 10.0

SURFACE_STATE_KEYS = {
    "surface_state_mode": "mode",
    "surface_state_wait_mode": "wait_mode",
    "surface_state_wait": "wait",
    "surface_state_wait_seconds": "wait_seconds",
    "surface_state_wait_min_pct": "wait_min_pct",
    "surface_state_wait_last_pct": "wait_last_pct",
    "surface_state_before_max_temp_c": "before_max_temp_c",
    "surface_state_before_processor_performance_pct": "before_processor_performance_pct",
    "surface_state_before_total_cpu_pct": "before_total_cpu_pct",
    "surface_state_during_samples": "during_samples",
    "surface_state_during_max_temp_c": "during_max_temp_c",
    "surface_state_during_min_processor_performance_pct": "during_min_processor_performance_pct",
    "surface_state_during_max_total_cpu_pct": "during_max_total_cpu_pct",
    "surface_state_during_mean_total_cpu_pct": "during_mean_total_cpu_pct",
    "surface_state_top_process": "top_process",
    "surface_state_after_max_temp_c": "after_max_temp_c",
    "surface_state_after_processor_performance_pct": "after_processor_performance_pct",
    "surface_state_after_total_cpu_pct": "after_total_cpu_pct",
}

POWER_KEYS = {
    "power_mode": "mode",
    "power_ac": "ac",
}


def number_or_none(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def during_performance_readings(path):
    """Return (readings, missing) of % Processor Performance during a run.

    The light state recorder writes surface-state.csv. The WMI counter
    occasionally returns 0 in the middle of a run: the 2026-11-11 08:54:44
    sample of LibrePaint-wait-on-20261111-085349-1 was 0 between 181 and 180
    while total CPU was 14 %. A processor that stopped for one sample could
    not have drawn, so a non-positive reading is a failed read, not a
    measurement: it counts as missing and does not lower the minimum.

    None means the runner did not write the CSV (older runs); meta.txt is
    then the only record.
    """
    csv_path = os.path.join(path, "surface-state.csv")
    if not os.path.exists(csv_path):
        return None
    readings, missing = [], 0
    with open(csv_path, encoding="utf-8", errors="replace") as handle:
        for row in csv.DictReader(handle):
            if row.get("phase") != "during":
                continue
            value = number_or_none(row.get("processor_performance_pct"))
            if value is None or value <= 0:
                missing += 1
            else:
                readings.append(value)
    return readings, missing


def surface_state(path):
    """Read the device state the Windows runner recorded around one run.

    Linux, macOS and runs from an older runner have no surface-state keys;
    they get an empty dict and never take part in a shift comparison.
    """
    meta = os.path.join(path, "meta.txt")
    state = {}
    if not os.path.exists(meta):
        return state
    with open(meta, errors="replace") as handle:
        for line in handle:
            key, _, value = line.strip().partition("=")
            field = SURFACE_STATE_KEYS.get(key)
            if field:
                state[field] = value
    sampled = during_performance_readings(path)
    if sampled is not None:
        readings, missing = sampled
        state["during_readings"] = str(len(readings) + missing)
        state["during_readings_missing"] = str(missing)
        state["during_min_processor_performance_pct"] = (
            "%.0f" % min(readings) if readings else "unavailable")
    return state


def power_state(path):
    """Read whether the runner saw AC power around one run.

    Runs without a power sampler have no keys and return an empty dict, so
    they never enter the AC count.
    """
    meta = os.path.join(path, "meta.txt")
    state = {}
    if not os.path.exists(meta):
        return state
    with open(meta, errors="replace") as handle:
        for line in handle:
            key, _, value = line.strip().partition("=")
            field = POWER_KEYS.get(key)
            if field:
                state[field] = value
    return state


def shift_notes(runs):
    """Mark the runs whose values jumped from the run before them.

    Returns one line per change of the run medians or of the recorded device
    state. It is a marker for the reader, not a verdict: the values stay in
    the table so a comparison across the change can be rejected by eye.
    """
    notes = []
    previous = None
    for current in runs:
        if previous is not None:
            name = current.get("name", "?")
            for metric, stat in sorted(current.get("metrics", {}).items()):
                before = (previous.get("metrics", {}).get(metric) or {})
                if not stat.get("n") or not before.get("n"):
                    continue
                base, value = before.get("p50"), stat.get("p50")
                if not base or value is None:
                    continue
                change = (value - base) / base
                if abs(change) >= SHIFT_RELATIVE:
                    notes.append("%s %s %+.0f%% (%.1f -> %.1f ms) from the run before" % (
                        name, metric, 100 * change, base, value))
            before_state = previous.get("surface_state") or {}
            state = current.get("surface_state") or {}
            for field, limit, unit in (
                    ("during_max_temp_c", SHIFT_TEMP_C, "C"),
                    ("during_min_processor_performance_pct", SHIFT_PROCESSOR_PERFORMANCE_PCT, "%"),
                    ("during_max_total_cpu_pct", SHIFT_TOTAL_CPU_PCT, "%")):
                base = number_or_none(before_state.get(field))
                value = number_or_none(state.get(field))
                if base is None or value is None:
                    continue
                if abs(value - base) >= limit:
                    notes.append("%s %s %.0f%s -> %.0f%s from the run before" % (
                        name, field, base, unit, value, unit))
        previous = current
    return notes


def aggregate(runs, metrics):
    """Median, P95 and spread of the run medians for every metric.

    Metrics derived from PresentMon leave out runs whose recording is
    partial; capture-based metrics keep every run.
    """
    aggregated = {}
    for metric in metrics:
        selected = [
            run["metrics"] for run in runs
            if run["metrics"].get(metric, {}).get("n")
            and (not metric_needs_presentmon(metric) or presentmon_status(run) == "present")
        ]
        medians = [entry[metric]["p50"] for entry in selected]
        p95s = [entry[metric]["p95"] for entry in selected]
        if not medians:
            continue
        center = statistics.median(medians)
        aggregated[metric] = {
            "median": center,
            "p95": statistics.median(p95s),
            "spread": (max(medians) - min(medians)) / center if center else None,
            "runs": len(medians),
        }
    return aggregated


def aggregate_thread_cpu(runs):
    """Median of the per-run thread CPU values inside the input window."""
    entries = [run.get("thread_cpu") for run in runs if run.get("thread_cpu")]
    if not entries:
        return None
    result = {"runs": len(entries)}
    for key in ("window_s", "cpu_seconds", "percent", "process_seconds", "unaccounted_seconds"):
        values = [entry[key] for entry in entries if entry.get(key) is not None]
        if values:
            result[key] = statistics.median(values)
    roles = [entry.get("roles") or {} for entry in entries]
    for key in ("main_seconds", "paint_seconds"):
        values = [role[key] for role in roles if role.get(key) is not None]
        if values:
            result[key] = statistics.median(values)
    return result


def run_on_ac(run):
    """Whether a run saw AC power and must stay out of the power aggregation.

    The runner writes power_ac=1 from the samples. The summary is a fallback
    for a run whose meta.txt is incomplete.
    """
    state = run.get("power_state") or {}
    if str(state.get("ac", "")).strip().lower() in ("1", "true"):
        return True
    return bool((run.get("power") or {}).get("ac"))


def aggregate_power(runs):
    """Median of the per-run discharge rate and energy, without AC runs."""
    entries, ac_runs = [], 0
    for run in runs:
        entry = run.get("power")
        if not entry:
            continue
        if run_on_ac(run):
            ac_runs += 1
            continue
        entries.append(entry)
    if not entries:
        return None
    result = {"runs": len(entries), "ac_runs": ac_runs}
    for key in ("window_s", "rate_mw", "energy_mwh", "remaining_mwh_delta", "charge_pct_delta"):
        values = [entry[key] for entry in entries if entry.get(key) is not None]
        if values:
            result[key] = statistics.median(values)
    return result


def summarize(run_root, hosts, scenario):
    """Analyze every run directory and group run medians by host, target and state."""
    summary = {"scenario": scenario["name"], "hosts": {}}
    for host in hosts:
        host_dir = os.path.join(run_root, host["name"])
        if not os.path.isdir(host_dir):
            continue
        offset = float(host.get("capture_offset_ms", 0.0))
        entry = {"kind": host["kind"], "system": host.get("system", host["kind"]),
                 "capture_offset_ms": offset, "targets": {}}
        for run_name in sorted(os.listdir(host_dir)):
            path = os.path.join(host_dir, run_name)
            if not os.path.exists(os.path.join(path, "capture.csv")):
                continue
            target = target_of(run_name, host["targets"])
            if target is None:
                continue
            result = capture_analyze.analyze_run(path)
            data = entry["targets"].setdefault(target, {"runs": []})
            data["runs"].append({
                "name": run_name,
                "metrics": {metric: shifted(result.get(metric, {"n": 0}), offset) for metric in scenario["metrics"]},
                "state": result.get("state", {"label": "unknown"}),
                "presentmon": result.get("presentmon", {"present": False}),
                "foreground": result.get("foreground", {}),
                "foreground_inject": result.get("foreground_inject", {}),
                "excluded": result.get("excluded"),
                "surface_state": surface_state(path),
                "thread_cpu": thread_cpu.summarize(path),
                "power": power.summarize(path),
                "power_state": power_state(path),
                "speed_groups": result.get("ink.speed_groups", []),
                "speed_change": result.get("ink.speed_change", []),
            })
        for target, data in entry["targets"].items():
            valid = usable_runs(data)
            data["excluded"] = sum(1 for run in data["runs"] if run_excluded(run))
            data["device_low"] = sum(
                1 for run in data["runs"]
                if not run_excluded(run) and device_performance_low(run))
            data["device_readings_missing"] = sum(
                1 for run in data["runs"]
                if (number_or_none((run.get("surface_state") or {}).get("during_readings_missing")) or 0) > 0)
            data["shifts"] = shift_notes(valid)
            data["foreground_inject_lost"] = sum(
                1 for run in valid if (run.get("foreground_inject") or {}).get("foreground") is False)
            data["metrics"] = aggregate(valid, scenario["metrics"])
            data["thread_cpu"] = aggregate_thread_cpu(valid)
            data["power"] = aggregate_power(valid)
            data["presentmon"] = {
                status: sum(1 for run in valid if presentmon_status(run) == status)
                for status in ("present", "partial", "none")
            }
            data["surface_state_modes"] = {
                mode: sum(1 for run in data["runs"]
                          if (run.get("surface_state") or {}).get("mode") == mode)
                for mode in SURFACE_STATE_MODES
            }
            waits = {}
            for run in data["runs"]:
                key = (run.get("surface_state") or {}).get("wait") or "unrecorded"
                waits[key] = waits.get(key, 0) + 1
            data["surface_state_waits"] = waits
            groups = {}
            for run in valid:
                groups.setdefault(run["state"].get("label", "unknown"), []).append(run)
            data["states"] = {}
            for state, state_runs in sorted(groups.items()):
                data["states"][state] = {
                    "runs": len(state_runs),
                    "presentmon": sum(1 for run in state_runs if presentmon_status(run) == "present"),
                    "presentmon_status": {
                        status: sum(1 for run in state_runs if presentmon_status(run) == status)
                        for status in ("present", "partial", "none")
                    },
                    "metrics": aggregate(state_runs, scenario["metrics"]),
                }
        summary["hosts"][host["name"]] = entry
    return summary


def baseline_metric(baseline, host, target, metric, state):
    base = (baseline or {}).get("hosts", {}).get(host, {}).get("targets", {}).get(target, {})
    if state is not None:
        state_data = base.get("states", {}).get(state)
        if not state_data:
            return None
        return state_data.get("metrics", {}).get(metric)
    return base.get("metrics", {}).get(metric)


def judge(summary, scenario, baseline=None):
    """Return rows of (host, target, metric, median, p95, spread, delta, verdict, notes, state)."""
    rows = []
    limit = scenario.get("spread_limit", 0.05)
    for host, entry in summary["hosts"].items():
        for target, data in entry["targets"].items():
            valid = usable_runs(data) if isinstance(data.get("runs"), list) else []
            states = [state for state in data.get("states", {}) if state in ("low", "high")]
            for metric, stat in data.get("metrics", {}).items():
                grouped = states if metric.startswith("hover.") else []
                if grouped:
                    groups = [(state, data["states"][state]["metrics"].get(metric), data["states"][state]["runs"])
                              for state in sorted(grouped)]
                    if "unknown" in data.get("states", {}):
                        groups.append(("unknown", data["states"]["unknown"]["metrics"].get(metric),
                                       data["states"]["unknown"]["runs"]))
                else:
                    groups = [(None, stat, stat["runs"])]
                for state, group_stat, group_runs in groups:
                    if not group_stat:
                        continue
                    notes, problems = [], []
                    if state is not None:
                        notes.append("%s state: %d of %d runs" % (
                            state, group_runs, len(valid) if valid else len(data["runs"])))
                        if group_stat["runs"] < 2:
                            problems.append("only %d run in state" % group_stat["runs"])
                    if group_stat["spread"] is not None and group_stat["spread"] >= limit:
                        problems.append("spread %.1f%% >= %.0f%%" % (100 * group_stat["spread"], 100 * limit))
                    total_runs = len(valid) if valid else stat["runs"]
                    if total_runs < scenario["runs"]:
                        problems.append("%d of %d runs" % (total_runs, scenario["runs"]))
                    if data.get("excluded"):
                        notes.append("%d run(s) left out: app not foreground or not presenting" % data["excluded"])
                    if data.get("device_low"):
                        notes.append("%d run(s) left out: processor performance below %.0f%% during the run" % (
                            data["device_low"], DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT))
                    if data.get("device_readings_missing"):
                        notes.append("%d run(s) with a failed processor performance reading (kept)" % (
                            data["device_readings_missing"]))
                    if data.get("foreground_inject_lost"):
                        notes.append("%d run(s) lost the foreground before input" % data["foreground_inject_lost"])
                    for shift in data.get("shifts", []):
                        notes.append("marked: " + shift)
                    partial = sum(1 for run in valid if presentmon_status(run) == "partial")
                    if metric_needs_presentmon(metric) and partial:
                        notes.append("%d partial PresentMon run(s) left out" % partial)
                    base_stat = baseline_metric(baseline, host, target, metric, state)
                    delta = None
                    if base_stat:
                        delta = group_stat["median"] - base_stat["median"]
                        threshold = scenario["metrics"][metric]["regression_ms"]
                        if delta > threshold:
                            problems.append("regression +%.1f ms" % delta)
                        elif delta < -threshold:
                            notes.append("improvement %.1f ms" % delta)
                    elif baseline is not None and state is not None:
                        notes.append("baseline has no %s state" % state)
                    verdict = "check" if problems else "ok"
                    rows.append((host, target, metric, group_stat["median"], group_stat["p95"], group_stat["spread"],
                                 delta, verdict, "; ".join(problems + notes), state))
    return rows


def table(rows):
    lines = ["| host | target | metric | state | median ms | P95 ms | spread | vs baseline | verdict | notes |",
             "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for host, target, metric, median, p95, spread, delta, verdict, notes, state in rows:
        lines.append("| %s | %s | %s | %s | %.1f | %.1f | %s | %s | %s | %s |" % (
            host, target, metric, state or "-", median, p95, "-" if spread is None else "%.1f%%" % (100 * spread),
            "-" if delta is None else "%+.1f" % delta, verdict, notes))
    return "\n".join(lines)


def speed_table(runs):
    """Median and spread of the delay for each pointer speed across runs.

    capture_analyze.py already groups each run's ink frames by the speed that
    applied where the frame's drawing was made. This collects those groups
    across runs so a speed series shows its trend without reading every
    per-run log.
    """
    entries = []
    for run in runs:
        for group in run.get("speed_groups", []):
            if group.get("n") and group.get("input_to_capture", {}).get("n"):
                entries.append((group["speed_px_ms"], group["input_to_capture"]["p50"],
                                group["input_to_capture"]["p95"],
                                group.get("pen_tip_to_edge_px", {}).get("p50")))
    clusters = []
    for speed, p50, p95, tip in sorted(entries, key=lambda entry: entry[0]):
        if not clusters or speed > clusters[-1]["speeds"][-1] * 1.2:
            clusters.append({"speeds": [], "p50": [], "p95": [], "tip": []})
        clusters[-1]["speeds"].append(speed)
        clusters[-1]["p50"].append(p50)
        clusters[-1]["p95"].append(p95)
        if tip is not None:
            clusters[-1]["tip"].append(tip)
    rows = []
    for cluster in clusters:
        center = statistics.median(cluster["p50"])
        rows.append((statistics.median(cluster["speeds"]), len(cluster["p50"]), center,
                     statistics.median(cluster["p95"]),
                     (max(cluster["p50"]) - min(cluster["p50"])) / center if center else None,
                     statistics.median(cluster["tip"]) if cluster["tip"] else None))
    return rows


def speed_change_lines(runs):
    """Largest delay just after a speed change and its settling time per run."""
    lines = []
    for run in runs:
        changes = run.get("speed_change") or []
        if not changes:
            continue
        worst = max(changes, key=lambda change: change["max_latency_ms"])
        lines.append("%.1f ms (steady %.1f, settle %s, after %.2f px/ms)" % (
            worst["max_latency_ms"], worst["steady_latency_ms"],
            "none" if worst["settle_ms"] is None else "%.1f ms" % worst["settle_ms"],
            worst["after_speed_px_ms"]))
    return lines


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--hosts", required=True)
    parser.add_argument("--out", default=os.path.join(ROOT, "build", "perf", "runs"))
    parser.add_argument("--label", default="run")
    parser.add_argument("--only", help="comma-separated host names")
    parser.add_argument("--baseline", help="summary JSON to compare with")
    parser.add_argument("--summary", help="where to write the summary JSON")
    parser.add_argument("--analyze-only", metavar="RUN_ROOT", help="analyze collected runs without measuring")
    parser.add_argument("--events", default=DEFAULT_EVENTS,
                        help="event log the auditor follows (default: build/perf/events.log)")
    args = parser.parse_args(argv)

    scenario = json.load(open(args.scenario))
    config = json.load(open(args.hosts))
    hosts = [h for h in config["hosts"] if not args.only or h["name"] in args.only.split(",")]

    events = None if args.analyze_only else args.events
    if args.analyze_only:
        run_root = args.analyze_only
    else:
        stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        run_root = os.path.join(args.out, "%s-%s-%s" % (scenario["name"], args.label, stamp))
        failures = measure_runs(hosts, scenario, run_root, args.label, events)
        if failures:
            print("failed runs: " + ", ".join(failures), file=sys.stderr)

    summary = summarize(run_root, hosts, scenario)
    summary["label"] = args.label
    summary["date"] = datetime.date.today().isoformat()
    summary["commit"] = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short=10", "HEAD"],
                                       capture_output=True, text=True).stdout.strip()
    baseline = json.load(open(args.baseline)) if args.baseline else None
    verdict_rows = judge(summary, scenario, baseline)
    print(table(verdict_rows))
    present_lines = []
    for host, entry in summary["hosts"].items():
        if entry.get("kind") != "windows":
            continue
        for target, data in entry["targets"].items():
            valid = usable_runs(data)
            counts = data.get("presentmon") or {
                "present": sum(1 for run in valid if presentmon_status(run) == "present"),
                "partial": sum(1 for run in valid if presentmon_status(run) == "partial"),
                "none": sum(1 for run in valid if presentmon_status(run) == "none"),
            }
            present_lines.append(
                "   %s/%s: PresentMon present %d of %d runs (partial %d, none %d)" % (
                    host, target, counts["present"], len(valid), counts["partial"], counts["none"]))
            if data.get("excluded"):
                present_lines.append(
                    "   %s/%s: %d run(s) left out: app not foreground or not presenting" % (
                        host, target, data["excluded"]))
    if present_lines:
        print("== PresentMon records")
        print("\n".join(present_lines))
    device_lines = []
    for host, entry in summary["hosts"].items():
        if entry.get("kind") != "windows":
            continue
        for target, data in entry["targets"].items():
            modes = data.get("surface_state_modes") or {}
            if not (modes.get("on") or modes.get("light")):
                continue
            counts = ", ".join("%s %d" % (mode, modes[mode])
                               for mode in ("on", "light") if modes.get(mode))
            device_lines.append("   %s/%s: device state recorded (on/light): %s" % (host, target, counts))
    if device_lines:
        print("== device state records")
        print("\n".join(device_lines))
    wait_lines = []
    low_lines = []
    reading_lines = []
    for host, entry in summary["hosts"].items():
        if entry.get("kind") != "windows":
            continue
        for target, data in entry["targets"].items():
            waits = data.get("surface_state_waits") or {}
            if waits:
                wait_lines.append("   %s/%s: recovery wait before the run: %s" % (
                    host, target, ", ".join("%s %d" % (state, waits[state]) for state in sorted(waits))))
            for run in data.get("runs", []):
                if run_excluded(run):
                    continue
                state = run.get("surface_state") or {}
                missing = number_or_none(state.get("during_readings_missing")) or 0
                if missing:
                    reading_lines.append("   %s/%s: %s %d of %s reading(s) missing (0 %%, kept)" % (
                        host, target, run.get("name", "?"), missing,
                        state.get("during_readings", "?")))
                if not device_performance_low(run):
                    continue
                low_lines.append("   %s/%s: %s %s%% (wait %s, below %.0f%%, left out)" % (
                    host, target, run.get("name", "?"),
                    state.get("during_min_processor_performance_pct", "?"),
                    state.get("wait", "unrecorded"), DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT))
    if wait_lines:
        print("== device recovery waits")
        print("\n".join(wait_lines))
    if reading_lines:
        print("== device state readings missing (kept)")
        print("\n".join(reading_lines))
    if low_lines:
        print("== device state below %.0f%% (left out)" % DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT)
        print("\n".join(low_lines))
    shift_lines = []
    for host, entry in summary["hosts"].items():
        for target, data in entry["targets"].items():
            for shift in data.get("shifts", []):
                shift_lines.append("   %s/%s: %s" % (host, target, shift))
    if shift_lines:
        print("== environment changes")
        print("\n".join(shift_lines))
    speed_lines = []
    for host, entry in summary["hosts"].items():
        for target, data in entry["targets"].items():
            valid = usable_runs(data) if isinstance(data.get("runs"), list) else []
            speed_rows = speed_table(valid)
            if speed_rows:
                speed_lines.append("   %s/%s (speed px/ms, runs, median ms, P95 ms, spread, tip px)" % (
                    host, target))
                for speed, count, median, p95, spread, tip in speed_rows:
                    speed_lines.append("      %.2f  %d  %.1f  %.1f  %s  %s" % (
                        speed, count, median, p95, "-" if spread is None else "%.1f%%" % (100 * spread),
                        "-" if tip is None else "%.0f" % tip))
            changes = speed_change_lines(valid)
            if changes:
                speed_lines.append("   %s/%s speed changes: %s" % (host, target, " ; ".join(changes)))
    if speed_lines:
        print("== ink speed groups")
        print("\n".join(speed_lines))
    thread_lines = []
    for host, entry in summary["hosts"].items():
        for target, data in entry["targets"].items():
            cpu = data.get("thread_cpu")
            if not cpu:
                continue
            parts = ["cpu %.2fs (%.1f%% of one CPU) over %.1fs" % (
                cpu.get("cpu_seconds", 0.0), cpu.get("percent", 0.0), cpu.get("window_s", 0.0))]
            if cpu.get("main_seconds") is not None:
                parts.append("main %.2fs" % cpu["main_seconds"])
            if cpu.get("paint_seconds") is not None:
                parts.append("paint %.2fs" % cpu["paint_seconds"])
            if cpu.get("process_seconds") is not None:
                parts.append("process %.2fs" % cpu["process_seconds"])
            parts.append("%d run(s)" % cpu.get("runs", 0))
            thread_lines.append("   %s/%s: %s" % (host, target, ", ".join(parts)))
    if thread_lines:
        print("== thread CPU (input interval)")
        print("\n".join(thread_lines))
    power_lines = []
    for host, entry in summary["hosts"].items():
        for target, data in entry["targets"].items():
            sample = data.get("power")
            if not sample:
                continue
            parts = []
            if sample.get("rate_mw") is not None:
                parts.append("discharge %.1f mW" % sample["rate_mw"])
            if sample.get("energy_mwh") is not None:
                parts.append("%.2f mWh" % sample["energy_mwh"])
            if sample.get("remaining_mwh_delta") is not None:
                parts.append("remaining %.2f mWh" % sample["remaining_mwh_delta"])
            if sample.get("charge_pct_delta") is not None:
                parts.append("charge %.1f%%" % sample["charge_pct_delta"])
            parts.append("over %.1fs" % sample.get("window_s", 0.0))
            parts.append("%d run(s)" % sample.get("runs", 0))
            if sample.get("ac_runs"):
                parts.append("%d AC run(s) left out" % sample["ac_runs"])
            power_lines.append("   %s/%s: %s" % (host, target, ", ".join(parts)))
    if power_lines:
        print("== battery power (input interval)")
        print("\n".join(power_lines))
    destination = args.summary or os.path.join(run_root, "summary.json")
    os.makedirs(os.path.dirname(os.path.abspath(destination)), exist_ok=True)
    with open(destination, "w") as handle:
        json.dump(summary, handle, indent=2, sort_keys=True)
    print("summary: " + destination)
    status = 1 if any(row[7] != "ok" for row in verdict_rows) else 0
    if events:
        events_log.write(events, "run-done", args.label, "-", "-", str(status))
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
