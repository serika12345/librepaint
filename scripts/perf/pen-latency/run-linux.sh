#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs capture-based latency measurements in the logged-in GNOME session of
# this Linux host (mutter_capture.py). Every run starts the application with
# a fresh home and configuration under WORK and a white canvas, measures,
# and stops the application. WORK/data keeps the resource database between
# runs.
#
# Usage: run-linux.sh NAME RUNS WORK OUT_DIR -- COMMAND [ARGS...]
#   COMMAND receives the canvas path as its last argument.
# Environment: KRITARC (default: newCursorStyle=0, newOutlineStyle=2,
#   fpsLimit=100 in the empty group), SETTLE_SECONDS before the capture tool
#   starts waiting for the canvas (default 5), TRAJECTORY_JSON, LATENCY_PROBE
#   (1 = the application writes <run>/probe.csv through LIBREPAINT_LATENCY_LOG). The
#   application receives REFERENCE_LOG=<run>/reference.log (used by the
#   reference applications). LibrePaint reads KisConfig and KisImageConfig
#   from the empty group (the keys before the first [Group] line), so the
#   defaults and the scenario settings must not be written to [General].
set -euo pipefail

if [ "$#" -lt 6 ] || [ "$5" != "--" ]; then
  sed -n '4,18p' "$0" >&2
  exit 2
fi
name=$1 runs=$2 work=$3 out=$4
shift 5
here=$(cd "$(dirname "$0")" && pwd)
uid=$(id -u)
export XDG_RUNTIME_DIR=/run/user/$uid DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus
eval "$(systemctl --user show-environment | grep -E '^(DISPLAY|WAYLAND_DISPLAY|XAUTHORITY)=' | sed 's/^/export /')"
settle=${SETTLE_SECONDS:-5}
trajectory=${TRAJECTORY_JSON:-}
[ -n "$trajectory" ] || trajectory='{}'
kritarc=${KRITARC:-$'newCursorStyle=0\nnewOutlineStyle=2\nfpsLimit=100\n'}
mkdir -p "$work" "$out"
python3 -I "$here/canvas.py" "$work/canvas.png"

# GNOME refuses to create a screen capture session ("Session creation
# inhibited") while the screen is blanked after the idle delay or locked.
# Wake a blanked screen before every run; a locked screen cannot be woken
# here and stops the run with a message.
wake_screen() {
  command -v gdbus > /dev/null || return 0
  local saver=(gdbus call --session --dest org.gnome.ScreenSaver --object-path /org/gnome/ScreenSaver)
  "${saver[@]}" --method org.gnome.ScreenSaver.SetActive false > /dev/null 2>&1 || true
  sleep 1
  if [ "$("${saver[@]}" --method org.gnome.ScreenSaver.GetActive 2> /dev/null)" = "(true,)" ]; then
    echo "the screen is locked or blanked and cannot be woken; unlock it at the machine" >&2
    return 1
  fi
}

stamp=$(date +%Y%m%d-%H%M%S)
for n in $(seq 1 "$runs"); do
  run="$out/$name-$stamp-$n"
  home="$work/home"
  wake_screen || exit 1
  rm -rf "$home"
  mkdir -p "$home/config" "$work/data" "$run"
  printf '%s' "$kritarc" > "$home/config/kritarc"
  if [ "${LATENCY_PROBE:-}" = 1 ]; then export LIBREPAINT_LATENCY_LOG="$run/probe.csv"; else unset LIBREPAINT_LATENCY_LOG; fi
  HOME=$home XDG_CONFIG_HOME=$home/config XDG_DATA_HOME=$work/data REFERENCE_LOG=$run/reference.log \
    setsid "$@" "$work/canvas.png" > "$run/app.log" 2>&1 < /dev/null &
  app=$!
  sleep "$settle"
  if ! kill -0 "$app" 2> /dev/null; then
    echo "run $n: application exited before the measurement" >&2
    exit 1
  fi
  status=0
  nix-shell "$here/linux-env.nix" --run \
    "python3 '$here/mutter_capture.py' --out '$run' --options '$trajectory'" || status=$?
  kill -TERM -- "-$app" 2> /dev/null || true
  sleep 2
  kill -KILL -- "-$app" 2> /dev/null || true
  wait "$app" 2> /dev/null || true
  [ "$status" -eq 0 ] || { echo "run $n: capture failed" >&2; exit 1; }
  echo "run $n: $run"
done
