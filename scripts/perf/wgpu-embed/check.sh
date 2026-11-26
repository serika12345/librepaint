#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Checks items 1 and 2 of wgpu_embed_check.cpp in the logged-in GNOME session
# of this Linux host, for one canvas and one Qt platform plugin:
#   1. a virtual pen tablet (scripts/perf/ui-check/virtual_tablet.py) draws a
#      stroke, and OUT/summary.txt counts the tablet events the canvas got
#      and their pressure range;
#   2. OUT/hover/first-frame.png shows the screen after a click in the
#      canvas, with the pointer left on the canvas: the ink of the stroke and
#      of the click, the hover square, the orange decoration circle, and the
#      two widgets of --overlay-widgets. Look at it, or compare it between
#      canvases.
#
# On GNOME Wayland with Qt's own window decoration (Qt 6.12.0), the wgpu
# canvas shows two limits in this snapshot. A widget that is not native and
# lies over the canvas is hidden under the canvas surface. A native widget
# (every child of the canvas, and with --native-siblings every sibling such as
# the status bar) shows its content moved down by the title bar height:
# QWaylandShmBackingStore::flush copies a native child window from the back
# buffer at the widget offset without the decoration margins. Without the
# decoration (QT_WAYLAND_DISABLE_WINDOWDECORATION=1) and on xcb, every
# widget shows in place.
#
# Usage: check.sh CANVAS PLATFORM OUT [PRESENT [APP_ARGS...]]
#   CANVAS: gl or wgpu; PLATFORM: wayland or xcb; PRESENT: fifo (default),
#   mailbox or immediate; APP_ARGS go to wgpu_embed_check. Needs a build from
#   build.sh and write access to /dev/uinput.
set -euo pipefail

if [ "$#" -lt 3 ]; then
  sed -n 4,30p "$0" >&2
  exit 2
fi
canvas=$1 platform=$2 out=$3 present=${4:-fifo}
shift $(( $# < 4 ? $# : 4 ))
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
pen=$root/scripts/perf/pen-latency
launcher=${WGPU_EMBED_LAUNCHER:-$root/build/perf/wgpu-embed/wgpu-embed-check}
uid=$(id -u)
export XDG_RUNTIME_DIR=/run/user/$uid DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus
eval "$(systemctl --user show-environment | grep -E '^(DISPLAY|WAYLAND_DISPLAY|XAUTHORITY)=' | sed 's/^/export /')"
rm -rf "$out"
mkdir -p "$out"

# The tablet exists before the application starts, as a plugged-in tablet is.
python3 -I "$root/scripts/perf/ui-check/virtual_tablet.py" --wait-for "$out/draw" > "$out/tablet.log" 2>&1 &
tablet=$!
sleep 2
REFERENCE_LOG=$out/app.csv QT_QPA_PLATFORM=$platform \
  setsid "$launcher" --canvas "$canvas" --present "$present" --overlay-widgets "$@" > "$out/app.log" 2>&1 < /dev/null &
app=$!
stop() {
  kill -TERM -- "-$app" 2> /dev/null || true
  kill "$tablet" 2> /dev/null || true
}
trap stop EXIT
sleep 4
kill -0 "$app" 2> /dev/null || { echo "the application exited: $(tail -3 "$out/app.log")" >&2; exit 1; }

touch "$out/draw"
wait "$tablet" || { echo "the virtual tablet failed: $(tail -3 "$out/tablet.log")" >&2; exit 1; }
sleep 1
# Click in the middle of the canvas and keep the pointer there, so the hover
# square and the decoration show in the second snapshot.
nix-shell "$pen/linux-env.nix" --run "
  python3 '$pen/mutter_capture.py' --out '$out/click' --snapshot --wait 5 > /dev/null &&
  python3 - '$out/click/first-frame.png' <<'EOF' > '$out/click.xy'
import struct, sys
# The window is centred and covers 80 % of the screen, so this point is on
# the canvas, away from the stroke and the overlay widgets.
width, height = struct.unpack('>II', open(sys.argv[1], 'rb').read()[16:24])
print('%d,%d' % (width * 0.45, height * 0.4))
EOF
  python3 '$pen/mutter_capture.py' --out '$out/click' --snapshot --keep-pointer --wait 5 --click \$(cat '$out/click.xy') > /dev/null &&
  sleep 1 &&
  python3 '$pen/mutter_capture.py' --out '$out/hover' --snapshot --keep-pointer --wait 5 > /dev/null"
stop
trap - EXIT
sleep 1

python3 -I - "$out/app.csv" > "$out/summary.txt" <<'EOF'
import csv, sys
info, tablet, frames, inputs = [], [], 0, 0
for row in csv.reader(open(sys.argv[1])):
    if row[0] == "info":
        info.append(",".join(row[1:]))
    elif row[0] == "tablet":
        tablet.append(row)
    elif row[0] == "frame":
        frames += 1
    elif row[0] == "input":
        inputs += 1
pressures = [float(r[5]) for r in tablet if float(r[5]) > 0]
print("\n".join(info))
print("tablet events %d, with pressure %d, pressure %.3f to %.3f" % (
    len(tablet), len(pressures), min(pressures, default=0), max(pressures, default=0)))
print("input events %d, frames %d" % (inputs, frames))
EOF
cat "$out/summary.txt"
