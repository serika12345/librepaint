#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Starts LibrePaint once in the logged-in GNOME session of this Linux host with
# the waylandcheck plugin (scripts/perf/ui-check/waylandcheck) and captures the
# screen at every state the plugin records: the main window, the brush preset
# popup, the floating Layers docker and a second main window. check_ui.py
# judges the result.
#
# Usage: run-ui-check.sh OUT_DIR -- COMMAND [ARGS...]
#   COMMAND receives the canvas path as its last argument. OUT_DIR receives
#   states.jsonl, <state>.png for every state and app.log. STATUS_BAR_CLICK
#   (default 700,948, captured pixels of a 1920x1080 screen) is the harmless
#   spot clicked after the start state.
set -euo pipefail

if [ "$#" -lt 3 ] || [ "$2" != "--" ]; then
  sed -n '4,12p' "$0" >&2
  exit 2
fi
out=$1
shift 2
here=$(cd "$(dirname "$0")" && pwd)
pen=$(cd "$here/../pen-latency" && pwd)
uid=$(id -u)
export XDG_RUNTIME_DIR=/run/user/$uid DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus
eval "$(systemctl --user show-environment | grep -E '^(DISPLAY|WAYLAND_DISPLAY|XAUTHORITY|XDG_SESSION_TYPE)=' | sed 's/^/export /')"

rm -rf "$out"
work=$out/work
mkdir -p "$work/home/config" "$work/data/krita/pykrita"
cp -r "$here/waylandcheck" "$here/kritapykrita_waylandcheck.desktop" "$work/data/krita/pykrita/"
printf 'newCursorStyle=0\nnewOutlineStyle=2\n\n[python]\nenable_waylandcheck=true\n' > "$work/home/config/kritarc"
python3 -I "$pen/canvas.py" "$work/canvas.png"
states=$out/states.jsonl
: > "$states"

LIBREPAINT_UI_CHECK_OUT=$states HOME=$work/home XDG_CONFIG_HOME=$work/home/config XDG_DATA_HOME=$work/data \
  setsid "$@" "$work/canvas.png" > "$out/app.log" 2>&1 < /dev/null &
app=$!
trap 'kill -TERM -- "-$app" 2> /dev/null || true; sleep 2; kill -KILL -- "-$app" 2> /dev/null || true' EXIT

seen=0
deadline=$((SECONDS + 90))
while [ "$SECONDS" -lt "$deadline" ]; do
  lines=$(wc -l < "$states")
  while [ "$seen" -lt "$lines" ]; do
    seen=$((seen + 1))
    state=$(sed -n "${seen}p" "$states" | python3 -c 'import json,sys; print(json.load(sys.stdin)["state"])')
    [ "$state" = done ] && exit 0
    # After the start state, click once on the status bar label below the
    # canvas: a Wayland compositor opens a popup only for a recent input event,
    # which a user always has and the plugin's programmatic click lacks.
    click=
    [ "$state" = start ] && click="--click ${STATUS_BAR_CLICK:-700,948}"
    nix-shell "$pen/linux-env.nix" --run \
      "python3 '$pen/mutter_capture.py' --out '$out/$state' --snapshot --keep-pointer --wait 3 $click" > /dev/null 2>&1 || true
    [ -f "$out/$state/first-frame.png" ] && mv "$out/$state/first-frame.png" "$out/$state.png"
    rm -rf "${out:?}/$state"
  done
  kill -0 "$app" 2> /dev/null || { echo "the application exited before the check finished" >&2; exit 1; }
  sleep 0.5
done
echo "the check did not finish within 90 seconds" >&2
exit 1
