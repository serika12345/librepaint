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
the run). A run
whose PresentMon CSV covers too little of the injected trajectory is
reported as partial and left out of PresentMon-based metrics. A run whose
application was not the foreground window when the capture began and that
presented far fewer frames than the display refresh (Windows throttles a
covered window) measured another window; it is left out of every metric and
counted in the table.

Usage:
  run.py --scenario scripts/perf/scenarios/pen-light.json --hosts build/perf/hosts.json \
         --label q1 --out build/perf/runs [--only windows,linux] [--baseline OLD.json] \
         [--summary docs/perf/results/<file>.json]
  run.py --analyze-only RUN_ROOT ...   (re-analyze collected runs)
"""

import argparse
import datetime
import json
import os
import re
import shlex
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(HERE, "pen-latency"))
import capture_analyze  # noqa: E402


def kritarc_text(scenario):
    lines = []
    for section, values in scenario.get("kritarc", {}).items():
        lines.append("[%s]" % section)
        lines.extend("%s=%s" % (key, value) for key, value in values.items())
    return "\n".join(lines) + "\n"


def run_command(argv, env=None, cwd=None):
    print("$ " + " ".join(shlex.quote(a) for a in argv), flush=True)
    merged = dict(os.environ)
    merged.update(env or {})
    return subprocess.run(argv, env=merged, cwd=cwd).returncode


ENV_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
KRITARC_KEY = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]*$")


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
               "TARGET_ENV": json.dumps(target_environment(target), sort_keys=True),
               "TARGET_KRITARC": json.dumps(target_kritarc(target), sort_keys=True)}
        return argv, env, None
    if kind in ("linux", "macos") and target.get("env"):
        raise SystemExit("target %s: env is supported only for Windows targets" % target["name"])
    if kind in ("linux", "macos") and target.get("kritarc"):
        raise SystemExit("target %s: kritarc is supported only for Windows targets" % target["name"])
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


def usable_runs(data):
    """Runs of one target that took part in the measurement and can be aggregated."""
    return [run for run in data.get("runs", []) if not run_excluded(run)]


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


def number_or_none(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


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
                "speed_groups": result.get("ink.speed_groups", []),
                "speed_change": result.get("ink.speed_change", []),
            })
        for target, data in entry["targets"].items():
            valid = usable_runs(data)
            data["excluded"] = len(data["runs"]) - len(valid)
            data["shifts"] = shift_notes(valid)
            data["foreground_inject_lost"] = sum(
                1 for run in valid if (run.get("foreground_inject") or {}).get("foreground") is False)
            data["metrics"] = aggregate(valid, scenario["metrics"])
            data["presentmon"] = {
                status: sum(1 for run in valid if presentmon_status(run) == status)
                for status in ("present", "partial", "none")
            }
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
    args = parser.parse_args(argv)

    scenario = json.load(open(args.scenario))
    config = json.load(open(args.hosts))
    hosts = [h for h in config["hosts"] if not args.only or h["name"] in args.only.split(",")]

    if args.analyze_only:
        run_root = args.analyze_only
    else:
        stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        run_root = os.path.join(args.out, "%s-%s-%s" % (scenario["name"], args.label, stamp))
        failures = []
        for round_index in range(scenario["runs"]):
            for host in hosts:
                for target in host["targets"]:
                    argv_, env, collect = runner_for(host, target, scenario, run_root)
                    status = run_command(argv_, env)
                    if collect and status == 0:
                        status = run_command(collect)
                    if status != 0:
                        failures.append("%s/%s round %d" % (host["name"], target["name"], round_index + 1))
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
    destination = args.summary or os.path.join(run_root, "summary.json")
    os.makedirs(os.path.dirname(os.path.abspath(destination)), exist_ok=True)
    with open(destination, "w") as handle:
        json.dump(summary, handle, indent=2, sort_keys=True)
    print("summary: " + destination)
    return 1 if any(row[7] != "ok" for row in verdict_rows) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
