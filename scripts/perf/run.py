#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run one latency scenario on several hosts and judge it against a baseline.

Inputs:
- a scenario file (scripts/perf/scenarios/*.json): trajectory, application
  settings, number of runs, the rest between Windows runs, and the
  per-metric regression threshold in milliseconds;
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
the run). A Linux or macOS target can set "kritarc" as well; its keys are
added to the scenario's kritarc for that target. A Windows target records the device state (surface_state.ps1) with
"surface_state": "light" (the default: one Get-Counter call per five seconds,
no per-second process list) or "on" (one WMI sample per second); only an
explicit "off" records nothing. Before every run the runner waits, unless the
target sets "surface_state_wait": "off", for the device's high-precision
temperature to stay below 45 C for two samples in a row (one WMI read per
five seconds, at most 900 s); a run whose % Processor Performance stayed
below 70 % during the measurement is marked and left out of the
aggregation. A Windows target that
sets "thread_cpu": "on"
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
the early stop of a target that failed twice in a row for the same reason, the
settle of every window and the exit code at the end (format:
scripts/perf/events_log.py). A target whose window counted fewer runs than the
scenario's "runs" is reported on standard error as an anomaly, and the exit
code is 4. A run whose recorded % Processor Performance stayed below 70 % is
counted; when three such runs arrive in a row across targets, the group stops
with an anomaly on standard error and the exit code 5.

The first "warmup_runs" successful runs of a target are measured but left out
of the aggregation. The aggregation uses the last "runs" usable runs (the
window) of every target. A target of the hosts file can set "before" to the
name of another target of the same host: it is then the changed version and
the named target the version before the change. The two alternate in every
round, and the i-th counted run of each forms a pair. For every judgement
unit the comparison is the 95 % Student t interval of the mean of the paired
differences of the run medians (after minus before; every metric is lower is
better). An interval wholly below zero is an improvement, wholly above zero a
regression, and one that contains zero is inconclusive. While a unit of the
pair is inconclusive, both targets get further runs up to "max_runs"; an
interval that still contains zero there is reported as inconclusive. A
target outside a pair gets no further runs after "runs". The spread of the
run medians is reported and plays no part in the verdict. Every run records its
canvas, frame size, scale and refresh rate (conditions), and the judge refuses
a comparison whose conditions differ from the baseline by more than 2 %. A
target of the hosts file can set "reference": true; the table then shows every
target's median minus the reference target's median. --baseline-out writes a
baseline JSON that carries only the keys the comparison reads.

Usage:
  run.py --scenario scripts/perf/scenarios/pen-light.json --hosts build/perf/hosts.json \
         --label q1 --out build/perf/runs [--only windows,linux] [--baseline OLD.json] \
         [--summary docs/perf/results/<file>.json] [--baseline-out docs/perf/baselines/<file>.json]
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


def kritarc_text(scenario, target=None):
    """Render the scenario's kritarc for the Linux and macOS runners.

    LibrePaint reads KisConfig and KisImageConfig from the empty group, which
    KConfig keeps before the first "[Group]" line; Krita does not read
    "General". The scenario names its runtime settings "General", so those
    keys are written as root keys and any other section follows as a group.
    A target's "kritarc" keys (target_kritarc) are root keys too and replace
    the scenario's keys of the same name, so that two targets of one host can
    differ in one setting, such as the stroke prediction.
    """
    sections = scenario.get("kritarc", {})
    general = dict(sections.get("General", {}))
    if target is not None:
        general.update(target_kritarc(target))
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


def latest_run_dir(run_root, host, target, started):
    """Return the directory of the run that this target just finished, or None.

    The runner names each run directory <target>-<date>-<time>-<n>; only the
    directory made after this run started belongs to it.
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
    return newest


def run_detail(run_root, host, target, started):
    """Return the state and medians of the run that just finished.

    The state label is classified for Windows hosts only and is "-" elsewhere.
    A run without a capture (the application did not start) has no medians.
    """
    newest = latest_run_dir(run_root, host, target, started)
    if newest is None or not os.path.exists(os.path.join(newest, "capture.csv")):
        return "state=unknown hover=- ink=-"
    result = capture_analyze.analyze_run(newest)
    offset = float(host.get("capture_offset_ms", 0.0))

    def median(metric):
        value = (result.get(metric) or {}).get("p50")
        return "-" if value is None else "%.1f" % (value - offset)

    state = "-"
    if host.get("kind") == "windows":
        state = (result.get("state") or {}).get("label", "unknown")
    return "state=%s hover=%s ink=%s" % (
        state, median("hover.input_to_capture"), median("ink.input_to_capture"))


def run_ink_missing(path):
    """Bands of a run whose captured stroke is far narrower than the injected one.

    A run without capture.csv has no analysis and no missing ink. The same
    analysis sets the run's "excluded" note, so --analyze-only keeps the run out
    of the aggregation instead of stopping the measurement.
    """
    if not path or not os.path.exists(os.path.join(path, "capture.csv")):
        return []
    return capture_analyze.analyze_run(path).get("ink_missing") or []


def settle_settings(scenario):
    """Return the scenario's (runs, warmup_runs, max_runs).

    "warmup_runs" (default 0) counts the first successful runs of every target
    that are measured but not aggregated. "max_runs" (default "runs") is the
    most counted runs a target of a before and after pair may collect while
    the pair's comparison is inconclusive.
    """
    runs = scenario.get("runs")
    if not isinstance(runs, int) or isinstance(runs, bool) or runs < 1:
        raise SystemExit("scenario: runs must be a positive integer")
    warmup_runs = scenario.get("warmup_runs", 0)
    if not isinstance(warmup_runs, int) or isinstance(warmup_runs, bool) or warmup_runs < 0:
        raise SystemExit("scenario: warmup_runs must be a non-negative integer")
    max_runs = scenario.get("max_runs", runs)
    if not isinstance(max_runs, int) or isinstance(max_runs, bool) or max_runs < 1:
        raise SystemExit("scenario: max_runs must be a positive integer")
    if runs > max_runs:
        raise SystemExit("scenario: max_runs (%d) is smaller than runs (%d)" % (max_runs, runs))
    return runs, warmup_runs, max_runs


def set_aside_unmeasured(run_root, hosts, measured):
    """Move the run directories this invocation did not measure out of the host directories.

    The collection copies the whole output directory of a host, so the
    directories left by an interrupted earlier invocation arrive with the
    first collected run. They would be counted as runs of this invocation.
    They move to <run_root>/_unmeasured/<host>/, where summarize does not
    look, and stay available for inspection. "measured" maps a host name to
    the set of directory names this invocation measured. Returns the moved
    directory names.
    """
    moved = []
    for host in hosts:
        out = os.path.join(run_root, host["name"])
        if not os.path.isdir(out):
            continue
        prefixes = tuple(target["name"] + "-" for target in host["targets"])
        for name in sorted(os.listdir(out)):
            path = os.path.join(out, name)
            if not os.path.isdir(path) or not name.startswith(prefixes) \
                    or name in measured.get(host["name"], ()):
                continue
            aside = os.path.join(run_root, "_unmeasured", host["name"])
            os.makedirs(aside, exist_ok=True)
            os.rename(path, os.path.join(aside, name))
            moved.append(name)
    return moved


def measure_runs(hosts, scenario, run_root, label, events=None):
    """Run every target and keep measuring until its window settles.

    A target whose run fails twice in a row for the same reason (the text
    after "failed: " of status.txt) skips its remaining rounds; the other
    targets continue. The first "warmup_runs" successful runs of a target are
    measured but not recorded. Every further successful run is recorded with
    run_record and counted as summarize counts it, so a run whose processor
    performance stayed below the limit or that measured another window is not
    usable. A target gets another run while fewer than "runs" runs are
    counted, or, for a target of a before and after pair, while fewer than
    "max_runs" runs are counted and a unit of the pair's comparison is
    inconclusive (comparison_units). A target
    starts at most warmup_runs + 2 * max_runs runs, failures and warm-ups
    included. After every run that leaves at least "runs" counted runs, a
    settle event reports the window. A Windows target rests for the scenario's
    "rest_seconds" after every run except the last one. Every start, result,
    early stop, settle and rest is written to the event log when one is given.
    A run whose recorded % Processor Performance stayed below 70 % is counted
    across targets, and DEVICE_LOW_STOP_RUNS such runs in a row stop the group.

    Run directories that this invocation did not measure (left by an
    interrupted earlier one) are set aside before the function returns.

    Returns the names of the failed runs.
    """
    failures = []
    measured = {host["name"]: set() for host in hosts}
    abandoned = set()
    previous_reason = {}
    rest_seconds = scenario.get("rest_seconds", 0)
    runs, warmup_runs, max_runs = settle_settings(scenario)
    starts_limit = warmup_runs + 2 * max_runs
    kinds = {(host["name"], target["name"]): host["kind"]
             for host in hosts for target in host["targets"]}
    before_of = {}
    afters_of = {}
    for host in hosts:
        for after, before in before_pairs(host).items():
            before_of[(host["name"], after)] = (host["name"], before)
            afters_of.setdefault((host["name"], before), []).append((host["name"], after))
    started_runs = {key: 0 for key in kinds}
    successes = {key: 0 for key in kinds}
    records = {key: [] for key in kinds}
    low_streak = 0

    def counted(key):
        """The target's usable records, as summarize counts them."""
        return usable_runs({"runs": records[key]})

    def window(key):
        """The last "runs" counted records."""
        return counted(key)[-runs:]

    def comparison(key):
        """The comparison units of an after target, or None for any other target."""
        if key not in before_of:
            return None
        return comparison_units(counted(before_of[key]), counted(key), kinds[key], scenario)

    def unstable(key):
        """Whether a pair the target belongs to has an inconclusive comparison unit.

        A target can be the after side of one pair and the before side of
        others; every such pair counts.
        """
        afters = ([key] if key in before_of else []) + afters_of.get(key, [])
        return any(inconclusive(comparison(after)) for after in afters)

    def needs_run(key):
        if key in abandoned or started_runs[key] >= starts_limit:
            return False
        count = len(counted(key))
        if count < runs:
            return True
        if count >= max_runs:
            return False
        return unstable(key)

    if events:
        events_log.write(events, "run-start", label, "-", "-",
                         "scenario %s, %d rounds, %d host(s)" % (
                             scenario["name"], runs, len(hosts)))
    while True:
        pending = [key for key in started_runs if needs_run(key)]
        if not pending:
            break
        for host in hosts:
            for target in host["targets"]:
                key = (host["name"], target["name"])
                if key not in pending:
                    continue
                number = started_runs[key] + 1
                started_runs[key] = number
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
                    successes[key] += 1
                    path = latest_run_dir(run_root, host, target, started)
                    if path:
                        measured[host["name"]].add(os.path.basename(path))
                    missing = run_ink_missing(path)
                    if missing:
                        for band in missing:
                            print("ANOMALY ink missing: %s %s" % (target["name"], band),
                                  file=sys.stderr)
                        raise SystemExit(3)
                    if path:
                        record = run_record(path, host, scenario)
                        record["start"] = number
                        if successes[key] > warmup_runs:
                            records[key].append(record)
                        low_streak = device_low_streak(low_streak, record)
                        if low_streak >= DEVICE_LOW_STOP_RUNS:
                            state = record.get("surface_state") or {}
                            print("ANOMALY processor performance low in %d consecutive runs: "
                                  "%s %s%%" % (
                                      low_streak, target["name"],
                                      state.get("during_min_processor_performance_pct")),
                                  file=sys.stderr)
                            raise SystemExit(5)
                    if events:
                        events_log.write(events, "target-done", label, target["name"], number,
                                         run_detail(run_root, host, target, started))
                    if events and len(counted(key)) >= runs:
                        capped = []
                        if len(counted(key)) >= max_runs and unstable(key):
                            capped.append("runs")
                        if started_runs[key] >= starts_limit and (
                                len(counted(key)) < runs
                                or (len(counted(key)) < max_runs and unstable(key))):
                            capped.append("starts")
                        events_log.write(events, "settle", label, target["name"], number,
                                         settle_line(window(key), len(counted(key)),
                                                     kinds[key], scenario, comparison(key),
                                                     unstable(key), capped))
                else:
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
                if rest_seconds and host["kind"] == "windows" \
                        and any(needs_run(other) for other in started_runs):
                    if events:
                        events_log.write(events, "rest", label, target["name"], number,
                                         "rest_seconds=%s" % rest_seconds)
                    time.sleep(float(rest_seconds))
    moved = set_aside_unmeasured(run_root, hosts, measured)
    if moved:
        print("set aside %d run directories that this invocation did not measure: %s"
              % (len(moved), ", ".join(moved)), file=sys.stderr)
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

    "light" (the default) reads only the CPU counters, with one Get-Counter
    call per five seconds, and samples the temperature and the busiest
    processes before and after the run. "on" keeps the per-second samples of
    temperature, processor performance, total CPU and the busiest processes.
    "off" records nothing: the per-second WMI samples were suspected of
    changing the device's power state during a measurement (2026-11-11 05:40).
    """
    mode = target.get("surface_state", "light")
    if not isinstance(mode, str) or mode not in SURFACE_STATE_MODES:
        raise SystemExit("target %s: surface_state must be one of %s" % (
            target["name"], ", ".join(SURFACE_STATE_MODES)))
    return mode


def target_surface_state_wait(target):
    """Return whether the Windows runner waits for the device to recover.

    The wait is on by default: each run starts only after % Processor
    Performance reached 75 % twice in a row (one Get-Counter read every five
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
        dense = "CAPTURE_DENSE=%d " % int(target["capture_dense"]) if target.get("capture_dense") else ""
        script = (
            ("cd %s && LATENCY_PROBE=%s " + dense + "TRAJECTORY_JSON=%s KRITARC=%s scripts/perf/pen-latency/run-linux.sh %s 1 %s %s -- %s")
            % (shlex.quote(host["repo"]), "1" if target.get("probe") else "0", shlex.quote(trajectory),
               shlex.quote(kritarc_text(scenario, target)),
               shlex.quote(target["name"]), shlex.quote(host["work"] + "/" + target["name"]),
               shlex.quote(remote_out), command)
        )
        collect = ["rsync", "-a", "--remove-source-files", "%s:%s/" % (host["ssh"], remote_out), out + "/"]
        return ["ssh", host["ssh"], script], {}, collect
    if kind == "macos":
        argv = [os.path.join(HERE, "pen-latency", "run-macos.sh"), target["name"], "1",
                os.path.join(host["work"], target["name"]), out, "--"] + target["command"]
        env = {"MACOS_CAPTURE": host["capture_tool"], "TRAJECTORY_JSON": trajectory,
               "KRITARC": kritarc_text(scenario, target), "LATENCY_PROBE": "1" if target.get("probe") else "0"}
        if target.get("capture_dense"):
            env["CAPTURE_DENSE"] = str(int(target["capture_dense"]))
        return argv, env, None
    raise SystemExit("unknown host kind: %s" % kind)


def target_of(run_name, targets):
    """Runners name each run directory <target name>-<date>-<time>-<n>."""
    for target in sorted(targets, key=lambda t: -len(t["name"])):
        if run_name.startswith(target["name"] + "-"):
            return target["name"]
    return None


def shifted(stat, offset):
    """Subtract the capture clock correction from a time statistic."""
    if not stat.get("n"):
        return stat
    return {key: (value - offset if key in ("p05", "p50", "p95") else value) for key, value in stat.items()}


def metric_unit(metric):
    """The unit of a metric's values: px for a distance, ms otherwise."""
    return "px" if metric.endswith("_px") else "ms"


def capture_metric(stat, metric, offset):
    """Return a metric of one run, corrected when it is a time to the capture.

    Only the input to capture times carry the capture clock; a distance such
    as l2p.residual_px stays as analyzed.
    """
    if metric.endswith(".input_to_capture"):
        return shifted(stat, offset)
    return stat


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
DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT = 70.0


def device_performance_low(run):
    """Whether the device's recorded % Processor Performance stayed below 70.

    A reading of 0 is a failed read rather than a measurement (see
    during_performance_readings), so it does not make the run low. Runs
    without a recorded device state (Linux, macOS, surface_state off)
    return False and stay in the aggregation.
    """
    state = run.get("surface_state") or {}
    value = number_or_none(state.get("during_min_processor_performance_pct"))
    return value is not None and 0 < value < DEVICE_MIN_PROCESSOR_PERFORMANCE_PCT


# Three low runs in a row mean the device slowed down, not the version under
# test. After the measuring Windows device restarted on 2026-11-21, "% Processor
# Performance" reached 236 % while "% Performance Limit" showed 72 %, so that
# value does not mark a slowdown and the earlier rule stopped every time. A
# single "% Processor Performance" reading before a run also varied from 73 %
# to 219 % in normal hours, so it cannot serve as the mark either. After the
# slowdown from 11:30 on, every target's run in a row was left out of the
# aggregation by device_performance_low. The caller stops on the streak.
DEVICE_LOW_STOP_RUNS = 3


def device_low_streak(previous, run):
    """Count the runs in a row whose recorded processor performance was low."""
    return previous + 1 if device_performance_low(run) else 0


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
    "surface_state_wait_first_c": "wait_first_c",
    "surface_state_wait_last_c": "wait_last_c",
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


CANVAS_FILES = ("canvas.json", "capture-meta.json")


def canvas_conditions(path):
    """Return the capture conditions of one run, or None when they were not recorded.

    The writers record the canvas rectangle in captured pixels as
    [left, top, width, height]: windows_capture.cpp and macos_capture.mm write
    canvas.json with "frame" [width, height] and, on macOS, "scale" and
    "refresh" (Windows writes "refresh" too); mutter_capture.py writes
    capture-meta.json with "frame" and neither scale nor refresh. A missing or
    unreadable file leaves the conditions unknown instead of guessing values.
    """
    for name in CANVAS_FILES:
        file_path = os.path.join(path, name)
        if not os.path.exists(file_path):
            continue
        try:
            with open(file_path, encoding="utf-8") as handle:
                data = json.load(handle)
        except (OSError, ValueError):
            return None
        canvas = data.get("canvas")
        if not (isinstance(canvas, list) and len(canvas) == 4
                and all(isinstance(value, (int, float)) for value in canvas)):
            return None
        width, height = canvas[2], canvas[3]
        if not (width > 0 and height > 0):
            return None
        frame = data.get("frame")
        return {
            "canvas": [int(width), int(height)],
            "frame": [int(value) for value in frame]
            if isinstance(frame, list) and len(frame) == 2 else None,
            "scale": number_or_none(data.get("scale")),
            "refresh": number_or_none(data.get("refresh")),
        }
    return None


def conditions_within(first, other):
    """Whether two runs recorded the same canvas (2 %), scale and refresh (1 Hz)."""
    if not first or not other:
        return False
    for index in (0, 1):
        if abs(other["canvas"][index] - first["canvas"][index]) > 0.02 * first["canvas"][index]:
            return False
    for key in ("scale", "refresh"):
        now, before = first.get(key), other.get(key)
        if now is None or before is None:
            if now is not before:
                return False
            continue
        tolerance = 1.0 if key == "refresh" else 0.0
        if abs(now - before) > tolerance:
            return False
    return True


def conditions_differ(current, base):
    """Whether a target's conditions differ from the baseline's by more than allowed."""
    for index in (0, 1):
        if abs(current["canvas"][index] - base["canvas"][index]) > 0.02 * base["canvas"][index]:
            return True
    for key, tolerance in (("scale", 0.0), ("refresh", 1.0)):
        now, before = current.get(key), base.get(key)
        if now is None or before is None:
            continue
        if abs(now - before) > tolerance:
            return True
    return False


def target_conditions(window):
    """Return the window's conditions, or None when no run recorded them.

    "stable" is false when a run of the window recorded another canvas
    (width or height beyond 2 %), scale or refresh (beyond 1 Hz), or none at
    all.
    """
    recorded = [run.get("conditions") for run in window]
    values = [value for value in recorded if value]
    if not values:
        return None
    first = values[0]
    stable = len(values) == len(recorded) and all(
        conditions_within(first, value) for value in values)
    return {"canvas": first["canvas"], "frame": first["frame"], "scale": first["scale"],
            "refresh": first["refresh"], "stable": stable}


def run_medians(runs, metric):
    """The metric's per-run medians, leaving out runs without a value."""
    values = []
    for run in runs:
        stat = (run.get("metrics") or {}).get(metric) or {}
        if stat.get("n") and stat.get("p50") is not None:
            values.append(stat["p50"])
    return values


# Two-sided 95 % quantiles of Student's t for 1 to 30 degrees of freedom.
# Above 30 the value for 30 is used, which widens the interval slightly.
T_975 = (12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228,
         2.201, 2.179, 2.160, 2.145, 2.131, 2.120, 2.110, 2.101, 2.093, 2.086,
         2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042)


def paired_interval(before, after):
    """Return the 95 % t interval of the mean of after[i] - before[i], or None.

    The i-th values of the two lists form a pair; values without a partner
    are left out. Fewer than two pairs give None. The verdict is
    "improvement" when the interval is wholly below zero, "regression" when
    it is wholly above zero and "inconclusive" otherwise.
    """
    differences = [later - earlier for earlier, later in zip(before, after)]
    count = len(differences)
    if count < 2:
        return None
    mean = statistics.mean(differences)
    quantile = T_975[min(count - 1, len(T_975)) - 1]
    half = quantile * statistics.stdev(differences) / (count ** 0.5)
    low, high = mean - half, mean + half
    if high < 0:
        verdict = "improvement"
    elif low > 0:
        verdict = "regression"
    else:
        verdict = "inconclusive"
    return {"pairs": count, "mean": mean, "low": low, "high": high, "verdict": verdict}


def before_pairs(host_config):
    """Return {after target name: before target name} of one host.

    A target names the version before its change with "before". The name
    must be another target of the same host.
    """
    targets = host_config.get("targets", [])
    names = {target["name"] for target in targets}
    pairs = {}
    for target in targets:
        before = target.get("before")
        if before is None:
            continue
        if before not in names or before == target["name"]:
            raise SystemExit("hosts: target %s names an unknown before target %r" % (
                target["name"], before))
        pairs[target["name"]] = before
    return pairs


def comparison_units(before_runs, after_runs, host_kind, scenario):
    """Return [(metric, state, paired_interval)] of an after target against its before target.

    The units are those of settle_units; a unit the before target lacks has
    the interval None.
    """
    before_units = {(metric, state): values
                    for metric, state, values in settle_units(before_runs, host_kind, scenario)}
    return [(metric, state, paired_interval(before_units.get((metric, state), []), values))
            for metric, state, values in settle_units(after_runs, host_kind, scenario)]


def inconclusive(units):
    """Whether a comparison has a unit without an interval or with an inconclusive one."""
    return any(interval is None or interval["verdict"] == "inconclusive"
               for _, _, interval in units or [])


def unit_name(metric, state):
    """The judgement unit's full name, as it is recorded in settle.unsettled."""
    return metric if state is None else "%s[%s]" % (metric, state)


def settle_short_name(metric, state):
    """The metric's family name, with the state for a Windows state group."""
    base = metric.split(".", 1)[0]
    return base if state is None else "%s[%s]" % (base, state)


def settle_units(window, host_kind, scenario):
    """Return [(metric, state, run medians)] for the window's judgement units.

    Windows hover metrics are split by the run's state label, as judge splits
    them; every other metric is one unit. A run without the metric's value
    stays out of that unit's medians.
    """
    units = []
    for metric in scenario["metrics"]:
        groups = {}
        if host_kind == "windows" and metric.startswith("hover."):
            for run in window:
                label = (run.get("state") or {}).get("label", "unknown")
                groups.setdefault(label, []).append(run)
        labels = sorted(label for label in groups if label in ("low", "high"))
        if labels:
            if "unknown" in groups:
                labels.append("unknown")
            for label in labels:
                units.append((metric, label, run_medians(groups[label], metric)))
        else:
            units.append((metric, None, run_medians(window, metric)))
    return units


def settle_line(window, counted_runs, host_kind, scenario, comparison=None, unsettled=False,
                capped=()):
    """Return the detail of a settle event: the window's spreads, the comparison and the outcome.

    The spreads are informational. "comparison" holds the units of an after
    target (comparison_units) and is shown as mean[low,high] in milliseconds.
    "unsettled" says whether the target's pair is still inconclusive.
    """
    units = settle_units(window, host_kind, scenario)
    short = {}
    for metric, state, _ in units:
        name = settle_short_name(metric, state)
        short[name] = short.get(name, 0) + 1
    parts = []
    if window:
        parts.append("window=%s..%s" % (window[0].get("start", "?"), window[-1].get("start", "?")))
    parts.append("counted=%d" % counted_runs)
    for metric, state, values in units:
        name = settle_short_name(metric, state)
        if short[name] > 1:
            name = unit_name(metric, state)
        if len(values) < 2:
            parts.append("%s=n/a" % name)
            continue
        center = statistics.median(values)
        spread = (max(values) - min(values)) / center if center else None
        parts.append("%s=%s" % (name, "n/a" if spread is None else "%.1f%%" % (100 * spread)))
    for metric, state, interval in comparison or []:
        name = "vs-before:%s" % unit_name(metric, state)
        if interval is None:
            parts.append("%s=n/a" % name)
        else:
            parts.append("%s=%+.1f[%+.1f,%+.1f]" % (
                name, interval["mean"], interval["low"], interval["high"]))
    parts.append("settled=%s" % ("no" if unsettled else "yes"))
    parts.extend("capped=%s" % value for value in capped)
    return " ".join(parts)


def metric_groups(data, host_kind, metric, stat):
    """Return [(state, group stat, runs)] of one metric, split for Windows hover."""
    states = [state for state in data.get("states", {}) if state in ("low", "high")]
    if host_kind == "windows" and metric.startswith("hover.") and states:
        groups = [(state, data["states"][state]["metrics"].get(metric), data["states"][state]["runs"])
                  for state in sorted(states)]
        if "unknown" in data.get("states", {}):
            groups.append(("unknown", data["states"]["unknown"]["metrics"].get(metric),
                           data["states"]["unknown"]["runs"]))
        return groups
    return [(None, stat, stat.get("runs"))]


def valid_runs(data):
    """The runs the judgement counts: the window when the summary has one."""
    runs = data.get("runs") if isinstance(data.get("runs"), list) else []
    if any("in_window" in run for run in runs):
        return [run for run in runs if run.get("in_window")]
    return usable_runs(data)


def states_of(host, window, scenario):
    """The window's state groups; a target that is not a Windows target is not split."""
    if host["kind"] != "windows":
        return {}
    groups = {}
    for run in window:
        groups.setdefault(run["state"].get("label", "unknown"), []).append(run)
    states = {}
    for state, state_runs in sorted(groups.items()):
        states[state] = {
            "runs": len(state_runs),
            "presentmon": sum(1 for run in state_runs if presentmon_status(run) == "present"),
            "presentmon_status": {
                status: sum(1 for run in state_runs if presentmon_status(run) == status)
                for status in ("present", "partial", "none")
            },
            "metrics": aggregate(state_runs, scenario["metrics"]),
        }
    return states


def run_record(path, host, scenario):
    """Return the record of one run: medians, state, device state and conditions.

    summarize and measure_runs build their records with this function, so the
    judgement during a measurement and after it sees the same values. A
    successful run without capture.csv (the application started but the
    capture did not finish) has no medians.
    """
    offset = float(host.get("capture_offset_ms", 0.0))
    if os.path.exists(os.path.join(path, "capture.csv")):
        result = capture_analyze.analyze_run(path)
    else:
        result = {}
    return {
        "name": os.path.basename(path),
        "metrics": {metric: capture_metric(result.get(metric, {"n": 0}), metric, offset)
                    for metric in scenario["metrics"]},
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
        "conditions": canvas_conditions(path),
    }


def summarize(run_root, hosts, scenario):
    """Analyze every run directory and group the window's medians by host, target and state.

    The first "warmup_runs" runs of a target are kept in "runs" as warm-ups
    and stay out of the aggregation and the run counts. The aggregation uses
    the last "runs" usable runs (the window) of every target; "settle" reports
    how many runs were counted, how large the window is and which judgement
    units of an after target stayed inconclusive against its before target.
    An after target also carries "comparison": the before target's name and
    every unit's interval. Only a Windows target is split by state.
    """
    runs_needed, warmup_runs, _ = settle_settings(scenario)
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
            data = entry["targets"].setdefault(target, {"runs": []})
            data["runs"].append(run_record(path, host, scenario))
        counted_of = {}
        for target, data in entry["targets"].items():
            warmup = data["runs"][:warmup_runs]
            for index, run in enumerate(data["runs"]):
                run["warmup"] = index < warmup_runs
            measured = data["runs"][len(warmup):]
            counted = usable_runs({"runs": measured})
            counted_of[target] = counted
            window = counted[-runs_needed:]
            window_ids = {id(run) for run in window}
            for run in data["runs"]:
                run["in_window"] = id(run) in window_ids
            data["excluded"] = sum(1 for run in measured if run_excluded(run))
            data["device_low"] = sum(
                1 for run in measured
                if not run_excluded(run) and device_performance_low(run))
            data["device_readings_missing"] = sum(
                1 for run in measured
                if (number_or_none((run.get("surface_state") or {}).get("during_readings_missing")) or 0) > 0)
            data["shifts"] = shift_notes(window)
            data["foreground_inject_lost"] = sum(
                1 for run in window if (run.get("foreground_inject") or {}).get("foreground") is False)
            data["metrics"] = aggregate(window, scenario["metrics"])
            data["thread_cpu"] = aggregate_thread_cpu(window)
            data["power"] = aggregate_power(window)
            data["presentmon"] = {
                status: sum(1 for run in window if presentmon_status(run) == status)
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
            data["states"] = states_of(host, window, scenario)
            data["conditions"] = target_conditions(window)
            data["settle"] = {
                "warmup_runs": len(warmup),
                "counted_runs": len(counted),
                "window_runs": len(window),
                "unsettled": [],
            }
        for after, before in before_pairs(host).items():
            data = entry["targets"].get(after)
            if data is None:
                continue
            units = comparison_units(counted_of.get(before, []), counted_of[after],
                                     host["kind"], scenario)
            data["comparison"] = {"before": before, "units": [
                dict({"metric": metric, "state": state}, **(interval or {"verdict": "inconclusive"}))
                for metric, state, interval in units]}
            data["settle"]["unsettled"] = [
                unit_name(metric, state) for metric, state, interval in units
                if interval is None or interval["verdict"] == "inconclusive"]
        summary["hosts"][host["name"]] = entry
    return summary


def short_targets(summary, hosts, scenario):
    """Return (host, target, window_runs, excluded, device_low) for short windows.

    A target is short when its window counted fewer runs than the scenario's
    "runs". A target or host missing from the summary counts as 0 runs.
    """
    runs = settle_settings(scenario)[0]
    short = []
    for host in hosts:
        host_data = (summary.get("hosts") or {}).get(host["name"]) or {}
        targets = host_data.get("targets") or {}
        for target in host.get("targets", []):
            data = targets.get(target["name"]) or {}
            window_runs = (data.get("settle") or {}).get("window_runs") or 0
            if window_runs < runs:
                short.append((host["name"], target["name"], window_runs,
                              data.get("excluded") or 0, data.get("device_low") or 0))
    return short


def baseline_target(baseline, host, target):
    """Return the baseline's record of one target, or {}."""
    host_data = (baseline or {}).get("hosts", {}).get(host, {})
    return (host_data.get("targets", {}) or {}).get(target) or {}


def baseline_metric(baseline, host, target, metric, state):
    base = baseline_target(baseline, host, target)
    if state is not None:
        state_data = base.get("states", {}).get(state)
        if not state_data:
            return None
        return state_data.get("metrics", {}).get(metric)
    return base.get("metrics", {}).get(metric)


def judge(summary, scenario, baseline=None):
    """Return rows of (host, target, metric, median, p95, spread, delta, verdict, notes, state)."""
    rows = []
    runs = settle_settings(scenario)[0]
    for host, entry in summary["hosts"].items():
        kind = entry.get("kind")
        for target, data in entry["targets"].items():
            valid = valid_runs(data)
            base_target = baseline_target(baseline, host, target)
            base_unsettled = (base_target.get("settle") or {}).get("unsettled") or []
            conditions = data.get("conditions")
            for metric, stat in data.get("metrics", {}).items():
                for state, group_stat, group_runs in metric_groups(data, kind, metric, stat):
                    if not group_stat:
                        continue
                    notes, problems = [], []
                    if state is not None:
                        notes.append("%s state: %d of %d runs" % (
                            state, group_runs, len(valid) if valid else len(data["runs"])))
                        if group_stat["runs"] < 2:
                            problems.append("only %d run in state" % group_stat["runs"])
                    compared = data.get("comparison") or {}
                    for unit in compared.get("units", []):
                        if unit["metric"] != metric or unit["state"] != state:
                            continue
                        if "mean" not in unit:
                            notes.append("vs %s: too few pairs" % compared["before"])
                            continue
                        text = "vs %s %+.1f %s [%+.1f, %+.1f] over %d pairs" % (
                            compared["before"], unit["mean"], metric_unit(metric),
                            unit["low"], unit["high"], unit["pairs"])
                        if unit["verdict"] == "regression":
                            problems.append("regression " + text)
                        elif unit["verdict"] == "improvement":
                            notes.append("improvement " + text)
                        else:
                            notes.append("not shown " + text)
                    total_runs = len(valid) if valid else stat["runs"]
                    if total_runs < runs:
                        problems.append("%d of %d runs" % (total_runs, runs))
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
                    if unit_name(metric, state) in base_unsettled:
                        notes.append("baseline unsettled")
                    differ = False
                    if baseline is not None:
                        base_conditions = base_target.get("conditions")
                        if base_conditions and conditions:
                            if conditions_differ(conditions, base_conditions):
                                problems.append("conditions differ from baseline: canvas %dx%d vs baseline %dx%d" % (
                                    conditions["canvas"][0], conditions["canvas"][1],
                                    base_conditions["canvas"][0], base_conditions["canvas"][1]))
                                differ = True
                        else:
                            notes.append("conditions unknown")
                    if conditions and conditions.get("stable") is False:
                        notes.append("conditions vary inside the window")
                    base_stat = baseline_metric(baseline, host, target, metric, state)
                    delta = None
                    if base_stat and not differ:
                        delta = group_stat["median"] - base_stat["median"]
                        threshold = scenario["metrics"][metric]["regression_ms"]
                        if delta > threshold:
                            problems.append("regression +%.1f ms" % delta)
                        elif delta < -threshold:
                            notes.append("improvement %.1f ms" % delta)
                    elif base_stat is None and baseline is not None and state is not None:
                        notes.append("baseline has no %s state" % state)
                    verdict = "check" if problems else "ok"
                    rows.append((host, target, metric, group_stat["median"], group_stat["p95"], group_stat["spread"],
                                 delta, verdict, "; ".join(problems + notes), state))
    return rows


def table(rows, extra=None):
    columns = ["host", "target", "metric", "state", "median ms", "P95 ms", "spread", "vs baseline"]
    if extra is not None:
        columns.append("minus reference")
    columns.extend(["verdict", "notes"])
    lines = ["| " + " | ".join(columns) + " |",
             "| " + " | ".join("---" for _ in columns) + " |"]
    for host, target, metric, median, p95, spread, delta, verdict, notes, state in rows:
        cells = [host, target, metric, state or "-", "%.1f" % median, "%.1f" % p95,
                 "-" if spread is None else "%.1f%%" % (100 * spread),
                 "-" if delta is None else "%+.1f" % delta]
        if extra is not None:
            cells.append(extra.get((host, target, metric, state), "-"))
        cells.extend([verdict, notes])
        lines.append("| " + " | ".join(cells) + " |")
    return "\n".join(lines)


def reference_pairs(host_config):
    """Return {target name: reference target name} of one host.

    A target names its reference with "against". Without it, the host's only
    reference target ("reference": true) is the target's reference; a host
    with several reference targets leaves a target that does not say which
    one without a reference. Reference targets have no reference.
    """
    targets = host_config.get("targets", [])
    references = [target["name"] for target in targets if target.get("reference") is True]
    pairs = {}
    for target in targets:
        if target.get("reference") is True:
            continue
        against = target.get("against")
        if against is not None:
            if against in references:
                pairs[target["name"]] = against
        elif len(references) == 1:
            pairs[target["name"]] = references[0]
    return pairs


def reference_columns(summary, baseline, hosts):
    """Return the target-minus-reference cell of every judged row.

    The reference of a target is given by reference_pairs; the baseline side
    uses the targets of the same names. The value is the target's median
    minus the reference's median of the same metric (the metrics aggregate),
    in milliseconds, with a sign. When the baseline has both medians, the
    baseline difference follows in parentheses. Reference targets and
    targets without a reference have no cell.
    """
    pairs_of = {host["name"]: reference_pairs(host) for host in hosts or []}
    cells = {}
    for host, entry in summary.get("hosts", {}).items():
        pairs = pairs_of.get(host, {})
        for target, data in (entry.get("targets") or {}).items():
            reference = pairs.get(target)
            if reference is None:
                continue
            reference_data = (entry.get("targets") or {}).get(reference) or {}
            for metric, stat in (data.get("metrics") or {}).items():
                reference_stat = (reference_data.get("metrics") or {}).get(metric)
                if not reference_stat:
                    continue
                difference = stat["median"] - reference_stat["median"]
                base_target = baseline_metric(baseline, host, target, metric, None)
                base_reference = baseline_metric(baseline, host, reference, metric, None)
                if base_target and base_reference:
                    value = "%+.1f (base %+.1f)" % (
                        difference, base_target["median"] - base_reference["median"])
                else:
                    value = "%+.1f" % difference
                for state, group_stat, _ in metric_groups(data, entry.get("kind"), metric, stat):
                    if group_stat:
                        cells[(host, target, metric, state)] = value
    return cells


BASELINE_FORMAT = 1


def baseline_document(summary, hosts, scenario):
    """Return the baseline document of a summary: only the keys the comparison reads.

    The document carries no run names, paths, ssh names, environment variables
    or tool locations, so it can be kept in the repository.
    """
    runs, warmup_runs, max_runs = settle_settings(scenario)
    reference_names = {}
    for host in hosts or []:
        reference_names[host["name"]] = {
            target["name"]: bool(target.get("reference")) for target in host.get("targets", [])}

    def stats(metrics):
        return {metric: {key: stat.get(key) for key in ("median", "p95", "spread", "runs")}
                for metric, stat in (metrics or {}).items()}

    document = {
        "format": BASELINE_FORMAT,
        "label": summary.get("label"),
        "date": summary.get("date"),
        "commit": summary.get("commit"),
        "scenario": summary.get("scenario"),
        "settings": {"runs": runs, "warmup_runs": warmup_runs, "max_runs": max_runs},
        "hosts": {},
    }
    for host, entry in summary.get("hosts", {}).items():
        targets = {}
        for target, data in (entry.get("targets") or {}).items():
            window = [run for run in data.get("runs", []) if run.get("in_window")]
            medians = {}
            for metric in data.get("metrics") or {}:
                values = run_medians(window, metric)
                if values:
                    medians[metric] = values
            targets[target] = {
                "reference": reference_names.get(host, {}).get(target, False),
                "metrics": stats(data.get("metrics")),
                "states": {state: {"runs": state_data.get("runs"),
                                   "metrics": stats(state_data.get("metrics"))}
                           for state, state_data in (data.get("states") or {}).items()},
                "run_medians": medians,
                "conditions": data.get("conditions"),
                "settle": data.get("settle"),
            }
        document["hosts"][host] = {
            "kind": entry.get("kind"),
            "system": entry.get("system"),
            "capture_offset_ms": entry.get("capture_offset_ms"),
            "targets": targets,
        }
    return document


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
    parser.add_argument("--baseline-out", metavar="PATH",
                        help="write the allowlisted baseline JSON here")
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
    print(table(verdict_rows, reference_columns(summary, baseline, hosts) or None))
    present_lines = []
    for host, entry in summary["hosts"].items():
        if entry.get("kind") != "windows":
            continue
        for target, data in entry["targets"].items():
            valid = valid_runs(data)
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
            valid = valid_runs(data)
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
    if args.baseline_out:
        os.makedirs(os.path.dirname(os.path.abspath(args.baseline_out)), exist_ok=True)
        with open(args.baseline_out, "w") as handle:
            json.dump(baseline_document(summary, hosts, scenario), handle, indent=2, sort_keys=True)
        print("baseline: " + args.baseline_out)
    status = 1 if any(row[7] != "ok" for row in verdict_rows) else 0
    runs_needed = settle_settings(scenario)[0]
    short = short_targets(summary, hosts, scenario)
    for host, target, window_runs, excluded, device_low in short:
        print("ANOMALY too few counted runs: %s/%s %d of %d (excluded %d, processor performance low %d)"
              % (host, target, window_runs, runs_needed, excluded, device_low),
              file=sys.stderr)
    if short:
        status = 4
    if events:
        events_log.write(events, "run-done", args.label, "-", "-", str(status))
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
