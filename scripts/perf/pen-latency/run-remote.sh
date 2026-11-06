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
set -euo pipefail

if [ "$#" -lt 5 ]; then
  sed -n '4,16p' "$0" >&2
  exit 2
fi
host=$1 target=$2 runs=$3 app=$4 out=$5 inject=${6:-'{}'}
here=$(cd "$(dirname "$0")" && pwd)
remote=pen-latency/l0
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

capture=false
extra=()
if [ -n "${CAPTURE_TOOL:-}" ]; then
  capture=true
  trajectory_json=${TRAJECTORY_JSON:-}
  python3 -I "$here/trajectory.py" --options "${trajectory_json:-"{}"}" --out "$work/plan.csv"
  extra=("$CAPTURE_TOOL" "$work/plan.csv")
fi

python3 -I - "$target" "$runs" "$app" "$inject" "$capture" > "$work/request.json" <<'PY'
import json, sys
target, runs, app, inject, capture = sys.argv[1:]
print(json.dumps({"target": target, "runs": int(runs), "app": app, "inject": json.loads(inject),
                  "capture": capture == "true"}))
PY

ssh "$host" "New-Item -ItemType Directory -Force $remote | Out-Null; Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $remote/runs, $remote/status.txt; exit 0"
scp -q "$here/run-surface.ps1" "$here/setup-surface.ps1" "$here/PenInject.cs" "$here/PenProbe.cs" "$work/request.json" ${extra[@]+"${extra[@]}"} "$host:$remote/"
ssh "$host" "schtasks /run /tn LibrePaintPenLatency | Out-Null"

state=""
for _ in $(seq 1 720); do
  sleep 5
  state=$(ssh "$host" "if (Test-Path $remote/status.txt) { Get-Content $remote/status.txt | Select-Object -Last 1 }" | tr -d '\r')
  case "$state" in
    done|failed:*) break ;;
  esac
done
echo "remote status: $state"

mkdir -p "$out"
scp -q -r "$host:$remote/runs/*" "$out/"
scp -q "$host:$remote/status.txt" "$out/status-$target.txt"
[ "$state" = done ] || exit 1
