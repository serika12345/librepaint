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


def runner_for(host, target, scenario, run_root):
    """Return (argv, env, collect) to run one measurement of target on host."""
    trajectory = json.dumps(scenario["trajectory"])
    out = os.path.join(run_root, host["name"])
    os.makedirs(out, exist_ok=True)
    kind = host["kind"]
    if kind == "windows":
        argv = [os.path.join(HERE, "pen-latency", "run-remote.sh"), host["ssh"], target["runner_target"], "1",
                target.get("app", "none"), out]
        env = {"CAPTURE_TOOL": host["capture_tool"], "TRAJECTORY_JSON": trajectory, "RUN_NAME": target["name"]}
        return argv, env, None
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


def summarize(run_root, hosts, scenario):
    """Analyze every run directory and group run medians by host and target."""
    summary = {"scenario": scenario["name"], "hosts": {}}
    for host in hosts:
        host_dir = os.path.join(run_root, host["name"])
        if not os.path.isdir(host_dir):
            continue
        offset = float(host.get("capture_offset_ms", 0.0))
        entry = {"system": host.get("system", host["kind"]), "capture_offset_ms": offset, "targets": {}}
        for run_name in sorted(os.listdir(host_dir)):
            path = os.path.join(host_dir, run_name)
            if not os.path.exists(os.path.join(path, "capture.csv")):
                continue
            target = target_of(run_name, host["targets"])
            if target is None:
                continue
            result = capture_analyze.analyze_run(path)
            runs = entry["targets"].setdefault(target, {"runs": []})["runs"]
            runs.append({metric: shifted(result.get(metric, {"n": 0}), offset) for metric in scenario["metrics"]})
        for target, data in entry["targets"].items():
            data["metrics"] = {}
            for metric in scenario["metrics"]:
                medians = [run[metric]["p50"] for run in data["runs"] if run[metric].get("n")]
                p95s = [run[metric]["p95"] for run in data["runs"] if run[metric].get("n")]
                if not medians:
                    continue
                center = statistics.median(medians)
                data["metrics"][metric] = {
                    "median": center,
                    "p95": statistics.median(p95s),
                    "spread": (max(medians) - min(medians)) / center if center else None,
                    "runs": len(medians),
                }
        summary["hosts"][host["name"]] = entry
    return summary


def judge(summary, scenario, baseline=None):
    """Return rows of (host, target, metric, median, p95, spread, verdict)."""
    rows = []
    limit = scenario.get("spread_limit", 0.05)
    for host, entry in summary["hosts"].items():
        for target, data in entry["targets"].items():
            for metric, stat in data.get("metrics", {}).items():
                notes, problems = [], []
                if stat["spread"] is not None and stat["spread"] >= limit:
                    problems.append("spread %.1f%% >= %.0f%%" % (100 * stat["spread"], 100 * limit))
                if stat["runs"] < scenario["runs"]:
                    problems.append("%d of %d runs" % (stat["runs"], scenario["runs"]))
                base = (baseline or {}).get("hosts", {}).get(host, {}).get("targets", {}).get(target, {})
                base_stat = base.get("metrics", {}).get(metric)
                delta = None
                if base_stat:
                    delta = stat["median"] - base_stat["median"]
                    threshold = scenario["metrics"][metric]["regression_ms"]
                    if delta > threshold:
                        problems.append("regression +%.1f ms" % delta)
                    elif delta < -threshold:
                        notes.append("improvement %.1f ms" % delta)
                verdict = "check" if problems else "ok"
                rows.append((host, target, metric, stat["median"], stat["p95"], stat["spread"], delta, verdict,
                             "; ".join(problems + notes)))
    return rows


def table(rows):
    lines = ["| host | target | metric | median ms | P95 ms | spread | vs baseline | verdict | notes |",
             "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for host, target, metric, median, p95, spread, delta, verdict, notes in rows:
        lines.append("| %s | %s | %s | %.1f | %.1f | %s | %s | %s | %s |" % (
            host, target, metric, median, p95, "-" if spread is None else "%.1f%%" % (100 * spread),
            "-" if delta is None else "%+.1f" % delta, verdict, notes))
    return "\n".join(lines)


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
    rows = judge(summary, scenario, baseline)
    print(table(rows))
    destination = args.summary or os.path.join(run_root, "summary.json")
    os.makedirs(os.path.dirname(os.path.abspath(destination)), exist_ok=True)
    with open(destination, "w") as handle:
        json.dump(summary, handle, indent=2, sort_keys=True)
    print("summary: " + destination)
    return 1 if any(row[7] != "ok" for row in rows) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
