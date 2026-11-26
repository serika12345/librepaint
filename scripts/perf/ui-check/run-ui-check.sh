#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Starts LibrePaint once with the waylandcheck plugin
# (scripts/perf/ui-check/waylandcheck) and captures the screen at every state
# the plugin records: the main window, a stroke drawn with a virtual pen
# tablet (virtual_tablet.py), the brush preset popup, the floating Layers
# docker and a second main window. check_ui.py judges the result.
#
# Usage: [UI_CHECK_SESSION=gnome|xorg] run-ui-check.sh OUT_DIR -- COMMAND [ARGS...]
#   gnome (default) uses the logged-in GNOME session of this Linux host and
#   its screencast. xorg starts an Xorg server in a container
#   (xorg-server.sh) with an openbox window manager, so the application runs
#   on a real X server, and captures with xwd.
#   COMMAND receives the canvas path as its last argument. OUT_DIR receives
#   states.jsonl, <state>.png for every state and app.log. STATUS_BAR_CLICK
#   (default 700,948, captured pixels of a 1920x1080 screen) is the harmless
#   spot clicked after the start state in the GNOME session.
set -euo pipefail

if [ "$#" -lt 3 ] || [ "$2" != "--" ]; then
  sed -n '10,18p' "$0" >&2
  exit 2
fi
out=$1
shift 2
here=$(cd "$(dirname "$0")" && pwd)
pen=$(cd "$here/../pen-latency" && pwd)
session=${UI_CHECK_SESSION:-gnome}
case "$session" in gnome | xorg) ;; *) echo "UI_CHECK_SESSION is gnome or xorg" >&2; exit 2 ;; esac
uid=$(id -u)
export XDG_RUNTIME_DIR=/run/user/$uid DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus
if [ "$session" = gnome ]; then
  eval "$(systemctl --user show-environment | grep -E '^(DISPLAY|WAYLAND_DISPLAY|XAUTHORITY|XDG_SESSION_TYPE)=' | sed 's/^/export /')"
fi

rm -rf "$out"
mkdir -p "$out"
out=$(cd "$out" && pwd)
work=$out/work
mkdir -p "$work/home/config" "$work/data/krita/pykrita"
cp -r "$here/waylandcheck" "$here/kritapykrita_waylandcheck.desktop" "$work/data/krita/pykrita/"
printf 'newCursorStyle=0\nnewOutlineStyle=2\n\n[python]\nenable_waylandcheck=true\n' > "$work/home/config/kritarc"
python3 -I "$pen/canvas.py" "$work/canvas.png"
states=$out/states.jsonl
: > "$states"

# The virtual tablet exists before the application starts, as a plugged-in
# tablet does, and draws when the plugin reaches the tablet state.
python3 -I "$here/virtual_tablet.py" --wait-for "$out/draw" --device-file "$out/tablet.device" > "$out/tablet.log" 2>&1 &
tablet=$!
sleep 2
wm=
cleanup() {
  [ -n "${app:-}" ] && { kill -TERM -- "-$app" 2> /dev/null || true; }
  kill "$tablet" 2> /dev/null || true
  [ -n "$wm" ] && { kill "$wm" 2> /dev/null || true; }
  sleep 2
  [ -n "${app:-}" ] && { kill -KILL -- "-$app" 2> /dev/null || true; }
  [ "$session" = xorg ] && "$here/xorg-server.sh" stop
  return 0
}
trap cleanup EXIT

if [ "$session" = xorg ]; then
  [ -f "$out/tablet.device" ] || { echo "the virtual tablet failed: $(cat "$out/tablet.log")" >&2; exit 1; }
  "$here/xorg-server.sh" start "$out/xorg" "$(cat "$out/tablet.device")"
  xenv=$(realpath "$here/../../../build/perf/xorg-env")
  export DISPLAY=:${XORG_DISPLAY:-99} XAUTHORITY=$out/xorg/xauth XDG_SESSION_TYPE=x11
  unset WAYLAND_DISPLAY
  XDG_CONFIG_DIRS=$xenv/etc/xdg "$xenv/bin/openbox" > "$out/openbox.log" 2>&1 &
  wm=$!
  sleep 1
fi

snapshot() {
  local state=$1 click=
  if [ "$session" = xorg ]; then
    "$xenv/bin/xwd" -root -silent | "$xenv/bin/magick" xwd:- "$out/$state.png" || true
    return
  fi
  # After the start state, click once on the status bar label below the
  # canvas: a Wayland compositor opens a popup only for a recent input event,
  # which a user always has and the plugin's programmatic click lacks.
  [ "$state" = start ] && click="--click ${STATUS_BAR_CLICK:-700,948}"
  nix-shell "$pen/linux-env.nix" --run \
    "python3 '$pen/mutter_capture.py' --out '$out/$state' --snapshot --keep-pointer --wait 3 $click" > /dev/null 2>&1 || true
  [ -f "$out/$state/first-frame.png" ] && mv "$out/$state/first-frame.png" "$out/$state.png"
  rm -rf "${out:?}/$state"
}

LIBREPAINT_UI_CHECK_OUT=$states HOME=$work/home XDG_CONFIG_HOME=$work/home/config XDG_DATA_HOME=$work/data \
  setsid "$@" "$work/canvas.png" > "$out/app.log" 2>&1 < /dev/null &
app=$!

seen=0
deadline=$((SECONDS + 150))
while [ "$SECONDS" -lt "$deadline" ]; do
  lines=$(wc -l < "$states")
  while [ "$seen" -lt "$lines" ]; do
    seen=$((seen + 1))
    state=$(sed -n "${seen}p" "$states" | python3 -c 'import json,sys; print(json.load(sys.stdin)["state"])')
    [ "$state" = done ] && exit 0
    if [ "$state" = tablet ]; then
      touch "$out/draw"
      wait "$tablet" || echo "the virtual tablet failed: $(cat "$out/tablet.log")" >&2
      continue
    fi
    snapshot "$state"
  done
  kill -0 "$app" 2> /dev/null || { echo "the application exited before the check finished" >&2; exit 1; }
  sleep 0.5
done
echo "the check did not finish within 150 seconds" >&2
exit 1
