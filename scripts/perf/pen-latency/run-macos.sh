#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs capture-based latency measurements on this Mac (macos_capture.mm).
# Every run starts the application with a fresh home directory under WORK
# (CFFIXED_USER_HOME, so the user's preferences stay untouched), a kritarc
# with the measurement settings, and a white canvas; measures; and stops
# the application. The pointer is moved by the tool during the run.
#
# Usage: run-macos.sh NAME RUNS WORK OUT_DIR -- COMMAND [ARGS...]
#   COMMAND receives the canvas path as its last argument.
# Environment: MACOS_CAPTURE (built macos_capture, required), DISPLAY_INDEX
#   (default 0), KRITARC, SETTLE_SECONDS (default 5), TRAJECTORY_JSON.
#   The application receives REFERENCE_LOG=<run>/reference.log (used by the
#   reference applications).
set -euo pipefail

if [ "$#" -lt 6 ] || [ "$5" != "--" ]; then
  sed -n '4,14p' "$0" >&2
  exit 2
fi
name=$1 runs=$2 work=$3 out=$4
shift 5
here=$(cd "$(dirname "$0")" && pwd)
capture=${MACOS_CAPTURE:?set MACOS_CAPTURE to the built macos_capture}
settle=${SETTLE_SECONDS:-5}
trajectory=${TRAJECTORY_JSON:-}
[ -n "$trajectory" ] || trajectory='{}'
kritarc=${KRITARC:-$'[General]\nnewCursorStyle=0\nnewOutlineStyle=2\nfpsLimit=100\n'}
mkdir -p "$work" "$out"
# The measured application gets the plain user environment: variables of a
# development shell (Python and Qt search paths) would make it load foreign
# libraries, such as another Qt through PyQt.
clean_env=(env -u PYTHONPATH -u PYTHONHOME -u PYTHONNOUSERSITE -u PYTHONHASHSEED -u QT_PLUGIN_PATH
           -u QML2_IMPORT_PATH -u QML_IMPORT_PATH -u QT_ADDITIONAL_PACKAGES_PREFIX_PATH
           -u DYLD_LIBRARY_PATH -u DYLD_FRAMEWORK_PATH -u DYLD_FALLBACK_LIBRARY_PATH)
python3 -I "$here/trajectory.py" --options "$trajectory" --out "$work/plan.csv"
python3 -I "$here/canvas.py" "$work/canvas.png"

stamp=$(date +%Y%m%d-%H%M%S)
for n in $(seq 1 "$runs"); do
  run="$out/$name-$stamp-$n"
  home="$work/home"
  rm -rf "$home"
  mkdir -p "$home/Library/Preferences" "$run"
  printf '%s' "$kritarc" > "$home/Library/Preferences/kritarc"
  "${clean_env[@]}" CFFIXED_USER_HOME=$home HOME=$home REFERENCE_LOG=$run/reference.log \
    "$@" "$work/canvas.png" > "$run/app.log" 2>&1 < /dev/null &
  app=$!
  sleep "$settle"
  status=0
  "$capture" --display "${DISPLAY_INDEX:-0}" --out "$run" --plan "$work/plan.csv" > "$run/capture.log" 2>&1 || status=$?
  kill -TERM "$app" 2> /dev/null || true
  sleep 2
  kill -KILL "$app" 2> /dev/null || true
  wait "$app" 2> /dev/null || true
  [ "$status" -eq 0 ] || { echo "run $n: capture failed: $(cat "$run/capture.log")" >&2; exit 1; }
  echo "run $n: $run"
done
