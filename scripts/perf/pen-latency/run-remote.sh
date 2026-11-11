#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs a pen-latency measurement on a Windows host over ssh and collects the
# results. The host needs PresentMon.exe in ~/pen-latency and the task
# registered by setup-surface.ps1.
#
# Usage: run-remote.sh HOST TARGET RUNS APP OUT_DIR [INJECT_JSON]
#   TARGET       LibrePaint | Krita | PenProbe
#   APP          Windows path of the executable (ignored for PenProbe)
#   OUT_DIR      local directory that receives one subdirectory per run
#   INJECT_JSON  optional PenInject.Options overrides, e.g. '{"HoverSeconds":15}'
#
# Capture mode: set CAPTURE_TOOL to a built windows_capture.exe. The script
# writes plan.csv with trajectory.py (overrides in TRAJECTORY_JSON) and the
# run records the screen; analyze the runs with capture_analyze.py.
# RUN_NAME names the run directories (default: TARGET), so that two
# versions of one application can be told apart.
# PRESENTMON=off skips PresentMon for one measurement; the default is on.
# SURFACE_STATE is off (the default), on or light: whether run-surface.ps1
# records the device state around the run. run.py fills it from the target's
# "surface_state" in hosts.json.
# SURFACE_STATE_WAIT is on (the default) or off: whether run-surface.ps1 holds
# each run until the device's % Processor Performance has recovered (two
# samples of at least 70 %, at most 120 s). run.py fills it from the target's
# "surface_state_wait" in hosts.json.
# THREAD_CPU is off (the default) or on: whether thread_cpu.ps1 samples the
# process and thread CPU counters during the run. run.py fills it from the
# target's "thread_cpu" in hosts.json.
# POWER is off (the default) or on: whether power.ps1 samples the battery once
# a second during the run. run.py fills it from the target's "power" in
# hosts.json; runs on AC power are left out of the power aggregation.
# TARGET_ENV is a JSON object of environment variables for the measured
# application; run.py fills it from the target's "env" in hosts.json.
# TARGET_KRITARC is a JSON object of root kritarc keys (for example the hidden
# useMaximumFrameLatencyOne switch); run-surface.ps1 changes them for the run
# and restores the file afterwards.
# TARGET_DISPLAY_CHECK is a JSON object such as {"image": "C:\\...\\x.png"}.
# It opens that image and captures the whole screen at 100 % and 400 % zoom
# instead of injecting a trajectory.
# CHECK_JSON is a JSON object that switches run-surface.ps1 to the Q4
# automatic check (clipboard, tab titles, Japanese layout, resize/dialog
# flicker and synthetic-pen checks). It writes evidence files; scripts/perf/
# check.py judges them. The capture tool is still copied for the screen
# snapshots, but no trajectory plan is generated.
# While the target runs, this script prints each new line of status.txt as
# "status: <line>" and leaves the last line in "remote status:"; check.py
# records the step lines in the event log.
set -euo pipefail

if [ "$#" -lt 5 ]; then
  sed -n '4,33p' "$0" >&2
  exit 2
fi
host=$1 target=$2 runs=$3 app=$4 out=$5 inject=${6:-'{}'}
here=$(cd "$(dirname "$0")" && pwd)
remote=pen-latency/l0
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

capture=false
target_env=${TARGET_ENV:-'{}'}
target_kritarc=${TARGET_KRITARC:-'{}'}
target_display_check=${TARGET_DISPLAY_CHECK:-'{}'}
check_json=${CHECK_JSON:-'{}'}
surface_state=${SURFACE_STATE:-off}
surface_state_wait=${SURFACE_STATE_WAIT:-on}
thread_cpu=${THREAD_CPU:-off}
power=${POWER:-off}
extra=()
if [ -n "${CAPTURE_TOOL:-}" ]; then
  capture=true
  if [ "$check_json" = "{}" ]; then
    trajectory_json=${TRAJECTORY_JSON:-}
    python3 -I "$here/trajectory.py" --options "${trajectory_json:-"{}"}" --out "$work/plan.csv"
    extra=("$CAPTURE_TOOL" "$work/plan.csv")
  else
    extra=("$CAPTURE_TOOL")
  fi
fi

python3 -I "$here/run_request.py" "$target" "$runs" "$app" "$inject" "$capture" \
    "${RUN_NAME:-$target}" "${PRESENTMON:-on}" \
    "$(python3 -I "$here/presentmon_command.py")" "$target_env" "$target_kritarc" \
    "$target_display_check" "$surface_state" "$surface_state_wait" "$thread_cpu" "$power" \
    "$check_json" > "$work/request.json"

ssh "$host" "New-Item -ItemType Directory -Force $remote | Out-Null; Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $remote/runs, $remote/status.txt; exit 0"
scp -q "$here/run-surface.ps1" "$here/setup-surface.ps1" "$here/surface_state.ps1" "$here/thread_cpu.ps1" "$here/power.ps1" "$here/PenInject.cs" "$here/PenProbe.cs" "$here/Q4Check.cs" "$work/request.json" ${extra[@]+"${extra[@]}"} "$host:$remote/"
ssh "$host" "schtasks /run /tn LibrePaintPenLatency | Out-Null"

state=""
seen=0
for _ in $(seq 1 720); do
  sleep 5
  lines=$(ssh "$host" "if (Test-Path $remote/status.txt) { Get-Content $remote/status.txt }" | tr -d '\r')
  if [ -n "$lines" ]; then
    total=$(printf '%s\n' "$lines" | wc -l)
    # Print the lines this loop has not seen yet, prefixed so check.py can
    # pick out the step lines. The auditor follows the steps as events while
    # the target still runs.
    if [ "$total" -gt "$seen" ]; then
      printf '%s\n' "$lines" | tail -n "+$((seen + 1))" | sed 's/^/status: /'
      seen=$total
    fi
    state=$(printf '%s\n' "$lines" | tail -n 1)
  fi
  case "$state" in
    done|failed:*) break ;;
  esac
done
echo "remote status: $state"

mkdir -p "$out"
scp -q -r "$host:$remote/runs/*" "$out/"
scp -q "$host:$remote/status.txt" "$out/status-$target.txt"
[ "$state" = done ] || exit 1
