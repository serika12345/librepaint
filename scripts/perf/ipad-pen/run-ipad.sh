#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Measures the pen-to-display delay of LibrePaint on a USB-connected iPad with
# synthesized Apple Pencil strokes, from the Mac. Each run:
#   1. starts LibrePaint with a new 2048x1536 image and its latency probe,
#   2. starts one screen capture of the iPad (ipad_capture.mm),
#   3. flashes the probe application to map the iPad clock to the Mac clock,
#   4. plays the trajectory.py strokes into LibrePaint (testPlayPlan),
#   5. flashes again, stops the capture and collects the probe file,
#   6. writes inject.csv and capture.csv for capture_analyze.py.
# The capture corrects its clock in steps of about 0.8 ms as the two clocks
# drift apart, so each sample's offset is interpolated between the flash
# before and the flash after the strokes.
#
# Usage: run-ipad.sh NAME RUNS OUT_DIR
# Environment:
#   IPAD_DEVELOPMENT_TEAM  team that signs the probe (required)
#   IPAD_DEVICE            CoreDevice identifier (default: the first available)
#   LIBREPAINT_BUNDLE      installed LibrePaint (default local.librepaint.ipad.xcodedev)
#   TRAJECTORY_JSON        trajectory.py options (default: the pen-light strokes)
#   SETTLE_SECONDS         wait after starting LibrePaint (default 15)
# The first run asks for the camera permission of IPadCapture.app, and the
# iPad needs UI Automation enabled in its developer settings. When the capture
# gets no frames, reconnecting the iPad's cable switches it to screen capture.
set -euo pipefail

[ "$#" -eq 3 ] || { sed -n '15,25p' "$0" >&2; exit 2; }
name=$1 runs=$2 out=$3
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
team=${IPAD_DEVELOPMENT_TEAM:?set IPAD_DEVELOPMENT_TEAM to the team that signs the probe}
bundle=${LIBREPAINT_BUNDLE:-local.librepaint.ipad.xcodedev}
trajectory=${TRAJECTORY_JSON:-'{"strokes":6,"speed":0.5,"dwell":0.3,"rate":240.0,"margin":0.1}'}
settle=${SETTLE_SECONDS:-15}
# The CoreDevice connection drops now and then; each devicectl call is tried
# three times.
dctl() {
  local attempt
  for attempt in 1 2 3; do
    if xcrun devicectl "$@"; then return 0; fi
    [ "$attempt" = 3 ] || sleep 3
  done
  return 1
}
device=${IPAD_DEVICE:-$(dctl list devices | awk '/available|connected/ && !/unavailable/ { for (i = 1; i <= NF; i++) if (length($i) == 36 && $i ~ /^[[:xdigit:]-]+$/) { print $i; exit } }')}
[ -n "$device" ] || { echo "no available iPad" >&2; exit 1; }
udid=$(dctl device info details --device "$device" 2>/dev/null | awk '/udid:/ { print $NF; exit }')
work="$root/build/perf/ipad-pen"
capture_app="$work/IPadCapture.app"
probe="$work/probe"
derived="$work/derived"
measure=(python3 -I "$here/ipad_measure.py")
probe_bundle=local.librepaint.pencilprobe
runner_bundle=local.librepaint.pencilprobe.uitests.xctrunner
mkdir -p "$work" "$out"

host_ns() { python3 -c 'import time; print(time.clock_gettime_ns(time.CLOCK_UPTIME_RAW))'; }
xctest() {
  (cd "$probe" && xcodebuild test-without-building -scheme PencilProbe -destination "id=$udid" \
     -derivedDataPath "$derived" -only-testing:"PencilProbeUITests/PencilProbeTests/$1") > "$2" 2>&1
}
# Flashes the probe application and copies its log of target times to $1.
flash() {
  dctl device process launch --device "$device" --terminate-existing "$probe_bundle" -- -flash > /dev/null
  sleep 11
  local file
  file=$(dctl device info files --device "$device" --domain-type appDataContainer \
           --domain-identifier "$probe_bundle" --subdirectory Documents | grep -oE 'flash-[0-9]+\.csv' | sort | tail -1)
  dctl device copy from --device "$device" --domain-type appDataContainer \
    --domain-identifier "$probe_bundle" --source "Documents/$file" --destination "$1" > /dev/null
}
launch_librepaint() {
  dctl device process launch --device "$device" --terminate-existing \
    --environment-variables '{"LIBREPAINT_LATENCY_LOG":"Library/Application Support/latency.csv","LIBREPAINT_SKIP_AUTOSAVE_RECOVERY":"1"}' \
    "$bundle" --nosplash --new-image RGBA,U8,2048,1536 > /dev/null
  sleep "$settle"
}

# Probe project, capture application and the native-to-window calibration.
rm -rf "$probe"
mkdir -p "$probe"
cp -R "$here/probe/App" "$here/probe/UITests" "$probe/"
python3 -I "$here/probe/genproj.py" "$probe" "$team"
(cd "$probe" && xcodebuild build-for-testing -scheme PencilProbe -destination "id=$udid" \
   -derivedDataPath "$derived" -allowProvisioningUpdates -quiet) > "$work/probe-build.log" 2>&1 ||
  { cat "$work/probe-build.log" >&2; exit 1; }
"$here/build-capture-app.sh" "$work" > /dev/null
xctest testCalibrate "$work/calibrate.log" || { tail -20 "$work/calibrate.log" >&2; exit 1; }
"${measure[@]}" calibration "$work/calibrate.log" > "$work/calibration.json"

# The canvas stays in the same place from one launch to the next, so one
# snapshot gives the stroke plan and the capture bands of every run.
launch_librepaint
rm -rf "$work/snapshot"
open -W "$capture_app" --args "$work/snapshot" 2
"${measure[@]}" canvas "$work/snapshot/first-frame.pgm" > "$work/canvas.json"
width=$(head -c 64 "$work/snapshot/first-frame.pgm" | awk 'NR == 2 { print $1 }')
"${measure[@]}" plan --calibration "$work/calibration.json" --canvas "$work/canvas.json" \
  --capture-width "$width" --bundle "$bundle" --options "$trajectory" \
  --plan-out "$work/plan.json" --bands-out "$work/bands.txt"
dctl device copy to --device "$device" --domain-type appDataContainer \
  --domain-identifier "$runner_bundle" --source "$work/plan.json" --destination tmp/plan.json > /dev/null

stamp=$(date +%Y%m%d-%H%M%S)
for n in $(seq 1 "$runs"); do
  run="$out/$name-$stamp-$n"
  mkdir -p "$run"
  launch_librepaint
  open "$capture_app" --args "$run/capture" 600 "$work/bands.txt"
  for _ in $(seq 100); do [ -f "$run/capture/ready" ] && break; sleep 0.3; done
  [ -f "$run/capture/ready" ] || { echo "ANOMALY capture got no frames; reconnect the iPad" >&2; exit 1; }
  before_from=$(host_ns); flash "$run/flash-before.csv"; before_to=$(host_ns)
  xctest testPlayPlan "$run/play.log"
  after_from=$(host_ns); flash "$run/flash-after.csv"; after_to=$(host_ns)
  touch "$run/capture/stop"
  while [ ! -f "$run/capture/done" ]; do sleep 0.5; done
  dctl device copy from --device "$device" --domain-type appDataContainer --domain-identifier "$bundle" \
    --source "Library/Application Support/latency.csv" --destination "$run/probe.csv" > /dev/null
  status=0
  # set -e does not apply inside a || list, so the steps are chained with &&.
  "${measure[@]}" offset "$run/capture/frames.csv" "$run/flash-before.csv" --from-ns "$before_from" --to-ns "$before_to" > "$run/clock-before.json" &&
    "${measure[@]}" offset "$run/capture/frames.csv" "$run/flash-after.csv" --from-ns "$after_from" --to-ns "$after_to" > "$run/clock-after.json" &&
    "${measure[@]}" inject --probe "$run/probe.csv" --plan "$work/plan.json" \
      --calibration "$run/clock-before.json" --calibration "$run/clock-after.json" --out "$run/inject.csv" &&
    cp "$run/capture/capture.csv" "$run/capture.csv" || status=$?
  echo "run $n: $run (status $status)"
done
